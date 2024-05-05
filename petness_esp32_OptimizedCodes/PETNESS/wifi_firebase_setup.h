#ifndef WIFI_FIREBASE_SETUP_H
#define WIFI_FIREBASE_SETUP_H

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WIFI.h>
#endif
#include <Firebase_ESP_Client.h>
// #include <addons/TokenHelper.h>
#include <ArduinoJson.h>
#include "soc/rtc.h"

// Declare FirebaseData, FirebaseAuth, and FirebaseConfig objects as extern
extern FirebaseData fbdo;
extern FirebaseAuth auth;
extern FirebaseConfig config;

// WIFI configuration
#define WIFI_SSID "WIFIWIFIWIFI"
#define WIFI_PASSWORD "LorJun21"

// FIREBASE configuration
#define API_KEY "AIzaSyDPcMRU9x421wP0cS1sRHwEvi57W8NoLiE"
#define FIREBASE_PROJECT_ID "petness-92c55"
#define USER_EMAIL "petnessadmin@gmail.com"
#define USER_PASSWORD "petness"

void setupWiFi();
void setupFirebase();

#endif
