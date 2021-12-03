/*
This is a rudimentary piece of code to read the "1000 blinks per KWh" LED on smart energy meters in europe.
It simply counts a change of brightness on a light dependent resistor and does some simple math to calculate 
the actual energy usage in watts.
The basic code is simulated in tinkercad just to get the calculations right in the first place
The meter blinks 1000 times for every KWh => 1000b/KWh
1000b/KWh = 1000b/1000Wh = 1b/1Wh = 16,666b/KWm = 0,2777b/KWs
TO-DO
-build breadboard prototype to get real life measurements 
-adapt code to ESP8266s ADC
-add MQTT functionality
TO-DO later stages
-create light-tight housing with ring magnet for mounting onto the smart meter
-
*/

//uncomment to enable debug messages to serial console
//#define DEBUG

//uncomment to set measurement interval to one second instead of one minute
//#define TINKERCAD

const int ldrPin = A0;
long int ldrCount = 0;
int ldrStatus = 0;
int ldrStatusLast = 0;
float watt = 0;
unsigned long lastMillis = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ldrPin, INPUT);
  #ifdef DEBUG
  Serial.println("debug messages enabled");
  #endif
}

void loop()
{
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
  
  #ifdef TINKERCAD
   //just for testing as tinkercad runs much slower than realtime
  //triggers every second
  if (millis() - lastMillis > 1000)
    {
      #ifdef DEBUG  
      Serial.println("another second");
      #endif
      lastMillis = millis();
      Serial.println(ldrCount);
      watt = ldrCount / 0.0002777777;
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
  #else   
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
    }
  #endif
}
