/*
  GarDoor-Car:
  
  This project allows you to control (open/close) a "dumb" garage door opener for a garage door and independently report the garage door status (open/closed) and presence of a car in the garage via MQTT. 
  In addition you can monitor the Temperature and Humidity within your garage over MQTT as well. 
  This project requires two ultrasonic sensors: one pointed at the garage door, and the other pointed at the normal location of the car. 
  This allows the presence of the car to be detected whether the garage door is open or closed.

  The code includes implementation of a webpage, MQTT, and Blynk. It requires an ESP8266 microcontroller with sensors and a relay. 
  
  To use this code you will need the following dependencies:

  - Support for the ESP8266 boards.
      - You can add it to the board manager by going to File -> Preference and pasting http://arduino.esp8266.com/stable/package_esp8266com_index.json into the Additional Board Managers URL field.
      - Next, download the ESP8266 dependencies by going to Tools -> Board -> Board Manager and searching for ESP8266 and installing it.

  - You will also need to download the following libraries by going to Sketch -> Include Libraries -> Manage Libraries
      - ArduinoJson version 5.13.5> // Benoit Blanchon
      - NewPing
      - PubSubClient     
      - Blynk

   This project is based on DotNetDann's ESP-MQTT-GarageDoorSensor, but has been modified and expanded by SmbKiwi. 
   This code was forked from https://github.com/DotNetDann/ESP-MQTT-GarageDoorSensor

   Inspiration for the addition of Blynk came from the OpenGarage project.

   - Another like project https://hackaday.io/project/25090/instructions

   Copyright (c) 2019 SmbKiwi 
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.  
*/

//(Developer: SmbKiwi)
//2.10R Add code: if distance is zero then assume echo timeout, reset echo pin (set LOW - some models of sensor don't have own timeout), and assume door is open/car is absent
//2.00R Add support for Blynk App (you can use the QR code for OpenGarage project for Blynk App). Added MQTT publishing of ESP8266 data, ability to set relay type, and other small changes 
//1.10R Design for a single garage door with seperate car detection using 2 ultrasonic sensors 

//(Developer: DotNetDann)
// 1.8 Change pins and small clean
// 1.7 Add ability to only close garage doors
// 1.6 Add diagnose infomation on distances
// 1.4 Changes to use NewPing
// 1.3 adds DHT Sensor for Temp and Humidty  

// ------------------------------
// ---- all config in auth.h ----
// ------------------------------
#define VERSION F("v2.10R - GarDoor-Car - https://github.com/SmbKiwi/GarDoor-Car ")

#include <BlynkSimpleEsp8266.h> // Volodymyr Shymanskyy
#include <ArduinoJson.h> // Benoit Blanchon
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h> // Nick O'Leary
#include <NewPing.h> // Tim Eckel
#include <ArduinoOTA.h>
#include <DHT.h> // Adafruit
#include <Adafruit_Sensor.h> // Adafruit Unified Sensor
#include "auth.h"

#define DOOR_UNKNOWN         0x00
#define DOOR_OPENED          0x01
#define DOOR_CLOSED          0x02
#define CAR_NO               0x03
#define CAR_YES              0x04
#define CAR_UNKNOWN          0x05
#define DOOR_OPENEDNOCAR     0x06
#define DOOR_CLOSEDWITHCAR   0x07
#define DOOR_CLOSEDNOCAR     0x08
#define DOOR_OPENEDWITHCAR   0x09
#define NOTKNOWN             0x10
#define SONAR_NUM 2      // Number of sensors required

/**************************************** GLOBALS ***************************************/

char* blynkup = "no"; // For displaying Blynk connection status in webpage
char auth[] = BlynkAuthToken;
bool curr_cloud_access_en() {
    if((unsigned)strlen(auth)==32) {
      return true;
    }
    return false;
  }
//if Blynk is enabled then initialise
#if BLYNK_ENABLED == true
  BlynkTimer timer;
  #define BLYNK_PIN_DOOR  V0
  #define BLYNK_PIN_RELAY V1
  #define BLYNK_PIN_LCD   V2
  #define BLYNK_PIN_DIST  V3
  #define BLYNK_PIN_CAR   V4
  #define BLYNK_PIN_IP    V5
  WidgetLED blynk_door(BLYNK_PIN_DOOR);
  WidgetLED blynk_car(BLYNK_PIN_CAR);
  char bdomain[] = BLYNK_DEFAULT_DOMAIN;
  uint16_t bport = BLYNK_DEFAULT_PORT;
  char* blynkonMess = "blynkon";  // MQTT topic Blynk is availiable
  char* blynkoffMess = "blynkoff";  //MQTT topic Blynk is not availiable  
