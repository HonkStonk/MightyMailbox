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

RX side:
![rx_open](https://github.com/user-attachments/assets/60e46502-5a22-4d48-904a-930cece4e9c9)
