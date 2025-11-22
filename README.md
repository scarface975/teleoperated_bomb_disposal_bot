# Teleoperated Bomb Disposal Robot

## Project Overview

This project implements a comprehensive teleoperated robot system designed for explosive ordnance disposal (EOD), hazardous material inspection, confined space reconnaissance, and remote object manipulation. The system integrates three main hardware platforms: an ESP8266-based mobile platform with dual-motor drive and 3-DOF robotic arm, an ESP32-CAM for real-time video surveillance, and a sophisticated web-based control interface for remote operation over local area networks.

The architecture is modular and scalable, allowing operators to maintain safe distance during dangerous operations. All wireless communication occurs over 2.4 GHz WiFi on a shared local area network, with hardware safety constraints embedded at the firmware level (motor PWM limited to 40% maximum duty cycle to prevent burnout).

### Key Applications

- Explosive ordnance disposal (EOD) training and field operations
- Hazardous material inspection and sample collection
- Confined space and underground reconnaissance
- Remote object manipulation and transport
- Real-time surveillance in toxic or radioactive environments
- Post-incident damage assessment and evidence collection

---

## System Architecture

This section details the complete hardware and software stack of the teleoperated bot system.

### Hardware Components

#### Mobile Base Platform (ESP8266-based)

**Primary Microcontroller:**
- Device: ESP8266 (NodeMCU 1.0 or D1 Mini variant)
- Processing Power: 80 MHz or 160 MHz (configurable)
- RAM: 160 KB
- Flash Storage: 4 MB
- WiFi: 2.4 GHz 802.11 b/g/n
- I/O Pins: 11 usable GPIO pins (multiplex shared SPI/UART)

**Motor Drive System:**
- Motor Driver IC: L298N or L293D (dual H-bridge configuration)
- Drive Motors: Two DC motors (typically 3–6 V, 0.5–2 A each)
- Motor Connections: Motor A (left side) and Motor B (right side) via L298N pins IN1–IN4
- Speed Control: PWM on GPIO pins; limited to 40% maximum duty cycle (~409/1023 value)
- Enable Lines: Jumpered to 5V for full PWM control (alternative: separate enable pins possible)
- Rotation Scheme: Forward moves both motors in same direction; turning reverses one motor while keeping other forward (pivot turn)

**Robotic Arm Assembly (3-DOF Servo-based):**
- Joint 1 (Shoulder): Standard servo motor, 0–180° range, mounted on mobile base
- Joint 2 (Elbow): Standard servo motor, 0–180° range, connected to shoulder
- Joint 3 (Gripper): Standard servo motor, 0–180° range, end-effector for grasping
- Control Method: Parallel independent PWM signals from ESP8266 GPIO pins
- Mechanical Linkage: Rigid aluminum or 3D-printed segments; joint angles commanded independently
- Servo Specifications: Typical 180° servos with 2–4 kg-cm torque (model-dependent)
- Positioning: Closed-loop servo feedback; no external encoders required for basic positioning

**Power Distribution:**
- Main Battery: Typical 2S–3S LiPo (7.4–11.1 V) or equivalent rechargeable pack
- ESP8266 Supply: Via AMS1117 3.3V regulator (at least 500 mA capacity)
- Motor Supply: Direct from main battery (unregulated) to L298N
- Servo Supply: Either 5V regulated output from step-up converter or direct from battery through XT60/XT30 connector
- Total Current Draw: 2–4 A typical under full load (motors + arm + WiFi)
- Voltage Drop Compensation: Thick wire traces or power rails to minimize voltage sag during high-current transients

#### Camera Subsystem (ESP32-CAM)

**Camera Module:**
- Microcontroller: ESP32-S (dual-core Xtensa 32-bit, 160 MHz typical, configurable to 240 MHz)
- RAM: 520 KB SRAM + optional PSRAM (up to 8 MB external via SPI)
- Flash Storage: 4 MB via SPI
- Camera Sensor: OV2640 (2 megapixel) or OV3660 (3 megapixel) CMOS sensor
- Camera Resolution: Up to 2048×1536 (OV2640) or 2560×1920 (OV3660)

**Camera Firmware Configuration:**
- Default Resolution: VGA (640×480) for reduced bandwidth and faster frame rates
- JPEG Compression: Quality set to 15 (higher value = more compression, smaller file, lower quality)
- Frame Buffer Location: DRAM (not PSRAM) for reliable operation without external RAM
- Frame Count: 1 (single frame buffer to conserve memory)
- Pixel Format: JPEG (hardware compression at sensor level)
- XCLK Frequency: 20 MHz (standard, drives sensor timing)
- Grab Mode: CAMERA_GRAB_WHEN_EMPTY (waits for frame buffer to be ready before capturing)

**Camera Streaming:**
- Protocol: HTTP with multipart JPEG over TCP/IP (MJPEG)
- Endpoint: `/capture` (returns individual JPEG frames)
- Frame Rate: Configurable from 1–10 FPS in web UI
- Bandwidth Consumption: Approximately 1–5 Mbps depending on frame rate and JPEG quality
- Latency: 50–500 ms typical due to compression and WiFi transmission delays

**WiFi Integration:**
- Connection: Same 2.4 GHz network as ESP8266 mobile base
- IP Assignment: Static or DHCP (DHCP recommended for initial setup)
- Protocol Stack: Full TCP/IP with HTTP server
- Power Management: WiFi sleep disabled for consistent connectivity (higher power draw but better responsiveness)

#### Mechanical Integration

**Chassis Design:**
- Base Platform: Custom aluminum or 3D-printed frame accommodating motors, driver, and ESP8266
- Wheel Configuration: Two DC motors with wheels (typically 3" or 4" diameter for ground clearance)
- Arm Mounting: Servo shoulder joint bolted to top-center of chassis
- Center of Gravity: Positioned over the wheel axles to maximize stability during arm extension

**Servo Linkage:**
- Shoulder-to-Elbow: Rigid aluminum or composite link, approximately 150–200 mm length
- Elbow-to-Wrist: Similar rigid link, approximately 150–200 mm length
- Gripper Mechanism: Two-finger parallel gripper driven by servo, adjustable closing force

---

---

## Software Stack

This project consists of three main firmware applications and one web interface:

### 1. Firmware: Mobile Base Controller (ESP8266)

**File:** `Arm_car_code/Arm_car_code.ino`

**Language:** C++ (Arduino framework)

