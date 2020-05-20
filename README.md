# GarDoor-Car:
Version: 2.10R

## ESP8266 MQTT Garage Door Opener, Garage Door and Car Sensor, and Temperature Monitor using Home Assistant/Blynk
This project allows you to control (open/close) a "dumb" garage door opener for a garage door and independently reports the garage door status (open/closed) and presence of a car in the garage via MQTT and Blynk. In addition you can monitor the Temperature and Humidity within your garage over MQTT as well. This project requires two ultrasonic sensors: one pointed at the garage door, and the other pointed at the normal location of the car. This allows the presence of the car to be detected whether the garage door is open or closed.  

The code covered in this repository utilises Home Assistant's [MQTT Cover Component](https://www.home-assistant.io/components/cover.mqtt/), [MQTT Binary Sensor Component](https://www.home-assistant.io/components/binary_sensor.mqtt/), and [MQTT Sensor Component](https://www.home-assistant.io/components/sensor.mqtt/). There is an example configuration in this repository. GarDoor will respond to HA's open and close commands and reports door/car status to keep HA's GUI in sync with the door and car state. GarDoor should also be controllable via any home automation software that can work with MQTT.  

GarDoor can also be used with the Blynk app (iOS or Android). If you enable this feature, you can monitor the garage door/car status as well as open/close the door with the app. You can also choose to be notified of garage door events on your phone. 

GarDoor requires both hardware and software components. It requires an ESP8266 microcontroller with sensors and a relay. If you select the appropriate parts, you can build a GarDoor-Car device with no soldering! The software component is found in this repo. 

The code is based on DotNetDann's ESP-MQTT-GarageDoorSensor project (thanks Daniel) (which took inspiration from GarHAge and OpenGarage) with added additional features. Parts of the documentation is based on GarHAge documentation (thanks Mark).   

### Supported Features Include
- Reports status of the garage door
- Reports presence of a car in the garage whether the door is open or closed
- Reports ultrasonic sensors distance readings
- Reports door is open / car is absent if distance reading is 0 (because wave has not been received back by sensors)
- Automatically resets echo pin(s) if distance is 0 to support ultrasonic sensors that do not timeout where no wave is received
- Reports information about the ESP8266
- Reports information on temperature and humidity
- Ability to select Celius or Fahrenheit for temperature
- Web page for status of the garage door, car presence, and temperature/humidity
- Ability to use the Blynk App (iOS or Android) to open/close the garage door and monitor door/car status
- Ability to receive Blynk notifications when garage door is opened/closed and when door is left open for more than 15 minutes
- Events and status sent via MQTT
- MQTT commands to toggle relay to open/close the garage door
- Ability to select an Active High or Active Low relay
- Over-the-Air (OTA) Upload from the ArduinoIDE!

### How GarDoor-Car Operates

GarDoor-Car subscribes to a configurable MQTT topic for the garage door (by default, gardoor/1/action).

When the OPEN payload is received on this topic, GarDoor-Car momentarily activates the relay connected to the garage door opener to cause the door to open.

When the CLOSE payload is received on this topic, GarDoor-Car momentarily activates the relay connected to the garage door opener to cause the door to close. By default, GarDoor-Car is configured to activate the same relay for the OPEN and CLOSE payloads, as most (if not all) garage door openers operate manually by momentarily closing the same circuit to both open and close.

When the STATE payload is received on this topic, GarDoor-Car publishes the status of the garage door (open or closed) to the topic gardoor/1/status and the occupied status of the car (true or false) to the topic gardoor/2/status. The distance measurements for the door and car sensors are also included in these messages. These messages are published with the "retain" flag set. To ensure that HA displays up-to-date garage door and car status after a HA restart, it is recommended that you create an automation in Home Assistant that publishes the STATE payload via MQTT on HA start. An example is provided in the Home Assistant Configuration section of this documentation.

When the state of the garage door changes (either because GarDoor-Car (via HA/MQTT or Blynk app) has triggered the door to open or close, or because the door has been opened or closed via a remote, pushbutton switch, key switch, or manually), GarDoor-Car publishes the status (open or closed) and sensor distance measurement of the garage door to gardoor/door/1/status. These messages are published with the "retain" flag set.

When the occupied state of the car changes because the car leaves or enters the garage, GarDoor-Car publishes the occupied status (true or false) and sensor distance measurement of the car to gardoor/2/status. These messages are published with the "retain" flag set.

