#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// -------- WiFi CONFIG --------
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// -------- WEB SERVER --------
ESP8266WebServer server(80);

// -------- SERVO CONFIG --------
Servo servoShoulder;
Servo servoElbow;
Servo servoGripper;

// NodeMCU D-pins
const int PIN_SHOULDER   = D1; // GPIO5
const int PIN_ELBOW      = D2; // GPIO4
const int PIN_GRIPPER    = D6; // GPIO12

int shoulderAngle   = 90;
int elbowAngle      = 90;
int gripperAngle    = 90;

// -------- CAR / MOTOR CONFIG --------
// L298N / L293D, ENA & ENB jumpered to 5V
const int IN1 = D7; // GPIO13  -> Left motor +
const int IN2 = D8; // GPIO15  -> Left motor -
const int IN3 = D3; // GPIO0   -> Right motor +
const int IN4 = D4; // GPIO2   -> Right motor -

// PWM: use 40% of full (0–1023 -> max ~409)
const int MAX_PWM_40 = 409;
int lastPwm = 0;

// -------- HTML PAGE --------
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>ESP8266 Arm + Car</title>
<style>
body { font-family: Arial, sans-serif; margin: 20px; }
h2 { margin-top: 30px; }
.slider-container { margin-bottom: 15px; }
label { display: inline-block; width: 140px; }
button { width: 80px; height: 40px; margin: 5px; font-size: 14px; }
#car-controls { margin-top: 10px; }
</style>
</head>
<body>
<h1>ESP8266 Robot Arm & Car</h1>

<h2>Arm Control</h2>

<div class="slider-container">
  <label>Shoulder</label>
  <input type="range" min="0" max="180" value="90" id="shoulder"
         oninput="setServo('shoulder', this.value)">
  <span id="shoulderVal">90</span>
</div>

<div class="slider-container">
  <label>Elbow</label>
  <input type="range" min="0" max="180" value="90" id="elbow"
         oninput="setServo('elbow', this.value)">
  <span id="elbowVal">90</span>
</div>

<div class="slider-container">
  <label>Gripper</label>
  <input type="range" min="0" max="180" value="90" id="grip"
         oninput="setServo('grip', this.value)">
  <span id="gripVal">90</span>
</div>

<h2>Car Control</h2>

<div id="car-controls">
  <div class="slider-container">
    <label>Forward speed (%)</label>
    <input type="range" min="0" max="100" value="0" id="fwd"
           oninput="setDrive('F', this.value)">
    <span id="fwdVal">0</span>
  </div>

  <div class="slider-container">
    <label>Reverse speed (%)</label>
    <input type="range" min="0" max="100" value="0" id="rev"
           oninput="setDrive('B', this.value)">
    <span id="revVal">0</span>
  </div>

  <div>
    <button onclick="turn('L')">Left</button>
    <button onclick="stopCar()">Stop</button>
    <button onclick="turn('R')">Right</button>
  </div>
</div>

<script>
function setServo(joint, angle) {
  document.getElementById(joint + "Val").innerText = angle;
  fetch(`/setServo?joint=${joint}&angle=${angle}`).catch(err => console.log(err));
}

function setDrive(dir, val) {
  if (dir === 'F') {
    document.getElementById("fwdVal").innerText = val;
    if (val > 0) document.getElementById("rev").value = 0, document.getElementById("revVal").innerText = 0;
  } else if (dir === 'B') {
    document.getElementById("revVal").innerText = val;
    if (val > 0) document.getElementById("fwd").value = 0, document.getElementById("fwdVal").innerText = 0;
  }
  fetch(`/drive?cmd=${dir}&speed=${val}`).catch(err => console.log(err));
}

function turn(dir) {
  fetch(`/drive?cmd=${dir}`).catch(err => console.log(err));
}

function stopCar() {
  document.getElementById("fwd").value = 0;
  document.getElementById("fwdVal").innerText = 0;
  document.getElementById("rev").value = 0;
  document.getElementById("revVal").innerText = 0;
  fetch(`/drive?cmd=S`).catch(err => console.log(err));
}
</script>
</body>
</html>
)rawliteral";

