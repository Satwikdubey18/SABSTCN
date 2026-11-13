# SABSTCN — Product Design & Requirements

## 1. Purpose and scope

SABSTCN (Smart Autonomous Border Surveillance & Tactical Communication Network) is a single-post ESP32 surveillance demonstrator for MITS Gwalior ECE. This release covers Check Post 3 (CP3), a responsive operator dashboard, MQTT telemetry, alert evidence, a health watchdog, and optional Firebase Realtime Database persistence. It is a demonstrator, not a certified safety or defense system.

## 2. Problem statement

Remote perimeters need immediate, understandable awareness of movement, close objects, smoke, position and device health. Field connectivity can be unreliable, so a web dashboard must continue to demonstrate and record the local situation when hardware, Wi-Fi or cloud services fail.

## 3. System overview

CP3 uses a PIR, HC-SR04, MQ-2, NEO-6M GPS, OLED and buzzer connected to an ESP32. The ESP32 assesses the threat level, publishes telemetry over MQTT, sends escalation SMS through SIM800L, and maintains a heartbeat. The browser renders rather than decides threat levels.

## 4. Functional requirements

- FR-1: Show CP3 current L0–L4 assessment and last update.
- FR-2: Display PIR, distance, smoke/base/limit, GPS, battery, Wi-Fi, GSM/SMS and uptime.
- FR-3: Subscribe to validated MQTT telemetry, alert and heartbeat messages over WebSockets.
- FR-4: Reject malformed packets and count them without crashing.
- FR-5: Provide a usable built-in DEMO simulator, including all named failure cases.
- FR-6: Plot five-minute distance, smoke and stepped threat history, with pause/resume.
- FR-7: Map current position and retain L3/L4 alert pins; export evidence.
- FR-8: Keep 200 local event records, filter/search them, and export CSV.
- FR-9: Alert visibly and audibly for L3/L4; allow muting.
- FR-10: Classify health as ONLINE, DEGRADED or OFFLINE using heartbeats and log loss/recovery.
- FR-11: Optionally sync latest data, heartbeats and events to Firebase after user-supplied configuration.
- FR-12: Offer editable broker, topic, stored position, sound and demo controls.

## 5. Non-functional requirements

The dashboard shall work as static files and GitHub Pages, stay readable on a projector and phone, avoid credentials in source, tolerate missing fields, and use lightweight CDN dependencies only. It targets normal current desktop/mobile browsers with an internet connection for CDN assets and tiles.

## 6. Architecture

```text
PIR / HC-SR04 / MQ-2 ─┐
GPS NEO-6M ──────────┼──> ESP32 (threat assessment) ── MQTT/WSS ──> Web dashboard
OLED / buzzer <──────┘              │                         │              │
                                   ├──> SIM800L ── SMS backup ┘              └── Firebase RTDB
                                   └──> heartbeat / retained status
```

## 7. MQTT topics and JSON schema

Default broker: `wss://broker.hivemq.com:8884/mqtt`; prefix: `sabstcn/cp3`.

| Topic | Use |
|---|---|
| `<prefix>/telemetry` | 1–2 second current sensor sample |
| `<prefix>/alert` | L3/L4 evidence record |
| `<prefix>/heartbeat` | 10-second health sample |
| `<prefix>/status` | retained `online` / Last Will `offline` |

Every payload: `{"v":1,"node":"CP3","type":"telemetry|alert|heartbeat","ts":1730000000,"data":{...}}`.

Telemetry data includes `level`, `pir`, `dist` (`-1` means no echo), `smoke`, `base`, `lat`, `lng`, `gps` (`LIVE|STORED`), `batt`, and `rssi`. Alert data includes `level`, `trigger` (`OBJECT|OBJECT+SMOKE`), coordinates, GPS state and `sms` (`SENT|PENDING|FAILED`). Heartbeat data includes `uptime`, `rssi`, `batt`, `heap`, `boots`, `fw`, `csq` (0–31), and `sim` (`REGISTERED|SEARCHING|NONE`).

## 8. Threat-level table

