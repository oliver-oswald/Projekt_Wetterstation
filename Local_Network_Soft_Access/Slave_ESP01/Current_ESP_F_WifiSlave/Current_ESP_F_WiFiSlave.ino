#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EmonLib.h>  //energy monitor Lib

//IP configuration for Slave Current
IPAddress local_IP(192, 168, 178, 243);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);
unsigned int localUdpPort = 4213;           //local port to listen on

//IP configugation for Master
IPAddress remote_IP(192, 168, 178, 240);    //remote IP
unsigned int remote_Port = 4210;            //remote port

//WLAN to be connected
char ssid[] = "ZuHauseE";               // WIFI network name
char pass[] = "Plsletmeineo1";          // WIFI passwort

//Global Variable Declaration
unsigned long currentMillis;
unsigned long previousMillis = 0;          //use to store last current was read
const long intervalle = 30000;             //interval between two measurements (30000 = 30s)
unsigned long WLANWaitingCounter = 0;      //counter for WLAN waiting time out. (5 min with 500ms cycle counter max is 600),  
const int CurrentAnalogInput = A0;         //A0 is used as analog input for current measurement
const int RelayOutput = 5;                 //D1 [GPIO5] is used as digital output for the relay
float CurrentRms = 0;                     //Max current
float CurrentThreshold_OFF_ON = 3;        //current limit for relay ON switching (OFF before)
float CurrentThreshold_ON_OFF = 1;        //current limit for relay OFF switching (ON Before). Current Threshold ON to OFF is 33% of Current OFF to ON. (Histeris)
int RelayStatus = 0;                       // 0 = Relay is OFF; 1 = Relay is ON       

//Variable for current filtering [moving average]
int sample = 0;                            //sample counter
const int number_sample = 20;              //number of sample to be averaged
float Current_history[number_sample];      //current buffer
int count = 0;                             //counter for current averaging
float average =0;                          // used for average

//Variable for message decryption
int indexType = 0;
String MessageType;
String CurrentSetting;

//WifiUDP Class Initialisation
WiFiUDP Udp;                                   //Initialise Udp 

//EnergyMonitor Class Initialisation
EnergyMonitor emon1;                           //Initialise emon1 (Energy monitoring)

//Set Up
void setup() {
  //************DEBUG*********************
  //Serial.begin(9600);
  //**************************************
  delay(5000);  
  WiFi.config(local_IP, gateway, subnet); //Set local IP adreess
  WiFi.begin(ssid,pass);                 // Start WLAN 
  while ((WiFi.status()!= WL_CONNECTED)&&(WLANWaitingCounter < 600)){
    delay(500);
    WLANWaitingCounter = WLANWaitingCounter + 1;           //increment counter by 1 until 600 reached = 5 min (timeout for WLAN connecton)
    //********DEBUG************************
    //Serial.println("*");
    //*************************************
    }
  Udp.begin(localUdpPort);              //initialize Udp port
  pinMode(RelayOutput, OUTPUT);         //initialize Relay Output as an output and at low level.
  digitalWrite(RelayOutput, LOW); 
  RelayStatus = 0;                      //Relay is Off
  emon1.current(CurrentAnalogInput,15); //Current:Input pin, calibration factor
  for(count=0; count<number_sample ; count++){Current_history[count] = 0;} // Initialize current buffer at 0.
}

//Main loop
void loop() {
  int lenMeasurement;
  char Measurement[50];
  String MeasurementString="";                //Measurement String Current and Relay Status
  int packetSize, lenPacketBuffer;
  int i=0;
  char packetBuffer[255];
  String receivedMessageFromMaster="";
  
  currentMillis = millis();                   //get current time
  
  if (currentMillis - previousMillis >= intervalle){
      previousMillis = currentMillis;         //save the last time at sensor reading
      CurrentRms = emon1.calcIrms(512);       //calculate max current out of 512 sample
      Current_history[sample] = CurrentRms;
      sample = sample + 1;
      if(sample == (number_sample-1)){sample = 0;}
      average = 0;
      for(count=0; count<number_sample ; count++){
      average = average + Current_history[count]/number_sample;
      }
      CurrentRms = average;
      if (RelayStatus == 0){                            //If relay is OFF
        if (CurrentRms > CurrentThreshold_OFF_ON){      //switch relay ON if current above threshold OFF to ON
            digitalWrite(RelayOutput, HIGH);
            RelayStatus = 1;                            //Change relay status to ON
        }
        else {
            digitalWrite(RelayOutput, LOW);             //Keep Relay OFF
            RelayStatus = 0;    
        }
      }
      else {                                            //If relay is ON
       if (CurrentRms < CurrentThreshold_ON_OFF){       //switch relay OFF if current is below threshold ON to OFF
            digitalWrite(RelayOutput, LOW);
            RelayStatus = 0;                            //Change relay status to OFF
       }
       else {
            digitalWrite(RelayOutput, HIGH);            //keep relay ON
            RelayStatus = 1;                            
        }
      }
      if (RelayStatus == 1) {
        MeasurementString = "Val4 " + String(CurrentRms, 1) + " 100" +  "EE";  //'E' is the terminator for the string. 100 means Relay is ON
        }
        else {
        MeasurementString = "Val4 " + String(CurrentRms, 1) + " 000" +  "EE";  //'E' is the terminator for the string. 000 means Relay is OFF  
        }
        
      //*************************************************************
      //***********************DEBUG*********************************
      //Serial.println(MeasurementString);
      //Serial.println(CurrentRms);
      //*************************************************************
      //Sent new measurement via WLAN
      lenMeasurement = MeasurementString.length();        
      MeasurementString.toCharArray(Measurement, lenMeasurement);
      Measurement[lenMeasurement]=0; 
      Udp.beginPacket(remote_IP, remote_Port);
      Udp.write(Measurement);
      Udp.endPacket();
      }
  //Check WLAN communication
    packetSize = Udp.parsePacket(); //Listening
    if(packetSize){
        lenPacketBuffer = Udp.read(packetBuffer, 255);
        //If message received extract information from String "Val4I current"
        while ((packetBuffer[i] != 'E')||(i<lenPacketBuffer+1)){
            receivedMessageFromMaster += packetBuffer[i];
            i=i+1;
        }
        indexType = receivedMessageFromMaster.indexOf(' ');
        MessageType = receivedMessageFromMaster.substring(0,indexType);
        if (MessageType == "Val4"){
          CurrentSetting = receivedMessageFromMaster.substring(indexType+1,receivedMessageFromMaster.length());
          CurrentThreshold_OFF_ON = CurrentSetting.toFloat();
          CurrentThreshold_ON_OFF = CurrentThreshold_OFF_ON/3; //Current  Threshold ON to OFF is 33% of Current OFF to ON. (Histeris)
        }
    }
   }