**Purpose:** Control the mobile platform (motors) and arm joints (servos) via HTTP requests from the web interface.

**Dependencies:**
- `<ESP8266WiFi.h>` – WiFi connectivity
- `<ESP8266WebServer.h>` – HTTP server implementation
- `<Servo.h>` – PWM servo control

**Configuration (lines 5–6):**
```cpp
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
```
Replace with actual WiFi credentials before flashing.

**Hardware Pin Configuration (lines 11–32):**

| Function | Pin Constant | GPIO | D-Pin | Purpose |
|----------|--------------|------|-------|---------|
| Shoulder Servo | PIN_SHOULDER | GPIO5 | D1 | Upper arm joint |
| Elbow Servo | PIN_ELBOW | GPIO4 | D2 | Lower arm joint |
| Gripper Servo | PIN_GRIPPER | GPIO12 | D6 | End-effector |
| Left Motor + | IN1 | GPIO13 | D7 | Forward/reverse left motor |
| Left Motor − | IN2 | GPIO15 | D8 | Forward/reverse left motor |
| Right Motor + | IN3 | GPIO0 | D3 | Forward/reverse right motor |
| Right Motor − | IN4 | GPIO2 | D4 | Forward/reverse right motor |

**Motor Control Logic (lines 86–131):**

The firmware implements four motion primitives:

1. **Forward (F):** Both motors driven forward at specified speed
   - IN1=0, IN2=PWM (left motor reversed polarity)
   - IN3=PWM, IN4=0 (right motor normal polarity)

2. **Backward (B):** Both motors driven backward
   - IN1=PWM, IN2=0 (left motor reversed)
   - IN3=0, IN4=PWM (right motor reversed)

3. **Left Turn (L):** Left motor forward, right motor forward (pivot turn)
   - IN1=PWM, IN2=0; IN3=PWM, IN4=0
   - Both motors same polarity = same-side rotation

4. **Right Turn (R):** Right motor backward, left motor forward (opposite turn)
   - IN1=0, IN2=PWM; IN3=0, IN4=PWM
   - Opposite polarity = opposite-side rotation

**PWM Duty Cycle Safety:**
```cpp
const int MAX_PWM_40 = 409;  // 40% of 1023 (Arduino PWM max)
```
All motor speeds clamped to 409 maximum to prevent motor burnout during continuous operation. Speed percentage (0–100) mapped proportionally to this range.

**Servo Control (lines 168–182):**
- Each joint (shoulder, elbow, grip) controlled independently
- Angle constrained to 0–180 degrees (standard servo range)
- Servo writes executed immediately upon HTTP request

**HTTP Endpoints:**

| Route | Method | Parameters | Response | Function |
|-------|--------|------------|----------|----------|
| `/` | GET | — | HTML page | Serve basic diagnostics page |
| `/setServo` | GET | `joint` (shoulder/elbow/grip), `angle` (0–180) | "OK" or error | Set arm joint angle |
| `/drive` | GET | `cmd` (F/B/L/R/S), `speed` (0–100 optional) | "OK" or error | Motor control |

**Example Requests:**
```
http://192.168.1.50/drive?cmd=F&speed=75           (Forward at 75% speed)
http://192.168.1.50/drive?cmd=L                    (Turn left, use last speed)
http://192.168.1.50/setServo?joint=shoulder&angle=120  (Raise shoulder to 120°)
```

**Initialization (lines 197–231):**
1. Serial debug output at 115200 baud
2. Servo attachment and initialization to 90° (neutral position)
3. Motor pin setup and stop state
4. WiFi connection (blocking loop until connected)
5. HTTP route registration and server start

**Main Loop (lines 233–235):**
```cpp
void loop() {
  server.handleClient();  // Process incoming HTTP requests
}
```
Blocking call; no concurrent background tasks. Single-threaded design sufficient for typical EOD operation speeds.

---

### 2. Firmware: Camera Subsystem (ESP32-CAM)

**File:** `Camera_code/CameraWebServer_copy_20251120220511/CameraWebServer_copy_20251120220511.ino`

**Language:** C++ (Arduino framework for ESP32)

**Purpose:** Capture video frames from OV2640/OV3660 sensor and stream them via HTTP to the web interface.

**Dependencies:**
- `<esp_camera.h>` – Camera sensor control
- `<WiFi.h>` – WiFi connectivity
- `board_config.h` – Camera pin mapping (model-specific)

**Configuration (lines 8–10):**
```cpp
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";
```
Must match the same WiFi network as the ESP8266 for both devices to communicate.

**Camera Initialization (lines 29–51):**

**Pinout Configuration (board_config.h):**
The camera uses 8-bit parallel data bus (D0–D7) plus control signals:
- D0–D7: Video data lines
- XCLK: Master clock from ESP32 to sensor (20 MHz)
- VSYNC: Vertical sync signal (frame boundary)
- HREF: Horizontal sync signal (line boundary)
- PCLK: Pixel clock (frame data timing)
- SDA/SCL: I2C control bus for camera register access

**Frame Buffer Configuration (lines 53–62):**
```cpp
config.pixel_format = PIXFORMAT_JPEG;       // Hardware JPEG compression
config.frame_size = FRAMESIZE_VGA;          // 640×480 default
config.fb_location = CAMERA_FB_IN_DRAM;     // Force DRAM (no PSRAM)
config.jpeg_quality = 15;                   // High compression (0–63 range)
config.fb_count = 1;                        // Single frame buffer
config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;  // Wait for buffer ready
```

**Quality vs. Bandwidth Trade-off:**
- Quality 15 produces ~30–50 KB JPEG at 640×480
- At 10 FPS = ~300–500 KB/s bandwidth (fits typical WiFi)
- Higher quality values reduce compression ratio (larger file size)
- Lower values increase artifacts but reduce bandwidth

**Sensor Post-Processing (lines 73–95):**
```cpp
sensor_t *s = esp_camera_sensor_get();
if (s->id.PID == OV3660_PID) {
  s->set_vflip(s, 1);        // Flip image vertically
  s->set_brightness(s, 1);   // +1 brightness adjustment
  s->set_saturation(s, -2);  // -2 saturation adjustment
}
```
Auto-detects sensor model and applies calibration. For OV3660 (3MP):
- Vertical flip corrects upside-down mount
- Brightness and saturation tuned for typical EOD lighting conditions

