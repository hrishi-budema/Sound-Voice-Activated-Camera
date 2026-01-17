#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <Preferences.h>

Preferences preferences;

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define TRIGGER_PIN 3
#define FLASH_PIN   4
bool armed = true;

camera_fb_t* captureStable() {
  camera_fb_t *tmp = esp_camera_fb_get();
  if (tmp) esp_camera_fb_return(tmp);
  delay(120);
  return esp_camera_fb_get();
}

bool saveJpeg(uint32_t n, const uint8_t *buf, size_t len) {
  char path[32];
  snprintf(path, sizeof(path), "/PIC%04lu.jpg", (unsigned long)n);
  File f = SD_MMC.open(path, FILE_WRITE);
  if (!f) return false;
  size_t w = f.write(buf, len);
  f.flush();
  f.close();
  return (w == len);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);
  pinMode(TRIGGER_PIN, INPUT);
  if (!SD_MMC.begin("/sdcard", true) || SD_MMC.cardType() == CARD_NONE) {
    while (true) {
      digitalWrite(FLASH_PIN, HIGH); delay(100);
      digitalWrite(FLASH_PIN, LOW);  delay(100);
    }
  }
  preferences.begin("SD", false);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count     = 2;
  } else {
    config.frame_size   = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count     = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    while (true) {
      digitalWrite(FLASH_PIN, HIGH); delay(400);
      digitalWrite(FLASH_PIN, LOW);  delay(400);
    }
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 1);
    s->set_contrast(s, 1);
    s->set_saturation(s, -1);
  }

  digitalWrite(FLASH_PIN, HIGH); delay(200);
  digitalWrite(FLASH_PIN, LOW);  delay(200);
  digitalWrite(FLASH_PIN, HIGH); delay(200);
  digitalWrite(FLASH_PIN, LOW);  delay(200);
}

void loop() {
  int t = digitalRead(TRIGGER_PIN);
  if (t == LOW) {
    armed = true;
    delay(2);
    return;
  }

  if (t == HIGH && armed) {
    armed = false;
    digitalWrite(FLASH_PIN, HIGH);
    delay(120);

    camera_fb_t *fb = captureStable();
    digitalWrite(FLASH_PIN, LOW);
    if (!fb) return;

    uint32_t n = preferences.getUInt("number", 0) + 1;
    delay(150);

    bool ok = saveJpeg(n, fb->buf, fb->len);
    esp_camera_fb_return(fb);
    if (ok) preferences.putUInt("number", n);

    delay(2000);
  }

  delay(2);
}