GarDoor-Car also publishes (topic gardoor/availability) a "birth" message on connection to your MQTT broker, and a "last-will-and-testament" message on disconnection from the broker, so that your home automation software can respond appropriately to GarDoor-Car being online or offline.

GarDoor-Car also publishes to your MQTT broker on topic gardoor/blynkavailability (only if GarDoor-Car is already online; if GarDoor-Car goes offline first, then an automation in your home automation software is required to update the status on the gardoor/blynkavailability topic) a message when GarDoor-Car connects or disconnects from the Blynk server, so that your home automation software can respond appropriately to the GarDoor-Car Blynk connection being online or offline.

GarDoor-Car publishes data about the ESP8266 (topic gardoor/ESPtoMQTT) and temperature/humidity (topics gardoor/temperature and gardoor/humidity) to your MQTT broker.

GarDoor-Car can also connect (if enabled by the user) to a Blynk server. This enables door and car status updates, as well as the door sensor distance measurement to be published to a Blynk server/app. GarDoor-Car can also receieve button requests from the Blynk server/app, which causes GarDoor-Car to momentarily activate the relay connected to the garage door opener to cause the door to open or close. This feature gives a user the ability to press a button in the Blynk app to control the garage door. Notifications can also be published to the Blynk server/app when the garage door open/closes or is left open too long. 