#endif

int lastdoor1state = 2;  // Last state of door is unknown (used for Blynk notification when garage door is still open)
unsigned long timeropendoor = 0; // Time that garage door was first opened (used for Blynk notification when garage door is still open)
int n_opendoor = 1; // How many 15 minute periods the garage door has been continuously open for

const int BUFFER_SIZE = JSON_OBJECT_SIZE(10);
#define MQTT_MAX_PACKET_SIZE 512
unsigned long timersysdata = 0;

const int door_numValues = 10;
int door1_lastDistanceValues[door_numValues];
int door2_lastDistanceValues[door_numValues];
int door1_lastDistanceValue = 0;
int door2_lastDistanceValue = 0;

char* birthMessage = "online";
const char* lwtMessage = "offline";

const unsigned long dht_publish_interval_s = DHT_PUBLISH_INTERVAL;
unsigned long dht_lastReadTime = -1000;

const char* mqtt_broker = MQTT_SERVER;
const char* mqtt_clientId = WIFI_HOSTNAME;
const char* mqtt_username = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
String availabilityBase = WIFI_HOSTNAME;
String availabilitySuffix = "/availability";
String availabilityTopicStr = availabilityBase + availabilitySuffix;
const char* availabilityTopic = availabilityTopicStr.c_str();

// Set type of relay for turning relay on/off
#if RELAY_ACTIVE_TYPE == HIGH
  #define RELAYACTIVE HIGH
  #define RELAYINACTIVE LOW  
#endif
#if RELAY_ACTIVE_TYPE == LOW
  #define RELAYACTIVE LOW
  #define RELAYINACTIVE HIGH    
#endif

/******************************** GLOBAL OBJECTS *******************************/

NewPing sonar[SONAR_NUM] = {   // Sensor object array.
  NewPing(DOOR_TRIG_PIN, DOOR1_ECHO_PIN, ULTRASONIC_MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping. 0-2
  NewPing(DOOR_TRIG_PIN, DOOR2_ECHO_PIN, ULTRASONIC_MAX_DISTANCE)  
};

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);

// Get the state of the garage door based upon the sensor distance
byte getStateDoor(int distance1)
{
  if (distance1 < 0) {
    garagestillopen(2);
    return DOOR_UNKNOWN; // if no reading (-1) probably an error
  } else if (distance1 == 0) {
    garagestillopen(1);
    return DOOR_OPENED;   
  } else if (distance1 <= ULTRASONIC_DIST_MAX_CLOSE) {
    garagestillopen(0);
    return DOOR_CLOSED;
  } else if (distance1 <= ULTRASONIC_MAX_DISTANCE){
    garagestillopen(1);
    return DOOR_OPENED;   
  } else {
    garagestillopen(2);
    return DOOR_UNKNOWN;   
  }
}

// determine how long door has been open and if so generate Blynk app message at 15 minute intervals until closed
void garagestillopen(int currdoorstate) {
  byte bnote = BLYNK_NOTIFY;
  if(BLYNK_ENABLED && curr_cloud_access_en && bnote) {  //if Blynk not in use then don't do anything
    if (currdoorstate == 2) lastdoor1state = 2;  // Door is unknown
    else if (currdoorstate == 0) lastdoor1state = 0;  // Door is closed 
    else if (currdoorstate == 1){  // Check door is open
      if (lastdoor1state == 0 || lastdoor1state == 2){   //This is the first time door is open
        timeropendoor = millis();  // record time door was first open
        lastdoor1state = 1;  //Door is now open
      } else if (lastdoor1state == 1){   //Door was open last time
          unsigned long currtime = millis();
          if (currtime > (timeropendoor + (n_opendoor * 15000 * 60))) {  //if door has been open at least 15 minutes
            String garmess = "Garage door has been open for ";
            int openmins = (n_opendoor * 15); 
            char* minsmess = "";
            itoa(openmins, minsmess, 10);
            String endmess = " minutes";
            String doornotify = "";
            doornotify = garmess + minsmess + endmess;  // put the message together
            #if BLYNK_ENABLED == true
            perform_notify(doornotify); // notify via Blynk app how many minutes door has been open
            #endif
            n_opendoor = n_opendoor + 1;  // increment to check for a further 15 minute later next time
          }                 
        }
    }  
  }
}

