#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- WiFi Credentials for Wokwi ---
const char* ssid = "Wokwi-GUEST";         
const char* password = ""; 

// --- Server Configuration ---
// For Wokwi, use your computer's IP address or ngrok URL
const char* BASE_SERVER_URL = "http://10.251.130.138:3000";
const char* RIDE_REQUEST_ENDPOINT = "/request-ride";
const char* RIDE_STATUS_ENDPOINT = "/ride-status/";

// --- Device Identifiers ---
const char* RICKSHAW_PULLER_ID = "puller_001";
const char* PICKUP_LOCATION = "CUET Campus"; 
const char* DESTINATION_LOCATION = "Pahartoli";

// --- Pin Definitions for Wokwi ---
// LEDs (TEST CASE 4)
const int RED_LED_PIN = 14;    // Fixed naming - use RED_LED_PIN consistently
const int YELLOW_LED_PIN = 12; // Fixed naming  
const int GREEN_LED_PIN = 13;  // Fixed naming
const int BUZZER_PIN = 27;

// Ultrasonic Sensor (TEST CASE 1)
const int TRIG_PIN = 4;      
const int ECHO_PIN = 2;      

// Laser/Button & LDR (TEST CASE 2)
const int BUTTON_PIN = 18;
const int LDR_PIN = 34;

// --- OLED Display Setup (SSD1306 128x64) ---
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);

// --- Global Variables and Status ---
enum SystemState {
  STATE_IDLE,                 
  STATE_PRESENCE_DETECTING,   
  STATE_VERIFICATION_WAIT,    
  STATE_REQUEST_SENT,         
  STATE_OFFER_INCOMING,       
  STATE_RIDE_ACCEPTED,        
  STATE_RIDE_COMPLETED,       
  STATE_TIMEOUT_NO_PULLER     
};

SystemState currentState = STATE_IDLE;
unsigned long stateStartTime = 0;
unsigned long presenceStartTime = 0;

// Ride State Management
String currentRideId = "";
String currentRideStatus = "idle";

// TEST CASE 1: 3 second presence
const long PRESENCE_THRESHOLD_TIME = 3000;
const float MAX_DISTANCE_CM = 1000;

// TEST CASE 2: LDR threshold
const int LDR_THRESHOLD = 800;

// TEST CASE 4: Timeouts
const long PULLER_ACCEPT_TIMEOUT = 60000;
const long OFFER_INCOMING_DELAY = 10000;
const long PICKUP_CONFIRMED_DELAY = 25000;

// --- Function Prototypes ---
float measureDistance();
void updateOLED();
void setLEDs(bool yellow, bool red, bool green);
void transitionTo(SystemState newState);
void init_wifi();
void send_ride_request();
void get_ride_status();
void set_led_status(); // Fixed function name
void buzzerBeep(int duration = 200);
void buzzerAlert(int count = 3);
void test_server_connection();

