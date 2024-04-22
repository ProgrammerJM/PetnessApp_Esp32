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

const int HX711_dout = 13;
const int HX711_sck = 14;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

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

  LoadCell.begin();
  unsigned long stabilizingTime = 2000;
  boolean tare = true;
  LoadCell.start(stabilizingTime, tare);

  Serial.println("Startup is complete");
}

void loop()
{
  if (LoadCell.update())
  {
    float weight = LoadCell.getData();

    Serial.print("Weight: ");
    Serial.println(weight);

    String documentPath = "getWeight/LoadCell";
    FirebaseJson content;
    content.set("fields/Weight/stringValue", String(weight, 2));

    Serial.print("Updating Weight Data...");

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "Weight"))
    {
      Serial.println("ok");
    }
    else
    {
      Serial.print("failed, reason: ");
      Serial.println(fbdo.errorReason());
    }
  }
  else
  {
    Serial.println("Failed to read Weight Data.");
  }

  delay(1000); // Adjust delay as needed
}
