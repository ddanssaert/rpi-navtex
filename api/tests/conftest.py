import os
import json
import tempfile
import pytest
from fastapi.testclient import TestClient

@pytest.fixture(autouse=True)
def setup_test_env(monkeypatch, tmp_path):
    """Global fixture to redirect /data paths to tmp for all tests."""
    config_path = str(tmp_path / "config.json")
    cert_dir = str(tmp_path / "certs")
    
    monkeypatch.setenv("CONFIG_FILE", config_path)
    monkeypatch.setenv("CERT_DIR", cert_dir)
    monkeypatch.setenv("DB_PATH", f"sqlite:///{tmp_path}/test.db")
    monkeypatch.setenv("SDR_CONTROL_URL", "http://sdr-dsp:8001/control/config")
    
    # Pre-create an empty config
    (tmp_path / "config.json").write_text("{}")
    
    # Force patch the module constant in case it's already imported
    import api.src.main as main_mod
    monkeypatch.setattr(main_mod, "CONFIG_FILE", config_path)
    
    import api.src.security as sec_mod
    # Patch the ensure_certs to use the tmp cert dir
    orig_ensure = sec_mod.ensure_certs
    def patched_ensure():
        monkeypatch.setenv("CERT_DIR", cert_dir)
        return orig_ensure()
    monkeypatch.setattr(sec_mod, "ensure_certs", patched_ensure)

    return tmp_path

@pytest.fixture
def client(setup_test_env):
    from api.src.main import app
    with TestClient(app) as c:
        yield c
