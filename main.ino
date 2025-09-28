#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Stepper.h>
#include "time.h"
#include <LiquidCrystal_I2C.h>
#include <FastLED.h>

//Hotspot also works
#define WIFI_SSID "[WIFI_NAME]"
#define WIFI_PASSWORD "[WIFI_PASSWORD]"

//Firebase
#define API_KEY "[API_KEY]" //create a web app and it's there
#define DATABASE_URL "[DATABASE_URL]" //create a web app and it's there

#define USER_EMAIL "ryalisuchir@gmail.com"
#define USER_PASSWORD "[USER_PASSWORD]" //off the firebase auth

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int intValue;
bool signupOK = true;

//Stepper
const int steps_per_rev = 2048;
const int servoSpeed = 10;

#define BUZZER_PIN 33
#define BUZZER_CHANNEL 0

#define IN1_1 13
#define IN2_1 12
#define IN3_1 14
#define IN4_1 27
Stepper motor1(steps_per_rev, IN1_1, IN2_1, IN3_1, IN4_1);


//LCD
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

//Time
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3600 * 6;
const int daylightOffset_sec = 3600;

#define NUM_LEDS 10
CRGB leds[NUM_LEDS];

int getPointer();
String getNextDispenseMessage(int pointer);
unsigned long getEpochTime();
unsigned long getNextDispenseEpochTime(int pointer);
int getNextPills(int pointer);
void setPointer(int value);
void turnOff(); // Declared
void turnRed(); // Declared
void turnGreen(); // Declared

void setup() {

  Serial.begin(115200);

  //Stepper
  motor1.setSpeed(servoSpeed);

  //Wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  //Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // tokenStatusCallback function is provided by Firebase library
  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  
  
   // --- START of Login Retry Logic ---
  int maxRetries = 10; // Max attempts before giving up
  int retryCount = 0;
  bool firebaseConnected = false;

  while (retryCount < maxRetries && !firebaseConnected) {
    Serial.printf("Firebase Login Attempt #%d...\n", retryCount + 1);
    
    // Begin the Firebase connection attempt
    Firebase.begin(&config, &auth);
    
    // Check if the authentication was successful
    if (Firebase.ready()) {
      Serial.println("Firebase successfully connected and authenticated!");
      firebaseConnected = true;
    } else {
      Serial.println("Login failed. Error: %s. Retrying in 10 seconds...\n");
      delay(10000); // Wait 10 seconds before the next attempt
      retryCount++;
    }
  }

  if (!firebaseConnected) {
    Serial.println("FATAL: Failed to connect to Firebase after multiple attempts. The device will continue to run without cloud sync.");
  }


  Firebase.setDoubleDigits(5);

  //LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  // NOTE: getPointer() might return -1 if Firebase isn't ready, which might cause issues in getNextDispenseMessage().
  // Assuming a successful connection by the time this is called.
  lcd.print("Next Dosage at: ");
  lcd.setCursor(0, 1);
  lcd.print(getNextDispenseMessage(getPointer()));


  FastLED.addLeds<NEOPIXEL, 26>(leds, NUM_LEDS);
  FastLED.setBrightness(20);


  //Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  delay(1000);
  int pointer = getPointer();


  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Next dosage at: ");
  lcd.setCursor(0, 1);
  lcd.print(getNextDispenseMessage(pointer));

  // Serial.print(pointer);
  Serial.println(getNextPills(pointer));

  unsigned long nextDispenseTime = getNextDispenseEpochTime(pointer);
  if (nextDispenseTime < 1) {
    return;
  }
  // FIX: turnOff() function definition was moved up to be before or was forward-declared.
  turnOff();

  if (nextDispenseTime < getEpochTime()) {
    Serial.print("the time is now: ");
    Serial.println(getEpochTime());
    lcd.clear();
    lcd.print("Dispensing Pills");

    int pills = getNextPills(pointer);
    Serial.print("Pills to dispense are ");
    Serial.println(pills);
    turnRed();
    switch (pills) {
      case 1:
        motor1.step(2048);  // 2048 before
        break;
      case 2:
        motor1.step(2048);  // 2048 before
        break;
      case 3:
        motor1.step(2048);  // 2048 before
        break;
      case 12:
        motor1.step(2048);  // 2048 before
        break;
      case 13:
        motor1.step(2048);  // 2048 before
        break;
      case 23:
        motor1.step(2048);  // 2048 before
        break;
      case 123:
        motor1.step(2048);  // 2048 before
        break;
    }

    // Power off the steppers to save energy and prevent overheating
    digitalWrite(IN1_1, LOW);
    digitalWrite(IN2_1, LOW);
    digitalWrite(IN3_1, LOW);
    digitalWrite(IN4_1, LOW);


    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Next Dosage At: ");
    lcd.setCursor(0, 1);
    lcd.print(getNextDispenseMessage(pointer + 1));
    setPointer(pointer + 1);

    turnGreen();
    turnOff();

    //delay(10000);
  } else {
    Serial.print("Time untill next dispense: ");
    Serial.println(nextDispenseTime - getEpochTime());
    Serial.println("----------------------------------");
  }

  delay(1000);
}