**Startup Sequence (lines 97–123):**
1. Serial initialization and diagnostics (free heap, SPIRAM detection)
2. Camera sensor init with config struct
3. Sensor-specific tweaks (brightness, saturation, vflip)
4. WiFi connection to specified network
5. Start HTTP camera server (provided by esp32-camera library)
6. Print IP address for access

**Camera Server Endpoints (built-in library):**
The `startCameraServer()` function (not shown; part of esp32-camera library) exposes:

| Endpoint | Response | Purpose |
|----------|----------|---------|
| `/` | HTML page | Root web interface |
| `/capture` | JPEG binary | Single still image |
| `/stream` | multipart/x-mixed-replace | MJPEG stream (can timeout) |
| `/jpg` | JPEG binary | Alternative still image |

**Main Loop (lines 125–128):**
```cpp
void loop() {
  delay(10000);  // Background camera tasks handle streaming
}
```
All camera I/O handled in background WiFi/HTTP tasks (FreeRTOS kernel). Main loop idle to save power and prevent watchdog timeout.

---

### 3. Web Interface (HTML5 / CSS3 / JavaScript)

**Directory:** `web/`

**Technology Stack:**
- HTML5 semantic markup with CSS3 Grid layout
- Vanilla JavaScript ES6+ (no external frameworks)
- Fetch API for asynchronous HTTP communication
- Responsive design for desktop and tablet browsers

#### 3.1 Main UI Structure (`index.html`)

**Layout Components:**

1. **Header Section:**
   - Title and connection status indicator
   - Device IP configuration fields

2. **Stream Panel (left column, full height):**
   - Live camera feed display
   - Fallback modes: raw MJPEG → snapshot polling → iframe → error message
   - Snapshot polling FPS slider (1–10)

3. **Drive Control Panel (right column, upper):**
   - Bidirectional throttle slider (center = stop, positive = forward, negative = reverse)
   - Left/Right pivot turn buttons
   - Emergency stop button
   - Speed percentage display

4. **Arm Control Panel (right column, lower):**
   - Three sliders for shoulder, elbow, gripper (0–180°)
   - Individual angle displays for each joint
   - "Home Pose" button (reset all joints to 90°)

#### 3.2 Styling (`styles.css`)

**Grid Layout:**
```css
.layout {
  display: grid;
  grid-template-columns: 1.5fr 1fr;
  grid-template-areas:
    "stream drive"
    "stream arm";
}
```
Two-column layout: stream occupies left column and spans two rows, drive/arm stack on right column. Ensures all controls visible without scrolling on 1920×1080+ displays.

**Color Scheme:**
- Background: `#1e293b` (slate-900, dark professional aesthetic)
- Accents: Cyan/blue gradients for active controls
- Text: `#f1f5f9` (slate-100, high contrast on dark background)

**Component Styling:**
- Cards: Rounded corners (12px), drop shadow for depth
- Sliders: Custom thumb styling with color indicators
- Buttons: Accessible size (40+ px height), clear hover states
- Stream frame: Fixed 460px height, maintains aspect ratio

#### 3.3 Control Logic (`app.js`)

**State Management:**
```javascript
const state = {
  robotIp: '192.168.1.50',
  cameraIp: '192.168.1.60',
  pollMode: false,
  pollFps: 2,
  pollIntervalId: null,
  isConnected: false,
  lastMotorCmd: null
};
```

**Stream Detection Algorithm:**

Three-tier fallback mechanism:

1. **MJPEG Stream Detection:**
   - Probes common endpoints: `/stream`, `/mjpeg`, `/video`, `/?action=stream`
   - Sets image src to endpoint and monitors for load success/failure
   - If successful, displays continuous stream

2. **Snapshot Polling Fallback:**
   - If MJPEG fails, probes still endpoints: `/capture`, `/snapshot`, `/jpg`
   - Sets up polling interval: `Math.max(1000 / pollFps, 100)` ms
   - Repeatedly fetches JPEG and updates img src
   - Dynamic FPS slider adjusts polling rate (1–10 FPS)

3. **Iframe Embed Fallback:**
   - If polling fails, embeds camera root URL in iframe
   - Allows camera's native web interface to display
   - Lowest bandwidth option but less controlled appearance

**Motor Control:**

Throttle slider (−100 to +100 range):
- Positive values: Forward motion at corresponding speed percentage
- Negative values: Reverse motion
- Zero: Stop
- Exclusive forward/reverse: Cannot move forward and backward simultaneously

Drive commands sent via: `/drive?cmd=F|B|S&speed=0..100`

Turning (left/right buttons):
- Execute pivot turns by reversing one motor
- Maintain last forward/reverse speed during turn
- Release button or click Stop to halt

**Servo Control:**

Three independent sliders (0–180° each):
- Shoulder: `/setServo?joint=shoulder&angle=X`
- Elbow: `/setServo?joint=elbow&angle=X`
- Gripper: `/setServo?joint=grip&angle=X`

Immediate feedback: angle values displayed below sliders update in real-time.

**Keyboard Shortcuts:**
```javascript
document.addEventListener('keydown', e => {
  if (e.key === 'w' || e.key === 'W') increaseThrottle();    // Forward
  if (e.key === 's' || e.key === 'S') decreaseThrottle();    // Reverse
  if (e.key === 'a' || e.key === 'A') turn('L');             // Left
  if (e.key === 'd' || e.key === 'D') turn('R');             // Right
  if (e.key === ' ') stop();                                 // Emergency stop
});
```

**Connection Status Pinging:**
Sends GET request to `/` every 3 seconds to verify device online. Updates UI status indicator (green = online, red = offline).

---

## Hardware Pinout Reference

Complete pinout documentation for physical assembly and troubleshooting.

### ESP8266 (Mobile Base Controller)

**Package:** NodeMCU 1.0 (ESP-12E module)

**Servo Output Pins (PWM, 3.3V logic):**
```
PIN_SHOULDER (GPIO5, D1)  ---> Servo control wire (shoulder joint)
PIN_ELBOW    (GPIO4, D2)  ---> Servo control wire (elbow joint)
PIN_GRIPPER  (GPIO12, D6) ---> Servo control wire (gripper)
```

**Motor Control Pins (3.3V logic, drive L298N):**
```
IN1 (GPIO13, D7)  ---> L298N Input 1 (left motor +)
IN2 (GPIO15, D8)  ---> L298N Input 2 (left motor −)
IN3 (GPIO0, D3)   ---> L298N Input 3 (right motor +)
IN4 (GPIO2, D4)   ---> L298N Input 4 (right motor −)
```

