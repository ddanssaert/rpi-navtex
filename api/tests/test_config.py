import json
import httpx
import pytest
import respx

DSP_URL = "http://sdr-dsp:8001/control/config"

def test_post_config_saves_to_file(client, setup_test_env):
    with respx.mock(base_url="http://sdr-dsp:8001") as router:
        router.post("/control/config").mock(return_value=httpx.Response(200, json={"status":"ok"}))
        r = client.post("/config", json={"antenna":"B","lna_gain":4})
    assert r.status_code == 200
    assert r.json() == {"antenna":"B","lna_gain":4}
    saved_text = (setup_test_env / "config.json").read_text()
    saved = json.loads(saved_text)
    assert saved == {"antenna":"B","lna_gain":4}
    # Round-trip
    r2 = client.get("/config")
    assert r2.json() == {"antenna":"B","lna_gain":4}

def test_post_config_forwards_to_dsp(client):
    with respx.mock(base_url="http://sdr-dsp:8001") as router:
        route = router.post("/control/config").mock(
            return_value=httpx.Response(200, json={"status":"ok"}))
        r = client.post("/config", json={"antenna":"C","lna_gain":7})
    assert r.status_code == 200
    assert route.called
    assert route.call_count == 1
    sent = json.loads(route.calls[0].request.content.decode())
    assert sent == {"antenna":"C","lna_gain":7}

def test_post_config_degrades_on_dsp_timeout(client, setup_test_env):
    with respx.mock(base_url="http://sdr-dsp:8001") as router:
        router.post("/control/config").mock(side_effect=httpx.ConnectTimeout("boom"))
        r = client.post("/config", json={"antenna":"A","lna_gain":2})
    assert r.status_code == 200
    assert r.json() == {"antenna":"A","lna_gain":2}
    assert json.loads((setup_test_env / "config.json").read_text()) == {"antenna":"A","lna_gain":2}

def test_post_config_degrades_on_dsp_503(client, setup_test_env):
    with respx.mock(base_url="http://sdr-dsp:8001") as router:
        router.post("/control/config").mock(
            return_value=httpx.Response(503, json={"status":"error","detail":"starting"}))
        r = client.post("/config", json={"antenna":"A","lna_gain":1})
    assert r.status_code == 200
    assert json.loads((setup_test_env / "config.json").read_text()) == {"antenna":"A","lna_gain":1}

def test_post_config_uses_env_var(client, monkeypatch):
    monkeypatch.setenv("SDR_CONTROL_URL", "http://custom:9999/control/config")
    with respx.mock(base_url="http://custom:9999") as router:
        route = router.post("/control/config").mock(return_value=httpx.Response(200, json={"status":"ok"}))
        r = client.post("/config", json={"antenna":"A","lna_gain":0})
    assert r.status_code == 200
    assert route.called
