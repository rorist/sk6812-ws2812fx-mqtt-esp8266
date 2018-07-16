// LEDs
#define CONFIG_LED_PIN 14
#define CONFIG_LED_COUNT 300

// WiFi
#define CONFIG_WIFI_SSID "mywifissid"
#define CONFIG_WIFI_PASS "mywifipassword"
#define CONFIG_WIFI_HOST "esp8266_feather_leds"

// MQTT
#define CONFIG_MQTT_HOST "192.168.0.123"
#define CONFIG_MQTT_USER "homeassistant"
#define CONFIG_MQTT_PASS "BLAH"
#define CONFIG_MQTT_CLIENT_ID "feather_led" // Must be unique on the MQTT network

// In case of problem, you may need to reduce the CONFIG_MQTT_CLIENT_ID length or increase
// MQTT_MAX_PACKET_SIZE in PubSubClient.h  see https://github.com/knolleary/pubsubclient/blob/4c8ce14dada84af6783233d38d735958ff332362/examples/mqtt_auth/mqtt_auth.ino#L29

// MQTT Topics
#define CONFIG_MQTT_TOPIC_STATE "feather/leds"
#define CONFIG_MQTT_TOPIC_SET "feather/leds/set"
#define CONFIG_MQTT_PAYLOAD_ON "ON"
#define CONFIG_MQTT_PAYLOAD_OFF "OFF"

// OTA
#define CONFIG_OTA_NAME "feather_leds"
#define CONFIG_OTA_PASS "BLAH2"