// Get the state of the car based upon the sensor distance
byte getStateCar(int distance2)
{
  if (distance2 < 0) {
    return CAR_UNKNOWN; // if no reading (-1) probably an error
  } else if (distance2 == 0) {
    return CAR_NO; 
  } else if (distance2 <= ULTRASONIC_DIST_MAX_CAR) {
    return CAR_YES;
  } else if (distance2 <= ULTRASONIC_MAX_DISTANCE){
    return CAR_NO;
  } else {
    return CAR_UNKNOWN;
  }
}

#include "web.h"

/********************************** START SETUP *****************************************/
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);       // Initialize the LED_BUILTIN pin as an output (So it doesnt float as a LED is on this pin)
  digitalWrite(LED_BUILTIN, LOW);     // Turn the status LED on

  Serial.begin(115200);
  delay(10);
  Serial.println(F("Starting..."));

  // Setup Door 1 pins
  pinMode(DOOR1_RELAY_PIN, OUTPUT);
  digitalWrite(DOOR1_RELAY_PIN, RELAYINACTIVE);  // Turn off relay

  #if DHT_ENABLED == true
    dht.begin();
  #endif

  setup_wifi();

  client.setServer(mqtt_broker, MQTT_PORT);
  client.setCallback(callback);

  server.on("/", ServeWebClients);
  server.begin();

  #if BLYNK_ENABLED == true
  // If Blynk is enabled and has a valid token, then connect to Blynk server 
    if(curr_cloud_access_en) {
        Blynk.config(auth, bdomain, bport); // use the config function 
        Blynk.connect();
        while (Blynk.connect() == false) {Publish(BLYNK__AVAIL_TOPIC, blynkoffMess); }   // Wait until connected
        timer.setInterval(800L, reconnectBlynk);  // check every minute if still connected to server
        Serial.print(F("Blynk Connected"));
        Publish(BLYNK__AVAIL_TOPIC, blynkonMess);       
    }
  #endif
  
  //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(WIFI_HOSTNAME); // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setPassword((const char *)OTApassword); // No authentication by default

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

  Serial.println(F("Ready"));
  digitalWrite(LED_BUILTIN, HIGH);     // Turn the status LED off
}


/********************************** START SETUP WIFI *****************************************/
void setup_wifi() {
  delay(10);
  Serial.print(F("Connecting to SSID: "));
  Serial.println(WIFI_SSID);

  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  //wifi_set_sleep_type(LIGHT_SLEEP_T); // Enable light sleep
  WiFi.hostname(WIFI_HOSTNAME);

  if (WiFi.status() != WL_CONNECTED) {  // FIX FOR USING 2.3.0 CORE (only .begin if not connected)
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println(F(""));
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
}

// Find out what the IP address is for the ESP8266
String get_ip() {
  String ip = "";
  IPAddress _ip = WiFi.localIP();
  ip = _ip[0];
  ip += ".";
  ip += _ip[1];
  ip += ".";
  ip += _ip[2];
  ip += ".";
  ip += _ip[3];
  return ip;
}

/********************************** START ESP8266 DATA *****************************************/
void getESPdata(){
    unsigned long currtime = millis();
    if (currtime > (timersysdata + TimeReadSYS)) { //If required period has passed since last read of data then read ESP data again
      timersysdata = millis();
      StaticJsonBuffer<MQTT_MAX_PACKET_SIZE> jsonBuff;
      JsonObject& ESPdata = jsonBuff.createObject();        
      unsigned long uptime = millis()/1000;      
      ESPdata["uptime"] = uptime;
      uint32_t freemem;
      freemem = ESP.getFreeHeap();
      ESPdata["freeMem"] = freemem;
      long rssi = WiFi.RSSI();
      ESPdata["rssi"] = rssi;
      String SSID = WiFi.SSID();
      ESPdata["SSID"] = SSID;             
      char JSONmessageBuffer[100];
      ESPdata.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));  //put all JSON formatted ESP data into a variable 
      Publish(topicESPtoMQTT,JSONmessageBuffer); //publish ESP data from variable to MQTT topic
    }
}


