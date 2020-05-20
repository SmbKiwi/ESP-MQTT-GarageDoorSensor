/* Add your keys & customise your settings in this file */

#ifndef _AUTH_DETAILS
#define _AUTH_DETAILS

// Wifi Settings
#define WIFI_HOSTNAME "GarDoor"
#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"

// MQTT Settings
#define MQTT_SERVER "mqtt server ip"
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_PORT 1883
#define MQTT_AVAIL_TOPIC WIFI_HOSTNAME "/availability"
#define TimeReadSYS 100000 // time between system readings and MQTT publishing of ESP8266 data
#define topicESPtoMQTT WIFI_HOSTNAME "/ESPtoMQTT"

// OTA Settings for pushing updated software
#define OTApassword "ota password" //the password you will need to enter to upload remotely via the ArduinoIDE
#define OTAport 8266

// Blynk App Settings
#define BLYNK_ENABLED true  //choose to use Blynk app (scan the QR code for OpenGarage project for Blynk App) to monitor/control garage door
#define BlynkAuthToken "enter Blynk token here"
#define BLYNK_NOTIFY true   //choose to be notified in Blynk app when garage door is opened or closed and when garage door is left open for at least 15 minutes 
#define BLYNK_DEFAULT_DOMAIN "blynk-cloud.com"  //Change only if you use a custom Blynk server - public domain name or IP address of your custom server 
#define BLYNK_DEFAULT_PORT 80   // Change only if you use a custom Blynk server - public port of your custom Blynk server
#define BLYNK__AVAIL_TOPIC WIFI_HOSTNAME "/blynkavailability" // MQTT topic to notify if Blynk connected or disconnected

// Distance Parameters
#define ULTRASONIC_MAX_DISTANCE 400 // Maximum distance (in cm) to ping.
#define ULTRASONIC_DIST_MAX_CLOSE 160 // Maximum distance to indicate door is closed (in cm) - Used by sensor 1
#define ULTRASONIC_DIST_MAX_CAR 160 // Maximum distance to indicate car is present (in cm) - Used by sensor 2
#define ULTRASONIC_SETTLE_TIMEOUT 500 // ms to wait between pings (as all sensors get triggered at the same time)
#define RELAY_ACTIVE_TIMEOUT 500 // ms the time the relay will close to actuate the door opener
#define RELAY_ACTIVE_TYPE HIGH   // The digitalwrite to turn on the relay can be HIGH or LOW. If your relay uses LOW as active, enter LOW. If your relay instead uses HIGH as active, enter HIGH.    
#define DOOR_TRIG_PIN 14 // D5 on ESP8266

// Door 1 Parameters (Sensor 1)
#define DOOR1_ALIAS "Door"  //Name to use in MQTT topic and webpage
#define MQTT_DOOR1_ACTION_TOPIC WIFI_HOSTNAME "/1/action"
#define MQTT_DOOR1_STATUS_TOPIC WIFI_HOSTNAME "/1/status"
#define DOOR1_RELAY_PIN 12 // D6 on ESP8266
#define DOOR1_ECHO_PIN 4 // D2 on ESP8266
#define DOOR1_LIMIT_RELAY_CLOSE false  //Do not use the relay to open the door if true (which limits you to closing the door only)

// Car Parameters (Sensor 2)
#define CAR_ALIAS "Car"  //Name to use in MQTT topic and webpage
#define MQTT_CAR_STATUS_TOPIC WIFI_HOSTNAME "/2/status"
#define DOOR2_ECHO_PIN 5 // D1 on ESP8266

// DHT Parameters
#define DHT_ENABLED true
#define DHT_PIN 13 // D7 on ESP8266
#define DHT_TYPE DHT22 // or: DHT21 or DHT22
#define MQTT_TEMPERATURE_TOPIC WIFI_HOSTNAME "/temperature"
#define MQTT_HUMIDITY_TOPIC WIFI_HOSTNAME "/humidity"
#define DHT_PUBLISH_INTERVAL 300  // Number of seconds between each temperature/humidity reading and publish to MQTT 
#define DHT_TEMPERATURE_CELSIUS true  //Use celsius (true) or fahrenheit (false)
#define DHT_TEMPERATURE_ALIAS "Garage Temperature"  
#define DHT_HUMIDITY_ALIAS "Garage Humidity"

#endif
