#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Unfortunately, the board's rgb led and built in led share the same gpio
const uint8_t pin_rgb_led = 48;
const uint8_t pin_built_in_led = 48;
const uint8_t rgb_led_count = 1;

Adafruit_NeoPixel rgb_led(rgb_led_count, pin_rgb_led, NEO_GRB + NEO_KHZ800);

// put function declarations here:

// Triangle wave: ramps amplitude -> 0 -> amplitude over period_ms, offset_ms shifts the phase
uint8_t triangle_wave(uint32_t time_ms, uint32_t period_ms, uint8_t amplitude, uint32_t offset_ms) {
  uint32_t half = period_ms / 2;
  uint32_t t = (time_ms + offset_ms) % period_ms;
  return (abs((int) (t - half)) * amplitude) / half;
}

void print_device_info() {
  uint64_t mac = ESP.getEfuseMac();

  printf("\n=== Device Info ===\n");
  printf("Chip model:    %s rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  printf("CPU:           %d core(s) @ %u MHz (xtal %u MHz)\n", ESP.getChipCores(), ESP.getCpuFreqMHz(), getXtalFrequencyMhz());
  printf("Sketch core:   %d\n", xPortGetCoreID());
  printf("SDK version:   %s\n", ESP.getSdkVersion());
  printf("Efuse MAC:     %04X%08X\n", (uint16_t)(mac >> 32), (uint32_t)mac);
  printf("Flash:         %u bytes @ %u Hz\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  printf("Sketch size:   %u bytes (%u free)\n", ESP.getSketchSize(), ESP.getFreeSketchSpace());
  printf("Heap:          %u free / %u total bytes\n", ESP.getFreeHeap(), ESP.getHeapSize());
  printf("PSRAM:         %u free / %u total bytes\n", ESP.getFreePsram(), ESP.getPsramSize());
  printf("Die temp:      %.1f C\n", temperatureRead());
  printf("===================\n\n");
}

void setup() {
  digitalWrite(pin_built_in_led, 0);
  delayMicroseconds(1000000);

  print_device_info();

  rgb_led.begin();
  rgb_led.setPixelColor(0, rgb_led.Color(0, 1, 0));
  rgb_led.show();

  digitalWrite(pin_built_in_led, 0);
}

void update_rgb_rainbow(unsigned long ms) {
  rgb_led.begin();
  uint8_t rgb_brightness = 20;
  uint32_t period_ms = 1500;
  uint32_t offset_r = 0;
  uint32_t offset_g = period_ms / 3;
  uint32_t offset_b = period_ms * 2 / 3;
  auto color_r = triangle_wave(ms, period_ms, rgb_brightness, offset_r);
  auto color_g = triangle_wave(ms, period_ms, rgb_brightness, offset_g);
  auto color_b = triangle_wave(ms, period_ms, rgb_brightness, offset_b);
  rgb_led.setPixelColor(0, rgb_led.Color(color_r, color_g, color_b));
  rgb_led.show();
}

void loop() {
  const int loop_hz = 60;
  auto ms = millis();
  update_rgb_rainbow(ms);
  delay(1000 / loop_hz);
}
