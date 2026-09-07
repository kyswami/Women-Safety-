/*
 * Women Safety GPS + GSM Emergency Alert System
 * -----------------------------------------------
 * Button-triggered emergency alert device.
 * On trigger: sounds a buzzer and sends an SMS with a live Google Maps
 * location link over GSM. GPS coordinates and status are shown on a
 * 16x2 I2C LCD. Sending "STOP" back to the device's SIM silences the buzzer.
 *
 * Hardware: Arduino Uno/Nano, NEO-6M GPS module, SIM800L GSM module,
 *           16x2 I2C LCD (PCF8574 backpack, addr 0x27), buzzer, push button.
 *
 * See README.md for wiring, setup, and configuration instructions.
 */

#include <Wire.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

// ---------------- Configuration ----------------
// Set the emergency contact number the alert SMS is sent to.
// Format: "+<country code><number>", e.g. "+919876543210"
#define EMERGENCY_CONTACT "+91XXXXXXXXXX"

#define BUZZER_PIN 13
const int buttonPin = 6;
// -------------------------------------------------

LiquidCrystal_I2C lcd(0x27, 16, 2);

SoftwareSerial gsm(7, 8);
SoftwareSerial gps(10, 11);

bool btnState = HIGH;

// GPS fallback
String latitude = "";
String longitude = "";
String gpsBuffer = "";

// Convert raw NMEA GPS coordinate format (ddmm.mmmm) to decimal degrees
float convertToDecimal(String raw)
{
  float val = raw.toFloat();
  int deg = (int)(val / 100);
  float min = val - (deg * 100);
  return deg + (min / 60.0);
}

// GSM INIT
void gsm_init()
{
  Serial.println("Initializing GSM...");
  gsm.println("AT");
  delay(1000);

  gsm.println("AT+CMGF=1"); // Text mode
  delay(1000);

  gsm.println("AT+CNMI=2,2,0,0,0"); // Direct incoming SMS
  delay(1000);

  Serial.println("GSM Ready");
}

// SEND SMS
void send_sms()
{
  Serial.println("Sending SMS...");

  gsm.listen();

  gsm.print("AT+CMGS=\"");
  gsm.print(EMERGENCY_CONTACT);
  gsm.print("\"\r");

  // WAIT FOR '>' FROM GSM
  unsigned long t = millis();
  bool prompt = false;

  while (millis() - t < 5000)
  {
    if (gsm.available())
    {
      char c = gsm.read();
      Serial.write(c);

      if (c == '>')
      {
        prompt = true;
        break;
      }
    }
  }

  if (!prompt)
  {
    Serial.println("ERROR: No GSM prompt");
    return;
  }

  gsm.print("EMERGENCY ALERT!\nNeed Help!\n");

  gsm.print("Location:\n");
  gsm.print("https://maps.google.com/?q=");
  gsm.print(latitude);
  gsm.print(",");
  gsm.print(longitude);

  delay(500);

  gsm.write(26); // CTRL+Z

  delay(5000);

  Serial.println("SMS SENT");
}

// PARSE GPS ($GPGGA sentence)
void parseGPS(String data)
{
  if (data.indexOf("$GPGGA") == -1)
    return;

  int idx = 0;
  int commaCount = 0;

  String parts[15];

  for (int i = 0; i < data.length(); i++)
  {
    if (data[i] == ',')
    {
      commaCount++;
      idx++;
    }
    else
    {
      parts[idx] += data[i];
    }
  }

  // GPGGA format:
  // parts[2] = latitude
  // parts[3] = N/S
  // parts[4] = longitude
  // parts[5] = E/W
  // parts[6] = fix quality

  if (parts[6] == "0")
  {
    Serial.println("No GPS fix");
    return;
  }

  String rawLat = parts[2];
  String latDir = parts[3];
  String rawLon = parts[4];
  String lonDir = parts[5];

  if (rawLat.length() < 4 || rawLon.length() < 4)
    return;

  float lat = convertToDecimal(rawLat);
  float lon = convertToDecimal(rawLon);

  if (latDir == "S") lat = -lat;
  if (lonDir == "W") lon = -lon;

  latitude = String(lat, 6);
  longitude = String(lon, 6);

  Serial.print("Lat: ");
  Serial.println(latitude);
  Serial.print("Lon: ");
  Serial.println(longitude);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Lat:");
  lcd.print(latitude.substring(0, 10));

  lcd.setCursor(0, 1);
  lcd.print("Lon:");
  lcd.print(longitude.substring(0, 10));
}

// READ GPS
void gpsEvent()
{
  gps.listen();
  while (gps.available())
  {
    char c = gps.read();

    if (c == '\n')
    {
      parseGPS(gpsBuffer);
      gpsBuffer = "";
    }
    else
    {
      gpsBuffer += c;
    }
  }
}

void trigger_alert()
{
  lcd.clear();
  lcd.print("Sending Alert");

  // Buzzer ON (continuous)
  digitalWrite(BUZZER_PIN, HIGH);

  send_sms();

  lcd.clear();
  lcd.print("SMS Sent");
}

void serialEvent()
{
  gsm.listen();

  static String msg = "";

  while (gsm.available())
  {
    char c = gsm.read();
    Serial.write(c); // debug output

    msg += c;

    // Convert to uppercase safely
    String temp = msg;
    temp.toUpperCase();

    // Check STOP anywhere in stream
    if (temp.indexOf("STOP") != -1)
    {
      digitalWrite(BUZZER_PIN, LOW);
      lcd.clear();
      lcd.print("Buzzer stopped");

      Serial.println("\nBuzzer OFF via SMS");

      msg = ""; // clear buffer after detection
    }

    if (msg.length() > 150)
    {
      msg = "";
    }
  }
}

void setup()
{
  Serial.begin(9600);
  gsm.begin(9600);
  gps.begin(9600);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.print("System Start");
  delay(1000);

  gsm_init();

  lcd.clear();
  lcd.print("System Ready");
}

void loop()
{
  // GPS: poll for a short window each cycle
  gps.listen();
  unsigned long t1 = millis();

  while (millis() - t1 < 200)
  {
    while (gps.available())
    {
      char c = gps.read();

      if (c == '\n')
      {
        parseGPS(gpsBuffer);
        gpsBuffer = "";
      }
      else
      {
        gpsBuffer += c;
      }
    }
  }

  // GSM: longer listen window so incoming "STOP" SMS isn't missed
  gsm.listen();
  unsigned long t2 = millis();

  static String msg = "";

  while (millis() - t2 < 500)
  {
    while (gsm.available())
    {
      char c = gsm.read();
      Serial.write(c);

      msg += c;

      String temp = msg;
      temp.toUpperCase();

      if (temp.indexOf("STOP") != -1)
      {
        digitalWrite(BUZZER_PIN, LOW);
        lcd.clear();
        lcd.print("Buzzer stopped");
        msg = "";
      }

      if (msg.length() > 200)
        msg = "";
    }
  }

  btnState = digitalRead(buttonPin);

  if (btnState == LOW)
  {
    trigger_alert();
    delay(3000);
  }
}
