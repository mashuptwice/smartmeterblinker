/*
This is a rudimentary piece of code to read the "1000 blinks per KWh" LED on smart energy meters in europe.
It simply counts a change of brightness on a light dependent resistor and does some simple math to calculate 
the actual energy usage in watts.

The meter blinks 1000 times for every KWh => 1000b/KWh
1000b/KWh = 1000b/1000Wh = 1b/1Wh = 16,666b/KWm = 0,2777b/KWs = 0,01666b/Wm

TO-DO 
-rewrite using a different MQTT library
-change to measure the time between blink pulses to get instant measurements instead of an average over 60 seconds. This involves changing the hardware to use a digital pin and a schmitt trigger on the LDR
*/

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoMqttClient.h>

//uncomment to enable debug messages to serial console
//#define DEBUG


const int ldrPin = A0;
long int ldrCount = 0;
int ldrStatus = 0;
int ldrStatusLast = 0;
float watt = 0;
unsigned long lastMillis = 0;
unsigned long lastPoll = 0;

//mqtt configuration
const char broker[] = "debian-vm";
int        port     = 1883;
const char topic[]  = "wattmeter";

WiFiManager wm;
WiFiClient wc;
MqttClient mqttClient(wc);


void setup() 
{
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    // put your setup code here, to run once:
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(ldrPin, INPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    
    #ifdef DEBUG
    Serial.println("debug messages enabled");
    #endif
    
    //reset settings - wipe credentials for testing
    //wm.resetSettings();

    wm.setConfigPortalBlocking(false);

    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("AutoConnectAP"))
    {
        Serial.println("connected...yeey :)");
    }
    else 
    {
        Serial.println("Configportal running");
    }

    if (!mqttClient.connect(broker, port)) 
    {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    //while (1);
    }
    else
    {
      Serial.println("mqtt connection successful!");  
    }
    
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN, LOW);
     
}
void loop() 
{
     
    wm.process();   
    
  //do the measurement magic  
  ldrStatus = analogRead(ldrPin);
  if (ldrStatus != ldrStatusLast)
    {
      #ifdef DEBUG
      Serial.println("status changed");
      Serial.println(ldrStatus);
      #endif
      //hysteresis
      if ( (ldrStatus + 10) <= ldrStatusLast)
      {
        #ifdef DEBUG
        Serial.println("LED was on");
        #endif
        ldrCount++;
      }
      ldrStatusLast = ldrStatus;
    }
  //poll to avoid being disconnected
      if (millis() - lastPoll > 5000)
      {
      mqttClient.poll();
      lastPoll = millis();
      
      mqttClient.beginMessage("status");
      mqttClient.print("wattmeter is polling");
      mqttClient.endMessage();
      
      #ifdef DEBUG
      Serial.println("poll");
      #endif
      
      }
  //triggers every minute
    if (millis() - lastMillis > 60000)
    {
      #ifdef DEBUG
      Serial.println("another minute");
      #endif
      lastMillis = millis();
     
      watt = ldrCount / 0.01666;
      ldrCount = 0;
      
      #ifdef DEBUG
      Serial.println(ldrCount);
      Serial.println("");
      Serial.print(watt);
      Serial.print(" W");
      Serial.println("");
      #endif

      //publish measurement via mqtt
      mqttClient.beginMessage(topic);
      mqttClient.print(watt);
      mqttClient.endMessage();
      
      
      
      //reset device every 46 days to prevent millis() from overflowing a unsigned long (happens at 49 days)
      if ( lastMillis >= 4000000000 )
      {
      ESP.restart();
      }
    }
     //something prevents MQTT messages being sent from within if loops, if this part is not in the main loop
     mqttClient.beginMessage(topic);
     mqttClient.endMessage();
    
    //restart if connection to mqtt broker is lost
     if (!mqttClient.connect(broker, port)) 
    {
      ESP.restart();
    
    }
}
