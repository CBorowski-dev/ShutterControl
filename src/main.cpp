#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <U8x8lib.h>
#include <PrivateData.h>

#define BUTTON_PIN_NEXT 5
#define BUTTON_PIN_SELECT 18
#define BUTTON_PIN_DOWN 19
#define BUTTON_PIN_UP 2
#define BUTTON_PIN_SWITCH 23

// #define LED_PIN_DOWN 4
// #define LED_PIN_UP 23

#define UP 1
#define DOWN 0

#define TRUE 1
#define FALSE 0

#define MAX_DISPLAY_TIME 30000  // 30 sec

// -----------------------------------------------------------
// From the Tasmota configuration file
// #define PROJECT                "home/shutters/sd1"
// #define MQTT_GRPTOPIC          "home/shutters/all" | "home/lights/all"
// #define MQTT_FULLTOPIC         "%prefix%/%topic%/"
// 
// #define SUB_PREFIX             "cmnd"
// #define PUB_PREFIX             "stat"
// #define PUB_PREFIX2            "tele"
//
// MQTT Messages:
// Rollo Küche:           home/shutters/sd1
// Rollo Esszimmer:       home/shutters/sd2
// Rollo Wohnzimmer:      home/shutters/sd3
// Rollo Wohnzimmer:      home/shutters/sd4
// 
// Rollo Ankleide:  		  home/shutters/su1
// Rollo Schlafzimmer:	  home/shutters/su2
// Rollo Kinderzimmer 1: 	home/shutters/su3
// Rollo Kinderzimmer 2:  home/shutters/su4
//
// Lichter:
// Licht Garderobe: home/lights/wardrobe
//
// After flashing new firmware on Sonoff Dual R2 the following steps have to be performed:
//
// 1. Change the configuration to 'Sonoff Dual R2 (39)'
// 2. Set 'Interlock on' per Tasmota console
// 3. Used rules:
//    rule1 on Power1#state=1 do backlog delay 280; Power1 off endon
//    rule2 on Power2#state=1 do backlog delay 280; Power2 off endon
// -----------------------------------------------------------

// #define MQTT_SUBSCRIBE "stat/home/downstairs/shutters/#"
#define MQTT_TOPIC_CMND_BASE_DOWNSTAIRS "cmnd/home/shutters/sd"
#define MQTT_TOPIC_CMND_BASE_UPSTAIRS "cmnd/home/shutters/su"

// display declaration
U8X8_SSD1327_MIDAS_128X128_HW_I2C myDisplay(/* reset=*/ U8X8_PIN_NONE);

// SSID/Password for WLAN
const char* ssid = SSID;
const char* password = PASSWORD;

// MQTT Broker IP address
const char* mqtt_server = "192.168.188.47";

WiFiClient wifiClient;
IPAddress ipAddress;
PubSubClient mqttClient(wifiClient);

uint8_t isDownstairs = TRUE;
// holds the shutter the user points to
uint8_t shutterSelection = 0;
// holds the selected shutter... every bit represents one shutter
uint8_t selectedShutters = 0;
// is display cleared
uint8_t displayCleared = FALSE;
long lastUsage = 0;

void setup_wifi() {
  delay(10);
  // Connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  ipAddress = WiFi.localIP();
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(ipAddress);
}

/*
void setLED(int led_pin, int button_state) {
  // if condition checks if push button is pressed
  // if pressed LED will turn on otherwise remain off 
  if ( button_state == HIGH ) { 
    digitalWrite(led_pin, HIGH); 
  } else {
    digitalWrite(led_pin, LOW); 
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "stat/home/downstairs/shutters/s2/POWER2") {
    // up --> power2
    if(messageTemp == "ON"){
      Serial.println("--> Shutter up start");
      setLED(LED_PIN_UP, HIGH);
    } else if(messageTemp == "OFF") {
      Serial.println("--> Shutter up stop");
      setLED(LED_PIN_UP, LOW);
    }
  } else if (String(topic) == "stat/home/downstairs/shutters/s2/POWER1") {
    // down --> power1
    if(messageTemp == "ON"){
      Serial.println("--> Shutter down start");
      setLED(LED_PIN_UP, HIGH);
    } else if(messageTemp == "OFF") {
      Serial.println("--> Shutter down stop");
      setLED(LED_PIN_UP, LOW);
    }
  }
}
*/

