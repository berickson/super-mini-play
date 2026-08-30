#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ctype.h>
#include <string.h>

// Unfortunately, the board's rgb led and built in led share the same gpio
const uint8_t pin_rgb_led = 48;
const uint8_t pin_built_in_led = 48;
const uint8_t rgb_led_count = 1;

Adafruit_NeoPixel rgb_led(rgb_led_count, pin_rgb_led, NEO_GRB + NEO_KHZ800);

// desired on/off state of the built in led, kept in sync with the shared pin whenever the rgb led isn't mid-write
bool built_in_led_state = false;
bool rgb_busy = false;

void set_built_in_led(bool on) {
  built_in_led_state = on;
  if (!rgb_busy) {
    digitalWrite(pin_built_in_led, on ? HIGH : LOW);
  }
}

// bracket any rgb_led writes with these so the built in led gets restored afterward instead of staying stuck
void begin_rgb_write() {
  rgb_busy = true;
}

void end_rgb_write() {
  rgb_busy = false;
  digitalWrite(pin_built_in_led, built_in_led_state ? HIGH : LOW);
}

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

  begin_rgb_write();
  rgb_led.begin();
  rgb_led.setPixelColor(0, rgb_led.Color(0, 1, 0));
  rgb_led.show();
  end_rgb_write();

  set_built_in_led(false);
}

void update_rgb_rainbow(unsigned long ms) {
  begin_rgb_write();
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
  end_rgb_write();
}

// morse code timing unit, in ms; dash = 3 units, intra-char gap = 1 unit, inter-char gap = 3 units, word gap = 7 units
const uint16_t morse_unit_ms = 300;
const char* morse_message = "192.168.1.1";

const char* morse_code_for_char(char c) {
  switch (toupper(c)) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";
    case '0': return "-----";
    case '1': return ".----";
    case '2': return "..---";
    case '3': return "...--";
    case '4': return "....-";
    case '5': return ".....";
    case '6': return "-....";
    case '7': return "--...";
    case '8': return "---..";
    case '9': return "----.";
    case '.': return ".-.-.-";
    case ',': return "--..--";
    case '?': return "..--..";
    case '\'': return ".----.";
    case '!': return "-.-.--";
    case '/': return "-..-.";
    case '(': return "-.--.";
    case ')': return "-.--.-";
    case '&': return ".-...";
    case ':': return "---...";
    case ';': return "-.-.-.";
    case '=': return "-...-";
    case '+': return ".-.-.";
    case '-': return "-....-";
    case '_': return "..--.-";
    case '"': return ".-..-.";
    case '$': return "...-..-";
    case '@': return ".--.-.";
    default: return "";
  }
}

// non-blocking morse code blinker for the built in led, driven by set_built_in_led() so it survives rgb writes
void update_morse_led(unsigned long ms) {
  static size_t char_index = 0;
  static size_t symbol_index = 0;
  static unsigned long next_change_ms = 0;
  static bool led_on = false;
  static bool last_symbol_of_char = false;

  if (ms < next_change_ms) return;

  size_t message_len = strlen(morse_message);
  if (message_len == 0) return;

  if (led_on) {
    set_built_in_led(false);
    led_on = false;
    next_change_ms = ms + (last_symbol_of_char ? morse_unit_ms * 3 : morse_unit_ms);
    return;
  }

  char c = morse_message[char_index];
  if (c == ' ') {
    next_change_ms = ms + morse_unit_ms * 7;
    char_index = (char_index + 1) % message_len;
    symbol_index = 0;
    return;
  }

  const char* code = morse_code_for_char(c);
  if (code[0] == '\0') {
    // unknown character - skip it with a normal inter-character gap
    next_change_ms = ms + morse_unit_ms * 3;
    char_index = (char_index + 1) % message_len;
    symbol_index = 0;
    return;
  }

  char symbol = code[symbol_index];
  set_built_in_led(true);
  led_on = true;
  next_change_ms = ms + (symbol == '-' ? morse_unit_ms * 3 : morse_unit_ms);

  symbol_index++;
  last_symbol_of_char = (code[symbol_index] == '\0');
  if (last_symbol_of_char) {
    symbol_index = 0;
    char_index = (char_index + 1) % message_len;
  }
}

void loop() {
  const int loop_hz = 60;
  auto ms = millis();
  update_rgb_rainbow(ms);
  update_morse_led(ms);
  delay(1000 / loop_hz);
}
