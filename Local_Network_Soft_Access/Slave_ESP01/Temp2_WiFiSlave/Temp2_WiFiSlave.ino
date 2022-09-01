#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <DHT.h>


#define DHTTYPE DHT11
#define DHTPIN 2


//IP configuration for Slave Temp2
IPAddress local_IP(192, 168, 178, 241);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);
unsigned int localUdpPort = 4211;           //local port to listen on

//IP configugation for Master
IPAddress remote_IP(192, 168, 178, 240);    //remote IP
unsigned int remote_Port = 4210;            //remote port

//WLAN to be connected
char ssid[] = "ZuHauseE";               // WIFI network name
char pass[] = "Plsletmeineo1";          // WIFI passwort

//Global Variable Declaration
float humidity,temp_f;                    //values read from sensor DHT11
unsigned long previousMillis = 0;          //use to store last temp was read
const long intervalle = 15000;             //interval between two measurement (15000 = 15s)

//Class Initialisation
WiFiUDP Udp;                               //Initialise Udp 
DHT dht(DHTPIN, DHTTYPE);                 //Initialise DHT Sensor (11 workss fine for ESP8266

//Set Up
void setup() {
  delay(5000);
  WiFi.config(local_IP, gateway, subnet); //Set local IP adreess
  WiFi.begin(ssid,pass);                 // Start WLAN 
  while (WiFi.status()!= WL_CONNECTED){
    delay(500);
    }
  Udp.begin(localUdpPort);              //initialise Udp port
  dht.begin();                          //initialize sensor
}

//Main loop
void loop() {
  int lenMeasurement;
  unsigned long currentMillis;
  char Measurement[50];
  String MeasurementString="";              //Measurement String Temperature + Humidity

  currentMillis = millis();                  //get current time
  
  if (currentMillis - previousMillis >= intervalle){
     previousMillis = currentMillis;    //save the last time at sensor reading
     humidity = dht.readHumidity();     //Read humidity as percent
     temp_f = dht.readTemperature(false);    //Read temperature as true=Fahrenheit and false=Celcius
     if (isnan(humidity)||isnan(temp_f)){
        temp_f = 0;
        humidity = 0; 
      }
      MeasurementString = "Val2 " + String(temp_f, 1) + " " + String(int(humidity)) + "EE";  //'E' is the terminator for the string.
      lenMeasurement = MeasurementString.length();        
      MeasurementString.toCharArray(Measurement, lenMeasurement);
      Measurement[lenMeasurement]=0; 
      Udp.beginPacket(remote_IP, remote_Port);
      Udp.write(Measurement);
      Udp.endPacket();
      }
    }
