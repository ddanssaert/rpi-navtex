import os
import datetime
import socket
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa

def generate_ca(cert_dir):
    """Generates a self-signed Root CA."""
    ca_key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    
    subject = issuer = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, "NA"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Navtex"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Navtex Local Authority"),
        x509.NameAttribute(NameOID.COMMON_NAME, "Navtex Root CA"),
    ])
    
    ca_cert = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(ca_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.utcnow())
        .not_valid_after(datetime.datetime.utcnow() + datetime.timedelta(days=3650))
        .add_extension(x509.BasicConstraints(ca=True, path_length=None), critical=True)
        .sign(ca_key, hashes.SHA256())
    )
    
    with open(os.path.join(cert_dir, "rootCA.key"), "wb") as f:
        f.write(ca_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        ))
    with open(os.path.join(cert_dir, "rootCA.crt"), "wb") as f:
        f.write(ca_cert.public_bytes(serialization.Encoding.PEM))

import ipaddress

def generate_server_cert(cert_dir, host_ip):
    """Generates a server certificate signed by the local Root CA."""
    with open(os.path.join(cert_dir, "rootCA.key"), "rb") as f:
        ca_key = serialization.load_pem_private_key(f.read(), password=None)
    with open(os.path.join(cert_dir, "rootCA.crt"), "rb") as f:
        ca_cert = x509.load_pem_x509_certificate(f.read())
    # Normalize host_ip
    host_ip = host_ip.strip()
    print(f"Generating server cert for: {host_ip}")
    
    server_key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    
    subject = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, "NA"),
        x509.NameAttribute(NameOID.COMMON_NAME, host_ip),
    ])
    
    san_list = [
        x509.DNSName("navtex.local"),
        x509.DNSName("localhost"),
        x509.DNSName(host_ip) # Add as DNS name too
    ]
    
    # Add IP SAN if it looks like one
    try:
        ip_obj = ipaddress.ip_address(host_ip)
        san_list.append(x509.IPAddress(ip_obj))
        print(f"Added IP SAN: {host_ip}")
    except ValueError:
        print(f"Warning: {host_ip} is not a valid IP address, skipping IP SAN")
        pass
        
    server_cert = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(ca_cert.subject)
        .public_key(server_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.utcnow())
        .not_valid_after(datetime.datetime.utcnow() + datetime.timedelta(days=365))
        .add_extension(x509.SubjectAlternativeName(san_list), critical=False)
        .sign(ca_key, hashes.SHA256())
    )
    
    with open(os.path.join(cert_dir, "server.key"), "wb") as f:
        f.write(server_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        ))
    with open(os.path.join(cert_dir, "server.crt"), "wb") as f:
        f.write(server_cert.public_bytes(serialization.Encoding.PEM))

def ensure_certs():
    """Main entry point to ensure Root CA and server certs exist."""
    cert_dir = "/data/certs"
    os.makedirs(cert_dir, exist_ok=True)
    
    if not os.path.exists(os.path.join(cert_dir, "rootCA.crt")):
        generate_ca(cert_dir)
        
    # Attempt to auto-detect IP, prioritize environment variable
    host_ip = os.getenv("HOST_IP")
    if not host_ip:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            # doesn't even have to be reachable
            s.connect(("10.255.255.255", 1))
            host_ip = s.getsockname()[0]
            s.close()
        except Exception:
            host_ip = "192.168.2.246"
        
    generate_server_cert(cert_dir, host_ip)
    return host_ip
