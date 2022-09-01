/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


//WLAN to be connected
char ssid[] = "ZuHauseE";               // WIFI network name
char pass[] = "Plsletmeineo1";          // WIFI passwort

//IP configugation for Master
IPAddress remote_IP(192, 168, 178, 240);    //remote IP
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);
unsigned int remote_Port = 4210;            //remote port

//IP configuration for Slave Temp2
IPAddress local_Temp2_IP(192, 168, 178, 241);
unsigned int local_Temp2_UdpPort = 4211;    //local port to listen on

//IP configuration for Slave Temp3
IPAddress local_Temp3_IP(192, 168, 178, 242);
unsigned int local_Temp3_UdpPort = 4212;    //local port to listen on

//IP configuration for Slave Current
IPAddress local_Current_IP(192, 168, 178, 243);
unsigned int local_Current_UdpPort = 4213;    //local port to listen on

//Class Initialisation
WiFiUDP Udp;     

//Messages
String receivedMessageFromDisplay="";  //Information from display via serial link

void setup() {
  delay(1000);
  WiFi.config(remote_IP, gateway, subnet); //Set local IP adreess
  WiFi.begin(ssid,pass);                 // Start WLAN 
  while (WiFi.status()!= WL_CONNECTED){
    delay(500);
    }
  Udp.begin(remote_Port);
  delay(500);
  Serial.begin(115200);
  delay(500);
}
  
void loop() {
  int packetSize=0, lenPacketBuffer=0;
  char packetBuffer[255];
  char receivedFromDisplay;
  //Check WLAN communication if message are available, if yes read and sent via serial link to Display
      packetSize = Udp.parsePacket();  //Listening
      if (packetSize) {
          lenPacketBuffer = Udp.read(packetBuffer, 255); 
          if (lenPacketBuffer>0) {packetBuffer[lenPacketBuffer]=0;}
          Serial.print(packetBuffer);
     }
  //Check Serial communication if message available, if yes read and sent via WLAN to sensor
     while (Serial.available()){
      receivedFromDisplay = Serial.read(); //read char from serial line from Display
      receivedMessageFromDisplay += receivedFromDisplay;
      if (receivedFromDisplay == 'E'){               //E is terminator
        packetSize = receivedMessageFromDisplay.length();
        receivedMessageFromDisplay.toCharArray(packetBuffer, packetSize);
        packetBuffer[packetSize]=0;
        Udp.beginPacket(local_Current_IP, local_Current_UdpPort);
        Udp.write(packetBuffer);       //Sent Message via WLAN to Current Sensor
        Udp.endPacket();
        receivedMessageFromDisplay = "";
      }
     }
   }
