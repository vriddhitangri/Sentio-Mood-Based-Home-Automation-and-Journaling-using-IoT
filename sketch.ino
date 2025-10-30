#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <MPU6050.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define HRV_SENSOR_PIN      34
#define GSR_SENSOR_PIN      35
#define TEMP_SENSOR_PIN     32
#define ONE_WIRE_BUS        32

#define LED_HAPPY     12
#define LED_SAD       13
#define LED_ANGRY     14
#define LED_FEAR      27
#define LED_SURPRISE  26
#define LED_DISGUST   25
#define LED_NEUTRAL   23

#define LED_WARM_LIGHT       2
#define LED_COOL_BLUE_LIGHT  15
#define LED_FRESH_WHITE      16

#define SERVO_AROMA_PIN  17
#define SERVO_FAN_PIN    18

#define BUTTON_PIN    4

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo_aroma;
Servo servo_fan;
MPU6050 mpu;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

struct SensorData {
  float hrv_bpm;
  float gsr_us;
  float temp_celsius;
  float activity_g;
};

struct EmotionThresholds {
  float hrv_min, hrv_max;
  float gsr_min, gsr_max;
  float temp_min, temp_max;
  float activity_min, activity_max;
  String emotion_name;
  int led_pin;
};

EmotionThresholds emotions[] = {
  {85, 100, 10, 18, 33.1, 34.2, 1.2, 2.5, "Happy", LED_HAPPY},
  {60, 75,  5, 10, 31.8, 32.5, 0.2, 0.6, "Sad", LED_SAD},
  {95, 130, 18, 30, 33.5, 35.0, 2.0, 3.3, "Angry", LED_ANGRY},
  {90, 120, 16, 28, 32.8, 34.0, 1.8, 3.0, "Fear", LED_FEAR},
  {80, 110, 12, 20, 33.0, 33.8, 1.0, 2.2, "Surprise", LED_SURPRISE},
  {70, 85,  8, 15, 32.2, 33.1, 0.5, 1.2, "Disgust", LED_DISGUST},
  {65, 80,  6, 12, 32.0, 32.8, 0.3, 0.8, "Neutral", LED_NEUTRAL}
};

int numEmotions = 7;
int emotionCount[7] = {0};
bool lastButtonState = HIGH;
int sessionCount = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C for LCD and MPU6050
  Wire.begin(21, 22);
  delay(100);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mood-Based Home");
  lcd.setCursor(0, 1);
  lcd.print("Automation IoT");
  
  // Initialize DS18B20 (1-Wire on GPIO 32)
  sensors.begin();
  delay(100);
  
  // Initialize MPU6050 (I2C)
  mpu.initialize();
  delay(100);
  
  if (!mpu.testConnection()) {
    Serial.println("WARNING: MPU6050 connection failed!");
  }
  
  // Initialize emotion indicator LEDs
  pinMode(LED_HAPPY, OUTPUT);
  pinMode(LED_SAD, OUTPUT);
  pinMode(LED_ANGRY, OUTPUT);
  pinMode(LED_FEAR, OUTPUT);
  pinMode(LED_SURPRISE, OUTPUT);
  pinMode(LED_DISGUST, OUTPUT);
  pinMode(LED_NEUTRAL, OUTPUT);
  
  // Initialize home automation LEDs
  pinMode(LED_WARM_LIGHT, OUTPUT);
  pinMode(LED_COOL_BLUE_LIGHT, OUTPUT);
  pinMode(LED_FRESH_WHITE, OUTPUT);
  
  // Initialize servos
  servo_aroma.attach(SERVO_AROMA_PIN);
  servo_fan.attach(SERVO_FAN_PIN);
  delay(100);
  servo_aroma.write(0);
  servo_fan.write(0);
  delay(100);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready!");
  lcd.setCursor(0, 1);
  lcd.print("Press button");
  
  Serial.println("\n========================================");
  Serial.println("MOOD-BASED HOME AUTOMATION SYSTEM");
  Serial.println("With Real Sensors & Servo Motors");
  Serial.println("========================================\n");
}

SensorData readSensors() {
  SensorData data;
  
  // Read HRV from potentiometer (analog on GPIO 34)
  int hrv_raw = analogRead(HRV_SENSOR_PIN);
  data.hrv_bpm = map(hrv_raw, 0, 4095, 50, 150);
  
  // Read GSR from potentiometer (analog on GPIO 35)
  int gsr_raw = analogRead(GSR_SENSOR_PIN);
  data.gsr_us = (gsr_raw / 4095.0) * 30.0 + 2.0;
  
  // Read temperature from DS18B20 (1-Wire on GPIO 32)
  sensors.requestTemperatures();
  data.temp_celsius = sensors.getTempCByIndex(0);
  if (data.temp_celsius == -127.0) {
    data.temp_celsius = 32.0;  // Default if sensor fails
  }
  
  // Read accelerometer from MPU6050 (I2C on GPIO 21/22)
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  // Convert to G-force (magnitude of acceleration vector)
  float accel_x = ax / 16384.0;
  float accel_y = ay / 16384.0;
  float accel_z = az / 16384.0;
  data.activity_g = sqrt(accel_x * accel_x + accel_y * accel_y + accel_z * accel_z);
  
  return data;
}

void turnOffAllEmotionLEDs() {
  digitalWrite(LED_HAPPY, LOW);
  digitalWrite(LED_SAD, LOW);
  digitalWrite(LED_ANGRY, LOW);
  digitalWrite(LED_FEAR, LOW);
  digitalWrite(LED_SURPRISE, LOW);
  digitalWrite(LED_DISGUST, LOW);
  digitalWrite(LED_NEUTRAL, LOW);
}

