#include <WiFi.h>
#include <PicoMQTT.h>
#include <ArduinoJSON.h>

// WiFi Stuff

#define USE_EDUROAM 1
#define EDUROAM_WIFI_USER "zcabntw@ucl.ac.uk"
#define EDUROAM_WIFI_SSID "eduroam"
#define EDUROAM_WIFI_PASS "" // Left blank for security reasons - remember to enter this before running code!!!

#if USE_EDUROAM
#include "esp_wifi.h"
#include "esp_eap_client.h"
const char* WIFI_SSID = EDUROAM_WIFI_SSID;
const char* WIFI_USER = EDUROAM_WIFI_USER;
const char* WIFI_PASSWORD = EDUROAM_WIFI_PASS;
#else
const char* WIFI_SSID = "Glide_Resident";
const char* WIFI_USER = "";
const char* WIFI_PASSWORD = "StompPolioEasel";
#endif

// MQTT Client Broker information

const char* MQTT_HOST = "engf0001.cs.ucl.ac.uk";
const uint16_t MQTT_PORT = 1883;
const char* TOPIC = "bioreactor_sim/nofaults/telemetry/summary";

PicoMQTT::Client mqtt(MQTT_HOST, MQTT_PORT);

// JSON Data values for one sample (i.e. one second) stored in this struct

typedef struct telemetryData
{
  // Window
  unsigned long windowStart;
  unsigned long windowEnd;
  int samples;

  // Sensor Means
  float tempMean;
  float phMean;
  float rpmMean;

  // Actuators
  float heaterPwm;
  float motorPwm;
  float acidPwm;
  float basePwm;

  // Dosing & Energy
  double acidDosingL;
  double baseDosingL;
  float heaterEnergyWh;
  int photoevents;

  // Setpoints
  float tempSetpoint;
  float phSetpoint;
  float rpmSetpoint;

  // Faults
  int activeFaultCount;
  String lastFault;
} TelemetryData;

TelemetryData telemetryData;
volatile bool newDataReady = false;

void onMessage(const char* topic, const char* payload)
{
  Serial.print("[MQTT] ");
  Serial.print(topic);
  Serial.print(": ");
  if (payload) {
    Serial.println(payload);
  } else {
    Serial.println("(null)");
  }
}

void print_wifi_info()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to WiFi network: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway address: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
  }
  else
  {
    Serial.println("WiFi not connected");
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
  }
}

void wifi_connect(float timeout)
{
  unsigned long deadline;

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  #if USE_EDUROAM
  Serial.printf("Connecting to eduroam as user %s\n", WIFI_USER);
  esp_eap_client_set_identity((uint8_t *)WIFI_USER, strlen(WIFI_USER));
  esp_eap_client_set_username((uint8_t *)WIFI_USER, strlen(WIFI_USER));
  esp_eap_client_set_password((uint8_t *)WIFI_PASSWORD, strlen(WIFI_PASSWORD));
  esp_wifi_sta_enterprise_enable();
  WiFi.begin(WIFI_SSID);
  #else
  Serial.printf("Connecting to %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  #endif

  deadline = millis() + (unsigned long)(timeout * 1000);

  while ((WiFi.status() != WL_CONNECTED) && (millis() < deadline))
  {
    delay(500);
    Serial.print(".");
  } 
  Serial.println("");
  print_wifi_info();
}

void setup() 
{
  Serial.begin(115200);
  wifi_connect(15);

  // Subscribe once - PicoMQTT auto re-subscribes after reconnects
  mqtt.subscribe(TOPIC, onMessage);
  mqtt.begin(); // starts the MQTT client
}


void loop() 
{
  mqtt.loop(); // manages reconnections and messages

  static unsigned long last_check = 0;
  if (millis() - last_check > 5000) 
  {
    last_check = millis();
    if (!mqtt.connected())
    {
        Serial.println("[MQTT] Not connected to broker, attempting reconnect...");
    }
    /*
    else
    {
        Serial.println("[MQTT] Connected and listening...");
    }
    */
  }
}