### Hardware Parts List
- [NodeMCU](https://www.aliexpress.com/item/32665100123.html) or other ESP8266-based microcontroller 
- [2 x HC-SR04P Ultrasonic Sensors](https://www.aliexpress.com/item/32711959780.html) May also use HC-SR04 model
- [5V 1 Channel Relay Board](https://www.gearbest.com/relays/pp_226384.html) Active-High or Active-Low supported
- [DHT22 module](https://www.aliexpress.com/item/32899808141.html) (optional)  
- Electrical cable (two-conductor) to connect the relay to your garage door opener 
- Power source for NodeMCU: MicroUSB cable with USB 2.1A power adapter or 5V power supply  
- Male-to-female (and male-to-male if you use HC-SR04 model ultrasonic sensors and need to connect four components to 5v power using the breadboard power rails) breadboard jumper wires (Dupont)
- Breadboard 400 tie-point or larger
- Project box or case (optional)

#### Detailed Hardware

#### 1. ESP8266-based microcontroller
I recommend the NodeMCU as GarDoor-Car was developed and is tested on it. Its advantages are:

- it comes with header pins already soldered so that it can plug directly into a solderless breadboard;
- its VIN (or VU on the LoLin NodeMCU v3 variant) port can power the 5v relay module and DHT22 sensor;
- it can be powered and programmed via MicroUSB;
- it has Reset and Flash buttons, making programming easy.

This guide is written specifically for the NodeMCU. But, GarDoor-Car should also work with the Adafruit HUZZAH, Wemos D1, or similar, though you may need to adjust the GPIO ports used by the sketch to match the ESP8266 ports that your microcontroller makes available.

#### 2. Single 5v relay module
A single 5v relay module makes setup easy: just plug jumper wires from the module's VCC, S, and GND pins to the NodeMCU. Because the relay module is powered by 5v, its inputs can be triggered by the NodeMCU's GPIOs.

GarDoor-Car will work with relay modules that are active-high or active-low; if using an active-low relay, be sure to set the relevant configuration parameter in auth.h, described below, and test thoroughly to be sure that your garage door opener(s) are not inadvertently triggered after a momentary power-loss.

#### 3. Two Ultrasonic Sensors (model HC-SR04P recommended)
GarDoor-Car will work with either the HC-SR04 or HC-SR04P model. The HC-SR04P can use 3.3v or 5v power, while the HC-SR04 requires 5v power. When using the HC-SR04P model (as the wiring diagram below assumes), the maximum distance of the sensor is reduced slightly when using 3.3v power. For the purposes of this project, this shouldn't affect the results of the GarDoor-Car. Thus either model can be used, but it is recommended that you use the HC-SR04P powered using 3.3v.  

While a HC-SR04P works using 3.3v power without any other changes to the design (see the wiring diagram below), if using HC-SR04 sensors (which require you to connect them to the 5v VCC for power), a 1k Ω resistor will need to be used between the ECHO pin on each distance sensor and the related input pin on the NodeMCU. 

The code can be used with ultrasonic sensors which do not include a timeout. When the sensors do not receive an echo signal (because the wave has travelled further than the maximum supported distance and cannot or does not bounce back, they report 0 distance. When this occurs some sensors reset the echo pin to LOW when a timeout occurs, but some cheaper sensors do not have a timeout so they never reset the echo pin. The code includes a routine to reset the echo pin, if the read distance is 0.   

#### 4. 5v MicroUSB cable and power supply
Power your NodeMCU via the same type of power supply used to charge Android phones or power a RaspberryPi. Powering the NodeMCU via MicroUSB cable and power adapter is recommended since the relay module & DHT22 sensor can be powered via the NodeMCU VIN (or VU on the LoLin v3 variant) pin. A USB power adapter with 2.1A or above is recommended. 

#### 5. Solderless breadboard (400 tie-point or larger)
The NodeMCU may be mounted in the center of this breadboard nicely, leaving a female port next to each NodeMCU pin on both sides. However for this project it is important that the two ultrasonic sensors use the same trigger pin, and the relay and DHT22 sensor both use the 5v VIN power pin. 

To overcome this you could use male-to-male jumper wires. You would connect the 5v/ground and one set of 3.3v/ground NodeMCU pins to each set of outside power rails on the breadboard using male-to-male jumper wires. Then connect the male end of a male-to-female jumper wire for a relay/sensor's power and ground to a + (for power) or - (for ground) port on the appropriate 3.3V or 5V rail to provide power/ground to components using the rails.  

In the same way, you would connect the NodeMCU output to be used as the trigger (pin D5) to a port above the NodeMCU using a male-to-male jumper wire. Then you would connect the male end of the male-to-female jumper wires connected to the trig pin of each ultrasonic sensor, to two ports in the same line on the broadboard as the male end of the wire connected to pin D5 of the NodeMCU.  

You may also choose to mount the DHT22 and/or relay modules directly on the breadboard by inserting their pins vertically into ports above the NodeMCU. If do this you will require male-to-male jumper wires to connect the 3 ports of the DHT22 and 3 ports of the relay to the appropriate ports on the breadboard to link to the NodeMCU's pins.  

This makes for a clean and solderless installation. Finally, these breadboards often also have an adhesive backing, making mounting in your project box easy.

#### 6. A DHT22 module (optional)
This sensor provides temperature/humidity readings to GarDoor-Car, but is optional and can be disabled in the auth.h file. The module verion (which is mounted on a board) is recommended, since the board includes a pull-up resister to reduce the output to 3.3v (the DHT22 is powered using 5v). If you use a DHT22 sensor (without a module board), you will need to use a 10k Ω resistor wired to the power input and the data pin of the DHT22 to act as a pull-up.   

#### 7., 8., & 9. Miscellaneous parts
To install GarDoor-Car, you will also require:

- Low voltage/current two-conductor electrical wire of the appropriate length to make connections from the relay where GarDoor-Car is mounted to the garage door opener.
- Male-to-female breadboard jumper wires to make connections from the NodeMCU via the breadboard ports to the relay module (3 wires required), the DHT22 (3 wires required), and the two ultrasonic sensors (4 wires required for each). 
- You will also require male-to-male jumper wires to use the outside power rails (4 wires) and to connect both ultrasonic sensors trig pins to the same NodeMCU pin (1 wire). If you choose to insert the DHT22 or relay's pins vertically on the breadboard, you will need further male-to-male jumper wires (up to 6 wires) instead of male-to-female wires.   
- A project box to hold the NodeMCU, the relay module, the DHT22 sensor module (if required), and the two ultrasonic sensors. While the ultrasonic sensors can be mounted inside the same box as the nodeMCU, they will need to have unobstructed access outside the box to point at the garage door and car.  

### Building GarDoor-Car

See the wiring diagram below for an example of how the pins are wired up. 

- Attach your NodeMCU to the middle (between the outside power rails) of the solderless breadboard at one end; the MicroUSB connector should be facing one of the short outside edges of the breadboard (the power rails are on the longer outside edges). Ensure that there are at least two female ports next to each NodeMCU pin on each side of the NodeMCU.
- Mount the solderless breadboard in your project box.
- Mount the relay module in your project box. You could insert lengthways the relay's three pins into the breadboard above the position of the NodeMCU so that the relay stands up on top of the breadboard. If you do this then you will use male-to-male jumper wires plugged into ports on the breadboard to connect the relay's pins to the NodeMCU. 
- Mount the DHT22 module in your project box (if required). You could insert lengthways the DHT22's three pins into the breadboard above the position of the NodeMCU so that the DHT22 stands up on top of the breadboard. If you do this then you will use male-to-male jumper wires plugged into ports on the breadboard to connect the DHT22's pins to the NodeMCU.  
- Label your two HC-SR04P sensors as sensor 1 (door) and sensor 2 (car). Mount the two HC-SR04P sensors in your project box, ensuring there are holes for the front of the sensors to "see" the outside. Ensure that the direction you mount sensor 1 in will enable it to "see" the garage door once you position the box in its intended location in the garage. Also ensure that sensor 2 will be able to "see" the position of the car when the box is in the garage. You could mount the box above the garage floor or on the side of the garage wall.     
- Plug a m-to-m jumper wire from the VIN / VU port on the NodeMCU into the + power rail on one side of the breadboard.
- Plug a m-to-m jumper wire from the GND port (near the VIN / VU port) on the NodeMCU into the - power rail on one side of the breadboard. 
- Plug a m-to-m jumper wire from a 3.3v port on the NodeMCU into the + power rail on the other side of the breadboard.
- Plug a m-to-m jumper wire from the GND port (near the 3.3v port you have used) on the NodeMCU into the - power rail on the other side of the breadboard. 
- Plug a f-to-m jumper wire from VCC / + on the relay module to the 5v + power rail.
- Plug a f-to-m jumper wire from GND / - on the relay module to the 5v - power rail.
- Plug a f-to-m jumper wire from S on the relay module to port D6 on the NodeMCU (or Arduino/ESP8266 GPI012).
- Plug a f-to-m jumper wire from + on the DHT22 module to the 5v + power rail.
- Plug a f-to-m jumper wire from - on the DHT22 module to the 5v - power rail.
- Plug a f-to-m jumper wire from Data on the DHT22 module to D7 port on the NodeMCU (or Arduino/ESP8266 GPI013).
- Plug a m-to-m jumper wire from the D5 port (or Arduino/ESP8266 GPI014) of the NodeMCU into a port on a spare line above the NodeMCU on the breadboard. 
- Plug a f-to-m jumper wire from VCC on the HC-SR04P sensor 1 module to the 3.3v + power rail.
- Plug a f-to-m jumper wire from GND on the HC-SR04P sensor 1 module to the 3.3v - power rail.
- Plug a f-to-m jumper wire from Trig on the HC-SR04P sensor 1 module to a port on the same line as the other end of the wire connected to D5 of the NodeMCU is plugged into.
- Plug a f-to-m jumper wire from Echo on the HC-SR04P sensor 1 module to D2 port on the NodeMCU (or Arduino/ESP8266 GPI04).
- Plug a f-to-m jumper wire from VCC on the HC-SR04P sensor 2 module to the 3.3v + power rail.
- Plug a f-to-m jumper wire from GND on the HC-SR04P sensor 2 module to the 3.3v - power rail.
- Plug a f-to-m jumper wire from Trig on the HC-SR04P sensor 2 module to a port on the same line as the other end of the wire connected to D5 of the NodeMCU is plugged into.
- Plug a f-to-m jumper wire from Echo on the HC-SR04P sensor 2 module to D1 port on the NodeMCU (or Arduino/ESP8266 GPI05).

*Done!*

#### Wiring Diagram
![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/master/Wiring%20Diagram-RollerDoor.png?raw=true "Wiring Diagram")

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/GarDoor-Carv2.00_breadboard.png?raw=true "Breadboard Diagram")


### Software

#### 1. Set up the Arduino IDE

You will modify the configuration parameters and upload the sketch to the NodeMCU with the Arduino IDE.

1. Download the Arduino IDE for your platform from [here](https://www.arduino.cc/en/Main/Software) and install it.
2. Add support for the ESP8266 to the Arduino IDE by following the instructions under "Installing with Boards Manager" [here](https://github.com/esp8266/arduino).
3. Add the "PubSubClient", "NewPing", and "Blynk" libraries to the Arduino IDE: follow the instructions for using the Library Manager [here](https://www.arduino.cc/en/Guide/Libraries#toc3), and search for and install the PubSubClient, NewPing, and Blynk libraries.
4. You may need to install a driver for the NodeMCU for your OS - Google for instructions for your specific microcontroller and platform, install the driver if necessary, and restart the Arduino IDE.
5. Select your board from Tools - Boards in the Arduino IDE (e.g. "Generic ESP8266 Module").

#### 2. Install Blynk App and Project and get Token

- Install the Blynk app on your phone by finding the Blynk app in your app store ([Android](https://play.google.com/store/apps/details?id=cc.blynk&hl=en_US) and [Apple iOS](https://apps.apple.com/us/app/blynk-iot-for-arduino-esp32/id808760481) and installing it.
- Run the Blynk app, and create an account (if you don't already have one). 
- In the Blynk app find the menu option to scan a QR code, click it and then scan the QR code that’s located [here](https://github.com/OpenGarage/OpenGarage-Firmware/blob/master/OGBlynkApp/og_blynk_1.1.png) (OpenGarage Project for Blynk App). If you have just created a new Blynk account you will have enough energy credits to use the project in the app for free. If you already have other project(s) in your Blynk account, you may need to purchase further energy credits to use the OpenGarage project in the app.  
- Once the project is scanned, go to the Project Settings, and copy or email the Blynk authorisation token to yourself. This is the token you will need to put into the auth.h file in step 3 below.
- You can configure the project in the app, such as changing the name of the button, but DO NOT change the virtual pin numbers for any button or widget (V1, V2 etc).  

*NOTE: You can logon to the same Blynk account on more than one phone. If you do this on other family members phones, they can use the same project in the app to control the same GarDoor-Car.* 

#### 3. Load the sketch in the Arduino IDE and modify the user parameters in auth.h

Download the files from this repo to your computer, and open the GarDoor-Car-Vx.xx.ino file from the folder. GarDoor-Car's configuration parameters are found in auth.h. Select the auth.h tab in the Arduino IDE. This section describes the configuration settings and parameters and their permitted values. There are some settings that you must set, while others can be left as they are unless you have a specific reason to change them. 

If you wish to get up and running easily, these are the only settings you need to set:
- WIFI_SSID
- WIFI_PASSWORD
- MQTT_SERVER
- MQTT_USER and MQTT_PASSWORD (only if you use authentication on your MQTT Broker)
- MQTT_PORT (only if you use a non-default port)
- OTApassword (set your own password for this)
- BlynkAuthToken (unless you disable Blynk)
- ULTRASONIC_DIST_MAX_CLOSE
- ULTRASONIC_DIST_MAX_CAR
- RELAY_ACTIVE_TYPE
- DHT_ENABLED (if using a DHT22)
- DHT_TEMPERATURE_CELSIUS (if using a DHT22 and you wish to use the fahrenheit scale)

Important!! After making any desired changes save the auth.h file. 

*IMPORTANT: No modification of the sketch code in GarDoor-Car-Vx.xx.ino is necessary (or advised, unless you are confident you know what you are doing and are prepared for things to break unexpectedly).*

**Wifi Settings**

*NOTE: An available IPv4 network with a DHCP for IPv4 service is assumed* 

WIFI_HOSTNAME "your-device-name"

The wifi hostname GarDoor-Car will use. Should be unique among all the devices connected to your network. Must be placed within quotation marks. (Default: GarDoor)

WIFI_SSID "your-wifi-ssid"

The wifi ssid GarDoor-Car will connect to. Must be placed within quotation marks.

WIFI_PASSWORD "your-wifi-password"

The wifi ssid's password. Must be placed within quotation marks.

**MQTT Settings**

MQTT_SERVER "w.x.y.z"

The IPv4 address of your MQTT broker. Must be placed within quotation marks.

MQTT_USER "your-mqtt-username"

The username required to authenticate to your MQTT broker. Must be placed within quotation marks. Use "" (i.e. a pair of quotation marks) if your broker does not require authentication.

MQTT_PASSWORD "your-mqtt-password"

The password required to authenticate to your MQTT broker. Must be placed within quotation marks. Use "" (i.e. a pair of quotation marks) if your broker does not require authentication.

MQTT_PORT your-mqtt-server-port-number

The port on your MQTT server GarDoor-Car will use to connect to the MQTT service. Must be a number. (Default: 1883)

MQTT_AVAIL_TOPIC WIFI_HOSTNAME "mqtt-topic-for-gardoor-availability"

Name of the topic that the MQTT broker will host and HA will use to determine if GarDoor-Car is online or offline. Must be placed within quotation marks. (Default: /availability)

TimeReadSYS milliseconds

Number of milliseconds (1000 per second) between which system readings of ESP8266 data will be taken and published using MQTT. Must be a number. (Default: 100000)

topicESPtoMQTT WIFI_HOSTNAME "mqtt-topic-for-esp8266-data"

Name of the topic that the MQTT broker will host and HA will use to get running data about your ESP8266. Must be placed within quotation marks. (Default: /ESPtoMQTT)

**OTA Settings**

OTApassword "your-ota-password"

The password you will need to enter to upload remotely via the ArduinoIDE. Must be placed within quotation marks. (Default: ota password)

OTAport your-esp8266-device-port-number

The port you will need to enter into the ArduinoIDE to allow it to connect to your running ESP8266. Must be a number. (Default: 8266)

**Blynk App Settings**

BLYNK_ENABLED true

Set to true to allow you to use the Blynk app. Set to false to disable use of Blynk. (Default: true)

BlynkAuthToken "your-blynk-project-token"

The Blynk token from the project in the Blynk app (see step 2 above). Must be placed within quotation marks.  

BLYNK_NOTIFY true

Set to true to enable notifications via the Blynk app. Set to false to disable Blynk notifications. (Default: true)

BLYNK_DEFAULT_DOMAIN "blynk-server-address-or-name"

The name or IP address of the Blynk server the Blynk app/GarDoor-Car connects to. Only change this if you have setup a local Blynk server. Must be placed within quotation marks. (Default: blynk-cloud.com)  

BLYNK_DEFAULT_PORT blynk-server-port-number

The port the Blynk app/GarDoor-Car uses to connect to the Blynk server. Only change this if you have setup a local Blynk server. Must be a number. (Default: 80) 

BLYNK__AVAIL_TOPIC WIFI_HOSTNAME "mqtt-topic-for-blynk-availability"

Name of the topic that the MQTT broker will host and HA will use to determine if GarDoor-Car is connected to the Blynk server. Must be placed within quotation marks. (Default: /blynkavailability)

**Distance Parameters**

ULTRASONIC_MAX_DISTANCE number-in-cm

Maximum distance (in cm) to ping. If using HC-SR04P sensors should be 400. If using HC-SR04 sensors, should be 450. Must be a number. (Default: 400)  

ULTRASONIC_DIST_MAX_CLOSE number-in-cm

Maximum distance (in cm) to indicate that the garage door is closed (used by sensor 1). If the measured distance to the object is less than this, then the door is published as closed. If the measured distance to the object is more than this (or is 0 because the wave does not or cannot bounce back off an object within 400cm), then the door is published as open. Must be a number. (Default: 120)   

ULTRASONIC_DIST_MAX_CAR number-in-cm

Maximum distance (in cm) to indicate that the car is present (used by sensor 2). If the measured distance to the object is less than this, then the car is present. If the measured distance to the object is more than this (or is 0 because the wave does not or cannot bounce back off an object within 400cm), then the car is absent. Must be a number. (Default: 120)   

ULTRASONIC_SETTLE_TIMEOUT milliseconds 

Number of milliseconds (1000 per second) to wait between pings (as both sensors get triggered at the same time). Waiting between pings improves readings for each sensor. Must be a number. (Default: 500)

RELAY_ACTIVE_TIMEOUT milliseconds

Number of milliseconds (1000 per second) the relay will close to actuate the door opener.  While it is "closed" the NO connector on the relay will allow current to flow, while the NC connector will cut the flow of current. Must be a number. (Default: 500)

RELAY_ACTIVE_TYPE HIGH

Set to LOW if using an active-low relay module. Set to HIGH if using an active-high relay module. (Default: HIGH)

DOOR_TRIG_PIN 14

The ESP8266 GPIO pin that triggers the two ultrasonic sensors to determine the distance to an object and output the results on their Echo pins. Must be a number. (Default: 14) (which is D5 on the NodeMCU)

**Door 1 Parameters**

DOOR1_ALIAS "name"

The alias to be used for Door 1 (garage door) in serial messages, MQTT topic and webpage. Must be placed within quotation marks. (Default: Door)

MQTT_DOOR1_ACTION_TOPIC WIFI_HOSTNAME "mqtt-topic"

The MQTT broker topic GarDoor-Car will subscribe to for action commands for Door 1 (garage door). Must be placed within quotation marks. (Default: /1/action)

MQTT_DOOR1_STATUS_TOPIC WIFI_HOSTNAME "mqtt-topic"

The Mqtt broker topic GarDoor-Car will publish Door 1's (garage door) status to. Must be placed within quotation marks. (Default: /1/status)

DOOR1_RELAY_PIN 12

The ESP8266 GPIO pin connected to the relay signal pin which causes the relay to "close" allowing current to flow. The relay is in turn connected to the garage door opener's terminals, using either the NO (normally open) or NC (normally close) connector plus the common connector. This assumes that the same terminals control open and close via a momentary connection of the terminals, which most garage door opener's do. (Default: 12) (which is D6 on the NodeMCU)

DOOR1_ECHO_PIN 4

The ESP8266 GPIO pin that receives the echo pin output of sensor 1 to measure the distance from the sensor pointed towards the garage door. Must be a number. (Default: 4) (which is D2 on the NodeMCU)

DOOR1_LIMIT_RELAY_CLOSE

Set to true to prevent the relay from opening the garage door (only close will work). Set to false to be able to use the relay to both open and close the door. (Default: false)

**Car Parameters**

CAR_ALIAS "name"

The alias to be used for the car in serial messages, MQTT topic and webpage. Must be placed within quotation marks. (Default: Car)

MQTT_CAR_STATUS_TOPIC WIFI_HOSTNAME "mqtt-topic"

The Mqtt broker topic GarDoor-Car will publish the Car's occupied status to. Must be placed within quotation marks. (Default: /2/status)

DOOR1_ECHO_PIN 5

The ESP8266 GPIO pin that receives the echo pin output of sensor 2 to measure the distance from the sensor pointed towards the car's normal location. Must be a number. (Default: 5) (which is D1 on the NodeMCU)

**DHT Parameters**

DHT_ENABLED true

Set to true if you include a DHT sensor in your hardware. Set to false to disable the use of a DHT sensor. (Default: true)

DHT_PIN 13 // D7 on ESP8266

The ESP8266 GPIO pin that receives the output of the DHT sensor to measure temperature and humidity. Must be a number. (Default: 13) (which is D7 on the NodeMCU)

DHT_TYPE DHT22 

Set to DHT22 (for DHT22 / AM2302 / AM2321) or DHT21 (for DHT21 / AM2301) or DHT11 depending on the type of sensor you use. (Default: DHT22)

MQTT_TEMPERATURE_TOPIC WIFI_HOSTNAME "mqtt-topic"

The Mqtt broker topic GarDoor-Car will publish the DHT temperature readings to. Must be placed within quotation marks. (Default: /temperature)

MQTT_HUMIDITY_TOPIC WIFI_HOSTNAME "mqtt-topic"

The Mqtt broker topic GarDoor-Car will publish the DHT humidity readings to. Must be placed within quotation marks. (Default: /humidity)

DHT_PUBLISH_INTERVAL seconds 

Number of seconds between between each temperature/humidity reading that will be taken and published using MQTT. Must be a number. (Default: 300)

DHT_TEMPERATURE_CELSIUS true

Set to true if you wish temperature to be reported using celsius scale, or false to report temperature using fahrenheit scale. (Default: true)

DHT_TEMPERATURE_ALIAS "name"  

The alias to be used for the temperature output in serial messages, MQTT topic and webpage. Must be placed within quotation marks. (Default: Garage Temperature)

DHT_HUMIDITY_ALIAS "name"
 
The alias to be used for the humidity output in serial messages, MQTT topic and webpage. Must be placed within quotation marks. (Default: Garage Humidity)


#### 4. Upload the sketch to your NodeMCU / microcontroller

Ensure that the Auduino IDE is running and the sketch is loaded and configured. If using the NodeMCU, connect it to your computer via MicroUSB. The NodeMCU will connect to a virtual COM port within the Auduino IDE.   

Configure the Arduino IDE for the upload using the settings shown in the figure below:  

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/arduino-settings.png?raw=true "Arduino IDE Settings")

There are two uploads methods you can use:
- In the Arduino IDE, Tools - Ports menu ensure that the COM port is selected that the NodeMCU is connected to. Also ensure you have configured the settings for the Generic ESP8266 Board as shown in the figure above. Then select Sketch - Upload. 
- Press and hold the reset button on the NodeMCU, press and hold the Flash button on the NodeMCU, then release the Reset button. Select Sketch - Upload in the Arduino IDE.

If using a different ESP8266 microcontroller, follow that device's instructions for putting it into flashing/programming mode.

#### 5. Check the Arduino IDE Serial Monitor

Open the Serial Monitor via Tools - Serial Monitor. Reset your microcontroller. If all is working correctly, you should see something similar to the following messages:

```
Starting GarDoor-Car...

Connecting to your-wifi-ssid.. WiFi connected - IP address: 192.168.1.100
Attempting MQTT connection...Connected
Publishing birth message "online" to GarDoor/availability...
Attempting BLYNK connection...Connected!
Publishing message "blynkon" to GarDoor/blynkavailability...
Subscribing to GarDoor/1/action...
Door closed! Publishing to GarDoor/1/status...
Car Absent! Publishing to GarDoor/2/status...
```

If you receive these (or similar) messages, all appears to be working correctly. 

#### 6. Check Blynk App

Make sure the Blynk project is running (in the project there is a triangle-shaped Run button). You should get a distance reading in the app and be able to click the button to activate the relay. 

#### 7. Reserve an IPv4 address on your DHCP service

Logon onto your router (or device/server) that runs your DHCP service. Reserve the IP address of the GarDoor-Car device (also known as a DHCP static binding). This ensures that GarDoor-Car always uses the same IP address when it reboots. This makes it easier to open the webpage or upload a new sketch using Arduino OTA.  

Disconnect GarDoor-Car from your computer and prepare to install it in your garage.

### Installing GarDoor-Car in your Garage

- Mount GarDoor-Car in your garage (above the garage floor or on a wall).
- Point Sensor 1 at the garage door and secure the position of the sensor.
- Point Sensor 2 at the normal location of the car in the garage and secure the position of the sensor. 
- Connect two-conductor voltage wire to the NO (or NC if that suits your opener) and COMMON terminals of your relay module;          run the wire to the garage door opener for the door and connect to the opener's terminals (the same terminals that the pushbutton or key switch for your door is attached to). Two of the opener's terminals connected to a wall switch or key switch or pushbutton are the ones you need to connect the relay to. You can determine which ones they are, by using a single wire and shorting two of the terminals at a time until the door opens/closes. Once you have identified the two terminals you need to connect to, connect a wire to each one. The other end of one of the wires goes to the COMMON terminal on the relay, and the other end of the second wire connects to the NO (normally open) terminal.
- Plug the MicroUSB cable into a USB power supply (2.1A or above). 

*Done!*

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/GarDoor-Installed.png?raw=true "GarDoor Installed")
    
### Home Assistant Configuration 

GarDoor-Car supports Home Assistant's "MQTT Cover", "MQTT Binary Sensor", and "MQTT Sensor" platforms.

HA configuration examples are shown in the yaml file in this repo (click the link below). 

[YAML Configuration](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/Example%20Home%20Assistant%20Configuration.yaml)

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/HA-entities1.png?raw=true "HA Example")

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/HA-entities2.png?raw=true "HA Example")

A status update from GarDoor-Car can be triggered by pressing the stop button (shown in the figure above) for the door in the HA GUI. 

### Blynk App

If you enabled Blynk, on your smartphone run the Blynk app and click the triangle-shaped Run button to make sure the Blynk project is running. You can now monitor the status of the door/car and open/close your garage door. If you left Blynk notifications enabled in the auth.h file, you will receive notifications when the garage door is opened or closed, and every 15 minutes when the door is left open.   

![](https://community.blynk.cc/uploads/default/original/2X/c/c3313591520198323d736bb8b994f1150ffc2d26.jpg)

### Ariela App Notifications

If you use the Ariela App, you can configure HA to send notifications about the garage door to the app. Click the link below to learn how to set this up. 

[Ariela Push Notifications](http://ariela.surodev.com/2019/05/08/push-notifications-2/)

### Web page Examples

In a browser, enter the IP address w.x.y.z of the GarDoor-Car in the address bar. You can monitor the garage door, car, temperature, humidity and view some of the settings (see the figures below). 

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/webpagestatus1.png?raw=true "Webpage Status")

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/webpagestatus2.png?raw=true "Webpage Status")

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/webpagestatus3.png?raw=true "Webpage Status")

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/webpagestatus4.png?raw=true "Webpage Status")

![alt text](https://github.com/SmbKiwi/GarDoor-Car/blob/v2.00R/webpagestatus5.png?raw=true "Webpage Status")

#### Sample MQTT commands

Listen to MQTT commands
> mosquitto_sub -h 172.17.0.1 -t '#'

Open the garage door
> mosquitto_pub -h 172.17.0.1 -t GarDoor/1/action -m "OPEN"

#### OTA Uploading

This code also supports remote uploading to the ESP8266 using Arduino's OTA library. To utilise this, you'll need to first upload the sketch using the traditional USB method. However, if you need to update your code after that, your WIFI-connected ESP chip should show up in Arduino IDE as an option under Tools -> Port -> 'HostName'at your.ip.address.xxx. 

More information on OTA uploading can be found [here](http://esp8266.github.io/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html). 