void moveShutter(uint8_t direction) {
  // the following code works correct when 'interlock' is 
  // set on Tasmota (enter 'interlock on' in Tasmota console)

  uint8_t z = 1;

  uint8_t currentShutter = 128;
  uint8_t minShutter = 16;
  if ( isDownstairs == TRUE ) {
    currentShutter = 8;
    minShutter = 1;
  }

  while (currentShutter >= minShutter) {
    if ((selectedShutters & currentShutter) > 0) {
      String cmnd;
      if ( isDownstairs == TRUE ) {
        cmnd = MQTT_TOPIC_CMND_BASE_DOWNSTAIRS;
      } else {
        cmnd = MQTT_TOPIC_CMND_BASE_UPSTAIRS;
      }
      cmnd += z + '0' - 48;

      if (direction == UP) {
        // up --> power2
        Serial.println((cmnd + "/POWER2").c_str());
        if(!mqttClient.publish((cmnd + "/POWER2").c_str(), "ON")) {
          Serial.println("Could not send up MQTT message");
        }
      } else {
        // down --> power1
        Serial.println((cmnd + "/POWER1").c_str());
        if(!mqttClient.publish((cmnd + "/POWER1").c_str(), "ON")) {
          Serial.println("Could not send up MQTT message");
        }
      }
    }
    currentShutter /= 2;
    z += 1;
  }
}

void setup_MQTT() {
  mqttClient.setServer(mqtt_server, 1883);
  // mqttClient.setCallback(callback);
  // mqttClient.subscribe(MQTT_SUBSCRIBE);
}

void setup_Buttons() {
  // This statements will declare pins as digital input 
  pinMode(BUTTON_PIN_NEXT, INPUT);
  pinMode(BUTTON_PIN_SELECT, INPUT);
  pinMode(BUTTON_PIN_UP, INPUT);
  pinMode(BUTTON_PIN_DOWN, INPUT);
  pinMode(BUTTON_PIN_SWITCH, INPUT);
}

/*
void setup_LEDs() {
  // This statements will declare pins as digital output 
  pinMode(LED_PIN_UP, OUTPUT);
  pinMode(LED_PIN_DOWN, OUTPUT);
}
*/

void displayLevelInformation() {
  myDisplay.print("   Rollos ");
  if (isDownstairs == TRUE) {
    myDisplay.println("EG");
  } else {
    myDisplay.println("OG");
  }
  myDisplay.println("_______________");
  myDisplay.println();
  if (isDownstairs == TRUE) {
    myDisplay.println("K   E   W1  W2");
  } else {
    myDisplay.println("A   S   K1  K2");
  }
}

void displaySelectedShutters() {
  // encoding of selected shutters in selectedShutters
  // 128 --> shutter in kitchen
  // 64  --> shutter in dining room
  // 32  --> 1st shutter in living room
  // 16  --> 2nd shutter in living room
  // 8   --> shutter in dressing room
  // 4   --> shutter in bedroom
  // 2   --> shutter in child's room (Julia)
  // 1   --> shutter in child's room (Anna)
  
  String selecedShuttersLine;

  uint8_t currentShutter = 128;
  uint8_t minShutter = 16;
  if (isDownstairs == TRUE) {
    currentShutter = 8;
    minShutter = 1;
  }
  Serial.print("----> selectedShutters : ");
  Serial.println(selectedShutters);
  Serial.println("---------------");
  
  while (currentShutter >= minShutter) {
    if ((selectedShutters & currentShutter) > 0) {
      selecedShuttersLine += "[X] ";
    } else {
      selecedShuttersLine += "[ ] ";
    }
    currentShutter /= 2;
  }
  char buffer[16];
  strncpy(buffer, selecedShuttersLine.c_str(), 15);
  buffer[15] = '\0';

  myDisplay.setCursor(0,4);
  myDisplay.println(buffer);
}

