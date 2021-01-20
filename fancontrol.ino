#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int TACH_PIN = 2;    // Tachometer input pin
const int PWM_PIN = 3;     // Only works with Pin 1(PB1)
const int PWM_TOP = 320;   // PWM TOP_value
const int LOWER_BOUND = 4; // 1.25% mimimum (1.25 * (320 / 100))
const unsigned long SERIAL_UPDATE_INTERVAL = 1000; // every 1 second

int speed = 5; // Initial fan speed
unsigned long serialLastUpdateMillis = 0;
unsigned long calcRPMLastMillis = 0;
volatile unsigned long tachPulseCount = 0;

LiquidCrystal_I2C lcd(0x3f, 16, 2); // LCD 1602

void CountTachPulse()
{
  // Count each pulse from the tach pin of the fan
  tachPulseCount++;
}

void setup()
{
  // LCD Initialization
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Initializing....");
  
  // Setup tach
  Serial.begin(9600);
  Serial.println("INIT");
  pinMode(TACH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TACH_PIN), CountTachPulse, FALLING);

  // Setup PWM control
  pinMode(PWM_PIN, OUTPUT);
  // Reset Timer 2
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;

  // Phase Corrected PWM mode
  // PWM on Pin 3, Pin 11 disabled
  // 16Mhz / 2 / 320 = 25Khz
  TCCR2A = _BV(COM2B1) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS20);

  // Set TOP
  OCR2A = PWM_TOP; // TOP - DO NOT CHANGE, SETS PWM PULSE RATE
  int out = map(speed, 0, 100, LOWER_BOUND, PWM_TOP / 5);
  OCR2B = out;
}

unsigned long CalculateRPM(float currentMillis)
{
  // Calculate RPM based on time passed and pulses counted.
  noInterrupts();
  // Calculate RPM
  float elapsedMS = (currentMillis - calcRPMLastMillis);
  unsigned long rpm = (tachPulseCount / (elapsedMS / 1000.0) / 2.0) * 60;
  // Reset for next round
  calcRPMLastMillis = currentMillis;
  tachPulseCount = 0;
  interrupts();
  return rpm;
}

void loop()
{
  // Update serial monitor with RPM every second
  float currentMillis = millis();
  if (currentMillis - serialLastUpdateMillis >= SERIAL_UPDATE_INTERVAL)
  {
    unsigned long rpm = CalculateRPM(currentMillis);
    lcd.setCursor(0,0);
    lcd.print("Fan Speed = ");
    lcd.print(speed);
    lcd.print("    ");
    lcd.setCursor(0,1);
    lcd.print("Fan RPM = ");
    lcd.print(rpm);
    lcd.print("     ");
    
    Serial.print("[");
    Serial.print(currentMillis);
    Serial.print("]");
    Serial.print("Speed=");
    Serial.print(speed);
    Serial.print(",RPM=");
    Serial.println(rpm);
    serialLastUpdateMillis = millis();
  }
  while (Serial.available() > 0)
  {
    speed = Serial.parseInt();
    if (speed <= 0 || speed > 100)
    {
      continue;
    }
    int out = map(speed, 0, 100, LOWER_BOUND, PWM_TOP / 5);
    OCR2B = out;
    Serial.read();
  }
}
