#include <SoftwareSerial.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define pins for sensors
#define DHTPIN 2
#define DHTTYPE DHT11
#define MQ2PIN A0

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Check your LCD address, commonly 0x27 or 0x3F

// Setup SoftwareSerial for ESP32 Gateway (RX = D8, TX = D9)
SoftwareSerial espSerial(8, 9);

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 2000; // Send standard updates every 2 seconds

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600); // Communication with ESP32 Gateway
  dht.begin();
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("IoT Node Started");
  
  pinMode(MQ2PIN, INPUT);
  
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensors
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int gas = analogRead(MQ2PIN);
  
  // Validate sensor readings
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // Update local LCD Dashboard
    lcd.setCursor(0, 0);
    lcd.print("T:"); lcd.print(t, 1); lcd.print("C H:"); lcd.print(h, 0); lcd.print("%  ");
    
    // Check QoS conditions: Gas level above 400 is considered an emergency
    bool isEmergency = (gas > 400);

    if (isEmergency) {
      lcd.setCursor(0, 1);
      lcd.print("ALERT! Gas:"); lcd.print(gas); lcd.print(" ");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Gas: "); lcd.print(gas); lcd.print("      ");
    }
    
    // QoS Logic: If it is an emergency, we send data IMMEDIATELY (bypassing the interval)
    if (currentMillis - lastSendTime >= sendInterval || isEmergency) { 
      lastSendTime = currentMillis;
      
      // Construct a simple JSON payload
      // e.g. {"t": 25.5, "h": 60.0, "g": 350, "q": "high"} 
      // q represents the QoS priority level
      static unsigned long seq = 0;

      String qosLevel = isEmergency ? "critical" : "normal";

      String payload =
      "{\"t\":" + String(t,1) +
      ",\"h\":" + String(h,1) +
      ",\"g\":" + String(gas) +
      ",\"q\":\"" + qosLevel +
      "\",\"seq\":" + String(seq++) +
      ",\"node\":\"UNO\"}";
      Serial.println("TX to Gateway: " + payload);
      espSerial.println(payload);
      
      // Add a slight delay if it's an emergency to avoid flooding the UART too quickly
      if (isEmergency) {
        delay(500); 
      }
    }
  }
  
  delay(100); // Small stability delay
}
