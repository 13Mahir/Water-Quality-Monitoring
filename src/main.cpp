#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Pin Definitions
#define PIN_PH 34
#define PIN_TDS 35
#define PIN_TEMP 4
#define PIN_BUZZER 25
#define PIN_LED_RED 26
#define PIN_LED_GREEN 27

// Wi-Fi Configuration
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ===== HiveMQ Configuration (Dashboard) =====
const char* hivemq_server = "broker.hivemq.com";
const int hivemq_port = 1883;
const char* dataTopic = "water/quality/data";
const char* controlTopic = "water/quality/control";

WiFiClient hivemqWifi;
PubSubClient hivemqClient(hivemqWifi);

// ===== ThingSpeak MQTT Configuration =====
const char* ts_server = "mqtt3.thingspeak.com";
const int ts_port = 1883;
const char* ts_clientID = "DQsUHQwBChYEBDYzJjQFCAM";
const char* ts_user = "DQsUHQwBChYEBDYzJjQFCAM";
const char* ts_pass = "fztBjs7t+ofGb4XHTmCJMfgd";
const char* ts_topic = "channels/3342954/publish";

WiFiClient tsWifi;
PubSubClient tsClient(tsWifi);

// Timing
unsigned long lastDashboardTime = 0;
const long DASHBOARD_INTERVAL = 2000;   // 2s for HiveMQ dashboard

unsigned long lastThingSpeakTime = 0;
const long THINGSPEAK_INTERVAL = 20000; // 20s for ThingSpeak rate limit

LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(PIN_TEMP);
DallasTemperature sensors(&oneWire);

// State vars
int tds_limit = 500;
bool forceBuzzer = false;
bool forceLED = false;

// Latest sensor readings (shared between both publish cycles)
float temp_c = 0.0;
float ph_val = 0.0;
int tds_val = 0;
bool isUnsafe = false;
bool isWarning = false;