// -------- MOTOR FUNCTIONS (Fixed left motor pointing) --------
void setMotorStop() {
  analogWrite(IN1, 0);
  analogWrite(IN2, 0);
  analogWrite(IN3, 0);
  analogWrite(IN4, 0);
}

void setMotorForward(int pwm) {
  pwm = constrain(pwm, 0, MAX_PWM_40);
  analogWrite(IN1, 0);    // Left motor reversed polarity
  analogWrite(IN2, pwm);  // (inverted logic)
  analogWrite(IN3, pwm);  // Right motor normal
  analogWrite(IN4, 0);
}

void setMotorBackward(int pwm) {
  pwm = constrain(pwm, 0, MAX_PWM_40);
  analogWrite(IN1, pwm);  // Left reversed
  analogWrite(IN2, 0);
  analogWrite(IN3, 0);    // Right reversed  
  analogWrite(IN4, pwm);
}

void setMotorLeft(int pwm) {
  pwm = constrain(pwm, 0, MAX_PWM_40);
  analogWrite(IN1, pwm);
  analogWrite(IN2, 0);
  analogWrite(IN3, pwm);
  analogWrite(IN4, 0);
}

void setMotorRight(int pwm) {
  pwm = constrain(pwm, 0, MAX_PWM_40);
  analogWrite(IN1, 0);
  analogWrite(IN2, pwm);
  analogWrite(IN3, 0);
  analogWrite(IN4, pwm);
}

// -------- HTTP HANDLERS --------
void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

void handleSetServo() {
  if (!server.hasArg("joint") || !server.hasArg("angle")) {
    server.send(400, "text/plain", "Missing args");
    return;
  }

  String joint = server.arg("joint");
  int angle = server.arg("angle").toInt();
  angle = constrain(angle, 0, 180);

  if (joint == "shoulder") {
    shoulderAngle = angle;
    servoShoulder.write(angle);
  } else if (joint == "elbow") {
    elbowAngle = angle;
    servoElbow.write(angle);
  } else if (joint == "grip") {
    gripperAngle = angle;
    servoGripper.write(angle);
  }

  server.send(200, "text/plain", "OK");
}

void handleDrive() {
  if (!server.hasArg("cmd")) {
    server.send(400, "text/plain", "Missing cmd");
    return;
  }

  char cmd = server.arg("cmd")[0];
  int speedPercent = 0;

  if (server.hasArg("speed")) {
    speedPercent = server.arg("speed").toInt();
    speedPercent = constrain(speedPercent, 0, 100);
  }

  int pwm = map(speedPercent, 0, 100, 0, MAX_PWM_40);

  switch (cmd) {
    case 'F':
      lastPwm = pwm;
      if (pwm == 0) setMotorStop();
      else setMotorForward(pwm);
      break;

    case 'B':
      lastPwm = pwm;
      if (pwm == 0) setMotorStop();
      else setMotorBackward(pwm);
      break;

    case 'L':
      if (lastPwm == 0) lastPwm = MAX_PWM_40 / 2;
      setMotorLeft(lastPwm);
      break;

    case 'R':
      if (lastPwm == 0) lastPwm = MAX_PWM_40 / 2;
      setMotorRight(lastPwm);
      break;

    case 'S':
    default:
      setMotorStop();
      break;
  }

  server.send(200, "text/plain", "OK");
}

// -------- SETUP & LOOP --------
void setup() {
  Serial.begin(115200);

  // Servos
  servoShoulder.attach(PIN_SHOULDER);
  servoElbow.attach(PIN_ELBOW);
  servoGripper.attach(PIN_GRIPPER);

  servoShoulder.write(shoulderAngle);
  servoElbow.write(elbowAngle);
  servoGripper.write(gripperAngle);

  // Motors
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  setMotorStop();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

  // Web routes
  server.on("/", handleRoot);
  server.on("/setServo", handleSetServo);
  server.on("/drive", handleDrive);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