/********************************** START CALLBACK *****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(F(""));
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  String topicToProcess = topic;
  payload[length] = '\0';
  String payloadToProcess = (char*)payload;

  // Action Command
  if (payloadToProcess == "OPEN") {
    Serial.print(F("Triggering OPEN relay!"));

    if (topicToProcess == MQTT_DOOR1_ACTION_TOPIC && // Door 1
         (getStateDoor(door1_lastDistanceValue) == DOOR_CLOSED) && // Garage is currently closed
         door1_lastDistanceValues[0] > 0 && // Last value was valid (Known state)
         !DOOR1_LIMIT_RELAY_CLOSE) { // We are not limiting the open command
      toggleRelay(DOOR1_RELAY_PIN);
      Serial.print(F("Door1 relay OPEN!"));   
    } else {
      Serial.print(F("criteria not meet!"));
    }
    
    Serial.println(" -> DONE");
  }
  else if (payloadToProcess == "CLOSE") {
    Serial.print(F("Triggering CLOSE relay!"));
    
    if (topicToProcess == MQTT_DOOR1_ACTION_TOPIC && 
         getStateDoor(door1_lastDistanceValue) == DOOR_OPENED &&  // Garage is currently OPEN
         door1_lastDistanceValues[0] > 0) { // Last value was valid (Known state)
      toggleRelay(DOOR1_RELAY_PIN);
      Serial.print(F("Door1 relay CLOSED!"));
    } else {
      Serial.print(F("criteria not meet!"));
    }

    Serial.println(F(" -> DONE"));
  }
  else if (payloadToProcess == "STATE") {
    Serial.print(F("Publishing on-demand status update!"));
    Publish(MQTT_AVAIL_TOPIC, birthMessage);
    sendState(1);
    sendState(2);
    Serial.println(F(" -> DONE"));
  }
  else if (payloadToProcess == "STOP") {
    Serial.print(F("We don’t know status of door while moving!"));
  } else {
    Serial.println(F("Unknown command!"));
  }
}


/********************************** START SEND STATE *****************************************/
void sendState(int door) {
  //Example JSON output on MQTT state topics
  //{"state":"open","occupied":unknown,"distance":12,"name":"Door1"}
  //{"state":"unknown","occupied":true,"distance":1000,"name":"Car"}
 
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  // Determine state of door or car based on distance and buffer name and distance for MQTT topic
  byte statedoor = DOOR_UNKNOWN;
  byte statecar = CAR_UNKNOWN;  
  char* topic = "";
  if (door == 1) {
    statedoor = getStateDoor(door1_lastDistanceValue);
    topic = MQTT_DOOR1_STATUS_TOPIC;
    root["name"] = DOOR1_ALIAS;
    root["distance"] = door1_lastDistanceValue;
  } else if (door == 2) {
    statecar = getStateCar(door2_lastDistanceValue);
    topic = MQTT_CAR_STATUS_TOPIC;
    root["name"] = CAR_ALIAS;
    root["distance"] = door2_lastDistanceValue;
  } else {
    return;
  }

  //using state of door setup variables/buffer for MQTT topic and Blynk
  char* doorState = "unknown";  
  int door_status = 0; 
  if (statedoor == DOOR_OPENED) {
    doorState = "open";
    door_status = 1;
  } else if (statedoor == DOOR_CLOSED) {
    doorState = "closed";
  } else if (statedoor == DOOR_UNKNOWN) {
    doorState = "unknown";
  } 
//using state of car setup variables/buffer for MQTT topic and Blynk 
  char* occupiedState = "unknown";
  int vehicle_status = 0;
  if (statecar == CAR_YES) {
    occupiedState = "true";
    vehicle_status = 1;
  } else if (statecar == CAR_NO) {
    occupiedState = "false";
  } else if (statecar == CAR_UNKNOWN) {
    occupiedState = "unknown";    
  }
  
  root["state"] = doorState;
  root["occupied"] = occupiedState;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Publish(topic, buffer);  //Publish JSON data from buffer to MQTT topic

  // For HA. If you wish to use simple state result instead of JSON data
  if (door == 1) {
    char* extraTopicState = "/value";
    char extraTopic[100];
    snprintf(extraTopic, sizeof extraTopic, "%s%s", topic, extraTopicState);
    Publish(extraTopic, doorState);
  } else if (door == 2) {
      char* extraTopicState = "/value";
      char extraTopic[100];
      snprintf(extraTopic, sizeof extraTopic, "%s%s", topic, extraTopicState);
      Publish(extraTopic, occupiedState);
  }

  //send notification via Blynk App when garage door is opened or closed
  #if BLYNK_ENABLED == true
    static byte old_door_status = 0xff; 
    byte bnote = BLYNK_NOTIFY;
    char* compareState = "unknown";
    if(curr_cloud_access_en && bnote && (doorState != compareState)) {  //check if Blynk has valid token and notification is on, and if door is either closed or open
      if(door_status != old_door_status){  //check if door status has changed
        Serial.println(F(" Notify Blynk with text notification"));
        String garmess = "Garage door is currently ";
        String doormess = doorState;
        String doornotify = "";
        doornotify = garmess + doormess;  //Put the message together
        perform_notify(doornotify);
      }     
    }
  
  
    // Update door status, car status, door distance, and IP address to Blynk App
    if(Blynk.connected()) {
      Serial.println(F(" Update Blynk (State Refresh)"));  
      static String old_ip = "";      
      static byte old_vehicle_status = 0xff;
      static uint old_distance = 0;           
      if(door1_lastDistanceValue != old_distance) {  //If sensor 1 (garage door) distance changed update Blynk
        Blynk.virtualWrite(BLYNK_PIN_DIST, door1_lastDistanceValue);
        old_distance = door1_lastDistanceValue;        
      }
      if(door_status != old_door_status) {  //If sensor 1 (garage door) status changed update Blynk
        if(door_status==1) {
          blynk_door.on();
        } else {
          blynk_door.off(); 
        }
        old_door_status = door_status;
      }
      if(vehicle_status != old_vehicle_status) {  //If sensor 2 (car) status changed update Blynk
        if(vehicle_status==1) {
          blynk_car.on();
        } else {
          blynk_car.off(); 
        }
        old_vehicle_status = vehicle_status;
      }   
      if(old_ip != get_ip()) {  //If IP address has changed update Blynk
        Blynk.virtualWrite(BLYNK_PIN_IP, get_ip());
        old_ip = get_ip();
      }

    }
  #endif
  
}