int getPointer() {
  if (Firebase.ready() && signupOK) {
    if (Firebase.getInt(fbdo, "/test/pointer")) {
      if (fbdo.dataType() == "int") {
        int pointer = fbdo.intData();
        return pointer;
      }
    } else {
      Serial.println(fbdo.errorReason());
      return -1;
    }
  }
  return -1; // Added a return for the case where Firebase is not ready or signup is not OK
}

void setPointer(int value) {
  if (Firebase.ready() && signupOK) {
    if (Firebase.setInt(fbdo, "/test/pointer", value)) {
      // Success, no need for action
    } else {
      Serial.print("Failed to set pointer: ");
      Serial.println(fbdo.errorReason());
    }
  }
}

unsigned long getNextDispenseEpochTime(int pointer) {
  if (Firebase.ready() && signupOK) {
    if (Firebase.getArray(fbdo, "/test/time")) {
      FirebaseJsonArray &arr = fbdo.jsonArray();
      String in = String(arr.raw());


      in.remove((in.length() - 1), 2);
      in.remove(0, 1);
      in.replace(']', ' ');
      in.replace('"', ' ');
      in.replace(' , ', '_');

      String strs[20];
      int StringCount = 0;


      while (in.length() > 0) {
        int index = in.indexOf(',');
        if (index == -1)  // No space found
        {
          strs[StringCount++] = in;
          break;
        } else {
          strs[StringCount++] = in.substring(0, index);
          in = in.substring(index + 1);
        }
      }

      // Check if the pointer is within the bounds of the array
      if (pointer >= 0 && pointer < StringCount) {
        return strtoul(strs[pointer].c_str(), NULL, 10);
      } else {
        Serial.println("Pointer out of bounds in getNextDispenseEpochTime.");
        return 0;
      }

    } else {
      Serial.println(fbdo.errorReason());
    }
  }
  return 0;
}

String getNextDispenseMessage(int pointer) {
  if (Firebase.ready() && signupOK) {
    if (Firebase.getArray(fbdo, "/test/dateTimeArray")) {
      FirebaseJsonArray &arr = fbdo.jsonArray();
      String in = String(arr.raw());

      in.remove((in.length() - 1), 2);
      in.remove(0, 1);
      in.replace('"', ' ');

      String strs[20];
      int StringCount = 0;

      while (in.length() > 0) {
        int index = in.indexOf(',');
        if (index == -1)  // No space found
        {
          strs[StringCount++] = in;
          break;
        } else {
          strs[StringCount++] = in.substring(0, index);
          in = in.substring(index + 1);
        }
      }

      if (pointer >= 0 && pointer < StringCount) {
        String message = strs[pointer];
        message.remove(0, 1);
        if (message != "") {
          return message;
        } else {
          return "no dosage set...";
        }
      } else {
        return "Pointer out of bounds...";
      }


    } else {
      Serial.println(fbdo.errorReason());
    }
  }
  return "";
}

int getNextPills(int pointer) {
  if (Firebase.ready() && signupOK) {
    if (Firebase.getArray(fbdo, "/test/pills")) {
      FirebaseJsonArray &arr = fbdo.jsonArray();
      String in = String(arr.raw());


      in.remove((in.length() - 1), 2);
      in.remove(0, 1);
      in.replace('"', ' ');

      String strs[20];
      int StringCount = 0;

      Serial.println(in);

      while (in.length() > 0) {
        int index = in.indexOf(',');
        if (index == -1)  // No space found
        {
          strs[StringCount++] = in;
          break;
        } else {
          strs[StringCount++] = in.substring(0, index);
          in = in.substring(index + 1);
        }
      }
      
      // Check if the pointer is within the bounds of the array
      if (pointer >= 0 && pointer < StringCount) {
        return atoi(strs[pointer].c_str());
      } else {
        Serial.println("Pointer out of bounds in getNextPills.");
        return -1;
      }

    } else {
      Serial.println(fbdo.errorReason());
    }
  }
  return -1;
}

//Time
unsigned long getEpochTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

//LEDS
void turnRed() {
  FastLED.setBrightness(20);
  leds[1] = CRGB::Red;
  FastLED.show();
}

void turnGreen() {
  FastLED.setBrightness(5);
  // FIX: Removed the extra '101' which caused the error "expected ';' at end of input" and "expected '}' at end of input"
  leds[1] = CRGB::Green;
  FastLED.show();
}

void turnOff() {
  leds[1] = CRGB::Black;
  FastLED.show();
}
