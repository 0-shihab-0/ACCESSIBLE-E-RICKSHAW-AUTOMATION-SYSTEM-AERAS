// --- REQUIRED LIBRARIES ---
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// --- PIN DEFINITIONS ---
// I2C Pins for OLED (User specified)
#define OLED_SDA_PIN    21
#define OLED_SCL_PIN    22
#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64  // OLED display height, in pixels
#define OLED_RESET      -1  // Reset pin # (or -1 if sharing Arduino reset pin)

// GPS Pins (Using ESP32's UART2)
// NEO-6M TX (Data Out) -> ESP32 RX2
#define GPS_RX_PIN      16
// NEO-6M RX (Data In) -> ESP32 TX2 (only needed if sending commands to GPS)
#define GPS_TX_PIN      17 
#define GPS_BAUD_RATE   9600

// Button Pins (User specified, configured as INPUT_PULLUP)
#define ACCEPT_BUTTON   4  // Button Accept
#define REJECT_BUTTON   2  // Button Reject
#define BUZZER_PIN      27 // Buzzer

// --- OBJECT INITIALIZATION ---
// Create the OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create the GPS object
TinyGPSPlus gps;

// Create the second hardware serial port for the GPS
HardwareSerial GPS_Serial(2); 

// --- STATE MANAGEMENT ---
enum RequestState {
  IDLE,               // Default state, showing GPS data
  REQUEST_PENDING,    // A new ride request has arrived (buzzer on)
  ACCEPTED,           // Ride has been accepted
  REJECTED,           // Ride has been rejected
};

RequestState currentState = IDLE;

// Timing variables for simulating a request and for button debouncing
unsigned long lastRequestTime = 0;
unsigned long requestInterval = 30000; // Trigger a new request every 30 seconds
unsigned long buttonPressTime = 0;
const long debounceDelay = 200; // Debounce delay in ms

// --- BUZZER / TONE FUNCTIONS ---
// Replaced ledc functions with standard Arduino tone() and noTone() 
// to avoid "not declared" errors.

void startBuzzer() {
  // Play a 2000 Hz tone continuously
  tone(BUZZER_PIN, 2000); 
}

void stopBuzzer() {
  noTone(BUZZER_PIN);
}

// --- OLED DISPLAY FUNCTIONS ---

void displayStatus() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // 1. Display Current GPS Data (Always visible)
  display.setCursor(0, 0);
  display.print("STATUS: ");

  // 2. Handle Request State Display
  if (currentState == IDLE) {
    display.println("IDLE");
    
    // Show current location details
    display.setTextSize(2);
    display.setCursor(0, 15);
    if (gps.location.isValid()) {
      // Print Latitude
      display.print(gps.location.lat(), 4);
      display.setCursor(0, 35);
      // Print Longitude
      display.print(gps.location.lng(), 4);
    } else {
      display.println("Waiting for GPS...");
    }
    display.setTextSize(1);

  } else if (currentState == REQUEST_PENDING) {
    display.println("PENDING");

    display.fillRect(0, 15, SCREEN_WIDTH, 50, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.println("NEW RIDE!");
    display.setTextSize(1);
    display.setCursor(15, 45);
    display.println("ACCEPT (D4) / REJECT (D2)");
    display.setTextColor(SSD1306_WHITE);
    startBuzzer(); // Keep buzzer on while pending

  } else if (currentState == ACCEPTED) {
    display.println("ACCEPTED");
    display.setTextSize(2);
    display.setCursor(0, 30);
    display.println("RIDE ON WAY!");
    stopBuzzer();

  } else if (currentState == REJECTED) {
    display.println("REJECTED");
    display.setTextSize(2);
    display.setCursor(0, 30);
    display.println("RIDE SKIPPED");
    stopBuzzer();
  }

  display.display();
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Rickshaw Dispatcher Starting...");

  // 1. Setup GPS Serial (UART2)
  // RX2 pin, TX2 pin, Baud rate
  GPS_Serial.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // 2. Setup I2C for OLED
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring/address."));
    for(;;); // Don't proceed if initialization fails
  }

  // Initial OLED clear and message
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("System Ready.");
  display.display();

  // 3. Setup Buttons (Input with internal pull-up resistor)
  pinMode(ACCEPT_BUTTON, INPUT_PULLUP);
  pinMode(REJECT_BUTTON, INPUT_PULLUP);

  // 4. Setup Buzzer pin 
  // No explicit setup needed for tone(), but setting pin mode is good practice.
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure it starts off

  // Set initial request time
  lastRequestTime = millis();
}

// --- LOOP ---
void loop() {
  // 1. Process GPS Data (Non-blocking)
  while (GPS_Serial.available() > 0) {
    gps.encode(GPS_Serial.read());
  }

  // 2. Handle State Logic
  switch (currentState) {
    case IDLE:
      // Check if it's time to simulate a new ride request
      if (millis() - lastRequestTime >= requestInterval) {
        currentState = REQUEST_PENDING;
        lastRequestTime = millis(); // Reset timer for next request
        Serial.println("!!! NEW RIDE REQUEST TRIGGERED !!!");
      }
      break;

    case REQUEST_PENDING:
      // Check for button presses (Accept or Reject)
      if (millis() - buttonPressTime > debounceDelay) {
        if (digitalRead(ACCEPT_BUTTON) == LOW) { // Button is active LOW
          currentState = ACCEPTED;
          buttonPressTime = millis();
          Serial.println("Ride Accepted!");
        } else if (digitalRead(REJECT_BUTTON) == LOW) { // Button is active LOW
          currentState = REJECTED;
          buttonPressTime = millis();
          Serial.println("Ride Rejected.");
        }
      }
      break;

    case ACCEPTED:
    case REJECTED:
      // After acceptance/rejection, stay in this state for 5 seconds
      if (millis() - buttonPressTime > 5000) {
        currentState = IDLE;
        lastRequestTime = millis(); // Don't trigger another request immediately
      }
      break;
  }

  // 3. Update the OLED display
  displayStatus();
  
  // A small delay to keep the loop from running too fast
  delay(10);
}