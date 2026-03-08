#include <Arduino.h>

// ===== Pins from original letterbox-sensor-v2 =====
#define irled1    PA0
#define irdiode1  PA1
#define irsens1   PB4

#define irled2    PA8
#define irdiode2  PA9
#define irsens2   PA10

#define vdiv      PB2

// Test interval in seconds for CR2032 field test
#define SAMPLE_PERIOD_S 30

struct SensorReading {
  uint16_t off_raw;
  uint16_t on_raw;
  uint16_t delta_raw;
};

static void die(const char *msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}

static void p2p_setup() {
  if (!api.lora.nwm.set()) {
    die("ERROR: api.lora.nwm.set() failed");
  }

  if (!api.lora.pfreq.set(868000000)) die("ERROR: pfreq.set failed");
  if (!api.lora.psf.set(9))           die("ERROR: psf.set failed");
  if (!api.lora.pbw.set(125))         die("ERROR: pbw.set failed");
  if (!api.lora.pcr.set(1))           die("ERROR: pcr.set failed");

  if (!api.lora.ppl.set(8))           Serial.println("NOTE: ppl.set not supported");
  if (!api.lora.ptp.set(14))          Serial.println("NOTE: ptp.set not supported");

  Serial.println("P2P configured: 868MHz SF9 BW125 CR4/5 PWR14");
}

static bool p2p_send_ascii(const char *msg) {
  const uint16_t len = (uint16_t)strlen(msg);
  bool ok = api.lora.psend(len, (uint8_t *)msg);

  Serial.print("TX: ");
  Serial.print(msg);
  Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FAIL");

  return ok;
}

static uint16_t read_battery_adc_raw() {
  analogReadResolution(12);

  analogRead(vdiv);
  delay(5);

  uint32_t sum = 0;
  const int samples = 8;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(vdiv);
    delay(2);
  }

  return (uint16_t)(sum / samples);
}

static uint16_t battery_mv_from_raw(uint16_t raw) {
  return (uint16_t)((raw * 3350UL + 653UL) / 1306UL);
}

static uint16_t average_adc_reads(uint8_t adc_pin, int samples, int delay_ms) {
  analogRead(adc_pin);
  delay(2);

  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(adc_pin);
    delay(delay_ms);
  }

  return (uint16_t)(sum / samples);
}

static SensorReading read_sensor_delta(uint8_t led_pin, uint8_t diode_pin, uint8_t adc_pin) {
  SensorReading r = {0, 0, 0};

  analogReadResolution(12);

  // Detector bias ON, LED OFF
  digitalWrite(led_pin, LOW);
  digitalWrite(diode_pin, HIGH);
  delay(10);
  r.off_raw = average_adc_reads(adc_pin, 8, 5);

  // Detector bias ON, LED ON
  digitalWrite(led_pin, HIGH);
  delay(10);
  r.on_raw = average_adc_reads(adc_pin, 8, 5);

  // Fully OFF again
  digitalWrite(led_pin, LOW);
  digitalWrite(diode_pin, LOW);

  if (r.on_raw >= r.off_raw) {
    r.delta_raw = r.on_raw - r.off_raw;
  } else {
    r.delta_raw = 0;
  }

  return r;
}

static void sample_and_send(void *) {
  SensorReading s1 = read_sensor_delta(irled1, irdiode1, irsens1);
  SensorReading s2 = read_sensor_delta(irled2, irdiode2, irsens2);

  uint16_t bat_raw = read_battery_adc_raw();
  uint16_t bat_mv  = battery_mv_from_raw(bat_raw);

  Serial.print("D1: ");
  Serial.print(s1.delta_raw);
  Serial.print("  D2: ");
  Serial.print(s2.delta_raw);
  Serial.print("  BAT mV: ");
  Serial.println(bat_mv);

  char msg[48];
  snprintf(
    msg,
    sizeof(msg),
    "BAT=%u,D1=%u,D2=%u",
    bat_mv,
    s1.delta_raw,
    s2.delta_raw
  );

  p2p_send_ascii(msg);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("=== MightyMailbox P2P TX (RAK3172) ===");

  pinMode(irled1, OUTPUT);
  pinMode(irdiode1, OUTPUT);
  pinMode(irsens1, INPUT);

  pinMode(irled2, OUTPUT);
  pinMode(irdiode2, OUTPUT);
  pinMode(irsens2, INPUT);

  pinMode(vdiv, INPUT);

  digitalWrite(irled1, LOW);
  digitalWrite(irdiode1, LOW);
  digitalWrite(irled2, LOW);
  digitalWrite(irdiode2, LOW);

  p2p_setup();

  // Enable low-power mode
  if (!api.system.lpm.set(1)) {
    Serial.println("Set low power mode failed!");
  }

  // Create/start periodic timer
  api.system.timer.create(RAK_TIMER_0, sample_and_send, RAK_TIMER_PERIODIC);
  api.system.timer.start(RAK_TIMER_0, SAMPLE_PERIOD_S * 1000, NULL);

  // Send one sample immediately on boot
  sample_and_send(NULL);
}

void loop() {
  // Let RUI3 destroy the busy loop task so the system can idle/sleep
  api.system.scheduler.task.destroy();
}
