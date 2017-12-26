// Based on
// https://github.com/kitesurfer1404/WS2812FX
// https://github.com/corbanmailloux/esp-mqtt-rgb-led

// TODO
// Use setSegment() and multiple colors
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

#include <WS2812FX.h>
WS2812FX ws2812fx = WS2812FX(CONFIG_LED_COUNT, CONFIG_LED_PIN, NEO_GRBW + NEO_KHZ800);

const char* ssid = CONFIG_WIFI_SSID;
const char* password = CONFIG_WIFI_PASS;

const char* mqtt_server = CONFIG_MQTT_HOST;
const char* mqtt_username = CONFIG_MQTT_USER;
const char* mqtt_password = CONFIG_MQTT_PASS;
const char* client_id = CONFIG_MQTT_CLIENT_ID;
const char* light_state_topic = CONFIG_MQTT_TOPIC_STATE;
const char* light_set_topic = CONFIG_MQTT_TOPIC_SET;
const char* on_cmd = CONFIG_MQTT_PAYLOAD_ON;
const char* off_cmd = CONFIG_MQTT_PAYLOAD_OFF;

const size_t BUFFER_SIZE = JSON_OBJECT_SIZE(20);

// Maintained state for reporting to HA
byte red = 127;
byte green = 127;
byte blue = 127;
//byte white = 0;
byte brightness = 127;
uint16_t fxspeed = 200;
bool stateOn = true;

const char* setEffect = NULL;
String currentEffect = "Static";
String allEffects[MODE_COUNT];

void setup() {
  ws2812fx.init();
  ws2812fx.setBrightness(brightness);
  ws2812fx.setSpeed(fxspeed);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.setColor(red, green, blue);
  ws2812fx.start();
  if(stateOn) {
    ws2812fx.service(); // turn on the leds immediatly
  }

  // Store all mode names (for faster lookup ?)
  for(uint8_t i=0; i < ws2812fx.getModeCount(); i++) {
    allEffects[i] = ws2812fx.getModeName(i);
  }

  setup_ota();
  setup_wifi();
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
  processJson(message);
  sendState();
}

void processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE + 80> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(message);
  if (!root.success()) {
    return;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
      if(!root.containsKey("effect")) {
        setMode(currentEffect);
      }
      if(!root.containsKey("color")) {
        ws2812fx.setColor(red, green, blue);
      }
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

  if (root.containsKey("speed")) {
    fxspeed = root["speed"];
    ws2812fx.setSpeed(fxspeed);
  }
 
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

/*
void setAllColors() {
    for(uint16_t i=0; i<ws2812fx.numPixels(); i++) {
      ws2812fx.setPixelColor(i, red, green, blue, white);
      ws2812fx.show();
    }
}
*/

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  JsonObject& color = root.createNestedObject("color");
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;
  
  root["state"] = (stateOn) ? on_cmd : off_cmd;
  root["brightness"] = brightness;
  root["effect"] = currentEffect;
  root["speed"] = fxspeed;
  
  //root["white_value"] = white;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(light_state_topic, buffer, true);
}

void loop() {
  if (!client.connected()) {
   if (client.connect(client_id, mqtt_username, mqtt_password)) {
      client.subscribe(light_set_topic);
      sendState();
    }
  }
  client.loop();
  ws2812fx.service();
  ArduinoOTA.handle();
}

