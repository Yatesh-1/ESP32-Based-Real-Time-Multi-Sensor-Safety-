#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Access Point Settings
const char* ap_ssid = "ESP32_SENSORS";
const char* ap_password = "12345678";

WebServer server(80);

// Pin definitions
#define MQ_PIN       34    // Gas sensor (analog VP)
#define LDR_PIN      32    // Light sensor (analog)
#define FLAME_PIN    2     // Flame sensor (digital)
#define VOICE_PIN    33    // Voice module A0
#define DHT_PIN      4     // DHT11 data pin
#define DHTTYPE      DHT11

DHT dht(DHT_PIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// *** FUNCTION PROTOTYPES - FIXES COMPILATION ***
void handleRoot();
void handleData();
void updateLCD();

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Wire.begin(21, 22);  // Explicit I2C pins
  
  // Pin configurations
  pinMode(FLAME_PIN, INPUT_PULLUP);
  pinMode(LDR_PIN, INPUT);
  
  // DHT stabilization
  delay(2000);
  dht.begin();
  
  // LCD initialization
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sensors Starting");
  delay(2000);
  
  // Access Point
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("=== ESP32 SENSORS ===");
  Serial.println("SSID: ESP32_SENSORS");
  Serial.println("Pass: 12345678");
  Serial.println("IP: 192.168.4.1");
  
  lcd.clear();
  lcd.print("Connect WiFi:");
  lcd.setCursor(0,1);
  lcd.print("ESP32_SENSORS");
  delay(2000);
  
  // Web server routes - FIXED ORDER
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  
  lcd.clear();
  lcd.print("IP: 192.168.4.1");
  lcd.setCursor(0,1);
  lcd.print("Ready!");
}

void loop() {
  server.handleClient();
  
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 3000) {
    updateLCD();
    lastUpdate = millis();
  }
}

void updateLCD() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int gas = analogRead(MQ_PIN);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:");
  if (!isnan(temp)) {
    lcd.print(temp, 1);
    lcd.print("C");
  } else {
    lcd.print("ERR");
  }
  
  lcd.setCursor(8,0);
  lcd.print("H:");
  if (!isnan(hum)) {
    lcd.print(hum, 0);
    lcd.print("%");
  }
  
  lcd.setCursor(0,1);
  lcd.print("Gas:");
  lcd.print(gas/50);
}

void handleRoot() {
  server.send(200, "text/html", getDashboard());
}

void handleData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (isnan(temp)) temp = 25.0;
  if (isnan(hum)) hum = 50.0;
  
  int gas = analogRead(MQ_PIN);
  int ldr = analogRead(LDR_PIN);
  int flame = digitalRead(FLAME_PIN);
  int voice = analogRead(VOICE_PIN);
  
  // Flame inverted logic
  bool flameDetected = (flame == LOW);
  
  String json = "{";
  json += "\"gas\":" + String(gas/10) + ",";
  json += "\"temp\":" + String(temp,1) + ",";
  json += "\"hum\":" + String(hum,0) + ",";
  json += "\"flame\":" + String(flameDetected?1:0) + ",";
  json += "\"voice\":" + String(map(constrain(voice,50,3000),50,3000,0,100)) + ",";
  json += "\"light\":" + String(map(ldr,0,4095,0,100));
  json += "}";
  
  server.send(200, "application/json", json);
}

