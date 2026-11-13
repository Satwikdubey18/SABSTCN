# SABSTCN Command Center

Static command-center dashboard for the Smart Autonomous Border Surveillance & Tactical Communication Network, MITS Gwalior ECE.

## Run

1. Double-click `index.html` or publish these files with GitHub Pages.
2. It opens in **DEMO** mode, which needs no hardware. Use the Settings gear for L1–L4 and failure simulations.
3. To connect hardware, select **LIVE**, open Settings, enter the WebSocket broker and matching topic prefix, then save.

No build step, server, account, or hardcoded secret is required. Internet access is needed for the CDN assets and map tiles.

## ESP32 connection

The default broker is `wss://broker.hivemq.com:8884/mqtt`; default prefix is `sabstcn/cp3`. The ESP32 should publish to:

```text
sabstcn/cp3/telemetry
sabstcn/cp3/alert
sabstcn/cp3/heartbeat
sabstcn/cp3/status        # retained online; Last Will offline
```

Use a unique prefix for a real deployment. The dashboard subscribes to `<prefix>/+`, accepts only node `CP3`, and ignores malformed messages.

### Telemetry example

```json
{"v":1,"node":"CP3","type":"telemetry","ts":1760000000,"data":{"level":0,"pir":false,"dist":-1,"smoke":124,"base":120,"lat":26.2170,"lng":78.1828,"gps":"LIVE","batt":4.08,"rssi":-54}}
```

### Alert example

```json
{"v":1,"node":"CP3","type":"alert","ts":1760000002,"data":{"level":3,"trigger":"OBJECT","lat":26.2172,"lng":78.1831,"gps":"LIVE","sms":"SENT"}}
```

### Heartbeat example

```json
{"v":1,"node":"CP3","type":"heartbeat","ts":1760000010,"data":{"uptime":86400,"rssi":-54,"batt":4.08,"heap":186432,"boots":1,"fw":"1.0.0","csq":24,"sim":"REGISTERED"}}
```

## Firebase (optional)

Paste the Firebase Web app configuration JSON in Settings, enable Cloud Sync and save. The browser writes `/nodes/CP3/latest`, `/heartbeats/CP3`, and alert/level-change records under `/events/<pushId>`. Keep Firebase configuration in browser storage only as appropriate for your project and never leave database test mode enabled.

## Viva quick demo (10 lines)

1. Open `index.html`; introduce CP3 and the DEMO indicator.
2. Point out L0, sensor cards and green 24-hour availability.
3. Trigger **SIM L1** to show motion/smoke observation.
4. Trigger **SIM L2** to show the 5–10 cm proximity warning.
5. Trigger **SIM L3** to show GPS/SMS escalation, banner, event and red map pin.
6. Trigger **SIM L4** to show critical object-plus-smoke handling.
7. Click the L4 log row and show it focuses the permanent map evidence.
8. Trigger **GPS LOST** and explain the explicit STORED fallback badge.
9. Trigger **HEARTBEAT LOSS** and explain the online/degraded/offline watchdog.
10. Switch to LIVE and show the MQTT fields that connect the actual ESP32.
