# Astro Orbit USB protocol 1

Transport: ESP32 USB/UART0, 115200 baud, 8N1, no handshake, LF terminated.
`Serial1` at 1 Mbaud remains dedicated to the ST3215. No new motor wiring.

Each request and reply starts with `@MOROTA ` followed by one compact JSON object.
Debug output may appear between replies and must be ignored. Replies begin with
an extra LF so an unfinished debug line cannot hide the prefix. Receive code still
accepts the prefix after other text. JSON serialization is done **before** adding
the prefix: ArduinoJson 7.4 clears its destination string.

Maximum request length: 255 characters excluding LF/CR. Oversized lines are discarded
until LF. A partial request idle for >1 second is discarded until LF. The firmware
consumes at most 64 bytes per loop iteration. Invalid JSON or an unprefixed line is
ignored; valid JSON with invalid fields receives an error where an ID is available.

Example (use fresh random IDs and a fresh session token in real clients):

```text
@MOROTA {"id":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","cmd":"hello"}
@MOROTA {"id":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","boot":"0123456789abcdef","error":0,"message":"","value":{"device":"MoMaRoTa","protocol":1,"firmware":"1.1.0-usb","leaseMs":15000}}
```

`id`: 32 lowercase hexadecimal characters, unique for every request.
`session`: 32 lowercase hexadecimal characters, fixed for one USB connection.
`boot`: random controller startup identifier, returned with every reply.
`error`: integer, 0 on success. `message`: human-readable error text.
`value`: optional response object; commands with no returned value omit it.

| Command | Request value | Result / behavior |
|---|---|---|
| `hello` | none; no session required | device, protocol, firmware, leaseMs |
| `connect` | session required | acquire USB lease, return status |
| `disconnect` | none | stop active motion, release lease |
| `status` | none | current status, renew lease |
| `absolute` | number, 0 ≤ x < 360 | move to synced sky angle |
| `mechanical` | number, 0 ≤ x < 360 | move ignoring sync offset |
| `move` | number, -360 ≤ x ≤ 360 | relative move preserving direction and full turns |
| `sync` | number, 0 ≤ x < 360 | set sky offset without moving |
| `reverse` | JSON boolean | set motor direction reversal |
| `halt` | none | call existing motor stop function |

Except Hello and Connect, every command requires the current session token.
Connect with the same token is idempotent; another token is rejected. Connect also
rejects an active Alpaca connection or a pre-existing movement. Unknown commands
and invalid values never dispatch to the motor. Mutations other than Halt and
Disconnect require the motor to be idle. Movement and Sync require recent valid
motor feedback. All sessions use the same firmware sky offset and target position.

Status fields: `position`, `mechanical`, `target`, `stepSize` (degrees), `moving`,
`reverse`, `motorHealthy` (booleans), `motionError` (string, empty if no motion fault).
`target` always uses sky coordinates, including after a mechanical move.

`zero` requires an active USB session and no value. It sets the mechanical and
synced positions to 0° and stores this virtual reference in the controller.

Errors: 1024 unsupported command, 1025 invalid value, 1031 no valid USB session,
1035 busy/owned, 1280 motor/movement failure. The C# driver maps these to ASCOM
exceptions; a busy response becomes DriverException. The numerical values here
are the serial protocol's error codes, not a claim of additional ASCOM methods.

A valid session request renews a 15-second lease. The Windows driver sends Status
every 2 seconds. On lease expiry the main loop stops active motion (best effort if
feedback is lost), releases ownership and does not retain the session. Hello does
not renew a lease. USB is available after setup finishes, including the existing
WiFi connection attempt; handshake allows 60 seconds for boot.

Request/reply exchanges are serialized by the driver. Only Hello may be retried
when no answer arrives. **Never retry a movement automatically:** an unanswered
request may already have moved the motor. Match responses by ID and reject a
changed boot ID. There is no replay cache in protocol 1; re-sending a movement is
another movement. Invalid/missing replies cannot be treated as success.

Firmware uses a recursive control mutex around the servo main-loop updates,
USB dispatch, Alpaca handlers and web control commands. The existing servo-bus
mutex remains separate. During USB ownership, HTTP writes are rejected except
Halt; the web Stop command is also allowed. Read-only network status remains
available. OTA/reboot is not blocked; it invalidates the USB session and virtual zero.

Known inherited limits: virtual zero and Sync reset on ESP32 restart, existing
motor feedback tolerances, and the existing cable window. Relative movements
preserve their requested direction/full turns; absolute movements choose the nearest
equivalent angle within that window. An additional physical motor-step ledger
prevents Reverse changes or software zeroing from bypassing the cable window.
It is reset only by a controller restart.
This protocol does not introduce a home sensor or absolute output encoder.