void turnOffAllHomeLEDs() {
  digitalWrite(LED_WARM_LIGHT, LOW);
  digitalWrite(LED_COOL_BLUE_LIGHT, LOW);
  digitalWrite(LED_FRESH_WHITE, LOW);
}

void stopAllServos() {
  servo_aroma.write(0);
  servo_fan.write(0);
}

String detectEmotion(SensorData data, int* emotionIndex) {
  int bestMatch = -1;
  float bestScore = -1;
  
  for (int i = 0; i < numEmotions; i++) {
    float score = 0;
    if (data.hrv_bpm >= emotions[i].hrv_min && data.hrv_bpm <= emotions[i].hrv_max) score++;
    if (data.gsr_us >= emotions[i].gsr_min && data.gsr_us <= emotions[i].gsr_max) score++;
    if (data.temp_celsius >= emotions[i].temp_min && data.temp_celsius <= emotions[i].temp_max) score++;
    if (data.activity_g >= emotions[i].activity_min && data.activity_g <= emotions[i].activity_max) score++;
    
    if (score > bestScore) {
      bestScore = score;
      bestMatch = i;
    }
  }
  
  turnOffAllEmotionLEDs();
  if (bestMatch >= 0) {
    digitalWrite(emotions[bestMatch].led_pin, HIGH);
    *emotionIndex = bestMatch;
    return emotions[bestMatch].emotion_name;
  }
  
  *emotionIndex = -1;
  return "Unknown";
}

void activateHomeAutomation(String emotion) {
  turnOffAllHomeLEDs();
  stopAllServos();
  
  if (emotion == "Happy") {
    digitalWrite(LED_WARM_LIGHT, HIGH);
    Serial.println("HOME AUTOMATION: Warm lights ON");
    Serial.println("ACTION: Playing upbeat music");
  }
  else if (emotion == "Sad") {
    digitalWrite(LED_WARM_LIGHT, HIGH);
    servo_aroma.write(90);
    Serial.println("HOME AUTOMATION: Warm lights + Aromatherapy diffuser spinning");
    Serial.println("ACTION: Playing calming music & releasing aroma");
  }
  else if (emotion == "Angry") {
    digitalWrite(LED_COOL_BLUE_LIGHT, HIGH);
    Serial.println("HOME AUTOMATION: Cool blue lights ON");
    Serial.println("ACTION: Playing soft classical music");
  }
  else if (emotion == "Fear") {
    digitalWrite(LED_COOL_BLUE_LIGHT, HIGH);
    Serial.println("HOME AUTOMATION: Soft yellow lighting ON");
    Serial.println("ACTION: Playing relaxing nature sounds");
  }
  else if (emotion == "Surprise") {
    Serial.println("HOME AUTOMATION: Colorful blinking lights ON");
    Serial.println("ACTION: Playing cheerful upbeat music");
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_WARM_LIGHT, HIGH);
      delay(300);
      digitalWrite(LED_WARM_LIGHT, LOW);
      
      digitalWrite(LED_COOL_BLUE_LIGHT, HIGH);
      delay(300);
      digitalWrite(LED_COOL_BLUE_LIGHT, LOW);
      
      digitalWrite(LED_FRESH_WHITE, HIGH);
      delay(300);
      digitalWrite(LED_FRESH_WHITE, LOW);
      delay(200);
    }
  }
  else if (emotion == "Disgust") {
    digitalWrite(LED_FRESH_WHITE, HIGH);
    servo_fan.write(90);
    Serial.println("HOME AUTOMATION: Fresh white lights + Fan spinning");
    Serial.println("ACTION: Purifying air with fan");
  }
  else if (emotion == "Neutral") {
    Serial.println("HOME AUTOMATION: Normal ambient lighting");
    Serial.println("ACTION: Background neutral music");
  }
}

void displayEmotion(String emotion, SensorData data) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mood:");
  lcd.print(emotion);
  lcd.setCursor(0, 1);
  lcd.print("HR:");
  lcd.print((int)data.hrv_bpm);
  
  Serial.println("\n========================================");
  Serial.print("Session #"); Serial.println(sessionCount);
  Serial.println("EMOTION DETECTED: " + emotion);
  Serial.println("========================================");
  Serial.print("Heart Rate: "); Serial.print(data.hrv_bpm); Serial.println(" BPM");
  Serial.print("GSR: "); Serial.print(data.gsr_us); Serial.println(" uS");
  Serial.print("Temperature: "); Serial.print(data.temp_celsius); Serial.println(" C");
  Serial.print("Activity: "); Serial.print(data.activity_g); Serial.println(" g");
  Serial.println("========================================\n");
}

void displayJournalSummary() {
  Serial.println("\n========================================");
  Serial.println("EMOTION JOURNAL SUMMARY");
  Serial.println("========================================");
  Serial.println("Emotions logged in current session:");
  for (int i = 0; i < numEmotions; i++) {
    Serial.print("  ");
    Serial.print(emotions[i].emotion_name);
    Serial.print(": ");
    Serial.println(emotionCount[i]);
  }
  Serial.println("========================================\n");
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);
  
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50);
    
    sessionCount++;
    
    SensorData sensorData = readSensors();
    int emotionIdx;
    String detectedEmotion = detectEmotion(sensorData, &emotionIdx);
    
    if (emotionIdx >= 0) {
      emotionCount[emotionIdx]++;
    }
    
    displayEmotion(detectedEmotion, sensorData);
    activateHomeAutomation(detectedEmotion);
    
    delay(5000);
    
    if (sessionCount % 5 == 0) {
      displayJournalSummary();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Check Serial");
      lcd.setCursor(0, 1);
      lcd.print("for summary");
      delay(3000);
    }
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ready!");
    lcd.setCursor(0, 1);
    lcd.print("Press button");
  }
  
  lastButtonState = buttonState;
  delay(10);
}
