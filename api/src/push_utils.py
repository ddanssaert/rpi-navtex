import os
import json
from pywebpush import webpush, WebPushException

def _get_vapid_keys_file():
    return os.getenv("VAPID_KEYS_FILE", "/data/vapid_keys.json")

def load_or_generate_vapid_keys():
    """Load VAPID keys from file, or generate new ones if the file doesn't exist."""
    keys_file = _get_vapid_keys_file()
    if os.path.exists(keys_file):
        with open(keys_file, "r") as f:
            return json.load(f)
    
    # Generate new keys using py_vapid
    from py_vapid import Vapid
    from cryptography.hazmat.primitives import serialization
    import base64

    vapid = Vapid()
    vapid.generate_keys()

    # Export private key to DER bytes, then base64url encoded
    private_key_bytes = vapid.private_key.private_bytes(
        encoding=serialization.Encoding.DER,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )
    private_key_b64 = base64.urlsafe_b64encode(private_key_bytes).decode('utf-8').rstrip('=')

    # Export public key to uncompressed point bytes, then base64url encoded
    public_key_bytes = vapid.public_key.public_bytes(
        encoding=serialization.Encoding.X962,
        format=serialization.PublicFormat.UncompressedPoint
    )
    public_key_b64 = base64.urlsafe_b64encode(public_key_bytes).decode('utf-8').rstrip('=')

    keys = {
        "private_key": private_key_b64,
        "public_key": public_key_b64
    }

    # Ensure directory exists
    os.makedirs(os.path.dirname(keys_file), exist_ok=True)
    
    with open(keys_file, "w") as f:
        json.dump(keys, f)
        
    return keys

def send_push_notification(subscription, data, vapid_keys):
    """Send a web push notification to a specific subscription."""
    try:
        webpush(
            subscription_info=subscription,
            data=json.dumps(data),
            vapid_private_key=vapid_keys["private_key"],
            vapid_claims={
                "sub": os.getenv("VAPID_CONTACT", "mailto:admin@navtex.local")
            }
        )
        return True
    except WebPushException as ex:
        print("Web Push failed: {}", repr(ex))
        # If the error is 410 (Gone), the subscription has expired or been removed
        if ex.response and ex.response.status_code == 410:
            return False # Caller should remove subscription
        return True # Other errors might be temporary
