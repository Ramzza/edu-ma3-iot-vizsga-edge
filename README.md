# edu-ma3-iot-vizsga-edge

Arduino + NodeMCU kod a vizsgaprojekthez

1. # RTC Clock

- kod: sketch_dec14d.ino
- kiirja az idot az LCD-re
- timestamp-et ad az adatokhoz
- ido + adat, amit kulhetunk a DB-be
- ha ki akarod rajzolni ido fuggvenyeben a fenyvaltozast

2. # I2c light sensor

- szenzor felfele mutat
- kod: sketch_dec14d.ino
- include: "TSL2561.h" (library)
- i2c cim scanner: Arduino IDE -> file -> examples -> wire -> i2c_scanner
- i2c scanner tesztelni lehet - elvileg nincs szukseg ra
- i2c scan ha nem megy LCD

3. # DHT 11 sensor

- temperature + humidity
- kek kocka
- kockas resz kifele mutasson
- kod: sketch_dec14d.ino
- include: <SimpleDHT> (library)

4. # I2c LCD display

- kod: sketch_dec14d.ino
- include: <LiquidCrystal_I2C> (library)

5. # IR LED

- 7es pinen 0-lekapcs, 1-felkapcs
- blinking 36khz.ino canvasen

6. # Motor mount

- stepper
- Arduino IDE -> file -> examples -> stepper -> oneStepAtATime

7. # Switch mount

- logo szal
- log-be, bedug-ki

8. # Relay

- csak akkor kattog, ha 12V-al van taplalva (Jack)
- gazdatlan zold ize a 220-nak - nem hasznaljuk

# Overview

- ido - ora:perc:masodperc - datum
  Szenzorok:
- fenyero: lux
- homerseklet
- nedvesseg
- kapcsolo allapota
  Aktuatorok:
- rele
- IR LED
- motor

# Setup TODOs

- DS3231.h
  - download zip: https://github.com/jarzebski/Arduino-DS3231
  - put folder in Arduino/libraries
  - restart IDE
- TSL2561.h
  - download zip: https://github.com/adafruit/TSL2561-Arduino-Library
  - put folder in Arduino/libraries
  - restart IDE
- SimpleDHT.h
  - download with Arduino package manager
- LiquidCrystal_I2C.h
  - download with Arduino package manager
- Firebase: https://github.com/Rupakpoddar/ESP8266Firebase
  - this code had to be modified
  - updated version in the repo
