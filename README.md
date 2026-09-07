# Women Safety GPS + GSM Emergency Alert System

A wearable/portable emergency alert device. Pressing a button sounds a buzzer
and sends an SMS with a live Google Maps location link to a pre-configured
emergency contact via a GSM module. GPS coordinates and system status are
shown on a 16x2 I2C LCD. The buzzer can be silenced remotely by texting
`STOP` to the device's SIM.

## Features...

- One-button emergency trigger
- Live GPS location parsed from NMEA `$GPGGA` sentences
- SMS alert with a direct Google Maps link (`https://maps.google.com/?q=lat,lon`)
- Local buzzer alarm on trigger
- Remote buzzer silence via incoming SMS (`STOP`)
- 16x2 I2C LCD status display (system state, live lat/lon)

## Hardware

| Component | Notes |
|---|---|
| Arduino Uno / Nano | Main controller |
| SIM800L GSM module | SMS sending/receiving |
| NEO-6M GPS module | Location fix |
| 16x2 LCD + PCF8574 I2C backpack | Address `0x27` |
| Buzzer | Active buzzer, digital pin |
| Push button | Emergency trigger, uses internal pull-up |
| Active SIM card | Inserted in the GSM module, SMS-enabled |
| 5V supply (min 2A recommended) | SIM800L draws high transient current on transmit |

## Wiring

| Signal | Arduino Pin |
|---|---|
| GSM RX (module) | D7 (Arduino TX to GSM) |
| GSM TX (module) | D8 (Arduino RX from GSM) |
| GPS RX (module) | D10 |
| GPS TX (module) | D11 |
| Buzzer | D13 |
| Push button | D6 (to GND, uses `INPUT_PULLUP`) |
| LCD SDA / SCL | A4 / A5 (Uno) |

> SIM800L is sensitive to voltage drops — power it from a dedicated
> regulated 4V–4.2V source with a large capacitor across the rails, not
> directly from the Arduino 5V pin, or transmissions can brown out the board.

## Software setup

1. Install the Arduino IDE (1.8.x or 2.x).
2. Install required libraries via **Sketch → Include Library → Manage Libraries**:
   - `LiquidCrystal I2C` (Frank de Brabander or Marco Schwartz version)
   - `SoftwareSerial` (bundled with the IDE core — no install needed)
3. Open `Women_Safety_Alert_System/Women_Safety_Alert_System.ino`.
4. At the top of the file, set your emergency contact number:
   ```cpp
   #define EMERGENCY_CONTACT "+91XXXXXXXXXX"
   ```
5. Select your board and port, then upload.
6. Open the Serial Monitor at `9600` baud to watch GSM/GPS debug output.

## How it works

- `loop()` polls the GPS module for ~200 ms per cycle, parsing any complete
  `$GPGGA` sentence into decimal-degree latitude/longitude.
- It then listens on the GSM line for ~500 ms per cycle, watching for an
  incoming `STOP` command to silence the buzzer remotely.
- When the button is pressed (pulled low), `trigger_alert()` turns on the
  buzzer and calls `send_sms()`, which sends the emergency message with the
  latest known coordinates.
- `SoftwareSerial::listen()` is used to switch which serial port is actively
  received, since only one `SoftwareSerial` instance can listen at a time on
  AVR boards.

## Known limitations / notes

- `SoftwareSerial` on classic AVR boards (Uno/Nano) can only actively listen
  on one port at a time, which is why GPS and GSM are polled in separate
  timed windows rather than truly simultaneously.
- No GPS fix (`fix quality == 0`) is detected and skipped; the last known
  good coordinates are reused until a new fix comes in.
- This sketch is written for AVR-based Arduino boards. Porting to ESP32
  requires replacing `SoftwareSerial` with a `HardwareSerial` instance (ESP32
  has multiple hardware UARTs), since `SoftwareSerial` is not reliable at
  these baud rates on the ESP32 core.

## Disclaimer

This is a hobby/prototype safety device, not a certified emergency system.
Test SMS delivery and GPS fix reliability thoroughly in your deployment
environment before relying on it in a real emergency.

## License

MIT — see [LICENSE](LICENSE).
