import os
import json
import pytest
from src.push_utils import load_or_generate_vapid_keys

def test_load_or_generate_vapid_keys_creates_file(tmp_path, monkeypatch):
    vapid_file = tmp_path / "vapid_keys.json"
    monkeypatch.setenv("VAPID_KEYS_FILE", str(vapid_file))
    
    # Ensure it doesn't exist
    assert not vapid_file.exists()
    
    keys = load_or_generate_vapid_keys()
    
    # Should exist now
    assert vapid_file.exists()
    
    # Should contain private and public keys
    assert "private_key" in keys
    assert "public_key" in keys
    
    # Reload should return same keys
    keys2 = load_or_generate_vapid_keys()
    assert keys["private_key"] == keys2["private_key"]
    assert keys["public_key"] == keys2["public_key"]
