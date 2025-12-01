#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ==== CONFIGURE WIFI ====
const char* ssid = "YOUR_WIFI_SSID";    
const char* password = "YOUR_WIFI_PASSWORD";   

// ==== CAMERA MODEL ====
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Web server on port 80
WebServer server(80);

// ==== CAMERA SETUP ====
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Camera config
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Better quality settings for face recognition
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;   // 640x480
    config.jpeg_quality = 10;            // Better quality (lower number = better)
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;  // 320x240
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Camera sensor settings for better face recognition
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0=No Effect)
    s->set_whitebal(s, 1);       // 0=disable , 1=enable
    s->set_awb_gain(s, 1);       // 0=disable , 1=enable
    s->set_wb_mode(s, 0);        // 0 to 4 (if awb_gain enabled)
    s->set_exposure_ctrl(s, 1);  // 0=disable , 1=enable
    s->set_aec2(s, 0);           // 0=disable , 1=enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0=disable , 1=enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0=disable , 1=enable
    s->set_wpc(s, 1);            // 0=disable , 1=enable
    s->set_raw_gma(s, 1);        // 0=disable , 1=enable
    s->set_lenc(s, 1);           // 0=disable , 1=enable
    s->set_hmirror(s, 0);        // 0=disable , 1=enable
    s->set_vflip(s, 0);          // 0=disable , 1=enable
    s->set_dcw(s, 1);            // 0=disable , 1=enable
    s->set_colorbar(s, 0);       // 0=disable , 1=enable
  }

  // WiFi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("Camera IP: ");
  Serial.println(WiFi.localIP());
  
  // Setup web server routes
  server.on("/", HTTP_GET, [](){
    String html = "<html><body>";
    html += "<h1>ESP32 Camera Server</h1>";
    html += "<p>Camera IP: " + WiFi.localIP().toString() + "</p>";
    html += "<p>Available endpoints:</p>";
    html += "<ul>";
    html += "<li><a href='/cam-lo.jpg'>/cam-lo.jpg</a> - Low resolution image</li>";
    html += "<li><a href='/cam-hi.jpg'>/cam-hi.jpg</a> - High resolution image</li>";
    html += "<li><a href='/capture'>/capture</a> - Capture image</li>";
    html += "</ul>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  // Low resolution image endpoint (for Python face recognition)
  server.on("/cam-lo.jpg", HTTP_GET, [](){
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      server.send(500, "text/plain", "Camera capture failed");
      return;
    }
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  });

  // High resolution image endpoint
  server.on("/cam-hi.jpg", HTTP_GET, [](){
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      server.send(500, "text/plain", "Camera capture failed");
      return;
    }
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  });

  // Alternative capture endpoint
  server.on("/capture", HTTP_GET, [](){
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      server.send(500, "text/plain", "Camera capture failed");
      return;
    }
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Ready for face recognition!");
  Serial.println("Test URLs:");
  Serial.println("http://" + WiFi.localIP().toString() + "/cam-lo.jpg");
  Serial.println("http://" + WiFi.localIP().toString() + "/cam-hi.jpg");
}

void loop() {
  server.handleClient();
  delay(1);

}
