#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "ThingSpeak.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"
#include <EEPROM.h>
#include "Secret.h"

#define I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;
#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;
WiFiClient  client;

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN D4     // what digital pin we're connected to
DHT dht(DHTPIN, DHTTYPE);

long debouncing_time = 15; //Debouncing Time in Milliseconds
volatile int state = HIGH;
volatile unsigned long last_micros;
const byte interruptPin = D7;

float humidity, temperature;
float tempOffSet = 0;
const int UPDATE_INTERVAL_MINUTES = 60*5;

void setup() {
  Serial.begin(115200);
  dht.begin();

  Wire.begin();         
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.set400kHz();  
  oled.setFont(Adafruit5x7);  

  oled.clear(); 
  oled.set2X(); 
  oled.println("DHT22 \nLogger!");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("WiFi Connect..");
    oled.println("WiFi Connect..");
  }
 
  ThingSpeak.begin(client);
  attachInterrupt(digitalPinToInterrupt(interruptPin), Interrupt, FALLING);
 
}

void DisplayReadings(){
   oled.clear();
   oled.set2X();
   oled.print("Temp: ");
   oled.println(temperature - tempOffSet ,1);
   oled.print("Huni: ");
   oled.println(humidity,1);
   oled.set1X();
   //oled.print("Raw Temp"); oled.println(temperature ,1);  
   //oled.print("tempOffSet: "); oled.println(tempOffSet ,1);
   //oled.println(WiFi.localIP());
}

void Interrupt() {
  
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    state = !state;
    if(state == HIGH){
      DisplayReadings();
    }
    else{
      oled.clear();
    }
    last_micros = micros();
  }
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  if(state == HIGH){
    DisplayReadings();
  }
  
  float hic = dht.computeHeatIndex(temperature, humidity, false);

  Serial.print("Humidity: ");
  Serial.print(humidity,1);
  Serial.print("% ");
  Serial.print("Temperature: ");
  Serial.print(temperature,1);
  Serial.print(" *C ");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.println(" *C ");
  Serial.print("State: ");
  Serial.println(state);
  Serial.println();
  
  ThingSpeak.setField(1,temperature);
  ThingSpeak.setField(2,humidity);
  ThingSpeak.writeFields(THINKSPEAK_CHANNEL, THINGSPEAK_API_KEY);
  
  delay(1000 * UPDATE_INTERVAL_MINUTES); 
}
