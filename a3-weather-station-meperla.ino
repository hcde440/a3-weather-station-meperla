//A3-weather-station-meperla

#include <ESP8266WiFi.h>    //Requisite Libraries . . .
#include "Wire.h"           //
#include <PubSubClient.h>   //
#include <ArduinoJson.h>    //
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_MPL115A2.h>  //include for MPL115A2
//included for ssd1306 128x32 i2c
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

Adafruit_MPL115A2 mpl115a2;

#define wifi_ssid "Half-G Guest"   
#define wifi_password "BeOurGuest"  

#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

WiFiClient espClient;             //espClient
PubSubClient mqtt(espClient);     //tie PubSub (mqtt) client to WiFi client

char mac[6];
char message[201];
unsigned long currentMillis, timerOne, timerTwo, timerThree; 

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

// pin connected to DH22 data line
#define DATA_PIN 12

// create DHT22 instance
DHT_Unified dht(DATA_PIN, DHT22);

void setup() {
  // start the serial connection
  Serial.begin(115200);

  //setup for the 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initailize with the i2c addre 0x3C
  display.clearDisplay();                    //Clears any existing images
  display.setTextSize(1);                    //Set text size
  display.setTextColor(WHITE);               //Set text color to white
  display.setCursor(0,0);                    //Puts cursor back on top left corner
  display.println("Starting up...");         //Test and write up
  display.display();                         //Displaying the display

  // initialize dht22
  dht.begin();

  // initializing MPL115A2
  mpl115a2.begin();
  
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback); //register the callback function
  timerOne = timerTwo = timerThree = millis();

}

/////SETUP_WIFI/////
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
}                                     //5C:CF:7F:F0:B0:C1 for example

/////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("maira/+"); //we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'

  /////
  //This demo uses sprintf, which is very similar to printf,
  //read more here: https://en.wikipedia.org/wiki/Printf_format_string
  /////

  //Here we just send a regular c-string which is not formatted JSON, or json-ified.
  if (millis() - timerOne > 10000) {
    //Here we would read a sensor, perhaps, storing the value in a temporary variable
    //For this example, I will make something up . . .
    String legoBatmanIronyLevel = "Boil 'em, mash 'em, stick 'em in a stew";
    sprintf(message, "{\"po-ta-to\":\"%s%\"}", legoBatmanIronyLevel.c_str()); // %d is used for an int
    mqtt.publish("maria/LBIL", message);
    timerOne = millis();
  }

  //Here we will deal with a JSON string
  if (millis() - timerTwo > 15000) {
    //Here we would read a sensor, perhaps, storing the value in a temporary variable
    //For this example, I will make something up . . .

  sensors_event_t event;
  dht.temperature().getEvent(&event);

  float celsius = event.temperature;
  float fahrenheit = (celsius * 1.8) + 32;
  float pressureKPA = 0;

  Serial.print("celsius: ");
  Serial.print(celsius);
  Serial.println("C");

  Serial.print("fahrenheit: ");
  Serial.print(fahrenheit);
  Serial.println("F");

  dht.humidity().getEvent(&event);

  Serial.print("humidity: ");
  Serial.print(event.relative_humidity);
  Serial.println("%");
  Serial.println();
  
  pressureKPA = mpl115a2.getPressure();  
  Serial.print("Pressure (kPa): "); 
  Serial.print(pressureKPA, 4); 
  Serial.println(" kPa");
  Serial.println();

    // wait 5 seconds (5000 milliseconds == 5 seconds)
  delay(5000);

  //clear display
  display.clearDisplay();

  //display temperature
  display.setCursor(0,0); 
  display.print("Temperature: ");
  display.println(fahrenheit);
  display.println(" F");
  //display humidity
  display.print("Humidity: ");
  display.print(event.relative_humidity);
  display.println(" %");
  //display pressure
  display.print("Pressure: ");
  display.print(pressureKPA);
  display.print(" kPa");
  display.display();
  delay(2000);

  display.clearDisplay();

    /////
    //Unfortunately, sprintf (under Arduino) does not like floats  (bug!), so we can not
    //use floats in the sprintf function. Further, we need to send the temp and humidity as
    //a c-string (char array) because we want to format this message as JSON, for use it with NODE-Red, perhaps . . .
    //
    //We need to make these floats into c-strings via the function dtostrf(FLOAT,WIDTH,PRECISION,BUFFER).
    //To go from the float 3.14159 to a c-string "3.14" you would put in the FLOAT, the WIDTH or size of the
    //c-string (how many chars will it take up), the decimal PRECISION you want (how many decimal places, and
    //the name of a little BUFFER we can stick the new c-string into for a brief time. . .
    /////

    char str_temp[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    char str_hum[6]; //a hum array of size 6 to hold "XX.XX" + the terminating character
    char str_kPa[6]; 
    //take temp, format it into 5 char array with a decimal precision of 2, and store it in str_temp
    dtostrf(fahrenheit, 5, 2, str_temp);
    //ditto
    dtostrf(event.relative_humidity, 5, 2, str_hum);
    //ditto
    dtostrf(pressureKPA, 5, 2, str_kPa);

    /////
    //For proper JSON, we need the "name":"value" pair to be in quotes, so we use internal quotes
    //in the string, which we tell the compiler to ignore by escaping the inner quotes with the '/' character
    /////

    sprintf(message, "{\"temp\":\"%s\", \"hum\":\"%s\", \"kPa\":\"%s\"}", str_temp, str_hum, str_kPa);
    mqtt.publish("maria/tempHum", message);
    timerTwo = millis();
  }
  
}//end Loop

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  /////
  //We can use strcmp() -- string compare -- to check the incoming topic in case we need to do something
  //special based upon the incoming topic, like move a servo or turn on a light . . .
  //strcmp(firstString, secondString) == 0 <-- '0' means NO differences, they are ==
  /////


  else if (strcmp(topic, "maria/tempHum") == 0) {
    Serial.println("Some weather info has arrived . . .");
  }

  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out
}

