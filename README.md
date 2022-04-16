nRF52 Cowboy e-bike remote control fob
====

I wanted to control my awesome Cowboy bike without my phone. I set out to create a car key remote control using a bluetooth SoC.

I'm using a Volkswagen (VW) 3 button key fob. You can find them for about EUR 6 online.

This is a work in progress, the code is terrible, I have very little electronics experience and this is my first ever NRF, BLE or custom PCB project. Not the combination of these three, I've never done any of them before.

This prototype works and I'm using it now.

![P1](p1.JPG)

![P2](p2.JPG)

![P3](p3.JPG)

The next step is to create a proper PCB. You can find the data below. The board should be 1mm thick to fit in the VW enclosure.

## Code

There are two samples, one for ESP32 ( sketch_esp32_cowboy.ino ) which I used to get started. There is also one for the NRF Connect SDK (main.c) which I am using. In the key fob I started using an NRF52805 module (the smallest and cheapest) which works well. It fits better than the BT832A which I used previously.

Youl will need the mac address and the 6 digit passkey for your bike. This will be hardcoded in the firmware. Using 3 buttons a passkey entry will be tough.

## Schematic

NOTE: The PCB design is untested, I'm waiting for the shipments.

In the pcb-design folder you'll find the Gerber files for the enclosure.

![Schematic](pcb-design/Schematic_Cowboy-key_2022-04-16.svg)


![Design](pcb-design/PCB_PCB_Cowboy-key_2022-04-16.svg)


## How can you help?

If you see stupid mistakes or other room for improvement, let me know! I'm a n00b.

## Do you need help?

Submit an issue if you need some pointers to get it to work, but I give 0 guarantees or warranty.

## The key stickers

On the back of the key fob you can place a 14mm sticker. These are called Doming 3D stickers and can be ordered in NL from e.g:

* https://mijn.stickerwinkel.com/
* https://www.drukwerkdeal.nl/nl/delivery

The Cowboy logo is included in this repo if you want to order.

Stickers cost about 60 euros for 1 to 50 pieces...
