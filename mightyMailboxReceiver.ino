#include <RadioLib.h>
#include <SPI.h>

// Use a dedicated SPI instance (ESP32-S3 has multiple SPI peripherals)
SPIClass spi = SPIClass(FSPI);

// Tell RadioLib to use *our* SPI instance
SX1262 radio = new Module(
  10,  // NSS (CS)
  7,   // DIO1
  8,   // RESET
  9,   // BUSY
  spi  // <--- IMPORTANT
);

void setup() {
  Serial.begin(115200);
  delay(200);

  // Define the SPI pins you physically wired
  // begin(SCK, MISO, MOSI, SS)
  spi.begin(
    12, // SCK  -> GPIO12
    13, // MISO -> GPIO13
    11, // MOSI -> GPIO11
    10  // SS/CS -> GPIO10 (same as NSS)
  );

  Serial.print("Initializing SX1262... ");

  int state = radio.begin(868.0);
  Serial.println(state);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.println("SX1262 init failed. Check wiring + SPI pins.");
    while (true) delay(1000);
  }

  // Many SX1262 modules use DIO2 to control the RF switch.
  // If your module does, enabling this is required for TX/RX to work reliably.
  radio.setDio2AsRfSwitch(true);

  radio.setSpreadingFactor(9);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);
  radio.setOutputPower(14);

  state = radio.startReceive();
  Serial.print("startReceive(): ");
  Serial.println(state);
}

void loop() {
  String str;
  int state = radio.receive(str);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("Received: ");
    Serial.println(str);

    Serial.print("RSSI: ");
    Serial.print(radio.getRSSI());
    Serial.print(" dBm, SNR: ");
    Serial.print(radio.getSNR());
    Serial.println(" dB");
  }
}