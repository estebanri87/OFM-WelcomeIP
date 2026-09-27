# OFM-WelcomeIP

Busch-Welcome IP door entry system on the KNX bus, through the Smart Access Point's
**local API** (MQTT). Ring, door opener, door state and the IP actuator outputs.
Video and audio are deliberately out of scope.

Status: **skeleton**. The transport works and is testable against any MQTT broker;
the ABB wire format and the ETS application are not finished. See *Open points*.

## What it is for

| Use case | Path |
|---|---|
| Open the front door from a KNX push-button. The motor lock hangs on the H8304-03 potential-free output | KNX → `setDataPoint` on the IP actuator |
| Ring signal as a group object, e.g. to trigger a chime over external logic | Outdoor station `ch0002` "Incoming call" → KNX |

## Requirements

- Smart Access Point firmware **6.36 or later**
- Local API enabled: *Settings → Connections & APIs → Local API & SIP configuration*
- An API user; the Smart Access Point issues a **generic** username, not the account name
- The device must reach the Smart Access Point on its home-network side (not the 10.x Welcome side)
- ESP32 only. RP2040 is excluded: `MQTT::Client` keeps a static instance pointer for its
  lwIP callbacks, so a second client would break the device's own one, and there is no TLS there.

## Design

```
KNX ── WelcomeIPModule ── WelcomeIPChannel[]      channels speak WipAddress only
             │
             ├─ WipMqttLink     connection, 30 s heartbeat, event queue
             │       └─ MQTT::Module (second instance, OFM-Network)
             └─ WipProtocol     the only place that knows the ABB format
```

**`WipProtocol` is the seam.** Topics, payload assembly and parsing live there and
nowhere else, so confirming the format against real hardware changes one file rather
than the whole module.

**Incoming datapoints are queued, not delivered inline.** On ESP32 the MQTT client runs
in its own FreeRTOS task, so a subscription callback does not run in the KNX loop;
writing a group object from there would race the stack. `WipMqttLink` therefore parses
in the callback and hands the result to `loop()` through a single-producer ring — the
same record-and-defer rule OFM-Network's webserver follows.

**A second MQTT client, not the device's own.** The Smart Access Point is a different
broker with its own credentials and topic scheme, so `MQTT::Module::configure()` points
a separate instance at it. Status publishes and the last will are off: a foreign broker
rejects `<prefix>status` by ACL.

**The RX buffer is 64 KB.** A `getAll` response is 17–50 KB of JSON. The 1 KB default
cannot hold it, and since the broker resends, an undersized buffer is a reconnect loop
rather than one lost message. Parsing uses an ArduinoJson filter that keeps only serial,
device type, channel index and output values.

## Console

```
wip                                status, SmartAP serial, channel count
wip connect <host> [user] [pass]   connect to the local API
wip notls                          plain MQTT on 1883 for the next connect
wip devices                        request the model (getAll) and list devices
wip set <sn> <ch> <dp> <val>       write an input datapoint
wip raw on|off                     log every received payload
```

## Open points

Marked `TODO(Stufe2)` (wire format) and `TODO(Stufe3)` (ETS) in the source.

1. **Topics and payloads are placeholders.** The ABB page `wip_local/definition` names
   GetAll, SetDataPoint and Notification but ships **empty code blocks**, and the
   reference tables for DeviceTypeId/ChannelID are the only concrete data published.
   The real strings are recovered by subscribing to `#` on the device and by reading the
   web UI's own WebSocket traffic — the `getAll` envelope (`method`, `queryid`, `jid`,
   `sessionjwt`) suggests the UI speaks the same JSON.
2. **Transport unconfirmed.** Port 8883 is documented (SmartAP product manual, "Ports and
   services"), but whether it is plain MQTT or MQTT over WebSocket is not. WebSocket would
   need framing in `MQTT::Client`, so it is checked first.
3. **No ETS application yet.** Channels carry the References-table defaults but read no
   parameters; the connection is configured from the console.
4. **Ring datapoint unverified.** The References table lists outdoor station `ch0002`
   "Incoming call" and SmartAP `ch0001` "Doorbell ring", which makes MQTT the likely
   source. If it turns out not to fire, the fallback is registering as a third-party SIP
   panel on the Smart Access Point.
5. **Trusted devices.** The H8304 releases its lock only for signed, trusted devices.
   Whether a command through the local API counts as trusted is untested.