| Level | Meaning | Dashboard color |
|---|---|---|
| L0 | Safe | green |
| L1 | PIR motion or smoke alone | yellow-green |
| L2 | Object at 5–10 cm; buzzer beeping | amber |
| L3 | Object under 5 cm; GPS + SMS | orange-red |
| L4 | Object near and smoke over limit | deep red |

## 9. GPS live/stored fallback logic

The ESP32 sends `gps: "LIVE"` only with a valid current fix. If no fix exists, it sends the configured college/post coordinates with `gps: "STORED"`. The dashboard preserves and labels that state in sensor card, map pin, popup and event log; invalid coordinates fall back safely to stored position.

## 10. Watchdog state diagram

```text
heartbeat seen (<15 s) ──> ONLINE
ONLINE -- 15–29 s stale --> DEGRADED
DEGRADED -- heartbeat --> ONLINE
DEGRADED -- >=30 s stale / LWT offline --> OFFLINE
OFFLINE -- heartbeat --> ONLINE (RECOVERED event)
```

## 11. UI design

Background is `#0B1220`, glass panels use thin blue-grey borders, operational teal is `#1D9E75`, warnings amber and incidents red. Rajdhani provides tactical headings, Inter readable copy, and JetBrains Mono telemetry. Layout consists of a top connection header, node rail, threat/health hero, sensor grid, charts, tactical map/log, then a future roadmap.

## 12. Test plan

| Test | Steps | Expected result |
|---|---|---|
| Live telemetry | Publish valid telemetry JSON | CP3 cards/gauge/chart update |
| Packet validation | Publish invalid JSON or wrong envelope | UI remains functional; bad-packet count rises |
| Threat alert | Use SIM L3/L4 or publish alert | banner, tone, edge flash, log row and permanent pin |
| Charts | Wait for samples; pause chart | three charts update; paused history stops |
| Map evidence | Click a log row; export pins | map focuses pin; JSON downloads |
| Event tools | filter, search and export CSV | matching records shown; CSV downloads |
| Heartbeat loss | Press Heartbeat loss | DEGRADED then OFFLINE, lost event/tone |
| Recovery | Reset demo after loss | ONLINE and RECOVERED event |
| GPS lost | Press GPS lost | STORED badge and configured coordinates |
| SMS failed | Press SMS failed | failed status and alert event |
| Firebase offline | Press Firebase offline | offline badge; local dashboard remains usable |
| Mobile layout | narrow browser to 390 px | cards stack/readably reflow |

## 13. Risks and mitigation

Public MQTT topics may receive unrelated traffic: use a unique production prefix and validate envelopes. Browser WebSocket availability and broker uptime can fail: DEMO mode preserves a viva. GPS can be absent: stored-position labeling avoids false live precision. Firebase rules can expose data: use authenticated writes and deploy restrictive production rules. The HC-SR04 / MQ-2 require calibration and are unsuitable as sole safety decisions.

## 14. Firebase rules

Use public reads only if project policy permits, and authenticated writes:

```json
{"rules":{".read":true,".write":"auth != null"}}
```

Do not leave Firebase in test mode. For deployment, restrict reads/writes further to intended paths and authorized roles.

## 15. Deployment

Commit the three static dashboard files to a GitHub repository. In GitHub Pages, select the branch root source. Open the generated HTTPS URL, then set the MQTT broker and topic in Settings. HTTPS is required by many browser security policies for production integrations.

## 16. Future roadmap

ESP32-CAM human detection, LoRa mesh relay, TinyML human/animal classification, solar node power, thermal camera, TLS-authenticated MQTT, two-way SMS commands and a field mobile application are planned extensions.

## 17. Glossary

- **CP3** — Check Post 3.
- **MQTT** — lightweight publish/subscribe telemetry protocol.
- **WSS** — MQTT WebSocket transport protected by TLS.
- **LWT** — MQTT Last Will, used to report unplanned disconnects.
- **CSQ** — GSM signal-quality value from 0 to 31.
- **RTDB** — Firebase Realtime Database.
