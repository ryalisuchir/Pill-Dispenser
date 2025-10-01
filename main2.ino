#include <Arduino.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>
#include <FastLED.h>

// Stepper
const int steps_per_rev = 2048;
const int servoSpeed = 10;

#define IN1_1 15
#define IN2_1 2
#define IN3_1 4
#define IN4_1 18
Stepper motor1(steps_per_rev, IN1_1, IN2_1, IN3_1, IN4_1);

#define IN1_2 13
#define IN2_2 12
#define IN3_2 14
#define IN4_2 27
Stepper motor2(steps_per_rev, IN1_2, IN2_2, IN3_2, IN4_2);

// LCD
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// LED
#define NUM_LEDS 5
#define LED_DATA_PIN 25
CRGB leds[NUM_LEDS];

// --- Function Declarations ---
void turnRed();
void turnGreen();
void turnOff();

// --- Setup ---
void setup() {
  Serial.begin(115200);

  // Stepper Setup
  motor1.setSpeed(servoSpeed);
  motor2.setSpeed(servoSpeed);

  // FastLED Setup
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(20);

  // LCD Setup
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // --- Start Test Sequence ---

  // 1. Initial State
  turnOff();
  Serial.println("Starting sequence...");
  lcd.setCursor(0, 0);
  lcd.print("Please add");
  lcd.setCursor(0, 1);
  lcd.print("medications!");
  delay(3000);

  // Countdown loop: 15 → 1
  // for (int i = 15; i > 0; i--) {
  //   lcd.clear();
  //   lcd.setCursor(0, 0);
  //   lcd.print("Next dose in");
  //   lcd.setCursor(0, 1);
  //   lcd.print(i);
  //   lcd.print(" second");
  //   if (i > 1) lcd.print("s"); // plural
  //   delay(1000);
  // }

  // 2. Dispensing State (RED LED)
  turnRed(); // Signal an action/alert
  Serial.println("Dispensing Pill 1...");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dispensing:");
  lcd.setCursor(0, 1);
  lcd.print("Pill 1");

  // Activate Motor 1 and Motor 2 for one full revolution
  motor1.step(steps_per_rev);

    digitalWrite(IN1_1, LOW);
  digitalWrite(IN2_1, LOW);
  digitalWrite(IN3_1, LOW);
  digitalWrite(IN4_1, LOW);

  // Power off motor 2 coils

  // 3. Success State (GREEN LED)
  turnGreen(); // Signal success
  Serial.println("Dispense Complete.");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Next dose in");
  lcd.setCursor(0, 1);
  lcd.print("11.3 hours");

  delay(5000); // Display success message for 5 seconds

  // 4. Final State (LED Off, LCD Instructions)
  turnOff();
  Serial.println("Setup complete. Entering infinite loop.");
}

// --- Loop ---
void loop() {
  // End the code after the sequence
  while (true) {
    delay(100);
  }
}

// --- LED Functions ---
void turnRed() {
  FastLED.setBrightness(20);
  leds[1] = CRGB::Red;
  FastLED.show();
}

void turnGreen() {
  FastLED.setBrightness(5);
  leds[1] = CRGB::Green;
  FastLED.show();
}

void turnOff() {
  leds[1] = CRGB::Black;
  FastLED.show();
}
