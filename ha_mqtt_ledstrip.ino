#include "config.h"
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

#include <Adafruit_NeoPixel.h>
#define PIN 14
#define NUM_LEDS 300
#define BRIGHTNESS 50
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRBW + NEO_KHZ800);

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
byte brightness = 0;
bool stateOn = false;
const char* currentEffect = NULL;

void setup() {
  
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  if (debug_mode) {
    Serial.begin(115200);
  }

  setup_wifi();
  setup_ota();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void setup_ota() {
  ArduinoOTA.setHostname("kitchen_leds");
  ArduinoOTA.setPassword((const char *)"trustno1");
  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  if(debug_mode) {
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if(debug_mode) {
      Serial.print(".");
    }
  }
  if(debug_mode) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

  /*
  SAMPLE PAYLOAD:
    {
      "brightness": 120,
      "color": {
        "r": 255,
        "g": 100,
        "b": 100
      },
      "white_value": 255,
      "flash": 2,
      "transition": 5,
      "state": "ON",
      "effect": "colorfade_fast"
    }
  */
void callback(char* topic, byte* payload, unsigned int length) {

  if(debug_mode) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
  }

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
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
    }
  }

  if (root.containsKey("effect")) {
    currentEffect = root["effect"];
  }

  if (root.containsKey("color")) {
    red = root["color"]["r"];
    green = root["color"]["g"];
    blue = root["color"]["b"];
  }

  if (root.containsKey("white_value")) {
    white = root["white_value"];
  }

  if (root.containsKey("brightness")) {
    brightness = root["brightness"];
  }
  

  return true;
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

  if (currentEffect != NULL) {
    root["effect"] = String(currentEffect);
  }

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  client.publish(light_state_topic, buffer, true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if(debug_mode) {
      Serial.print("Attempting MQTT connection...");
    }
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      if(debug_mode) {
        Serial.println("connected");
      }
      client.subscribe(light_set_topic);
    } else {
      if(debug_mode) {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
      }
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setColor(int inR, int inG, int inB, int inW) {

  for(int i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, inR, inG, inB, inW );
  }
  strip.show();

  if(debug_mode) {
    Serial.println("Setting LEDs:");
    Serial.print("r: ");
    Serial.print(inR);
    Serial.print(", g: ");
    Serial.print(inG);
    Serial.print(", b: ");
    Serial.print(inB);
    Serial.print(", w: ");
    Serial.println(inW);
  }
}

void loop() {
  
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if(stateOn) {
    if(currentEffect != NULL) {
      if(strcmp(currentEffect, "wipe") == 0) {
        colorWipe();
      } else if(strcmp(currentEffect, "solid") == 0) {
        setColor(red, green, blue, white);
      }
    } else {
      setColor(red, green, blue, white);
    }
  } else {
    setColor(0, 0, 0, 0);
  }
}


/*
 *  Effects
 */

// Fill the dots one after the other with a color
void colorWipeSingle(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(10);
  }
}
void colorWipe() {
  colorWipeSingle(strip.Color(255, 0, 0));    // Red
  //colorWipeSingle(strip.Color(0, 255, 0));    // Green
  colorWipeSingle(strip.Color(0, 0, 255));    // Blue
  //colorWipeSingle(strip.Color(0, 0, 0, 255)); // White
}