//Publish a topic to MQTT server
void Publish(char* topic, char* message) { 
  client.publish(topic, message, true);
   
  //Print what was sent to console
  Serial.println(F(""));
  Serial.print(F("Published ["));
  Serial.print(topic);
  Serial.print(F("] "));
  Serial.println(message);
}

/********************************** START RELAY *****************************************/
void toggleRelay(int pin) {
    digitalWrite(pin, RELAYACTIVE);  // turn on relay  
    delay(RELAY_ACTIVE_TIMEOUT);
    digitalWrite(pin, RELAYINACTIVE);  // turn off relay 
}

#if BLYNK_ENABLED == true
  // if button to close/open garage door pressed in Blynk app, then toggle relay to open/close the door 
  BLYNK_WRITE(BLYNK_PIN_RELAY) {
    Serial.println(F("Received Blynk generated button request"));
    int pinValue = param.asInt();
    if(pinValue==1) {  //if button has been pressed
      digitalWrite(DOOR1_RELAY_PIN, RELAYACTIVE);
      delay(RELAY_ACTIVE_TIMEOUT);
    } else {  //if button not pressed
      digitalWrite(DOOR1_RELAY_PIN, RELAYINACTIVE);
    }
    pinValue = 0;
  }
#endif

/********************************** START RECONNECT *****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(F("Attempting MQTT connection..."));
    // Attempt to connect
    if (client.connect(mqtt_clientId, mqtt_username, mqtt_password, availabilityTopic, 0, true, lwtMessage)) {
      Serial.println(F("connected"));

      // Publish the birth message on connect/reconnect
      Publish(MQTT_AVAIL_TOPIC, birthMessage);

      // Subscribe to the command topic to listen for action messages
      Serial.print(F("Subscribing to "));
      Serial.print(MQTT_DOOR1_ACTION_TOPIC);
      Serial.println(F("..."));
      client.subscribe(MQTT_DOOR1_ACTION_TOPIC);         

      // Publish the current door status on connect/reconnect to ensure status is synced with whatever happened while disconnected
      door1_lastDistanceValue = 0;
      door2_lastDistanceValue = 0;
      check_door_status();
       
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.println(F(" try again in 3 seconds"));
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}

#if BLYNK_ENABLED == true
  // function that checks if there is a connection to Blynk server
  void reconnectBlynk() {
    if (!Blynk.connected()) {
      Serial.println(" Blynk Lost connection");
      Publish(BLYNK__AVAIL_TOPIC, blynkoffMess); //Publish to MQTT server a message that Blynk is not connected
      blynkup = "no";
      if(Blynk.connect()) {
        Serial.println("Blynk Reconnected");
        Publish(BLYNK__AVAIL_TOPIC, blynkonMess); //Publish to MQTT server a message that Blynk is connected
      }
      else {
        Serial.println("Blynk Not reconnected");
      }
    }
  }
#endif

/********************************** START DOOR and CAR STATUS *****************************************/
void check_door_status() {
  byte state;
  byte stateVerify;
  int distance = -1;
  int distpin = -1;
  int lastDistance = 0;

  // Sensor 1 (--- Door 1 ---) Take a distance measurement
  delay(ULTRASONIC_SETTLE_TIMEOUT); // Let the last ping settle
  distance = sonar[0].ping_cm(); // Take a reading
  Serial.print(distance);
  Serial.print(".");

  // Test if sensor 1 reading is zero, then assume timeout (distance is more than max) so reset echo pin
  if (distance == 0) {                   // only when there was no echo
    pinMode(DOOR1_ECHO_PIN, OUTPUT);
    digitalWrite(DOOR1_ECHO_PIN, LOW); 
    delayMicroseconds(10);               // shorter periods are not reliable
    pinMode(DOOR1_ECHO_PIN, INPUT);
    delayMicroseconds(10);
    distpin = digitalRead(DOOR1_ECHO_PIN);    // the echo pin should now be low
    if (distpin == 1){
      Serial.println("echo still high"); // mainly for debug 
      delay(500);
    }
  }
  
  memmove(&door1_lastDistanceValues[1], &door1_lastDistanceValues[0], (door_numValues - 1) * sizeof(door1_lastDistanceValues[0])); // Move the array forward
  door1_lastDistanceValues[0] = distance;
  
  // Find the previous distance that was valid
  lastDistance = 0;
  for (int y=1; y<door_numValues; y++) {
    if (door1_lastDistanceValues[y] > 0) {
      lastDistance = door1_lastDistanceValues[y];
      break;
    }
  }

  if ((distance > -1) && (lastDistance > -1)) { //only update door state if it is above -1
    state = getStateDoor(distance);
    stateVerify = getStateDoor(lastDistance);
    
    if ((state == stateVerify) && (state != getStateDoor(door1_lastDistanceValue))) {  //If door state has changed from open to closed or vice versa then...
      digitalWrite(LED_BUILTIN, LOW);     // Turn the status LED on
      door1_lastDistanceValue = distance;
      sendState(1);  //publish to MQTT and Blynk
      digitalWrite(LED_BUILTIN, HIGH);     // Turn the status LED off
    }
  }


  // Sensor 2 (--- Car ---) Take a distance measurement
  delay(ULTRASONIC_SETTLE_TIMEOUT); // Let the last ping settle
  distance = sonar[1].ping_cm(); // Take a reading
  Serial.print(distance);
  Serial.print(".");

  // test if sensor 2 reading is zero, then assume timeout (distance is more than max) so reset echo pin
  if (distance == 0) {                   // only when there was no echo
    pinMode(DOOR2_ECHO_PIN, OUTPUT);
    digitalWrite(DOOR2_ECHO_PIN, LOW); 
    delayMicroseconds(10);               // shorter periods are not reliable
    pinMode(DOOR2_ECHO_PIN, INPUT);
    delayMicroseconds(10);
    distpin = digitalRead(DOOR2_ECHO_PIN);    // the echo pin should now be low
    if (distpin == 1){
      Serial.println("echo still high"); // mainly for debug 
      delay(500);
    }
  }
  
  memmove(&door2_lastDistanceValues[1], &door2_lastDistanceValues[0], (door_numValues - 1) * sizeof(door2_lastDistanceValues[0])); // Move the array forward
  door2_lastDistanceValues[0] = distance;
  
  // Find the previous distance that was valid
  lastDistance = 0;
  for (int y=1; y<door_numValues; y++) {
    if (door2_lastDistanceValues[y] > 0) {
      lastDistance = door2_lastDistanceValues[y];
      break;
    }
  }
  
  if ((distance > -1) && (lastDistance > -1)) {  //only update car state if it is above -1
    state = getStateCar(distance);
    stateVerify = getStateCar(lastDistance);
      
    if ((state == stateVerify) && (state != getStateCar(door2_lastDistanceValue))) { //If car state has changed from occupied to empty or vice versa then... 
      digitalWrite(LED_BUILTIN, LOW);     // Turn the status LED on
      door2_lastDistanceValue = distance;
      sendState(2);  // publish to MQTT and Blynk
      digitalWrite(LED_BUILTIN, HIGH);     // Turn the status LED off
    }
  }

  Serial.println(".");
}

