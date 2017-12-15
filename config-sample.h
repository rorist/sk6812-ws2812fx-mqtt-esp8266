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

// MQTT Topics
#define CONFIG_MQTT_TOPIC_STATE "feather/leds"
#define CONFIG_MQTT_TOPIC_SET "feather/leds/set"
#define CONFIG_MQTT_PAYLOAD_ON "ON"
#define CONFIG_MQTT_PAYLOAD_OFF "OFF"

// OTA
#define CONFIG_OTA_NAME "feather_leds"
#define CONFIG_OTA_PASS "BLAH2"