void displaySelection() {
  String spaces;

  for (int x = 0; x < 16; x++) {
    if (x != ((shutterSelection*3) + shutterSelection + 1)) {
      spaces += ' ';
    } else {
      spaces += '^';
    }
  }
  myDisplay.setCursor(0,5);
  myDisplay.println(spaces);
}

void displayMainScreen() {
  myDisplay.setFont(u8x8_font_victoriabold8_r);
  myDisplay.setCursor(0,0);
  displayLevelInformation();
  displaySelectedShutters();
  displaySelection();
  myDisplay.println("_______________");
  myDisplay.setFont(u8x8_font_victoriamedium8_r);
  myDisplay.println();
  myDisplay.println("ShutterCtl V0.1");
  myDisplay.println();
  myDisplay.println("IP: ");
  myDisplay.println(ipAddress.toString());
  myDisplay.println();
  myDisplay.println("MQTT:");
  myDisplay.println(mqtt_server);
  myDisplay.setFont(u8x8_font_victoriabold8_r);
  myDisplay.display();

  displayCleared = FALSE;
}

void setup_display() {
  myDisplay.setBusClock(200000);
  myDisplay.begin();
  myDisplay.setPowerSave(0);
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  setup_MQTT();
  setup_Buttons();
  // setup_LEDs();
  setup_display();
  displayMainScreen();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("connected");
      // Subscribe
      // mqttClient.setCallback(callback);
      // mqttClient.subscribe(MQTT_SUBSCRIBE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void changeLevel() {
  if ( isDownstairs == TRUE ) {
    isDownstairs = FALSE;
  } else {
    isDownstairs = TRUE;
  }
}

void loop() {
  // digitalRead function stores the Push button state 
  uint8_t button_state_next = digitalRead(BUTTON_PIN_NEXT);
  uint8_t button_state_select = digitalRead(BUTTON_PIN_SELECT);
  uint8_t button_state_up = digitalRead(BUTTON_PIN_UP);
  uint8_t button_state_down = digitalRead(BUTTON_PIN_DOWN);
  uint8_t button_state_switch = digitalRead(BUTTON_PIN_SWITCH);
  
  long now = millis();
  if (now - lastUsage > 250 && 
      (button_state_next == HIGH || button_state_select == HIGH || 
        button_state_up == HIGH || button_state_down == HIGH || button_state_switch == HIGH)) {
    lastUsage = now;
    if (displayCleared == TRUE) {
      setup_display();
      displayMainScreen();
    } else {
      if (!mqttClient.connected()) {
        reconnect();
      }
      mqttClient.loop();
    
      if ( button_state_next == HIGH ) {
        shutterSelection = (shutterSelection + 1) % 4;
        displaySelection();
        Serial.print("Rollo ");
        Serial.println(shutterSelection);
        Serial.print("Auswahl ");
        Serial.println(selectedShutters);
        Serial.println("---------------");
      } else if ( button_state_select == HIGH ) {
        uint8_t z = 7;
        if (isDownstairs == TRUE) {
          z = 3;
        }
        uint8_t encodedSelection = pow(2, z - shutterSelection);
        selectedShutters = selectedShutters ^ encodedSelection;
        displaySelectedShutters();
      } else if ( button_state_up == HIGH ) {
        Serial.print("Rollo up: ");
        Serial.print(shutterSelection);
        Serial.println("---------------");
        moveShutter(UP);
      } else if ( button_state_down == HIGH ) {
        Serial.print("Rollo down: ");
        Serial.println(shutterSelection);
        Serial.println("---------------");
        moveShutter(DOWN);
      } else if ( button_state_switch == HIGH ) {
        Serial.println("Wechsel unten <-> oben");
        Serial.println("---------------");
        changeLevel();
        displayMainScreen();
      }
    }
  } else if (now - lastUsage > MAX_DISPLAY_TIME) {
    // clear display
    myDisplay.clear();
    displayCleared = TRUE;
  }
}