#include <Arduino.h>

// MightyMailbox - RAK3172 (STM32WL) P2P transmitter test
// Board: "WisDuo RAK3172 Evaluation Board"
// This BSP exposes P2P setters as: pfreq, psf, pbw, pcr, ppl, ptp, psend

static void die(const char *msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}

static void p2p_setup() {
  // Put device into LoRa P2P mode
  if (!api.lora.nwm.set()) {
    die("ERROR: api.lora.nwm.set() failed");
  }

  // Match your ESP32 receiver: 868.0 MHz, SF9, BW125, CR 4/5
  if (!api.lora.pfreq.set(868000000)) die("ERROR: pfreq.set failed");
  if (!api.lora.psf.set(9))           die("ERROR: psf.set failed");
  if (!api.lora.pbw.set(125))         die("ERROR: pbw.set failed");
  if (!api.lora.pcr.set(1))           die("ERROR: pcr.set failed");   // 1 => 4/5

  // Optional but nice:
  if (!api.lora.ppl.set(8))           Serial.println("NOTE: ppl.set not supported (ok)"); // preamble length

  // TX power (dBm) - usually supported
  if (!api.lora.ptp.set(14))          Serial.println("NOTE: ptp.set not supported (ok)");

  Serial.println("P2P configured: 868MHz SF9 BW125 CR4/5 PWR14");
}

static void p2p_send_ascii(const char *msg) {
  const uint16_t len = (uint16_t)strlen(msg);

  if (!api.lora.psend(len, (uint8_t *)msg)) {
    Serial.println("WARN: psend failed");
  } else {
    Serial.print("TX: ");
    Serial.println(msg);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("=== MightyMailbox P2P TX (RAK3172) ===");

  p2p_setup();
}

void loop() {
  p2p_send_ascii("MAILBOX_TEST");
  delay(5000);
}