String getDashboard() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (isnan(temp)) temp = 25.0;
  if (isnan(hum)) hum = 50.0;
  
  int gasRaw = analogRead(MQ_PIN);
  int ldrRaw = analogRead(LDR_PIN);
  int flame = digitalRead(FLAME_PIN);
  int voiceRaw = analogRead(VOICE_PIN);
  
  int gas = gasRaw / 10;
  int light = map(ldrRaw, 0, 4095, 0, 100);
  int voiceLevel = map(constrain(voiceRaw, 50, 3000), 50, 3000, 0, 100);
  bool fire = (flame == LOW);
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP32 Sensors</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:0;padding:20px;background:linear-gradient(135deg,#667eea,#764ba2);color:#333;}";
  html += ".container{max-width:900px;margin:auto;background:rgba(255,255,255,0.95);padding:30px;border-radius:20px;box-shadow:0 20px 40px rgba(0,0,0,0.2);}";
  html += "h1{text-align:center;font-size:2.2em;margin-bottom:20px;color:#333;}";
  html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px;margin:20px 0;}";
  html += ".card{padding:25px;border-radius:15px;background:white;box-shadow:0 10px 25px rgba(0,0,0,0.1);border-left:5px solid #ddd;}";
  html += ".card:hover{transform:translateY(-3px);transition:0.3s;}";
  html += ".gas{border-left-color:#ff6b6b;}.temp{border-left-color:#4ecdc4;}.hum{border-left-color:#45b7d1;}";
  html += ".flame{border-left-color:#f0932b;}.voice{border-left-color:#6c5ce7;}.light{border-left-color:#feca57;}";
  html += ".header{display:flex;align-items:center;margin-bottom:15px;}";
  html += ".icon{width:50px;height:50px;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:24px;color:white;margin-right:15px;}";
  html += ".gas-icon{background:linear-gradient(45deg,#ff6b6b,#ff8e8e);}";
  html += ".temp-icon{background:linear-gradient(45deg,#4ecdc4,#44a08d);}";
  html += ".hum-icon{background:linear-gradient(45deg,#45b7d1,#96c93d);}";
  html += ".flame-icon{background:linear-gradient(45deg,#f0932b,#f5576c);}";
  html += ".voice-icon{background:linear-gradient(45deg,#6c5ce7,#a29bfe);}";
  html += ".light-icon{background:linear-gradient(45deg,#feca57,#ff9ff3);}";
  html += ".value{font-size:3em;font-weight:bold;margin:15px 0;text-shadow:2px 2px 4px rgba(0,0,0,0.1);}";
  html += ".normal{color:#27ae60;}.danger{color:#e74c3c;}.warning{color:#f39c12;}";
  html += ".status{padding:12px 24px;border-radius:30px;font-weight:bold;font-size:1.1em;display:inline-block;margin-top:10px;}";
  html += ".safe{background:#d5f4e6;color:#27ae60;}.danger{background:#fadbd8;color:#e74c3c;}";
  html += ".info{text-align:center;color:#666;margin-top:30px;padding:20px;background:rgba(0,0,0,0.05);border-radius:15px;font-size:1.1em;}";
  html += "@media(max-width:600px){.grid{grid-template-columns:1fr;}.value{font-size:2.2em;}}</style>";
  html += "<script>setTimeout(function(){location.reload();},3000);</script>";
  html += "</head><body><div class='container'>";
  html += "<h1>🏠 ESP32 Sensor Dashboard</h1>";
  html += "<div style='background:#e8f4f8;padding:20px;border-radius:15px;margin-bottom:25px;text-align:center;font-family:monospace;font-size:1.2em;'>";
  html += "<strong>🌐 ESP32_SENSORS</strong><br>📱 IP: <strong>192.168.4.1</strong><br>🔄 Auto Refresh: 3s</div>";
  html += "<div class='grid'>";
  
  // Gas
  html += "<div class='card gas'><div class='header'><div class='icon gas-icon'>☣️</div><h3>Gas Sensor</h3></div>";
  html += "<div class='value " + String(gas>150 ? "danger" : "normal") + "'>" + String(gas) + "</div>";
  html += "<div class='status " + String(gas>150 ? "danger" : "safe") + "'>" + String(gas>150 ? "🚨 LEAK!" : "✅ CLEAN") + "</div></div>";
  
  // Temp
  html += "<div class='card temp'><div class='header'><div class='icon temp-icon'>🌡️</div><h3>Temperature</h3></div>";
  html += "<div class='value " + String(temp>40 ? "danger" : "normal") + "'>" + String(temp,1) + "°C</div>";
  html += "<div class='status " + String(temp>40 ? "danger" : "safe") + "'>" + String(temp>40 ? "🚨 HOT!" : "✅ OK") + "</div></div>";
  
  // Humidity
  html += "<div class='card hum'><div class='header'><div class='icon hum-icon'>💧</div><h3>Humidity</h3></div>";
  html += "<div class='value normal'>" + String(hum,0) + "%</div>";
  html += "<div class='status safe'>✅ OK</div></div>";
  
  // Flame
  html += "<div class='card flame'><div class='header'><div class='icon flame-icon'>🔥</div><h3>Flame</h3></div>";
  html += "<div class='value " + String(fire ? "danger" : "normal") + "'>" + String(fire?"FIRE!":"SAFE") + "</div>";
  html += "<div class='status " + String(fire ? "danger" : "safe") + "'>" + String(fire?"🚨 FIRE!":"✅ SAFE") + "</div></div>";
  
  // Voice
  html += "<div class='card voice'><div class='header'><div class='icon voice-icon'>🔊</div><h3>Voice Level</h3></div>";
  html += "<div class='value " + String(voiceLevel>70 ? "warning" : "normal") + "'>" + String(voiceLevel) + "%</div>";
  html += "<div class='status " + String(voiceLevel>70 ? "warning" : "safe") + "'>" + String(voiceLevel>70 ? "🗣️ SPEAKING" : "🤫 QUIET") + "</div></div>";
  
  // Light
  html += "<div class='card light'><div class='header'><div class='icon light-icon'>☀️</div><h3>Light</h3></div>";
  html += "<div class='value normal'>" + String(light) + "%</div>";
  html += "<div class='status safe'>" + String(light>50?"BRIGHT":"DIM") + "</div></div>";
  
  html += "</div><div class='info'>";
  html += "📶 WiFi: <strong>ESP32_SENSORS</strong> | 🔑 <strong>12345678</strong><br>";
  html += "<a href='/data' style='color:#007bff;font-weight:bold;font-size:1.2em;'>📊 Raw Sensor Data</a>";
  html += "</div></div></body></html>";
  
  return html;
}