// ===================================
// SETUP FUNCTION
// ===================================
void setup() {
  Serial.begin(115200);
  Serial.println("=== AERAS ESP32 Wokwi Simulation ===");

  // Initialize LEDs and Buzzer
  pinMode(RED_LED_PIN, OUTPUT);      // Fixed: RED_LED_PIN
  pinMode(YELLOW_LED_PIN, OUTPUT);   // Fixed: YELLOW_LED_PIN
  pinMode(GREEN_LED_PIN, OUTPUT);    // Fixed: GREEN_LED_PIN
  pinMode(BUZZER_PIN, OUTPUT);
  setLEDs(LOW, LOW, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Startup sequence
  setLEDs(HIGH, HIGH, HIGH);
  buzzerBeep(500);
  delay(500);
  setLEDs(LOW, LOW, LOW);

  // OLED Display initialization
  u8g2.begin();
  u8g2.setFlipMode(0);
  u8g2.clearBuffer();
  u8g2.sendBuffer();

  // Connect to WiFi
  init_wifi();

  // Test server connection
  if (WiFi.status() == WL_CONNECTED) {
    test_server_connection();
  }

  transitionTo(STATE_IDLE);
}

// ===================================
// LOOP FUNCTION
// ===================================
void loop() {
  unsigned long currentTime = millis();
  updateOLED();

  // Handle server communication for active rides
  if (currentRideId.length() > 0 && currentRideStatus != "idle" && currentRideStatus != "completed") {
    get_ride_status();
    set_led_status(); 
  }

  switch (currentState) {
    case STATE_IDLE:
      transitionTo(STATE_PRESENCE_DETECTING);
      break;

    case STATE_PRESENCE_DETECTING: {
      float distance = measureDistance();
      
      if (distance > 0 && distance <= MAX_DISTANCE_CM) {
        if (presenceStartTime == 0) {
          presenceStartTime = currentTime;
          Serial.println("Presence detected within range. Timer started.");
        }
        if (currentTime - presenceStartTime >= PRESENCE_THRESHOLD_TIME) {
          Serial.println("Presence confirmed for 3 seconds. Moving to verification.");
          transitionTo(STATE_VERIFICATION_WAIT);
        }
      } else {
        if (presenceStartTime != 0) {
          Serial.println("Presence lost or out of range. Resetting timer.");
        }
        presenceStartTime = 0;
      }
      break;
    }

    case STATE_VERIFICATION_WAIT: {
      int buttonState = digitalRead(BUTTON_PIN);
      int ldrValue = analogRead(LDR_PIN);

      if (buttonState == LOW) {
        Serial.print("Button pressed. LDR value: ");
        Serial.println(ldrValue);

        if (ldrValue > LDR_THRESHOLD) {
          Serial.println("Privilege confirmed: Correct laser frequency detected!");
          buzzerBeep(300);
          transitionTo(STATE_REQUEST_SENT);
        } else {
          Serial.println("Verification Failed: Incorrect Laser Frequency (LDR too low).");
          buzzerAlert(2);
        }
      }
      break;
    }

    case STATE_REQUEST_SENT: {
      setLEDs(LOW, LOW, LOW);
      
      // Send ride request to server
      if (currentRideId.length() == 0) {
        send_ride_request();
      }
      
      // Check for timeout or simulated offer
      if (currentTime - stateStartTime >= PULLER_ACCEPT_TIMEOUT) {
        Serial.println("Timeout: No puller accepted the ride within 60 seconds.");
        transitionTo(STATE_TIMEOUT_NO_PULLER);
      } else if (currentTime - stateStartTime >= OFFER_INCOMING_DELAY && currentRideStatus == "pending") {
        Serial.println("Puller offer incoming simulation.");
        transitionTo(STATE_OFFER_INCOMING);
      }
      break;
    }

    case STATE_OFFER_INCOMING: {
      setLEDs(HIGH, LOW, LOW);
      Serial.println("Offer Incoming: Yellow LED ON.");

      if (currentRideStatus == "accepted") {
        Serial.println("Puller accepted the ride!");
        buzzerAlert(3);
        transitionTo(STATE_RIDE_ACCEPTED);
      }
      break;
    }

    case STATE_RIDE_ACCEPTED: {
      setLEDs(LOW, LOW, HIGH);
      Serial.println("Ride Accepted: Green LED ON.");

      if (currentRideStatus == "in_progress") {
        Serial.println("Puller confirmed pickup!");
        buzzerAlert(2);
      }

      if (currentRideStatus == "completed") {
        Serial.println("Ride completed!");
        buzzerBeep(1000);
        transitionTo(STATE_RIDE_COMPLETED);
      }
      break;
    }

    case STATE_RIDE_COMPLETED:
      setLEDs(LOW, LOW, LOW);
      if (currentTime - stateStartTime >= 5000) {
        Serial.println("Ride completed state. Resetting to IDLE.");
        currentRideId = "";
        currentRideStatus = "idle";
        transitionTo(STATE_IDLE);
      }
      break;

    case STATE_TIMEOUT_NO_PULLER:
      setLEDs(LOW, HIGH, LOW);
      Serial.println("No Puller: Red LED ON.");
      for(int i = 0; i < 5; i++) {
        buzzerBeep(200);
        delay(200);
      }
      
      if (currentTime - stateStartTime >= 5000) {
        Serial.println("Timeout state. Resetting to IDLE.");
        currentRideId = "";
        currentRideStatus = "idle";
        transitionTo(STATE_IDLE);
      }
      break;
  }

  delay(100);
}

// ===================================
// HELPER FUNCTIONS
// ===================================

void transitionTo(SystemState newState) {
  currentState = newState;
  stateStartTime = millis();
  presenceStartTime = 0;

  Serial.print("STATE TRANSITION TO: ");
  switch (newState) {
    case STATE_IDLE: Serial.println("IDLE"); break;
    case STATE_PRESENCE_DETECTING: Serial.println("PRESENCE_DETECTING"); break;
    case STATE_VERIFICATION_WAIT: Serial.println("VERIFICATION_WAIT"); break;
    case STATE_REQUEST_SENT: Serial.println("REQUEST_SENT"); break;
    case STATE_OFFER_INCOMING: Serial.println("OFFER_INCOMING"); break;
    case STATE_RIDE_ACCEPTED: Serial.println("RIDE_ACCEPTED"); break;
    case STATE_RIDE_COMPLETED: Serial.println("RIDE_COMPLETED"); break;
    case STATE_TIMEOUT_NO_PULLER: Serial.println("TIMEOUT_NO_PULLER"); break;
  }
}

void setLEDs(bool yellow, bool red, bool green) {
  digitalWrite(YELLOW_LED_PIN, yellow);  
  digitalWrite(RED_LED_PIN, red);        
  digitalWrite(GREEN_LED_PIN, green);    
}

// Buzzer functions
void buzzerBeep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void buzzerAlert(int count) {
  for (int i = 0; i < count; i++) {
    buzzerBeep(300);
    delay(200);
  }
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 20000);
  float distanceCm = duration * 0.0343 / 2;
  
  return distanceCm;
}

