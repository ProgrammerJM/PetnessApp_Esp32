#include "wifi_firebase_setup.h"
#include "hx711_setup.h"

// For READING DELAY OF ESP32 TO FIRESTORE
  unsigned long lastCheckTime = 0;
  const unsigned long checkInterval = 3000; //Check Every 5 Seconds
  bool codeExecuted = false; // Flag to indicate whether the code has been executed

//GLOBAL FUNCTIONS 
  const int numSamples = 20;  //Number of Samples will fetched on getting pets weight
  const int numIgnoreSamples = 10; //Number of First Samples to be ignore in getting final pets weight

// void tokenStatusCallback(bool status) {
//   Serial.printf("Token status changed: %s\n", status ? "true" : "false");
// }

void setup() {
  Serial.begin(115200);

  setupWiFi();
  setupFirebase();
  setupHX711();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastCheckTime >= checkInterval) {
    lastCheckTime = currentMillis;
    checkAndUpdateStatus();
    Serial.println("FIRESTORE READ HAPPENED");
  }
  
  delay(1000);
}

//Constantly Check and Update Status of getPetWeightTrigger
void checkAndUpdateStatus() {    String path = "trigger";

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
                    }
                }
            }
            
        } else {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
        }
    }
}

// Detects Pets Weight based on declared number of samples, send Averaged Pets Weight to Firestore and RESET getPetWeightTrigger to FALSE
void getPetsWeight () {
  FirebaseJson content; 
  //TEMP Variables in Getting Pets Weight
  float totalWeight = 0; // Variable to store the total weight
  int sampleCount = 0; // Counter for the number of samples collected

  // IGNORING the numIgnoreSamples
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
      
  // COLLECTING the needed numSamples
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
      
  // CALCCULATING the Average of Collected NumSamples
    float petsWeight = totalWeight / sampleCount;
    // Convert weight to Kilograms
    // float petsWeightKg = petsWeight / 1000.0;
    Serial.print("PETS WEIGHT:  ");
    Serial.println(petsWeight);

  // SENDING Computed Pets Weight to FIRESTORE
    String documentPath = "getWeight/LoadCell";
    content.set("fields/Weight/stringValue", String(petsWeight, 2));
    Serial.print("Updating Weight Data...");
      
    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "Weight")) { 
      Serial.println("Pets Weight Updated");   
    } else  { 
      Serial.print("failed, reason: "); Serial.println(fbdo.errorReason());
    }
    delay(200);
  
  // UPDATING GetPetWeightTrigger Status to FALSE / RESET TOGGLE
    String getPetWeightTrigger = "trigger/getPetWeight";
    content.set("fields/status/booleanValue", boolean(false));
    Serial.print("Updating getPetWeight Trigger Status Data...");
              
    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", getPetWeightTrigger.c_str(), content.raw(), "status")) { 
      Serial.println("Get Pet Weight Trigger Reset");   
    } else  { 
      Serial.print("failed, reason: "); Serial.println(fbdo.errorReason());
    }
                           
  //RESET TEMP Variables
    totalWeight = 0;
    sampleCount = 0;
}
