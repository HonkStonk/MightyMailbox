# MightyMailbox
Mailbox sensor using LoRa P2P

Problem to solve: do i have mail in my steel sheet cluster mailbox 5 houses from mine?

Sensor/TX board used is from this fantastic project where my specific problem was already solved:
https://github.com/hierle/letterbox-sensor-v2
I only made a 3D printed cover for it and using P2P mode for the radios instead of LoRaWAN.

RX side is a ESP32-S3-Zero with an SX1262 module with jumper wires put into a 3D printed box. And a simple dipole for now.

TX side:
![tx_top](https://github.com/user-attachments/assets/74f627c9-baf2-4557-8a20-e592548e3272)

![tx_bottom](https://github.com/user-attachments/assets/fc19430a-bea4-47a0-b881-f2b0352131ed)

<img width="1677" height="1052" alt="sensorInMailbox" src="https://github.com/user-attachments/assets/bc671a05-23bb-4b1c-b0b0-567e1b17490e" />



RX side:
![rx_open](https://github.com/user-attachments/assets/60e46502-5a22-4d48-904a-930cece4e9c9)

![antenna](https://github.com/user-attachments/assets/122f4626-4fe5-462b-a48b-05c29b294435)


## Current status

- LoRa P2P link verified
- TX board: custom letterbox board with RAK3172
- RX board: ESP32-S3-Zero + SX1262 Core1262-HF
- Verified payload reception on receiver serial monitor
- Typical desk-test RSSI: about -24 dBm
- Typical desk-test SNR: about 10 dB
- Now sleep/wake and sensor ambient-cancellation delta is implemented
- Test interval at 30s transmits for now - later set to 30m or more
- HA will handle sensor delta thresholds instead of sensor itself
- Receiver now forwards LoRa messages and repackages them to MQTT/HA

## Known-good radio settings

- Frequency: 868.0 MHz
- Spreading Factor: SF10
- Bandwidth: 125 kHz
- Coding Rate: 4/5
- TX Power: 14 dBm

## Receiver wiring

| SX1262 | ESP32-S3-Zero |
|---|---|
| VCC | 3.3V |
| GND | GND |
| MOSI | GPIO11 |
| MISO | GPIO13 |
| SCK | GPIO12 |
| NSS | GPIO10 |
| BUSY | GPIO9 |
| DIO1 | GPIO7 |
| RST | GPIO8 |

This is how the sensor live updates in HA overview looks now:
<img width="502" height="431" alt="overviewScreenshotMM" src="https://github.com/user-attachments/assets/e1c90ee3-d11c-45a6-a120-a7117080dc04" />


