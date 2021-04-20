//Dependencies
//--------------------------------------
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include "FS.h"
#include <ArduinoJson.h> //ArduinoJSON6
#include <FastLED.h>
//--------------------------------------

//WiFi Settings
//----------------------------------------------------------------------------------------
const IPAddress apIP(192, 168, 1, 1);
const char* apSSID = "Dual_RGB_Light";
boolean SettingMode = false;
String ssidList;
int maxtry = 30;
DNSServer dnsServer;
ESP8266WebServer webServer(80);
const char *ssid, *password, *mqtt_server, *mqtt_user, *mqtt_pass, *topic_LED1, *topic_LED2, *HostName;
DynamicJsonDocument WIFI(2048);
WiFiClient espClient;
PubSubClient client(espClient);
bool WiFiConnected = false;
//----------------------------------------------------------------------------------------

//LED
//----------------------------------
#define FASTLED_INTERNAL
#define NUM_LEDS 1
#define DATA_PIN1 14
#define DATA_PIN2 12
#define FASTLED_ALLOW_INTERRUPTS 0
CRGB LED1[NUM_LEDS];
CRGB LED2[NUM_LEDS];
DynamicJsonDocument CMD(2048);
int Ra, Ga, Ba, Rb, Gb, Bb = 0;
//----------------------------------

//MQTT
//-------------------------------------------------------------------
void MQTT_loop() {
  if (WiFiConnected == true) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }
}

void callback(String topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.print("Topic : ");
  Serial.println(topic);
  Serial.print("Payload : ");
  Serial.println(messageTemp);
  Settings(topic, messageTemp);
}
//-------------------------------------------------------------------

//Setup
//-------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(500);
  pinMode(LED_BUILTIN, OUTPUT);
  FastLED.addLeds<WS2811, DATA_PIN1, RGB>(LED1, NUM_LEDS);
  FastLED.addLeds<WS2811, DATA_PIN2, RGB>(LED2, NUM_LEDS);
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  load_wifi();
  WiFiConnected = setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
//-------------------------------------------------------------------

//LED Settings
//-------------------------------------------------
void Settings(String topic, String messageTemp) {
  deserializeJson(CMD, messageTemp);
  if (topic == topic_LED1) {
    if (CMD.containsKey("R")) {
      Ra = CMD["R"];
    }
    if (CMD.containsKey("G")) {
      Ga = CMD["G"];
    }
    if (CMD.containsKey("B")) {
      Ba = CMD["B"];
    }
    LED1[0] = CRGB( Ra, Ga, Ba);
    FastLED.show();
  }

  if (topic == topic_LED2) {
    if (CMD.containsKey("R")) {
      Rb = CMD["R"];
    }
    if (CMD.containsKey("G")) {
      Gb = CMD["G"];
    }
    if (CMD.containsKey("B")) {
      Bb = CMD["B"];
    }
    LED2[0] = CRGB( Rb, Gb, Bb);
    FastLED.show();
  }
}
//-------------------------------------------------

//LOOP
//-------------------------------------------------
void loop() {
  //Edit WiFi via captive portal
  //..................................
  if (SettingMode) {
    dnsServer.processNextRequest();
    webServer.handleClient();
    return;
  }
  //..................................

  MQTT_loop();

  if (Serial.available()) {
    String input = Serial.readString();
    String Select = input.substring(0, 1);
    String Command = input.substring(1);

    //HELP
    //....................................
    if (Select == "H" || Select == "h") {
      Serial.println("-------------------------------------------------------------");
      Serial.println("Available commands :");
      Serial.println("H or h : Help");
      Serial.println();
      Serial.println("R or r : Start a Web UI to edit de WiFi/MQTT settings");
      Serial.println();
      Serial.println("W or w + JSON: Edit the WiFi/MQTT With the provided JSON");
      Serial.println("Example : W {\"SSID\": \"WiFi SSID\", \"Pass\":\"WiFi Pass\", \"MQTT_Server\":\"Broker.IP\", \"MQTT_Pass\":\"Broker.Pass\", \"Topic_LED1\":\"topic/led/01\", \"Topic_LED2\":\"topic/led/02\", \"HostName\":\"HostName\"}");
      Serial.println();
      Serial.println("1 + JSON : Control the LED 1");
      Serial.println("Example : 1 {\"R\":255, \"G\":0, \"B\":0 }");
      Serial.println();
      Serial.println("2 + JSON : Control the LED 2");
      Serial.println("Example : 2 {\"R\":255, \"G\":0, \"B\":0 }");
      Serial.println();
      Serial.println("0 : Turn OFF LED 1 and LED 2");
      Serial.println("-------------------------------------------------------------");
    }
    //....................................

    //RESET WiFI Settings and start web GUI
    //....................................
    if (Select == "R" || Select == "r") {
      SettingMode = true;
      SetupMode();
    }
    //....................................

    //Edit WiFi Settings from Serial
    //.............................................
    if (Select == "W" || Select == "w") {
      deserializeJson(CMD, Command);
      if (CMD.containsKey("SSID")) {
        WIFI["SSID"] = CMD["SSID"];
      }
      if (CMD.containsKey("Pass")) {
        WIFI["Pass"] = CMD["Pass"];
      }
      if (CMD.containsKey("MQTT_Server")) {
        WIFI["MQTT_Server"] = CMD["MQTT_Server"];
      }
      if (CMD.containsKey("MQTT_Pass")) {
        WIFI["MQTT_Pass"] = CMD["MQTT_Pass"];
      }
      if (CMD.containsKey("Topic_LED1")) {
        WIFI["Topic_LED1"] = CMD["Topic_LED1"];
      }
      if (CMD.containsKey("Topic_LED2")) {
        WIFI["Topic_LED2"] = CMD["Topic_LED2"];
      }
      if (CMD.containsKey("HostName")) {
        WIFI["HostName"] = CMD["HostName"];
      }
      save_wifi();
    }
    //.............................................

    //Serial Control LED 1
    //................................
    if (Select == "1") {
      Settings(topic_LED1, Command);
      Serial.println("LED1");
      Serial.println(Command);
    }
    //................................

    //Serial Control LED 2
    //................................
    if (Select == "2") {
      Settings(topic_LED2, Command);
      Serial.println("LED2");
      Serial.println(Command);
    }
    //................................

    //OFF
    //..........................
    if (Select == "0") {
      Serial.println("OFF");
      LED1[0] = CRGB( 0, 0, 0);
      LED2[0] = CRGB( 0, 0, 0);
      FastLED.show();
    }
    //..........................
    
    Serial.read();
  }
}
