#!/usr/bin/env python3
"""Exercise production callsign download code against local TCP failures.
Usage: python3 tests/test_callsign_server.py /path/to/driver [--baseline]
"""
import concurrent.futures, os, socket, subprocess, sys, threading, time
DRIVER = sys.argv[1]
BASELINE = '--baseline' in sys.argv

def scenario(name):
    listener = socket.socket()
    listener.bind(('127.0.0.1', 0))
    listener.listen(1)
    listener.settimeout(4)
    port = listener.getsockname()[1]
    def serve():
        try:
            with listener, listener.accept()[0] as client:
                client.settimeout(40)
                request = bytearray()
                while len(request) < 29:
                    chunk = client.recv(29-len(request))
                    if not chunk: return
                    request.extend(chunk)
                if name == 'complete':
                    client.sendall(b'DCS123 127.0.0.2\n')
                    return
                if name == 'partial': client.sendall(b'DCS123 127.0.0.2\nDCS456 ')
                if name == 'trickle':
                    for _ in range(180):
                        client.sendall(b'D')
                        time.sleep(.2)
                else:
                    client.recv(1) # Wait for the client to terminate the stalled download.
        except (OSError, TimeoutError):
            pass
    thread = threading.Thread(target=serve, daemon=True)
    thread.start()
    started = time.monotonic()
    try:
        result = subprocess.run([DRIVER, name], env={**os.environ, 'TEST_PORT':str(port)},
            text=True, capture_output=True, timeout=3 if BASELINE else 35)
    except subprocess.TimeoutExpired:
        if BASELINE:
            print('CONFIRMED baseline startup blocks on a silent directory server')
            return
        raise
    finally:
        listener.close()
    if BASELINE: raise AssertionError('Expected original startup to block')
    assert result.returncode == 0, (name, result.stdout, result.stderr)
    duration = time.monotonic()-started
    if name in ('silent','partial'): assert 9 <= duration <= 13, (name,duration)
    if name == 'trickle': assert 29 <= duration <= 33, duration
    print(result.stdout.strip(), flush=True)

names=['startup'] if BASELINE else ['startup','complete','silent','partial','trickle']
with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:
    list(pool.map(scenario,names))
