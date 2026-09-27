#!/usr/bin/env python3
import os
import signal
import socket
import subprocess
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def recv_line(conn, timeout=5.0):
    conn.settimeout(timeout)
    data = bytearray()
    while True:
        ch = conn.recv(1)
        if not ch:
            raise RuntimeError("connection closed")
        data += ch
        if data.endswith(b"\r\n"):
            return data[:-2].decode("utf-8", "strict")

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("127.0.0.1", 0))
server.listen(1)
port = server.getsockname()[1]

with tempfile.NamedTemporaryFile("w", delete=False) as cfg:
    cfg.write(f"host=127.0.0.1\nport={port}\nnick=TiCle\n")
    cfg.write("user=ticle\nrealname=TiCle integration test\n")
    cfg.write("script=scripts/example.tcl\n")
    cfg_path = cfg.name

proc = None
conn = None
try:
    proc = subprocess.Popen(
        [os.path.join(ROOT, "ticle"), "-c", cfg_path],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    server.settimeout(5.0)
    conn, _ = server.accept()

    first = recv_line(conn)
    second = recv_line(conn)
    assert first == "NICK TiCle", first
    assert second == "USER ticle 0 * :TiCle integration test", second

    conn.sendall(b":fake 433 * TiCle :Nickname is already in use\r\n")
    fallback = recv_line(conn)
    assert fallback == "NICK TiCle_", fallback

    conn.sendall(b":fake 001 TiCle_ :Welcome\r\n")
    conn.sendall(b"PING :integration-token\r\n")
    pong = recv_line(conn)
    assert pong == "PONG :integration-token", pong

    proc.send_signal(signal.SIGTERM)
    rc = proc.wait(timeout=5.0)
    assert rc == 0, rc
    print("PASS: fake IRC integration")
finally:
    if conn is not None:
        conn.close()
    server.close()
    if proc is not None and proc.poll() is None:
        proc.kill()
        proc.wait()
    os.unlink(cfg_path)
