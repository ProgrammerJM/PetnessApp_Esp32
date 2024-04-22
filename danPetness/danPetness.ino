#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
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

                          FirebaseData fbdo;
                          FirebaseAuth auth;
                          FirebaseConfig config;

const int HX711_dout = 13; //mcu > HX711 dout pin
const int HX711_sck = 27; //mcu > HX711 sck pin
HX711_ADC LoadCell(HX711_dout, HX711_sck);
unsigned long t = 0;

void tokenStatusCallback(bool status)
{
  Serial.printf("Token status changed: %s\n", status ? "true" : "false");
}

void setup()
{
  Serial.begin(115200);

                            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                            Serial.print("Connecting to WiFi");
                            while (WiFi.status() != WL_CONNECTED)
                            {
                              Serial.print(".");
                              delay(300);
                            }
                            Serial.println("Connected to WiFi");

                            Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

                            config.api_key = API_KEY;
                            auth.user.email = USER_EMAIL;
                            auth.user.password = USER_PASSWORD;
                            config.token_status_callback = tokenStatusCallback;

                            Firebase.begin(&config, &auth);
                            Firebase.reconnectWiFi(true);

// Load Cell Calibration Value
  Serial.println("Load Cell Starting...");
  float calibrationValue = 22.17; 

// Load Cell TARE Precision
  LoadCell.begin();
  unsigned long stabilizingtime = 2000; 
  boolean _tare = true; 
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Load Cell Startup Timeout, check MCU>HX711 wiring and pin designations");
  }
  else {LoadCell.setCalFactor(calibrationValue); Serial.println("Load Cell Startup is complete");
  } // set calibration factor (float) 
  
  while (!LoadCell.update());
    Serial.print("Calibration value: "); Serial.println(LoadCell.getCalFactor());
    if (LoadCell.getSPS() < 7) {      Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
    }
    else if (LoadCell.getSPS() > 100) {  Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
    }


}

void loop()
{
  
  static boolean newDataReady = 0;
  const int serialPrintInterval = 500; //increase value to slow down serial print activity


  if (Serial.available() > 0) { 
    char receivedChar = Serial.read(); // Read the incoming byte

    if (receivedChar == 'y') {
      getPetsWeight();
    } else if (receivedChar == 'n') {
      Serial.println("No"); // Print "No" if 'n' is received
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }
  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

  delay(1000); // Adjust delay as needed
}




void getPetsWeight (){
  FirebaseJson content;
  const int numSamples = 10; // Total number of samples to collect
  const int numIgnoreSamples = 5; // Number of initial samples to ignore
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
        
        delay(500); 
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
      
      // Print the mean average
      Serial.print("PETS WEIGHT:  ");
      Serial.println(petsWeight);

                    // getting Weight Data and Sending to Firestore
              String documentPath = "getWeight/LoadCell";
              content.set("fields/Weight/stringValue", String(petsWeight, 2));
              Serial.print("Updating Weight Data...");
              if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "Weight")){ Serial.println("ok");   
              }
              else  { Serial.print("failed, reason: "); Serial.println(fbdo.errorReason());
              }


              String getPetWeightTrigger = "trigger/getPetWeight";
              content.set("fields/status/booleanValue", boolean(false));
              Serial.print("Updating getPetWeight Trigger Status Data...");
              if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", getPetWeightTrigger.c_str(), content.raw(), "status")){ Serial.println("ok");   
              }
              else  { Serial.print("failed, reason: "); Serial.println(fbdo.errorReason());
              }
              
        totalWeight = 0;
        sampleCount = 0;


}