void updateOLED() {
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);

  // Fixed ride information
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "PICKUP: CUET Campus");
  u8g2.drawStr(0, 24, "DEST: Pahartoli Block");

  // Current status with server updates
  u8g2.setFont(u8g2_font_6x10_tf);
  
  switch (currentState) {
    case STATE_IDLE:
    case STATE_PRESENCE_DETECTING:
      u8g2.drawStr(0, 40, "STATUS: STANDBY...");
      u8g2.setFont(u8g2_font_logisoso20_tf);
      u8g2.drawStr(0, 60, "ACTIVE: NO");
      break;

    case STATE_VERIFICATION_WAIT:
      u8g2.drawStr(0, 40, "STATUS: LASER VERIFY!");
      u8g2.setFont(u8g2_font_logisoso20_tf);
      u8g2.drawStr(0, 60, "ACTIVE: YES");
      break;
      
    case STATE_REQUEST_SENT:
      u8g2.drawStr(0, 40, "WAITING FOR PULLER...");
      u8g2.setFont(u8g2_font_logisoso20_tf);
      u8g2.drawStr(0, 60, "SEARCHING");
      break;
      
    case STATE_OFFER_INCOMING:
      u8g2.drawStr(0, 40, "PULLER FOUND!");
      u8g2.setFont(u8g2_font_logisoso20_tf);
      u8g2.drawStr(0, 60, "OFFER!");
      break;
      
    case STATE_RIDE_ACCEPTED:
      u8g2.drawStr(0, 40, "RICKSHAW ON THE WAY!");
      u8g2.setFont(u8g2_font_logisoso20_tf);
      u8g2.drawStr(0, 60, "ARRIVING");
      break;
      
    case STATE_RIDE_COMPLETED:
      u8g2.drawStr(0, 40, "RIDE COMPLETED!");
      u8g2.setFont(u8g2_font_logisoso20_tf);
      u8g2.drawStr(0, 60, "POINTS: +10");
      break;
      
    case STATE_TIMEOUT_NO_PULLER:
      u8g2.drawStr(0, 40, "NO PULLER AVAILABLE!");
      u8g2.setFont(u8g2_font_logisoso20_tf);
      u8g2.drawStr(0, 60, "FAILED");
      break;
  }
  
  u8g2.sendBuffer();
}

