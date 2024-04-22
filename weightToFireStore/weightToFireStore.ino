#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <Arduino.h>
#include "soc/rtc.h"

#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

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

//pins:
const int HX711_dout = 13; //mcu > HX711 dout pin
const int HX711_sck = 27; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_calVal_eepromAdress = 0;
unsigned long t = 0;

void tokenStatusCallback(bool status)
{
  Serial.printf("Token status changed: %s\n", status ? "true" : "false");
}

void setup() {
  Serial.begin(57600); 
  
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

  delay(10);
  Serial.println();
  Serial.println("Starting...");

  float calibrationValue; // calibration value
  calibrationValue = 22.14; // uncomment this if you want to set this value in the sketch
#if defined(ESP8266) || defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266 and want to fetch this value from eeprom
#endif
  //EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch this value from eeprom

  LoadCell.begin();
  //LoadCell.setReverseOutput();
  unsigned long stabilizingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration factor (float)
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
  Serial.print("Calibration value: ");
  Serial.println(LoadCell.getCalFactor());
  Serial.print("HX711 measured conversion time ms: ");
  Serial.println(LoadCell.getConversionTime());
  Serial.print("HX711 measured sampling rate HZ: ");
  Serial.println(LoadCell.getSPS());
  Serial.print("HX711 measured settlingtime ms: ");
  Serial.println(LoadCell.getSettlingTime());
  Serial.println("Note that the settling time may increase significantly if you use delay() in your sketch!");
  if (LoadCell.getSPS() < 7) {
    Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
  }
  else if (LoadCell.getSPS() > 100) {
    Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
  }
}

bool triggerDataSend = false; // Flag to trigger sending weight data

void loop() {
  static boolean newDataReady = false;
  const int serialPrintInterval = 500; // Increase value to slow down serial print activity

  // Check if the 'sent' field is true and triggerDataSend is false
  if (!triggerDataSend) {
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", "getWeight/LoadCell/sent")) {
      FirebaseJsonData jsonData;
      if (fbdo.jsonObject().get<bool>(jsonData, "fields/sent/booleanValue")) {
        bool sentValue = jsonData.to<bool>();
        if (sentValue) {
          triggerDataSend = true;
        }
      }
    }
  }

  // Trigger data send when flag is set and newDataReady is true
  if (triggerDataSend && newDataReady) {
    float i = LoadCell.getData();
    Serial.print("Load_cell output val: ");
    Serial.println(i);
    String documentPath = "getWeight/LoadCell";
    FirebaseJson content;
    content.set("fields/Weight/stringValue", String(i, 2));

    Serial.print("Updating Weight Data...");

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "Weight")) {
      Serial.println("ok");
    } else {
      Serial.print("failed, reason: ");
      Serial.println(fbdo.errorReason());
    }

    newDataReady = false; // Reset newDataReady flag
    triggerDataSend = false; // Reset triggerDataSend flag
  }

  // Check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  delay(100); // Delay to prevent rapid multiple readings
}
