# Callsign directory regression tests (Linux)

Build the production gateway and link the test driver against its Common library:

```sh
make -j2 ircDDBGateway/ircddbgatewayd
g++ -O2 -DwxUSE_GUI=0 $(wx-config --cxxflags) -ICommon \
  tests/callsign_server_driver.cpp Common/Common.a \
  -Wl,--wrap=connect -Wl,--wrap=gethostbyname \
  $(wx-config --libs base,net) -o /tmp/callsign-server-test
python3 tests/test_callsign_server.py /tmp/callsign-server-test
```

The test executable redirects directory connections to ephemeral loopback servers;
it never connects to the real directory service or the running gateway. It covers
nonblocking startup and cancellation, a normal complete response, silent peers,
a partial response that stalls, and a slow trickle exceeding the total deadline.
Existing cached reflectors must survive failed downloads. The suite takes about
30 seconds.

To demonstrate the original regression, link the driver against an unpatched
Common library and run with `--baseline`. It expects startup to remain blocked
past the three-second test deadline.
