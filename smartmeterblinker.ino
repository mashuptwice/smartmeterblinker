/*
  This is a small piece of code to read the "1000 blinks per KWh" LED on smart energy meters in europe.
  It simply counts a change of brightness on a light dependent resistor and does some simple math to calculate
  the actual energy usage in watts.
  The meter blinks 1000 times for every KWh => 1000b/KWh
  1000b/KWh = 1000b/1000Wh = 1b/1Wh = 16,666b/KWm = 0,2777b/KWs = 0,01666b/Wm
  
  TO-DO
  -(MAYBE)change to measure the time between blink pulses to get instant measurements instead of an average over 60 seconds. This involves changing the hardware to use a digital pin and a schmitt trigger on the LDR
  -add real debounce to light sensor
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager



// Update these with values suitable for your network.

const char* mqtt_server = "192.168.1.50";
const char* mqtt_user = "user";
const char* mqtt_passwd = "passwd";




//uncomment to enable debug messages to serial console
//#define DEBUG

//pin and variable setup
const int ldrPin = A0;
long int ldrCount = 0;
int ldrStatus = 0;
int ldrStatusLast = 0;
float watt = 0;
char wattstr[8];
unsigned long lastMillis = 0;
unsigned long lastPoll = 0;


WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");

  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");


  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 2 seconds before retrying
      delay(2000);

      //reset counters
      lastMillis = millis();
      ldrCount = 0;
    }
  }
}

void setup() {
  Serial.begin(115200);
  #ifdef DEBUG
  Serial.println("debug messages enabled");
  #endif

  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(ldrPin, INPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  #ifdef DEBUG
  Serial.println("begin wifi setup");
  #endif
  setup_wifi();
  #ifdef DEBUG
  Serial.println("begin mqtt setup");
  #endif
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  #ifdef DEBUG
  Serial.println("setup() done");
  #endif

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();


//read the light intensity from LDR and count up if blinking is detected
  ldrStatus = analogRead(ldrPin);
  if (ldrStatus != ldrStatusLast)
  {
    /*
    //just too chatty for debugging
    #ifdef DEBUG
    Serial.println("status changed");
    Serial.println(ldrStatus); 
    #endif
    */
    
    //small hysteresis to prevent ambient light triggering the counter
    if ( (ldrStatus + 100) <= ldrStatusLast)
    {
      ldrCount++;

      #ifdef DEBUG
      Serial.println("LED was on");
      Serial.println(ldrCount);
      #endif
      //"debounce"
      delay(100);
      //TO DO: add real debounce here
    }
    ldrStatusLast = ldrStatus;
  }
  
  //triggers every minute
  if (millis() - lastMillis > 60000)
  {
    #ifdef DEBUG
    Serial.println("another minute");
    #endif
    
    lastMillis = millis();
    //calculate watt from blink impulses over 60s time period
    watt = ldrCount / 0.0166666;
    //convert to char for MQTT
    itoa(watt,wattstr,10);
    #ifdef DEBUG
    Serial.println(ldrCount);
    Serial.println("");
    Serial.print(watt);
    Serial.print(" W");
    Serial.println("");
    Serial.println("String");
    Serial.println("");
    Serial.print(wattstr);
    Serial.print(" W");
    Serial.println("");
    #endif    
    //reset counter
    ldrCount = 0;

    //publish measurement via mqtt
    client.publish("wattmeter", wattstr);

    //reset device every 46 days to prevent millis() from overflowing a unsigned long (happens at 49 days)
    if ( lastMillis >= 4000000000 )
    {
      ESP.restart();
    }
  }


  //adding delay for pubsubclient
  delay(50);
}
