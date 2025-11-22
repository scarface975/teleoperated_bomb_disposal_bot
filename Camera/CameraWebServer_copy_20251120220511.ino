#include "esp_camera.h"
#include <WiFi.h>

// Select camera model in board_config.h (make sure CAMERA_MODEL_AI_THINKER is selected)
#include "board_config.h"

// ==== WiFi credentials - fill these ====
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

// Forward declarations (these are provided by the example library)
void startCameraServer();
void setupLedFlash();

void setup() {
  // Initialize serial first
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(50);

  // Diagnostics
  Serial.printf("Free heap: %u\n", esp_get_free_heap_size());
  Serial.printf("psramFound(): %d\n", psramFound());
  #ifdef CONFIG_SPIRAM_SUPPORT
  Serial.printf("SPIRAM support compiled\n");
  #endif
  Serial.println();

  // CAMERA CONFIGURATION
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  // XCLK freq (keep default)
  config.xclk_freq_hz = 20000000;

  // ===== Option B (No PSRAM) - force DRAM usage and smaller frame =====
  // Use a conservative default so web server doesn't need PSRAM
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;     // use VGA or SVGA/QVGA if you need less memory
  config.fb_location = CAMERA_FB_IN_DRAM; // force DRAM
  config.jpeg_quality = 15;              // higher = smaller images (lower quality)
  config.fb_count = 1;                   // must be 1 when no PSRAM
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  // If you do actually have PSRAM and want to use it, you could enable here.
  // But since you requested Option B (no PSRAM), we force DRAM settings above.
  // End camera config

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  // Optional sensor tweaks
  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // start with a smaller framesize for initial framerate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

  // Setup LED flash if defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  // Connect WiFi
  Serial.print("WiFi connecting");
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");

  // Start web server (from example)
  startCameraServer();

  Serial.printf("Camera Ready! Use 'http://%s' to connect\n", WiFi.localIP().toString().c_str());
}

void loop() {
  // All camera handling is done in server tasks
  delay(10000);
}