// Callback for incoming MQTT control commands (HiveMQ)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Received Command: ");
  Serial.println(message);

  if (message.indexOf("\"buzzer\":\"on\"") > 0) forceBuzzer = true;
  if (message.indexOf("\"buzzer\":\"off\"") > 0) forceBuzzer = false;
  
  if (message.indexOf("\"led\":\"on\"") > 0) forceLED = true;
  if (message.indexOf("\"led\":\"off\"") > 0) forceLED = false;

  int limitIndex = message.indexOf("\"tds_limit\":");
  if (limitIndex > 0) {
    int start = limitIndex + 12;
    int end = message.indexOf("}", start);
    if(end > start) {
      tds_limit = message.substring(start, end).toInt();
      Serial.printf("New TDS Limit set from dashboard: %d ppm\n", tds_limit);
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnectHiveMQ() {
  if (!hivemqClient.connected()) {
    Serial.print("Connecting to HiveMQ...");
    String clientId = "ESP32WaterQC-";
    clientId += String(random(0xffff), HEX);
    if (hivemqClient.connect(clientId.c_str())) {
      Serial.println("connected!");
      hivemqClient.subscribe(controlTopic);
    } else {
      Serial.print("failed(rc=");
      Serial.print(hivemqClient.state());
      Serial.println(")");
    }
  }
}

void reconnectThingSpeak() {
  if (!tsClient.connected()) {
    Serial.print("Connecting to ThingSpeak MQTT...");
    if (tsClient.connect(ts_clientID, ts_user, ts_pass)) {
      Serial.println("connected!");
    } else {
      Serial.print("failed(rc=");
      Serial.print(tsClient.state());
      Serial.println(")");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PH, INPUT);
  pinMode(PIN_TDS, INPUT);
  ledcSetup(0, 1000, 8);       // Channel 0, 1kHz default, 8-bit resolution
  ledcAttachPin(PIN_BUZZER, 0); // Attach buzzer pin to LEDC channel 0
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  ledcWriteTone(0, 0);  // Buzzer off initially
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Water Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");
  
  setup_wifi();
  
  // Setup both MQTT clients
  hivemqClient.setServer(hivemq_server, hivemq_port);
  hivemqClient.setCallback(mqttCallback);

  tsClient.setServer(ts_server, ts_port);

  sensors.begin();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!!");
  delay(1000);
  lcd.clear();
}

void readSensors() {
  sensors.requestTemperatures();
  temp_c = sensors.getTempCByIndex(0);
  
  int ph_raw = analogRead(PIN_PH);
  int tds_raw = analogRead(PIN_TDS);
  ph_val = (ph_raw / 4095.0) * 14.0;
  tds_val = map(tds_raw, 0, 4095, 0, 1000);

  isUnsafe = false;
  isWarning = false;
  
  if (ph_val < 6.5 || ph_val > 8.5 || tds_val > tds_limit || temp_c > 35.0 || temp_c == DEVICE_DISCONNECTED_C) {
    isUnsafe = true;
  } else if (tds_val > (tds_limit - 100) || temp_c > 32.0 || ph_val < 6.8 || ph_val > 8.0) {
    isWarning = true;
  }
}

void loop() {
  // Maintain both MQTT connections (non-blocking reconnect)
  reconnectHiveMQ();
  reconnectThingSpeak();
  hivemqClient.loop();
  tsClient.loop();

  unsigned long now = millis();

  // ===== DASHBOARD UPDATE (every 2 seconds) =====
  if (now - lastDashboardTime > DASHBOARD_INTERVAL) {
    lastDashboardTime = now;

    readSensors();

    // LCD Output
    lcd.setCursor(0, 0);
    lcd.print("pH:");
    lcd.print(ph_val, 1);
    lcd.print(" T:");
    lcd.print(temp_c, 1);
    lcd.print("C  ");
    lcd.setCursor(0, 1);
    lcd.print("TDS: ");
    lcd.print(tds_val);
    lcd.print(" ppm   ");

    // Actuate LEDs & Buzzer
    if (isUnsafe || forceLED) {
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_RED, HIGH);
    } else {
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_RED, LOW);
    }
    
    if (isUnsafe || forceBuzzer) {
       ledcWriteTone(0, 1000); // Channel 0, 1000 Hz continuous tone
    } else {
       ledcWriteTone(0, 0);    // Silence
    }

    // Build JSON payload for HiveMQ Dashboard
    String statusStr = isUnsafe ? "UNFIT" : (isWarning ? "WARNING" : "SAFE");
    
    String payload = "{";
    payload += "\"temperature\":" + String(temp_c, 1) + ",";
    payload += "\"ph\":" + String(ph_val, 1) + ",";
    payload += "\"tds\":" + String(tds_val) + ",";
    payload += "\"status\":\"" + statusStr + "\"";
    payload += "}";

    // Structured Terminal Logging
    Serial.print("\r\n==================================\r\n");
    Serial.printf(" Temp : %.1f C\r\n", temp_c);
    Serial.printf(" pH   : %.1f\r\n", ph_val);
    Serial.printf(" TDS  : %d ppm\r\n", tds_val);
    Serial.print("----------------------------------\r\n");
    Serial.printf(" LIMIT: %d ppm (Threshold)\r\n", tds_limit);
    Serial.print("----------------------------------\r\n");
    if (isUnsafe) {
      Serial.print(" [!] STATUS: UNFIT WATER\r\n");
    } else if (isWarning) {
      Serial.print(" [?] STATUS: WARNING\r\n");
    } else {
      Serial.print(" [V] STATUS: SAFE WATER\r\n");
    }
    Serial.print("==================================\r\n\r\n");

    // Publish to HiveMQ (Dashboard)
    if(hivemqClient.publish(dataTopic, payload.c_str())) {
      Serial.print("-> HiveMQ: ");
      Serial.println(payload);
    } else {
      Serial.println("-> HiveMQ PUBLISH FAILED");
    }
  }

  // ===== THINGSPEAK UPDATE (every 20 seconds) =====
  if (now - lastThingSpeakTime > THINGSPEAK_INTERVAL) {
    lastThingSpeakTime = now;

    int safe_status = isUnsafe ? 0 : 1;
    String tsPayload = "field1=" + String(temp_c, 1) + 
                       "&field2=" + String(ph_val, 1) + 
                       "&field3=" + String(tds_val) + 
                       "&field4=" + String(safe_status);

    if(tsClient.publish(ts_topic, tsPayload.c_str())) {
      Serial.print("-> ThingSpeak: ");
      Serial.println(tsPayload);
    } else {
      Serial.println("-> ThingSpeak PUBLISH FAILED");
    }
  }
}