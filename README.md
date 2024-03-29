nRF52 Cowboy e-bike remote control fob
====

I wanted to control my awesome Cowboy bike without my phone. I set out to create a car key remote control using a bluetooth SoC.

I'm using a Volkswagen (VW) 3 button key fob. You can find them for about EUR 6 online.

This is a work in progress, the code is terrible, I have very little electronics experience and this is my first ever NRF, BLE or custom PCB project. Not the combination of these three, I've never done any of them before.

This prototype works and I'm using it now.

It's using diodes to only power the SoC when you keep pressing a key.

![P1](p1.JPG)

![P2](p2.JPG)

![P3](p3.JPG)

The next step is to create a proper PCB. You can find the data below. The board should be 1mm thick to fit in the VW enclosure.

## Code

There are two samples, one for ESP32 ( sketch_esp32_cowboy.ino ) which I used to get started. There is also one for the NRF Connect SDK (main.c) which I am using. In the key fob I started using an NRF52805 module (the smallest and cheapest) which works well. It fits better than the BT832A which I used previously.

You will need the mac address and the 6 digit passkey for your bike. This will be hardcoded in the firmware. Using 3 buttons a passkey entry will be tough.

For flashing the SoC you need an NRF52-dk. You can reportedly also use an esp32 to flash, but I've not used it.

## What does it do?

* Long-press on unlock -> turns bike on
* Long-press on lock -> turns bike off
* Long-press trunk -> toggle light on/off
* Long-press trunk + long-press unlock -> go to 24 km/h speed limit
* Long-press trunk + long-press lock -> go to 25 km/h speed limit

Once the action is done, the LED will flash rapidly and the SoC will shut down. You can stop holding the button when you see the flashing.

## Costs

* Lots of time...
* Key enclosure: 6 EUR
* PCB: 0.5 EUR
* BC805M SoC: 4 EUR
* SMD parts: 0.5 EUR
* Sticker: 1 EUR (if you order 50 pieces)

Total: 12 EUR.

So one key is about Naturally there are also S&H costs. In low volumes this is about 2 times the component costs.

## Schematic

NOTE: The PCB design is untested, I'm waiting for the shipments.

UPDATE: the PCB has some minor problems, like the pin-hole alignment in the bottom left corner, and the height of some of the switches is wrong for some reason. Other than that, it seems to be working fine.

I designed this using easyeda.com and ordered the PCB and components through their site.

In the pcb-design folder you'll find the Gerber files for the enclosure.

![Schematic](pcb-design/Schematic_Cowboy-key_2022-04-16.svg)


![Design](pcb-design/PCB_PCB_Cowboy-key_2022-04-16.svg)


TO DO:
- order a reference PCB from a blank key so we can see the thickness, size of the buttons, proper alignment etc.
- re-do the design in EasyEDA, I didn't save it last time...
- the rear hole is not properly aligned and the rear end extends just a bit (0.1 - 0.2mm) too much
- the diodes do not fit in a blank case, reposition them
- the bc805m module can fit but needs better alignment. it has 0.05mm wiggle room in the ideal spot
- for the programming it'd be great if there is a flat/flex cable debug port
- the diode design doesn't work on the PCB. How does it work in the prototype?? (maybe it's the white vs red led?)
- measure the new beta version and see how long the battery lasts..

## Original board measurements

For reference I ordered an original VW key fob PCB. Here are the measurements of that:
- board thickness: 1mm
- width: 36mm
- height: 21.1mm
- height short: 10.6mm
- width short: 30mm
- button height: 3.5mm
- battery spring height: 1.9mm

![Original PCB Back](original-pcb-back.jpg)
![Original PCB Back - Measurements 1](original-pcb-back-with-measurements-pt1.jpg)
![Original PCB Front](original-pcb-front.jpg)
![Original PCB Front - Measurements 1](original-pcb-front-with-measurements-pt1.jpg)
![Original PCB Front - Measurements 2](original-pcb-front-with-measurements-pt2.jpg)
![Original PCB Front - Measurements 3](original-pcb-front-with-measurements-pt3.jpg)

Here it is in the enclosure:

![Original PCB fit](original-pcb-in-enclosure.jpg)

![Original PCB fit back](original-pcb-back-enclosure.jpg)

Note that it as about 1mm clearance at the edge. We need that space for our own version as the BC805m needs all the space it can get.

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