// WiFi and Server Functions
void init_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    buzzerBeep(200);
    delay(100);
    buzzerBeep(200);
  } else {
    Serial.println("\n❌ WiFi connection failed!");
    for (int i = 0; i < 5; i++) {
      buzzerBeep(100);
      delay(100);
    }
  }
}

void test_server_connection() {
  Serial.println("\n--- Testing Server Connection ---");
  
  HTTPClient http;
  String testUrl = String(BASE_SERVER_URL) + "/";
  http.begin(testUrl);
  
  Serial.printf("Testing connection to: %s\n", testUrl.c_str());
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.printf("✅ Server responded with HTTP code: %d\n", httpCode);
    String response = http.getString();
    Serial.printf("Server says: %s\n", response.c_str());
    buzzerBeep(300);
  } else {
    Serial.printf("❌ Server connection failed: %s\n", http.errorToString(httpCode).c_str());
    for (int i = 0; i < 3; i++) {
      buzzerBeep(500);
      delay(300);
    }
  }
  
  http.end();
}

void send_ride_request() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = String(BASE_SERVER_URL) + RIDE_REQUEST_ENDPOINT;
    
    StaticJsonDocument<200> doc;
    doc["pickup"] = PICKUP_LOCATION;
    doc["destination"] = DESTINATION_LOCATION;
    doc["pullerId"] = RICKSHAW_PULLER_ID;

    String requestBody;
    serializeJson(doc, requestBody);
    
    http.begin(serverPath);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "AERAS-ESP32");

    Serial.printf("\n--- Sending Ride Request ---\n");
    Serial.printf("URL: %s\n", serverPath.c_str());
    Serial.printf("Body: %s\n", requestBody.c_str());
    
    int httpResponseCode = http.POST(requestBody);
    
    if (httpResponseCode > 0) {
      Serial.printf("✅ HTTP Response Code: %d\n", httpResponseCode);
      String payload = http.getString();
      Serial.printf("Response: %s\n", payload.c_str());

      StaticJsonDocument<200> responseDoc;
      DeserializationError error = deserializeJson(responseDoc, payload);

      if (!error) {
        currentRideId = responseDoc["rideId"].as<String>();
        if (currentRideId.length() > 0) {
          currentRideStatus = "pending";
          Serial.printf("🎉 Ride Request Successful! ID: %s\n", currentRideId.c_str());
          buzzerBeep(150);
        }
      } else {
        Serial.printf("❌ JSON Parse Error: %s\n", error.c_str());
      }
    } else {
      Serial.printf("❌ HTTP POST Error: %s\n", http.errorToString(httpResponseCode).c_str());
      buzzerAlert(4);
    }
    
    http.end();
  } else {
    Serial.println("❌ WiFi not connected for request.");
  }
}

void get_ride_status() {
  if (WiFi.status() == WL_CONNECTED && currentRideId.length() > 0) {
    HTTPClient http;
    String serverPath = String(BASE_SERVER_URL) + RIDE_STATUS_ENDPOINT + currentRideId;
    
    http.begin(serverPath);
    http.addHeader("User-Agent", "AERAS-ESP32");

    Serial.printf("Checking status for ride: %s\n", currentRideId.c_str());
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      
      StaticJsonDocument<200> responseDoc;
      DeserializationError error = deserializeJson(responseDoc, payload);

      if (!error) {
        String newStatus = responseDoc["status"].as<String>();
        if (newStatus != currentRideStatus) {
          currentRideStatus = newStatus;
          Serial.printf("🔄 Status Updated: %s\n", currentRideStatus.c_str());
        }
      }
    } else if (httpResponseCode == 404) {
      Serial.printf("❌ Ride ID %s not found on server.\n", currentRideId.c_str());
    } else {
      Serial.printf("❌ Status check failed: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
  }
}
void set_led_status() {
  digitalWrite(RED_LED_PIN, currentRideStatus == "pending" ? HIGH : LOW);       
  digitalWrite(YELLOW_LED_PIN, currentRideStatus == "accepted" ? HIGH : LOW);    
  digitalWrite(GREEN_LED_PIN, (currentRideStatus == "in_progress" || currentRideStatus == "completed") ? HIGH : LOW); 
}
