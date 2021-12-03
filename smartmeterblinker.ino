/*
This is a rudimentary piece of code to read the "1000 blinks per KWh" LED on smart energy meters in europe.
It simply counts a change of brightness on a light dependent resistor and does some simple math to calculate 
the actual energy usage in watts.
The basic code is simulated in tinkercad just to get the calculations right in the first place
The meter blinks 1000 times for every KWh => 1000b/KWh
1000b/KWh = 1000b/1000Wh = 1b/1Wh = 16,666b/KWm = 0,2777b/KWs
TO-DO
-add MQTT functionality
TO-DO later stages
-create light-tight housing with ring magnet for mounting onto the smart meter
-
*/

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


//uncomment to enable debug messages to serial console
//#define DEBUG


const int ldrPin = A0;
long int ldrCount = 0;
int ldrStatus = 0;
int ldrStatusLast = 0;
float watt = 0;
unsigned long lastMillis = 0;



WiFiManager wm;

void setup() {
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    // put your setup code here, to run once:
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(ldrPin, INPUT);
    #ifdef DEBUG
    Serial.println("debug messages enabled");
    #endif
    
    //reset settings - wipe credentials for testing
    //wm.resetSettings();

    wm.setConfigPortalBlocking(false);

    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("AutoConnectAP")){
        Serial.println("connected...yeey :)");
    }
    else {
        Serial.println("Configportal running");
    }


  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("esp_ampmeter");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
void loop() {
    wm.process();
    // put your main code here, to run repeatedly:
    ArduinoOTA.handle();
   
    
    ldrStatus = analogRead(ldrPin);
  if (ldrStatus != ldrStatusLast)
    {
      #ifdef DEBUG
      Serial.println("status changed");
      Serial.println(ldrStatus);
      #endif
      if ( (ldrStatus + 10) <= ldrStatusLast)
      {
        #ifdef DEBUG
        Serial.println("LED was on");
        #endif
        ldrCount++;
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
      Serial.println(ldrCount);
      watt = ldrCount / 0.1666;
      Serial.println("");
      Serial.print(watt);
      Serial.print(" W");
      Serial.println("");
      ldrCount = 0;

      //reset device every 46 days to prevent millis() from overflowing a unsigned long (happens at 49 days)
      if ( lastMillis >= 4000000000 )
      {
      ESP.restart();
      }
    }
}