**Power Connections:**
```
GND (both sides)  ---> Common ground with L298N and servos
3.3V (out)        ---> (Optional: logic power only)
5V (via regulator)---> (Optional: servo power if regulated)
```

**L298N Motor Driver Pinout (DIP-16 package):**
```
1  GND         16  +5V (logic power, optional)
2  IN1         15  IN4
3  IN2         14  IN3
4  -OUT1       13  +OUT3
5  +Out2       12  -Out4
6  GND         11  ENB (jumpered to 5V)
7  ENA         10  GND
8  GND         9   +12V (motor power)
```

**Motor Output Pinout:**
- OUT1 (pin 4) & OUT2 (pin 5): Left motor connections
- OUT3 (pin 13) & OUT4 (pin 12): Right motor connections
- Motor power (+12V to pin 9, GND to pin 6/11): Unregulated from battery

### Servo Power Distribution

**If PSRAM-equipped ESP32-CAM available:**
Servo power via 5V step-up converter from 3.3V logic rail (limited capacity).

**Standard setup (recommended):**
Servo 5V rail connected directly to main battery through XT30 or XT60 connector with inline fuse (15–20 A).

**Servo Signal Ground:**
All three servo signal grounds connected to ESP8266 GND rail to ensure common reference.

---

## Setup Instructions

Complete step-by-step guide for assembly, firmware installation, and network configuration.

### Prerequisites

**Hardware Requirements:**
- ESP8266 microcontroller (NodeMCU D1 Mini recommended)
- ESP32-CAM module with OV2640/OV3660 sensor
- L298N or L293D motor driver IC
- Two DC motors (3–12 V, 0.5–2 A typical)
- Three servo motors (5V, standard 180° range)
- 2S–3S LiPo battery pack (7.4–11.1 V) or equivalent rechargeable
- USB-UART adapter (CP2102 or CH340 chip recommended)
- Jumper wires, connectors (XT30/XT60 for power)
- Aluminum profile or 3D-printed frame for chassis

**Software Requirements:**
- Arduino IDE (1.8.19 or later)
- ESP8266 Board Support Package (via Boards Manager)
- ESP32 Board Support Package (via Boards Manager)
- esp32-camera library (or clone from GitHub: `espressif/arduino-esp32-cam`)
- USB drivers for UART adapter (auto-installed on modern Windows/Mac/Linux)

**Network Requirements:**
- 2.4 GHz WiFi network (802.11 b/g/n)
- Static IP assignment recommended (or note DHCP addresses)
- Network accessible from control station (no firewall blocking local traffic)

