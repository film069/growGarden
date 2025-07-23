#define BLYNK_TEMPLATE_ID "YOUR-TEMPLATE-ID"
#define BLYNK_TEMPLATE_NAME "YOUR-TEMPLATE-NAME"
#define BLYNK_AUTH_TOKEN "YOUR-AUTH-TOKEN"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHTesp.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

//for Blynk
const char* wifiSsid = "Wokwi-GUEST";
const char* wifiPassword = "";
bool onlineStatus = false;
bool unstablepHSent = false;
bool coldTempSent = false;
bool hotTempSent = false;

#define controlVpin V0
#define statusVpin V1
#define tempVpin V2
#define humidVpin V3
#define phVpin V4
#define cVpin V5
#define nVpin V6
#define hVpin V7
#define sprinklerVpin V8

//Constants & Variables
int sensorValue; //variable to store sensor value
int pos = 0;
const int DHT_PIN = 32;
const int PH_Pin = 36;
const int servoPin = 2;
const int buzzerPin = 15;
const int buzztone = 523;
const int sensorReadingInterval = 1000;
unsigned long lastSensorReadingTime = 0;
const float minTemp = 18;
const float maxTemp = 25;
const float minHumid = 50;
const float maxHumid = 70;

DHTesp dhtSensor;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

void showStatus(bool status) {
    String statusString;
    Blynk.virtualWrite(statusVpin, status);
}

BLYNK_CONNECTED() {
    Serial.println("Blynk connected");
    Blynk.syncVirtual(controlVpin);
}

BLYNK_WRITE(controlVpin) {
    int pinValue = param.asInt();
    if (pinValue == 1) {
        onlineStatus = true;
        Serial.println("System Active");
    } else {
        onlineStatus = false;
        Serial.println("System Inactive");
    }
    showStatus(onlineStatus);
}

void setup() {
  pinMode(19,OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  lcd.init();
  lcd.backlight();
  servo.attach(servoPin);

  Blynk.begin(BLYNK_AUTH_TOKEN, wifiSsid, wifiPassword);
  Serial.println("Setup Complete");
  
  // Print Title
  lcd.setCursor(3, 0);
  lcd.print("Monitoring");
  lcd.setCursor(3, 1);
  lcd.print("Hidroponik");
  delay(3000);
}

void loop() {
  Blynk.run();
  unsigned long currentTime = millis();
  bool shouldMeasure = (currentTime - lastSensorReadingTime) > sensorReadingInterval;

if (onlineStatus && shouldMeasure){
 lastSensorReadingTime = currentTime;

  TempAndHumidity  data = dhtSensor.getTempAndHumidity();
  int16_t phValue = analogRead(PH_Pin);  //DHT & pH sensor
  
  Blynk.virtualWrite(tempVpin, data.temperature);
  Blynk.virtualWrite(humidVpin, data.humidity);
  Blynk.virtualWrite(phVpin, phValue);
  
  digitalWrite(5, LOW);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);

//temp conditions
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Temp: ");
  lcd.print(data.temperature,1);
  lcd.print((char)223); 
  lcd.print("C");
  lcd.setCursor(4, 1);
  if(data.temperature <= minTemp){
    lcd.print("Too Cold");
    digitalWrite(5, HIGH);
    Blynk.virtualWrite(cVpin, 255);
    Blynk.virtualWrite(nVpin, 0);
    Blynk.virtualWrite(hVpin, 0);
    if(!coldTempSent){
      Blynk.logEvent("cold_air", "The air temperature for your hydroponic plants is too cool!");
      coldTempSent = true;
    }
  }else if(data.temperature >= maxTemp){
    lcd.print("Too Hot");
    digitalWrite(19, HIGH);
    Blynk.virtualWrite(cVpin, 0);
    Blynk.virtualWrite(nVpin, 0);
    Blynk.virtualWrite(hVpin, 255);
    if(!hotTempSent){
      Blynk.logEvent("hot_air", "The air temperature for your hydroponic plants is too warm!");
      hotTempSent = true;
    }
  }else{
    lcd.print("Stable");
    digitalWrite(18, HIGH);
    Blynk.virtualWrite(cVpin, 0);
    Blynk.virtualWrite(nVpin, 255);
    Blynk.virtualWrite(hVpin, 0);
    coldTempSent = false; //Reset the temp notification Flag
    hotTempSent = false;
  }
  delay(1500);

//Humid conditions
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humidity: ");
  lcd.print(data.humidity,1);
  lcd.print("%");
  lcd.setCursor(2, 1);
  if(data.humidity <= minHumid){
    lcd.print("Watering....");
    Blynk.virtualWrite(sprinklerVpin, 255);
    for (pos = 0; pos <= 90; pos += 1) {
    servo.write(pos);
    delay(10);
  }
  for (pos = 90; pos >= 0; pos -= 1) {
    servo.write(pos);
    delay(10);
  }
  }else{
    lcd.print("");
    Blynk.virtualWrite(sprinklerVpin, 0);
  }
  delay(1500);

//pH conditions
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("pH: ");
  lcd.print(phValue,1);
  lcd.setCursor(2, 1);
  if(phValue < 5 || phValue > 7){
    lcd.print("Status: Bad");
    tone(buzzerPin, buzztone, 1250);
    if(!unstablepHSent){
      Blynk.logEvent("unstable_ph", "The hydroponic water's pH is unstable!");
      unstablepHSent = true;
    }
  }else{
    lcd.print("Status: Good");
    unstablepHSent = false; //Reset the pH Flag
  }
  delay(1500);

 }else{
  digitalWrite(5, LOW);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Inactive");
  delay(1000);
 }
}