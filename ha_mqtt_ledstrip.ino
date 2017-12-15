// Based on
// https://github.com/kitesurfer1404/WS2812FX
// https://github.com/corbanmailloux/esp-mqtt-rgb-led

// TODO
// Change Speed
// Make White work
// Get rid of the String with some char

#include "config.h"
#include <ArduinoJson.h>

#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

#define PIN 14
#define NUM_LEDS 300
#define SPEED 200

#include <WS2812FX.h>
WS2812FX ws2812fx = WS2812FX(NUM_LEDS, PIN, NEO_GRBW + NEO_KHZ800);

const bool debug_mode = CONFIG_DEBUG;

const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASS;

const char* mqtt_server = CONFIG_MQTT_HOST;
const char* mqtt_username = CONFIG_MQTT_USER;
const char* mqtt_password = CONFIG_MQTT_PASS;
const char* client_id = CONFIG_MQTT_CLIENT_ID;

// Topics
const char* light_state_topic = CONFIG_MQTT_TOPIC_STATE;
const char* light_set_topic = CONFIG_MQTT_TOPIC_SET;

const char* on_cmd = CONFIG_MQTT_PAYLOAD_ON;
const char* off_cmd = CONFIG_MQTT_PAYLOAD_OFF;

const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

// Maintained state for reporting to HA
byte red = 127;
byte green = 0;
byte blue = 0;
byte white = 0;
byte brightness = 127;
bool stateOn = false;

const char* setEffect = NULL;
String currentEffect = "Static";
String allEffects[MODE_COUNT];

void setup() {
  
  ws2812fx.init();
  ws2812fx.setBrightness(brightness);
  ws2812fx.setSpeed(SPEED);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.setColor(0, 0, 0);
  ws2812fx.start();

  // Store all mode names (for faster lookup ?)
  for(uint8_t i=0; i < ws2812fx.getModeCount(); i++) {
    allEffects[i] = ws2812fx.getModeName(i);
  }

  setup_wifi();
  setup_ota();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  sendState();
}

void setup_ota() {
  ArduinoOTA.setHostname(CONFIG_OTA_NAME);
  ArduinoOTA.setPassword((const char *)CONFIG_OTA_PASS);
  ArduinoOTA.onStart([]() {
      setMode("Static");
      ws2812fx.setColor(0, 0, 0);
  });
  ArduinoOTA.onEnd([]() {
      setMode(currentEffect);
      ws2812fx.setColor(red, green, blue);
  });
  ArduinoOTA.begin();
}

void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(CONFIG_WIFI_HOST);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  if(debug_mode) {
    Serial.println(message);
  }

  if (!processJson(message)) {
    return;
  }

  sendState();
}

bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    if(debug_mode) {
      Serial.println("parseObject() failed");
    }
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
      setMode(currentEffect);
      ws2812fx.setColor(red, green, blue);
      //setAllColors();
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
      setMode("Static");
      ws2812fx.setColor(0, 0, 0);
    }
  }

  if (root.containsKey("effect")) {
    setEffect = root["effect"];
    currentEffect = setEffect;
    setMode(currentEffect);
  }

  if (root.containsKey("color")) {
    red = root["color"]["r"];
    green = root["color"]["g"];
    blue = root["color"]["b"];
    ws2812fx.setColor(red, green, blue);
  }

  /*
  if (root.containsKey("white_value")) {
    white = root["white_value"];
    setAllColors();
  }
  */

  if (root.containsKey("brightness")) {
    brightness = root["brightness"];
    ws2812fx.setBrightness(brightness);
  }
 
  return true;
}

void setMode(String _mode) {
  int sizeEffects = sizeof(allEffects);
  for(uint8_t i=0; i<sizeEffects; i++) {
    if(_mode == allEffects[i]){
      ws2812fx.setMode(i);
      break;
    }
  }
}

void setAllColors() {
    for(uint16_t i=0; i<ws2812fx.numPixels(); i++) {
      ws2812fx.setPixelColor(i, red, green, blue, white);
      ws2812fx.show();
    }
}

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (stateOn) ? on_cmd : off_cmd;
  JsonObject& color = root.createNestedObject("color");
  
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;
  root["brightness"] = brightness;
  root["white_value"] = white;
  root["effect"] = currentEffect;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(light_state_topic, buffer, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      client.subscribe(light_set_topic);
    } else {
      delay(5000);
    }
  }
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  ws2812fx.service();
}