**Browser Requirements:**
- Chrome, Edge, Firefox, or Safari (ES6+ JavaScript support required)
- JavaScript enabled (required for web interface)
- Local HTTP server access (file:// protocol does not work due to CORS)

### Step 1: Install Arduino IDE and Board Support

1. Download Arduino IDE from `arduino.cc` and install.

2. Open Arduino IDE and go to **Preferences** (Ctrl+comma).

3. In "Additional Boards Manager URLs", add:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   http://dl.espressif.com/dl/package_esp32_index.json
   ```

4. Go to **Tools** → **Boards Manager**.

5. Search and install:
   - "esp8266 by ESP8266 Community" (latest version)
   - "esp32 by Espressif Systems" (latest version)

6. Restart Arduino IDE.

### Step 2: Flash Mobile Base Firmware (ESP8266)

1. Open `Arm_car_code/Arm_car_code.ino` in Arduino IDE.

2. Update WiFi credentials (lines 5–6):
   ```cpp
   const char* ssid     = "YOUR_ACTUAL_SSID";
   const char* password = "YOUR_ACTUAL_PASSWORD";
   ```

3. Go to **Tools** and configure:
   - Board: "NodeMCU 1.0 (ESP-12E Module)"
   - Upload Speed: 460800
   - CPU Frequency: 160 MHz (faster) or 80 MHz (lower power)
   - Flash Size: 4M (3M SPIFFS)
   - Port: COM port of your USB-UART adapter

4. Connect ESP8266 to UART adapter:
   ```
   ESP8266 GND  ---> UART GND
   ESP8266 RX   ---> UART TX
   ESP8266 TX   ---> UART RX
   ESP8266 VCC  ---> UART 3.3V (or separate 3.3V regulator)
   ```
   Alternatively, if board has USB-C, connect directly.

5. Click **Upload**. Wait for compilation and flash completion (30–60 seconds).

6. Open **Tools** → **Serial Monitor** (115200 baud). You should see:
   ```
   Connecting to WiFi.....
   Connected. IP: 192.168.1.50
   HTTP server started
   ```
   Note the IP address for later use.

7. Disconnect USB, connect motor power.

### Step 3: Flash Camera Firmware (ESP32-CAM)

1. Open `Camera_code/CameraWebServer_copy_20251120220511/CameraWebServer_copy_20251120220511.ino` in Arduino IDE.

2. Update WiFi credentials (same network as ESP8266):
   ```cpp
   const char *ssid = "YOUR_ACTUAL_SSID";
   const char *password = "YOUR_ACTUAL_PASSWORD";
   ```

3. Go to **Tools** and configure:
   - Board: "AI Thinker ESP32-CAM"
   - Upload Speed: 921600
   - CPU Frequency: 160 MHz
   - Flash Mode: DIO
   - Flash Size: 4M
   - Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
   - Port: COM port of your USB-UART adapter

4. Connect ESP32-CAM to UART adapter (same as ESP8266):
   ```
   ESP32-CAM GND   ---> UART GND
   ESP32-CAM RX    ---> UART TX
   ESP32-CAM TX    ---> UART RX
   ESP32-CAM 5V    ---> UART 5V (or separate regulator)
   ```

5. Before flashing, connect IO0 to GND (puts device in bootloader mode).

6. Click **Upload**. After successful flash, disconnect IO0 from GND and reset device.

7. Open **Tools** → **Serial Monitor** (115200 baud). You should see:
   ```
   WiFi connecting....
   WiFi connected
   Camera Ready! Use 'http://192.168.1.60' to connect
   ```
   Note this IP address for camera access.

8. Disconnect USB, connect camera power.

### Step 4: Host Web Interface Locally

Navigate to the `web/` directory in PowerShell and start an HTTP server:

**Option A: Python 3 (simplest)**
```powershell
cd "c:\Users\adiit\Documents\Teleoperated Bot\web"
python -m http.server 8080
```
Then open `http://localhost:8080` in browser.

**Option B: Node.js**
```powershell
npm install -g serve
serve -l 8080
```

**Option C: Built-in PowerShell Server (no external tools)**
```powershell
cd "c:\Users\adiit\Documents\Teleoperated Bot\web"

$h = [System.Net.HttpListener]::new()
$h.Prefixes.Add('http://localhost:8080/')
$h.Start()
Write-Host 'Serving http://localhost:8080 - Press Ctrl+C to stop'

while ($h.IsListening) {
  $ctx = $h.GetContext()
  $path = $ctx.Request.Url.LocalPath.TrimStart('/')
  if (-not $path) { $path = 'index.html' }
  $full = Join-Path $PWD $path
  
  if (Test-Path $full) {
    $bytes = [System.IO.File]::ReadAllBytes($full)
    $ct = if ($full.EndsWith('.js')) {'text/javascript'} `
          elseif ($full.EndsWith('.css')) {'text/css'} `
          else {'text/html'}
    $ctx.Response.ContentType = $ct
    $ctx.Response.OutputStream.Write($bytes, 0, $bytes.Length)
  } else {
    $ctx.Response.StatusCode = 404
  }
  $ctx.Response.Close()
}
```

### Step 5: Connect and Configure

1. Open browser to `http://localhost:8080`.

2. You should see the control dashboard with sections for:
   - Device IP configuration
   - Live camera feed (initially blank)
   - Drive controls (throttle, turn buttons)
   - Arm controls (three sliders)

3. Enter Robot IP (from Serial Monitor, e.g., `192.168.1.50`) in "Device IP" field.

4. Enter Camera IP (from Serial Monitor, e.g., `192.168.1.60`) in "Live Video" field.

5. Click **Connect**. Status should show "Online" (green).

6. Click **Start Camera**. Camera feed should populate with video or polling snapshot.

7. Test motors:
   - Drag throttle slider forward → robot should move forward
   - Click "Left" button → robot should turn left
   - Release or click "Stop" → robot should halt

8. Test arm:
   - Drag "Shoulder" slider → shoulder servo should move
   - Drag "Elbow" slider → elbow servo should move
   - Drag "Gripper" slider → gripper should open/close

### Step 6: Network Troubleshooting

If devices not responding:

1. Verify both ESP8266 and ESP32 connected to same WiFi:
   ```powershell
   # Check WiFi SSID in scope
   Get-NetConnectionProfile | Select-Object Name, InterfaceAlias
   ```

2. Ping ESP8266 from control station:
   ```powershell
   ping 192.168.1.50
   ```
   Should see replies (if ping enabled on ESP8266).

3. Open device IP directly in browser:
   ```
   http://192.168.1.50/     (ESP8266 status page)
   http://192.168.1.60/     (ESP32-CAM web interface)
   ```

4. Check firewall allows local network traffic:
   ```powershell
   Get-NetFirewallProfile -Profile Domain, Private | Set-NetFirewallProfile -Enabled $false
   ```
   (Careful: disables firewall. Only for trusted local networks.)

---

## Operation Guide

### Control Interface Overview

The web UI is organized into four primary sections:

#### 1. Connection Panel (Top)

**Device IP Configuration:**
- Input field for ESP8266 robot controller IP address
- Input field for ESP32-CAM IP address
- Status indicator (green = online, red = offline)
- Connect button to verify both devices are accessible

**Typical workflow:**
1. Power on robot and camera (both connect to WiFi)
2. Check Serial Monitor for assigned IP addresses
3. Enter IPs into web UI fields
4. Click "Connect" to verify both online
5. Begin operation

#### 2. Live Video Feed (Left Column)

**Auto-Detection Logic:**

When "Start Camera" is clicked, the system attempts:

1. **Raw MJPEG Stream:**
   - Probes `/stream`, `/mjpeg`, `/video`, `/?action=stream`
   - If successful, displays continuous video (lowest latency, highest bandwidth)

2. **Snapshot Polling (Fallback 1):**
   - If MJPEG unavailable, probes `/capture`, `/snapshot`, `/jpg`
   - Repeatedly fetches still JPEG and updates display
   - FPS slider (1–10) controls polling rate
   - Typical: 1–2 FPS for low-bandwidth operation, up to 10 FPS for LAN

3. **Iframe Embed (Fallback 2):**
   - If polling unavailable, embeds camera root page in iframe
   - Displays camera's native web interface
   - No operator control over format

4. **Error State:**
   - If all methods fail, displays error message
   - Suggests verifying camera IP and WiFi connection

**FPS Slider (Polling Mode Only):**
- Range: 1–10 FPS
- Adjustable in real-time while streaming
- Lower FPS: reduces bandwidth, increases latency
- Higher FPS: better responsiveness, higher bandwidth usage
- Recommendation for typical LAN: 5 FPS (good balance)

#### 3. Drive Control Panel (Upper Right)

**Throttle Slider:**
- Center (0%): Motor stop
- Forward (+1 to +100): Both motors drive forward proportionally
- Reverse (−1 to −100): Both motors drive backward proportionally
- Cannot be forward and backward simultaneously (exclusive)
- Real-time display of throttle percentage

**Turn Buttons (Left/Right):**
- Performs pivot turn in specified direction
- Maintains current throttle speed during turn
- Each click reverses one motor, drives other forward
- Release or click Stop to end turn
- Useful for precise positioning during object manipulation

**Stop Button:**
- Immediately halts all motors
- Equivalent to Space key or throttle slider to center
- Included for quick emergency response

**Keyboard Shortcuts:**
```
W   → Increase throttle (accelerate forward)
S   → Decrease throttle (accelerate backward)
A   → Quick left turn (tap for precise control)
D   → Quick right turn
Space → Emergency stop (all motors to 0)
```

#### 4. Arm Control Panel (Lower Right)

**Three Independent Sliders:**

1. **Shoulder (top slider):**
   - Range: 0–180°
   - 0° = fully lowered (arm points downward)
   - 90° = horizontal (arm level with body)
   - 180° = fully raised (arm points upward)
   - Use for gross positioning of arm height

2. **Elbow (middle slider):**
   - Range: 0–180°
   - 0° = fully extended (arm stretched straight)
   - 90° = bent 90 degrees (right angle)
   - 180° = fully folded (gripper near shoulder)
   - Use for approach angle and reach modification

3. **Gripper (bottom slider):**
   - Range: 0–180°
   - 0° = fully open (maximum grip opening)
   - 90° = neutral (half open/closed)
   - 180° = fully closed (gripper clamped)
   - Use for grasping force control
   - Note: Servo position reflects grip width; actual force depends on gripper mechanism and load

**Home Pose Button:**
- Resets all three joints to 90° simultaneously
- Useful for returning to neutral state before or after manipulation
- Safely positions arm for transport or standby

**Angle Displays:**
- Real-time readout of each joint's current angle in degrees
- Updated as sliders move for feedback verification

### Advanced Operation

#### Real-Time Control Sequences

**Example: Retrieve small object**

1. Drive robot near target (use throttle and turn controls)
2. Stop robot (Space or Stop button)
3. Raise shoulder to ~120° to get above object
4. Extend elbow to ~60° for forward reach
5. Open gripper fully (0°)
6. Approach object by fine-tuning shoulder/elbow
7. Close gripper to ~160° to grasp (adjust for object size)
8. Raise shoulder to ~60° to lift object
9. Drive to destination
10. Lower shoulder and open gripper to release
11. Return to home pose (90°/90°/90°)

#### Network Latency Compensation

Expected latencies:
- Command execution: 50–100 ms
- Video frame display: 100–300 ms
- Total round-trip: 150–400 ms

**Adaptation strategies:**
- Plan movements slightly ahead (anticipatory control)
- Avoid rapid button sequences; pause between commands
- Use keyboard shortcuts for smoother motion (W key hold vs. repeated clicks)
- For precise tasks, reduce throttle speed to allow reaction time
- Test network latency with simple forward/stop sequences first

#### EOD-Specific Protocols

For explosive ordnance disposal operations:

1. **Approach Phase:**
   - Move slowly (20–30% throttle)
   - Use video to maintain visual on target
   - Stop periodically to assess and adjust

2. **Manipulation Phase:**
   - Keep gripper open until final moment
   - Close gripper slowly to avoid sudden movements
   - Maintain contact with suspected device until confident in grip

3. **Retreat Phase:**
   - Verify gripper secure before movement
   - Drive backward slowly (reverse motion has higher inertia)
   - Keep video stream on target until safe distance achieved

4. **Safety Measures:**
   - Always have emergency stop ready (Space key)
   - Never reach full forward speed without line of sight
   - Test motion controls on dummy objects before live operations
   - Ensure WiFi signal strength good (−30 dBm or better)

---

## Safety Considerations

### Electrical Safety

1. **Power Supply Isolation:**
   - Main battery (7.4–11.1 V) isolated from control station
   - Only low-voltage signals (3.3V logic) cross network boundary
   - No lethal voltages accessible during normal operation

2. **Motor Current Limiting:**
   - Firmware limits PWM to 40% duty cycle maximum
   - Prevents motor thermal runaway during stalled conditions
   - Typical current draw: 1–2 A forward, up to 4 A during turns (depending on load)

3. **Servo Torque Limits:**
   - Servo stall current typically 500–800 mA per joint
   - Three servos total: 1.5–2.4 A under full load
   - Adequate for light objects; mechanical locks recommended for heavy loads

### Operational Safety

1. **Emergency Stop Mechanisms:**
   - Software: Space key or Stop button (halts both motors)
   - Hardware: Power switch or battery disconnect (hard shutdown)
   - Backup: Wireless kill switch (optional enhancement)

2. **Network Latency Awareness:**
   - Command delay of 50–500 ms depending on WiFi conditions
   - Do not assume instant motor response
   - Always plan movements with safety margin
   - Test in safe area before deployment

3. **Motion Constraints:**
   - Arm reaching beyond base footprint reduces stability
   - Long extension with heavy grip reduces precision (servo torque insufficient)
   - Recommend tests on mock targets before field operations

4. **Line of Sight:**
   - Maintain continuous camera feed monitoring
   - Video latency may obscure sudden obstacles
   - Conservative speed reduces collision risk
   - Test WiFi range before committing to deployment

5. **Mechanical Lockouts:**
   - Consider servo locking mechanisms for critical arm positions
   - Gripper force feedback would improve safety (not implemented)
   - Mechanical stops on arm joints prevent over-travel

### Environmental Considerations

1. **WiFi Range:**
   - Typical 2.4 GHz WiFi line-of-sight: 50–100 m (depending on antenna)
   - Through walls/metal: 10–30 m range reduction
   - Test WiFi strength before field deployment
   - Monitor signal strength indicator in control UI

2. **Interference:**
   - 2.4 GHz band shared with microwave ovens, cordless phones, Bluetooth
   - Potential interference in crowded WiFi environments (multiple networks)
   - Consider 5 GHz WiFi alternative if available (future enhancement)

3. **Temperature Tolerance:**
   - ESP8266/ESP32: −40°C to +85°C (nominal operation 0–70°C)
   - Servos: typically −10°C to +60°C (check datasheet)
   - Motors: −20°C to +50°C (check datasheet)
   - Battery: LiPo typically −20°C to +60°C (charge only 0–45°C)
   - For cold environments: pre-warm robot in warm case before deployment

4. **Moisture/Contamination:**
   - Electronics unprotected from water/dust
   - Consider conformal coating or potting for hazardous environments
   - Servo connectors exposed to corrosive substances (future enhancement: hermetic enclosure)

---

## Troubleshooting Guide

### Robot Offline / Won't Connect

**Symptoms:**
- Connection status shows red / "Offline"
- Ping to robot IP fails
- All control commands return no response

**Diagnosis & Resolution:**

1. **Verify device powered:**
   - Check LED indicators on ESP8266 board (should have WiFi indicator)
   - Measure voltage on 3.3V rail (should read 3.2–3.4 V)
   - Ensure battery fully charged (voltage above 3 V per cell for LiPo)

2. **Check WiFi connection:**
   - Connect USB-UART and open Serial Monitor at 115200 baud
   - Power-cycle robot; observe boot sequence in serial output
   - Should see: "Connecting to WiFi...", then "Connected. IP: XXX.XXX.XXX.XXX"
   - If stuck at "Connecting": SSID/password incorrect

3. **Verify network connectivity:**
   - From control station, ping robot IP:
     ```powershell
     ping 192.168.1.50
     ```
   - If no response, robot not on network (WiFi issue)
   - If reply received, network OK (may be HTTP server issue)

4. **Test HTTP directly:**
   - Open browser to `http://192.168.1.50/` (may show basic status page)
   - If page loads: HTTP server running (issue is web UI or configuration)
   - If page fails: HTTP server crashed; power-cycle and check serial output for errors

5. **Firewall check:**
   - Temporarily disable Windows Defender Firewall (Domain + Private profiles)
   - Attempt connection again
   - If successful: Firewall blocking local traffic (add exception or disable)

### Motors Not Responding

**Symptoms:**
- Throttle slider moves but robot doesn't move
- Turning buttons clicked but no rotation
- Stop button works (motors halt) but no forward/backward motion

**Diagnosis & Resolution:**

1. **Verify motor connections:**
   - Disconnect battery
   - Inspect L298N board for loose jumper wires or broken solder joints
   - Check motor leads for continuity with multimeter
   - Reattach any loose connectors

2. **Test motor power supply:**
   - Measure voltage on L298N +12V pin (battery through motor enable lines)
   - Should read close to battery voltage (7.2–11 V)
   - If 0V: battery not connected or fuse blown
   - If low voltage: check battery charge level

3. **Check GPIO-to-L298N connections:**
   - Verify IN1, IN2, IN3, IN4 wires connected to correct L298N pins
   - Test continuity: multimeter between ESP8266 pin and L298N input
   - Resolder if intermittent connection suspected

4. **Issue HTTP command directly:**
   - Open browser to `http://192.168.1.50/drive?cmd=F&speed=50`
   - Listen for motor sound (should have audible hum)
   - If motor spins: web UI has issue (JavaScript, Fetch API)
   - If no sound: HTTP command not reaching driver circuit

5. **Motor burnout check:**
   - Feel L298N IC and motors for excessive heat
   - If hot to touch: internal short circuit (likely failed H-bridge)
   - Replace L298N or swap motor terminals to isolate issue

### Servo / Arm Joints Not Moving

**Symptoms:**
- Arm control sliders move but servos don't respond
- Specific joint stuck (doesn't move while others respond)
- Servo makes buzzing sound but doesn't turn

**Diagnosis & Resolution:**

1. **Verify servo power:**
   - Measure 5V supply at servo connector
   - Should read 4.8–5.2 V (not 3.3V)
   - If wrong voltage: check step-up converter or battery connection
   - If 0V: servo power rail disconnected

2. **Check servo signal wires:**
   - Disconnect servo from servo plug
   - Use multimeter to verify continuity between GPIO pin and plug connector
   - Resolder if broken wire suspected

3. **Test servo in isolation:**
   - Disconnect servo from signal header
   - Connect servo to known-good PWM generator (Arduino, function generator)
   - Apply 90° pulse (1.5 ms) and observe rotation
   - If servo responds: GPIO output issue (check firmware)
   - If servo unresponsive: servo defective (replace)

4. **Check firmware servo attachment:**
   - Recompile and re-upload `Arm_car_code.ino` to ESP8266
   - Verify servo initialization prints to serial (or doesn't timeout)
   - Confirm `servoShoulder.attach(PIN_SHOULDER)` line matches actual GPIO

5. **Servo stall detection:**
   - Manually move servo horn to new position
   - Send angle command via web UI
   - If servo fights back: stall (motor working, load excessive)
   - If servo is floppy: detached or damaged gearbox
   - If silent/no movement: likely defective servo (replace)

### Camera Stream Not Displaying

**Symptoms:**
- Camera field remains blank after clicking "Start"
- "Stream not detected" error message appears
- Snapshot polling mode not activating

**Diagnosis & Resolution:**

1. **Verify camera powered:**
   - Check LED indicators on ESP32-CAM board
   - Should see WiFi symbol if connected; camera LED may flash
   - Measure 5V supply; should be 4.8–5.2 V
   - If no indicators: power not reaching device

2. **Test camera directly in browser:**
   - Open `http://192.168.1.60/` (camera root page) directly
   - Should see camera's native web interface (may include live view)
   - If page loads: Camera HTTP server working (issue is web UI fallback logic)
   - If page fails: Camera not on network (WiFi or serial output issue)

3. **Use DevTools to debug:**
   - Open browser DevTools (F12)
   - Go to **Network** tab
   - Click "Start Camera" in web UI
   - Observe HTTP requests:
     - Probing requests to `/stream`, `/mjpeg`, etc. (should see 404s if endpoints don't exist)
     - If seeing 200 responses: stream found (should display)
     - If all 404/timeout: camera not responding

4. **Check MJPEG endpoint:**
   - Try direct browser access: `http://192.168.1.60/stream`
   - If browser shows streaming video: MJPEG working (web UI should detect)
   - If timeout/error: endpoint doesn't exist (fallback to polling)

5. **Snapshot polling diagnostic:**
   - Try `/capture` directly: `http://192.168.1.60/capture` (should download JPEG)
   - If JPEG downloads: polling should work (check FPS slider is active)
   - If error: camera not exposing still endpoint
   - Use alternative `/snapshot` or `/jpg` if `/capture` unavailable

6. **WiFi signal quality:**
   - Check WiFi signal strength (look for RSSI in router settings or WiFi analyzer app)
   - Signal below −60 dBm may cause timeouts
   - Move camera closer to WiFi router or relocate router
   - Try 2.4 GHz band (some devices default to 5 GHz if available)

### Web UI Unresponsive / Control Commands Fail

**Symptoms:**
- Web page loads but controls don't respond
- Console errors in DevTools (F12 → Console)
- Buttons/sliders move but robot doesn't react

**Diagnosis & Resolution:**

1. **Verify HTTP server running:**
   - If using Python: check PowerShell window for "Serving HTTP on port 8080"
   - If using Node.js: check output shows "Accepting connections"
   - If using PowerShell: check script still running (hasn't exited)
   - If server exited: check error message, restart

2. **Check browser console for JavaScript errors:**
   - Open DevTools (F12)
   - Go to **Console** tab
   - Look for red error messages
   - Common errors:
     - "CORS error" → not served over HTTP (use Python/Node server, not file://)
     - "Fetch failed" → robot IP incorrect or offline
     - "Undefined variable" → missing app.js in web directory

3. **Verify IPs correctly configured:**
   - Check web UI form fields for robot and camera IPs
   - IP should match Serial Monitor output (e.g., 192.168.1.50)
   - Typos (e.g., 192.168.1.05 instead of 192.168.1.5) are common mistake
   - Verify connectivity with status indicator (should be green)

4. **Clear browser cache:**
   - Close and reopen browser tab
   - Or: Hard refresh (Ctrl+Shift+R) to bypass cache
   - Old JavaScript/CSS cached locally may cause stale behavior

5. **Try different browser:**
   - If Chrome fails: try Edge, Firefox, Safari
   - Different browsers may have different CORS/security policies
   - If one browser works: likely browser-specific cache or security setting

6. **Network latency check:**
   - Ping robot IP and note reply time:
     ```powershell
     ping -n 10 192.168.1.50 | Measure-Object -Property ResponseTime -Average
     ```
   - If average >200 ms: high latency (WiFi interference or distance)
   - Try moving control station closer to WiFi router or robot

---

## Performance Specifications

| Specification | Value | Notes |
|-----------|-------|-------|
| **Drive System** | | |
| Max forward speed | ~0.5 m/s | Limited by motor characteristics and 40% PWM |
| Max reverse speed | ~0.4 m/s | Slightly lower due to motor imbalance |
| Turn radius | ~0.3 m | Depends on wheel diameter and track width |
| Max payload | 2–5 kg | Depends on motor torque and battery capacity |
| **Arm System** | | |
| Shoulder reach | 0–180° | Full vertical range possible |
| Elbow reach | 0–180° | Full articulation available |
| Gripper opening | 0–180° | Full range controls grip width |
| Max gripper force | 1–2 kg-force | Servo-dependent; typical 2 kg-cm torque |
| Arm reach (max extension) | 0.5–1.0 m | Depends on link lengths |
| **Camera System** | | |
| Resolution | VGA (640×480) | Configurable up to SVGA (800×600) |
| Frame rate (MJPEG) | 15–30 FPS | Limited by camera sensor and bandwidth |
| Frame rate (polling) | 1–10 FPS | User-configurable in web UI |
| Latency (MJPEG) | 50–150 ms | Network and compression dependent |
| Latency (polling) | 100–300 ms | FPS slider controls this |
| **Network** | | |
| WiFi band | 2.4 GHz | IEEE 802.11 b/g/n |
| Max range | 50–100 m | LOS; reduced through obstacles |
| Latency (command) | 50–200 ms | Typical local network conditions |
| Bandwidth (drive) | ~1 KB/s | Minimal; HTTP request + response |
| Bandwidth (camera) | 1–5 Mbps | Depending on FPS and JPEG quality |
| **Power** | | |
| Battery voltage | 7.4–11.1 V | 2S–3S LiPo typical |
| Motor current | 1–2 A (normal) | Up to 4 A during stall/turn |
| Servo current | 0.5–0.8 A (total) | ~300 mA per joint at full extension |
| WiFi current | 70–150 mA | Depends on TX power and activity |
| Total draw | 2–4 A typical | Can exceed 5 A during full motion |
| Runtime | 1–4 hours | Depends on battery capacity and duty cycle |

---

## Future Enhancements

Potential improvements for future versions:

### Hardware Upgrades

- **6-DOF Arm:** Add wrist pitch and roll joints for full 3D positioning
- **Gripper Force Feedback:** Integrate load cell for precision grasping
- **LIDAR/Ultrasonic Sensors:** Autonomous navigation and obstacle detection
- **Thermal Camera:** Dual sensor for temperature measurement in EOD/hazmat
- **Multi-Rotor Camera Mount:** Pan/tilt stabilization for better video control
- **Wireless Kill Switch:** Independent RF circuit for hard emergency stop
- **Battery Monitoring:** Voltage/current telemetry to web UI

### Software Enhancements

- **WebRTC Streaming:** Lower-latency video (< 1 second) using real-time protocol
- **HLS Streaming:** Better compatibility with cellular/remote networks
- **Autonomous Waypoint Navigation:** Path planning and obstacle avoidance
- **ML-Based Object Detection:** Automated target identification and classification
- **Mobile App:** React Native or Flutter client for iOS/Android control
- **Multi-Robot Coordination:** Swarm control for coordinated multi-unit operations
- **Mission Logging:** SD card recording of all commands, telemetry, and video
- **Haptic Feedback:** Vibration motor on controller for tactile confirmation
- **Voice Control:** Wake-word activated verbal commands
- **Offline Operation:** Local mesh network fallback if main WiFi lost

### Mechanical Improvements

- **Hermetic Enclosures:** Waterproof/dustproof cases for hazardous environments
- **Mechanical Arm Locking:** Servo-driven brake for load-holding capability
- **Shock Absorption:** Spring/damper mounts for rough terrain
- **Modular Gripper:** Quick-change end-effector system (magnetic coupling)
- **All-Terrain Tires:** Improved grip on sand, gravel, rubble

---

## Project Structure

```
Teleoperated Bot/
├── Arm_car_code/
│   └── Arm_car_code.ino              # ESP8266 motor + servo controller
├── Camera_code/
│   └── CameraWebServer_copy_20251120220511/
│       └── CameraWebServer_copy_20251120220511.ino  # ESP32-CAM firmware
├── web/
│   ├── index.html                    # Main UI structure
│   ├── styles.css                    # Responsive styling
│   ├── app.js                        # Control logic & stream handling
│   └── README.md                     # Web UI documentation
├── README.md                         # This file (project overview)
└── .gitignore                        # Git exclusion patterns
```

---

## Contributors and Acknowledgments

This project integrates:
- Arduino ESP8266 core (Arduino community)
- Espressif ESP32-CAM camera library (Espressif Systems)
- L298N motor driver design principles (standard H-bridge)
- Open-source web technologies (HTML5/CSS3/JavaScript)

---

## License

This project is provided as-is for educational and authorized explosive ordnance disposal training purposes. Unauthorized use for surveillance, intrusion, property damage, or any illegal activity is strictly prohibited.

All hardware assemblies and software implementations remain the responsibility of the operator and their organization. Safety-critical operations (EOD, hazmat) must follow jurisdictional regulations and institutional safety protocols.

---

## Contact & Support

For questions, bug reports, or enhancement suggestions:
- Check `Troubleshooting Guide` section above for common issues
- Review hardware pin configurations if devices not responding
- Verify WiFi credentials and network topology in setup steps
- Test individual components (motors, servos, camera) in isolation before full integration

---

**Project Version:** 1.1  
**Last Updated:** November 2025  
**Status:** Active Development  
**Tested Hardware:** ESP8266 (NodeMCU), ESP32-CAM (AI Thinker), L298N, OV2640/OV3660  
**Target Use:** EOD Training, Hazmat Inspection, Remote Reconnaissance



