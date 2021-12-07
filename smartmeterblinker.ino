/*
  This is a rudimentary piece of code to read the "1000 blinks per KWh" LED on smart energy meters in europe.
  It simply counts a change of brightness on a light dependent resistor and does some simple math to calculate
  the actual energy usage in watts.
  The meter blinks 1000 times for every KWh => 1000b/KWh
  1000b/KWh = 1000b/1000Wh = 1b/1Wh = 16,666b/KWm = 0,2777b/KWs = 0,01666b/Wm
  
  TO-DO
  -(MAYBE)change to measure the time between blink pulses to get instant measurements instead of an average over 60 seconds. This involves changing the hardware to use a digital pin and a schmitt trigger on the LDR
  -add real debounce to light sensor
*/

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>

WiFiManager wm;
WiFiClient wc;
PubSubClient client(wc);

//uncomment to enable debug messages to serial console
//#define DEBUG

const int ldrPin = A0;
long int ldrCount = 0;
int ldrStatus = 0;
int ldrStatusLast = 0;
float watt = 0;
char wattstr[8];
unsigned long lastMillis = 0;
unsigned long lastPoll = 0;

//mqtt configuration
//set MQTT server IP here
IPAddress server(192,168,20,50);
//set MQTT credentials here
const char* user = "user";
const char* password = "pass";

//reconnect function in case connection to server is lost
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient", user, password)) {
      Serial.println("connected");
      //set LED to on
      digitalWrite(LED_BUILTIN, LOW);
      // Once connected, publish an announcement...
      client.publish("wattmeter/status","connected");
      // ... and resubscribe
      //client.subscribe("inTopic");
     
      //reset counters
      lastMillis = millis();
      ldrCount = 0;
    
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      
      //set LED to off
      digitalWrite(LED_BUILTIN, HIGH);
      // Wait 3 seconds before retrying
      delay(3000);
     //reset counters
      lastMillis = millis();
      ldrCount = 0;
    }
  }
}
void setup()
{
  Serial.begin(115200);
  #ifdef DEBUG
  Serial.println("debug messages enabled");
  #endif
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ldrPin, INPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Connect to MQTT
  client.setServer(server, 1883);
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  //reset settings - wipe credentials for testing
  //wm.resetSettings();

  wm.setConfigPortalBlocking(false);

  //automatically connect using saved credentials if they exist
  //If connection fails it starts an access point with the specified name
  if (wm.autoConnect("AutoConnectAP"))
  {
    Serial.println("connected to configured WiFi");
  }
  else
  {
    Serial.println("Configportal running");
  }
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //enable onboard LED to signal successful startup
  digitalWrite(LED_BUILTIN, LOW);

}
void loop()
{
  //WiFi config portal handling
  wm.process();

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
  
  //reconnect if MQTT connection is lost
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  //for some reason the delay fixed the MQTT socket errors
  delay(1);
}