/*************************************NOTIFY STATUS BLYNK*********************************************/


#if BLYNK_ENABLED == true
  // Sends notification to Blynk server
  void perform_notify(String s) {
    Serial.print(F("Sending Notify to connected systems, value:"));
    Serial.println(s);
    // Blynk notification    
    if(Blynk.connected()) {
      Serial.println(F(" Blynk Notify"));
      Blynk.notify(s);
    }
  
  }
#endif

/*************************************DHT SENSOR*********************************************/


void dht_read_publish() {
  // Read values from sensor
  float hum = dht.readHumidity();
  float tempRaw = dht.readTemperature();

  // Check if there was an error reading values
  if (isnan(hum) || isnan(tempRaw)) {
    Serial.print("Failed to read from DHT sensor; will try again in ");
    Serial.print(dht_publish_interval_s);
    Serial.println(" seconds...");
    return;
  }

  boolean celsius = DHT_TEMPERATURE_CELSIUS;
  float temp;
  if (celsius) {
    temp = tempRaw;
  } else {
    temp = (tempRaw * 1.8 + 32);
  }

  // Publish the temperature and humidity payloads via MQTT topics
  char payload[5];
  dtostrf(temp, 5, 1, payload);
  Publish(MQTT_TEMPERATURE_TOPIC, payload);

  dtostrf(hum, 5, 1, payload);
  Publish(MQTT_HUMIDITY_TOPIC, payload); 
}


