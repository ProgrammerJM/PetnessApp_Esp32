#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WIFI.h>
#endif

// LIBRARY TO USE
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <ArduinoJson.h>

#include <Arduino.h>
#include "soc/rtc.h"
#include <HX711_ADC.h>

  // WIFI Credentials
  #define WIFI_SSID "WIFIWIFIWIFI"
  #define WIFI_PASSWORD "LorJun21"

  // DEFINE FIREBASE API KEY, PROJECT ID
  #define API_KEY "AIzaSyDPcMRU9x421wP0cS1sRHwEvi57W8NoLiE"
  #define FIREBASE_PROJECT_ID "petness-92c55"

  // USER CREDENTIALS
  #define USER_EMAIL "petnessadmin@gmail.com"
  #define USER_PASSWORD "petness"

  // Define Firebase Data Object
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;

const int HX711_dout = 13; //mcu > HX711 dout pin
const int HX711_sck = 27; //mcu > HX711 sck pin
HX711_ADC LoadCell(HX711_dout, HX711_sck);
unsigned long t = 0;

// For READING DELAY OF ESP32 TO FIRESTORE
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 3000; // Check every 5 seconds
// bool statusChecked = false;

void tokenStatusCallback(bool status) {
  Serial.printf("Token status changed: %s\n", status ? "true" : "false");
}

void setup() {
  Serial.begin(115200);

      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.print("Connecting to WiFi");
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
      }
      Serial.println("Connected to WiFi");

      Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

      // Assign the api key (required)
      config.api_key = API_KEY;

      // Assign the user sign in credentials
      auth.user.email = USER_EMAIL;
      auth.user.password = USER_PASSWORD;

      // Assign the callback function for the long running token generation task
      config.token_status_callback = tokenStatusCallback;

      Firebase.begin(&config, &auth);
      Firebase.reconnectWiFi(true);

      // INITIALIZE WEIGHT TO TARE BEFORE GETTING TRUE WEIGHT
      getPetsWeightTare();
}

bool codeExecuted = false; // Flag to indicate whether the code has been executed

void loop() {

  unsigned long currentMillis = millis();

      // Check if it's time to perform the status check
    if (currentMillis - lastCheckTime >= checkInterval) { // It can be this "(currentMillis - lastCheckTime >= checkInterval || !statusChecked)"
        lastCheckTime = currentMillis; // Update last check time
        checkAndUpdateStatus(); // Perform the status check
        Serial.println("FIRESTORE READ HAPPENED");
    }
  
  delay(1000);
}

void checkAndUpdateStatus() {
    String path = "trigger";

    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), "")) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, fbdo.payload().c_str());

        if (!error) {
            bool statusChanged = false; // Flag to indicate if status changed to true

            for (JsonObject document : doc["documents"].as<JsonArray>()) {
                const char *document_name = document["name"];
                const bool statusValue = document["fields"]["status"]["booleanValue"];

                if (strstr(document_name, "getPetWeight") != nullptr && statusValue) {
                    if (statusValue && !codeExecuted) {
                        Serial.println("Status is ON");
                        getPetsWeight();
                        return;
                        // codeExecuted = true; // Set the flag to indicate that the code has been executed
                        // statusChanged = true; // Status changed to true
                    }
                }
            }

            // if (!statusChanged && codeExecuted) {
            //     // If status didn't change to true but code was executed, reset codeExecuted flag
            //     codeExecuted = false;
            // }

        } else {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
        }
    }
}

void getPetsWeightTare() {
  // Load Cell Calibration Value
  Serial.println("Load Cell Starting...");
  float calibrationValue = 22.71; 

// Load Cell TARE Precision
  LoadCell.begin();
  unsigned long stabilizingtime = 1000; 
  boolean _tare = true; 
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Load Cell Startup Timeout, check MCU>HX711 wiring and pin designations");
  } else {
    LoadCell.setCalFactor(calibrationValue); Serial.println("Load Cell Startup is complete");
  } // set calibration factor (float) 
  
  while (!LoadCell.update());
    Serial.print("Calibration value: "); Serial.println(LoadCell.getCalFactor());
    if (LoadCell.getSPS() < 7) {      
      Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
    } else if (LoadCell.getSPS() > 100) {  
      Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
    }
}

void getPetsWeight () {

  FirebaseJson content;
  const int numSamples = 20; // Total number of samples to collect
  const int numIgnoreSamples = 10; // Number of initial samples to ignore
  float totalWeight = 0; // Variable to store the total weight
  int sampleCount = 0; // Counter for the number of samples collected

        // Ignore the first 5 samples
      for (int i = 0; i < numIgnoreSamples; i++) {
        bool newDataReady = false; // Flag to indicate new data
        
        while (!newDataReady) {
          if (LoadCell.update()) newDataReady = true;
        }
        
        if (newDataReady) {
          float weightSample = LoadCell.getData(); // Get the weight sample
          Serial.print("Ignoring sample: "); 
          Serial.println(weightSample);
        }
        
        delay(200); 
      }
      
      // Collect the next 5 samples
      for (int i = 0; i < numSamples - numIgnoreSamples; i++) {
        bool newDataReady = false; // Flag to indicate new data
        unsigned long t = millis(); // Store the current time
        
        while (!newDataReady) {
          if (LoadCell.update()) newDataReady = true;
        }
        
        if (newDataReady) {
          float weightSample = LoadCell.getData(); // Get the weight sample
          totalWeight += weightSample; // Add the weight to the total
          sampleCount++; // Increment the sample count
          Serial.print("Load_cell output val: "); 
          Serial.println(weightSample);
        }
        
        delay(500); // Delay for 1 second between samples
      }
      
      // Calculate the mean average
      float petsWeight = totalWeight / sampleCount;

      // Convert weight to Kilograms
      // float petsWeightKg = petsWeight / 1000.0;
      
      // Print the mean average
      Serial.print("PETS WEIGHT:  ");
      Serial.println(petsWeight);

                    // getting Weight Data and Sending to Firestore
              String documentPath = "getWeight/LoadCell";
              content.set("fields/Weight/stringValue", String(petsWeight, 2));
              Serial.print("Updating Weight Data...");
              if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "Weight")) { 
                Serial.println("ok");   
              }
              else  { 
                Serial.print("failed, reason: "); Serial.println(fbdo.errorReason());
              }

              delay(200);
              // UPDATING STATUS TO FALSE THAT ACT AS A TOGGLE OFF
              String getPetWeightTrigger = "trigger/getPetWeight";
              content.set("fields/status/booleanValue", boolean(false));
              Serial.print("Updating getPetWeight Trigger Status Data...");
              if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", getPetWeightTrigger.c_str(), content.raw(), "status")) { 
                Serial.println("ok");   
              } else  { 
                Serial.print("failed, reason: "); Serial.println(fbdo.errorReason());
              }
              
        totalWeight = 0;
        sampleCount = 0;
}