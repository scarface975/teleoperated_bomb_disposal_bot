# Teleoperated Bomb Disposal Bot – Web UI

A neat, purpose-built frontend that controls your existing firmware endpoints without changing any of your `Arm_car_code.ino` logic. It adds a live video panel compatible with ESP32-CAM MJPEG streaming.

## What it talks to

This UI assumes your device exposes the same endpoints already present in your sketch:

- `GET /drive?cmd=F|B|L|R|S[&speed=0..100]`
- `GET /setServo?joint=shoulder|elbow|grip&angle=0..180`

No firmware changes required.

## How to use

1. Open `web/index.html` in Chrome/Edge (double-click or drag into the browser).
2. In the top-right, enter your device base URL (e.g. `http://192.168.1.42`) and click Connect.
3. Optional: For live video, paste your stream URL (e.g. `http://192.168.1.60:81/stream` for ESP32‑CAM) and click Start. If you only enter the IP, the page will attempt common endpoints automatically (`:81/stream`, `/stream`, `/mjpeg`, `/video`, `/?action=stream`). If none match a raw MJPEG stream it will try a snapshot polling fallback (`/capture`, `/snapshot`, `/snapshot.jpg`, etc.) at a configurable FPS slider (1–10, default 2). If that fails it embeds the root page in an iframe so you still see the camera UI.
4. Drive with the bidirectional throttle slider and Left/Right buttons. Stop square halts motors.
5. Set arm joints with the three sliders; Home sets all to 90°.

Keyboard shortcuts:
- W/S: increase/decrease throttle
- A/D: tap-turn left/right
- Space: emergency stop

## Notes

- The drive slider is centered (0). Positive values send `F`, negative send `B`, both with `speed=|value|`.
- Press-and-hold on Left/Right keeps turning; release to stop.
- Stream panel uses an `<img>` MJPEG viewer; it works with typical ESP32 / ESP32‑CAM firmwares that provide `/stream` or `/mjpeg` endpoints. If your stream uses WebRTC/HLS, you can still use the field to link an external viewer page.

## Troubleshooting

- If the status shows Offline, verify the IP, that you’re on the same Wi‑Fi, and that your device responds to `http://DEVICE_IP/`.
- If nothing moves, open the browser DevTools Network tab and confirm requests to `/drive` and `/setServo` are `200 OK`.
- For ESP32‑CAM streaming, ensure the camera firmware is flashed and the stream is reachable directly in a tab first.