/********************************** START MAIN LOOP *****************************************/
void loop() {

  if (!client.connected()) {
    reconnect();
  }

  unsigned long currentTime = millis();
  
  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    Serial.print(F("WIFI Disconnected. Attempting reconnection."));
    setup_wifi();
    return;
  }

  client.loop(); // Check MQTT

  ArduinoOTA.handle(); // Check OTA Firmware Updates

  server.handleClient(); // Check Web page requests

  #if BLYNK_ENABLED == true
    if(curr_cloud_access_en()) {
    reconnectBlynk();  //Check if Blynk connected
    }
    
    if(Blynk.connected()) { 
      timer.run();  //timer which runs a periodic Blynk connection check 
      Blynk.run();  //check in with the Blynk server for updates
      blynkup = "yes";       
    }
  #endif

  check_door_status(); // Check the sensors and publish any changes
  
  getESPdata(); // Gather ESP8266 data and publish to MQTT 
  
  #if DHT_ENABLED == true  //Read DHT sensor periodically and publish results to MQTT
    if (currentTime - dht_lastReadTime > (dht_publish_interval_s * 1000)) {
      dht_read_publish();
      dht_lastReadTime = millis();
    }
  #endif

  if (!Blynk.connected()) { 
    //delay(500); // We have enabled Light sleep so this delay should reduce the power used
    delay(800); // We have enabled Light sleep so this delay should reduce the power used
    //delay(2000); // We have enabled Light sleep so this delay should reduce the power used
    //Serial.print(".");      
  }
  
}
