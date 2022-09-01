#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>   // TouchScreen library
#include <DHT.h>           // DHT library

//**************************************************************************************************
// Color definition
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

//***************************************************************************************************
// TFT is connected via analog pin
#define LCD_CS A3                                         // Chip Select goes to Analog 3
#define LCD_CD A2                                         // Command/Data goes to Analog 2
#define LCD_WR A1                                         // LCD Write goes to Analog 1
#define LCD_RD A0                                         // LCD Read goes to Analog 0
#define LCD_RESET A4                                      // LCD Reset to Analog 4
#define BACKLIGHT 52                                      // LCD BACKLIGHT control (External Transistor control by digital I/O 52)
// TFT dimension [with TFT rotation = 1]
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);    //TFT display
//**************************************************************************************************

//****************************************************************************************************
// Touchscreen initialisation (connected via Analog pin)
#define YP A3                                              // must be an analog pin, use "An" notation!
#define XM A2                                              // must be an analog pin, use "An" notation!
#define YM 9                                               // can be a digital pin
#define XP 8                                               // can be a digital pin
#define TCHSCR 13                                          // Touschscren I/O control
// Define min/max pressure for Touchscreen 
#define MINPRESSURE 10
#define MAXPRESSURE 1000
// Touchscreen dimension [ILI9341 TP]
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);         //300 is the resistance between XP [X+] and XM [X-]
//*****************************************************************************************************

//*****************************************************************************************************
// DHT sensor HW interface initialisation (Internal Sensor)
#define DHTTYPE DHT11
#define DHTPIN 22

DHT dht(DHTPIN, DHTTYPE);                     //Temperature sensor inside Master (Typical Inner Sensor)
//*****************************************************************************************************

//*****************************************************************************************************
//Display grid definition
int intervale_i = 106;  //Horizontal Grid
int intervale_j = 48;   //Vertical Grid
int number_line = 5;    //Number of lines in Grid
int number_column = 3;  //Numbe of columns in Grid 
//*****************************************************************************************************

//*****************************************************************************************************
//Task time management
unsigned long previoustime1 = 0;    // Buffer time used for screen update and internal sensor reading
unsigned long previoustime2 = 0;    // Buffer time used for power management 
unsigned long interval_read = 5000; //Cycle for reading internal sensor and updating screen
unsigned long interval_on = 30000;  // Cycle for Power ON time before sleep.
int SLEEP_WAKE = 1;                 //1 = WAKE and 0 = SLEEP
//*****************************************************************************************************

//*****************************************************************************************************
//Weather variables {0 = Innen; 1 = Winter Garten; 2 = Aussen} /Solar initialization  / Menu definition /Divers
String text_line[] = {"INNEN","W.GARTEN","AUSSEN"};
float temp[] =        {20,20,20};      //Initial temperature value   
float humid[] =       {60,60,60};      //Initial humidity value
float tempsens0 = 0;
float tempsens1 = 0;
float tempsens2 = 0;
float humidsens0 = 0;
float humidsens1 = 0;
float humidsens2 = 0;                       //intermediate sensor storage without calibration
float cal_temp[] =    {-12.8,-16.4,-22.1};        //Initial calibration(offset) for temperature
float cal_humid[] =    {27,35,31};          //Initial calibration(offset) for humidity
String header_weather[] = {"TEMP.","HUMID."};
String solar_line[] = {"CURRENT.","LIMIT","ON.TIME"};
float current[] =       {0,3,0};        //Initial current setting. Limit initial value is set to 3.
String header_solar[] = {"CUR.","STAT"};
int solar_stat = 0; // 0 Means relay is OFF; 100 Means releay is ON
float on_time = 0; //load on_time in ms

//Menu Text
String text_menu[] = {"SOLAR","CAL_T","CAL_H","RESET","SAVE"}; 
int MENU = 0; //0=weather datas; 1=humidity calibration; 2=temperature calibration; 3=solar; 
int MENU_PAGE_CHANGE = 0; //1 Menu page should be change and 0 Menu page should not be changed
int CAL_VALUE_CHANGE = 0; //0 no calibration value change and 1 calibration value was changed

//Message Variables
String receivedMessageFromMaster="";  //Information from satellite via serial link (Master ESP)
String sentMessageToMaster="";        //Information to satellite via serial link (Master ESP)
int MESSAGE_COMPLETED = 0; //0 complet message still not received / 1 complet message received same as "E" character.
//**************************************************************************************************

//************************************************
//Function Declaration****************************
//************************************************
void Grid_Menu(){
  int i;  //column
  int j;  //lines
  for (i=0; i<=number_column; i++){
    tft.drawLine(intervale_i*i, 0, intervale_i*i, 240, WHITE); //Vertical lines
  }
  for (j=0; j<=number_line; j++){
    tft.drawLine(0, intervale_j*j, 320, intervale_j*j, WHITE); //Horizontal lines
  }
}

void Weather_Menu(){
  int i;  //column
  int j;  //lines
  tft.fillScreen(BLACK); //erase screen
  Grid_Menu();           //draw grid menu
  //draw text
  tft.setTextColor(YELLOW);  
  tft.setTextSize(2);
  for (i=1; i<number_column; i++){
    tft.setCursor(10+intervale_i*i, 16);
    tft.println(header_weather[i-1]);     //Header Text
  }
  for (j=1; j<number_line-1; j++){
    tft.setCursor(10, 16 + intervale_j*j);
    tft.println(text_line[j-1]);         //Line Text
  }
  tft.setTextColor(RED);
  for (i=0; i<number_column; i++){
    tft.fillRect(10+intervale_i*i, 200, 86, 32, WHITE);
    tft.setCursor(20+intervale_i*i, 208);
    tft.println(text_menu[i]);           //Menu Text
  }
}

void Weather_Value(){
  int i;  //column
  int j;  //lines
  tft.setTextColor(WHITE);  
  tft.setTextSize(2);
  for (j=1; j<number_line-1; j++){
    tft.fillRect(107, 1+intervale_j*j, 100, 45, BLACK); //Erase previous value
    tft.setCursor(116, 16+intervale_j*j);
    tft.println(temp[j-1],1);            //Write temperature with one decimal
    tft.fillRect(213, 1+intervale_j*j, 100, 45, BLACK); //Erase previous value
    tft.setCursor(222, 16+intervale_j*j);
    tft.println(humid[j-1],0);          //Write humidity withoutdecimal
  }
}

void Solar_Menu(){  
  int i;  //column
  int j;  //lines
  tft.fillScreen(BLACK); //erase screen
  Grid_Menu();           //draw grid menu
  //draw text
  tft.setTextColor(YELLOW); 
  tft.setTextSize(2);
  for (i=1; i<number_column; i++){
    tft.setCursor(10+intervale_i*i, 16);
    tft.println(header_solar[i-1]);    //Header Text
    }
  for (j=1; j<number_line-1; j++){
    tft.setCursor(10, 16 + intervale_j*j);
    tft.println(solar_line[j-1]);      //Line Text
    }
  tft.setTextColor(RED);
  for (i=1; i<number_column; i++){
    tft.fillRect(10+intervale_i*i, 200, 86, 32, WHITE);
    tft.setCursor(20+intervale_i*i, 208);
    tft.println(text_menu[i+2]);         //Menu Text [index 3 = RESET, index 4 = SAVE] 
    }
   // draw "+" and "-" button
  tft.setTextSize(3);
  tft.fillRect(234, 13+intervale_j*2, 21, 21, WHITE);  //Highlight for "+" on line 3
  tft.setCursor(237, 14+intervale_j*2);
  tft.println("+");                         //"+"
  tft.fillRect(276, 13+intervale_j*2, 21, 21, WHITE);  //Highlight for "-" on line 3
  tft.setCursor(279, 14+intervale_j*2);
  tft.println("-");                         //"-"
}

void Solar_Value(){
  int i;  //column
  int j;  //lines
  tft.setTextColor(WHITE);  
  tft.setTextSize(2);
  for (j=1; j<number_line-1; j++){
    tft.fillRect(107, 1+intervale_j*j, 100, 45, BLACK); //Erase previous value
    tft.setCursor(116, 16+intervale_j*j);
    tft.println(current[j-1],1);                       //Write current parameter (actual value, limit, ON time with one decimal
  }
  tft.fillRect(213, 49, 100, 45, BLACK);               //Erase previous value Relay Status
  tft.setCursor(222, 64);
  if (solar_stat == 0) {                               //Write actual Relay Status
    tft.println("OFF");
  }
  else
  {
    tft.println("ON");
  }
}

void Cal_Menu(String cal_text){  
  int i;  //column
  int j;  //lines
  tft.fillScreen(BLACK); //erase screen
  Grid_Menu();           //draw grid menu
  //draw text
  tft.setTextColor(YELLOW);  
  tft.setTextSize(2);
  tft.setCursor(116, 16);
  tft.println(cal_text);     //Header Text "TEMP." or "HUMID."
  for (j=1; j<number_line-1; j++){
    tft.setCursor(10, 16+intervale_j*j);
    tft.println(text_line[j-1]);         //Line Text
    }
  tft.setTextColor(RED);
  tft.fillRect(222, 200, 86, 32, WHITE);
  tft.setCursor(232, 208);
  tft.println(text_menu[4]);           //Menu Text index 4 = SAVE
  // draw "+" and "-" button
  tft.setTextSize(3);
  for (j=1; j<(number_line-1); j++){
    tft.fillRect(234, 13+intervale_j*j, 21, 21, WHITE);  //Highlight for "+"
    tft.setCursor(237, 14+intervale_j*j);
    tft.println("+");                         //"+"
    tft.fillRect(276, 13+intervale_j*j, 21, 21, WHITE);  //Highlight for "-"
    tft.setCursor(279, 14+intervale_j*j);
    tft.println("-");                         //"-"
    }
  }

void Cal_Value(int cal_menu){                            //cal menu = 2 for temperature and 1 for humidity
  int i;  //column
  int j;  //lines
  tft.setTextColor(WHITE);  
  tft.setTextSize(2);
  for (j=1; j<number_line-1; j++){
    if (cal_menu == 2){                                  //Cal Temperature Menu selected
      tft.fillRect(107, 1+intervale_j*j, 100, 45, BLACK); //Erase previous value
      tft.setCursor(116, 16+intervale_j*j);
      tft.println(temp[j-1],1);            //Write temperature with one decimal
    }
    else{                                                //Cal Humidity Menu selected
    tft.fillRect(107, 1+intervale_j*j, 100, 45, BLACK);   //Erase previous value
    tft.setCursor(116, 16+intervale_j*j);
    tft.println(humid[j-1],0);            //Write humidity withoutdecimal
    }
  }
}

//Set Up
void setup() {
  //Intialise Serial link between display and USB for DEBUG Information
  Serial.begin(115200);
  //Initialise serial link between display and Master ESP8266 [TX1, RX1]
  Serial1.begin(115200);
  Serial1.setTimeout(200);
  //Initialise Local Temperature Sensor (indoor temperature)
  dht.begin();
  //Initialise TFT*****************************************
  tft.reset();
  tft.begin(0x9341);   //TFT ID 0x9341
  tft.fillScreen(WHITE);
  tft.setRotation(1);
  delay(1000);
  //Initialise I/O for Touchscreen and Backlight
  pinMode(TCHSCR, OUTPUT);
  pinMode(BACKLIGHT, OUTPUT);
  //Clear Serial buffer to ESP Master [RX1/TX1]************************************
  while(Serial1.available()>0){Serial1.read();}
  //draw first page: weather page
  Weather_Menu();
  Weather_Value();
}

void loop() {
  unsigned long time1=millis();   //get current time in millisecond
  String Sensor;
  String Temperature;
  String Humidity;
  char receivedFromMaster;
  int indexTemperature, indexHumidity;
  int receivedFromMasterLength;
  long touchPosition_Store;
  TSPoint touchPosition;
  int i;  //column
  int j;  //lines 
  int count;          //counter for multiple purpose
  float average = 0; //used for average current calculation
  int packetSize;
  char packetBuffer[255];

//Power Management [Sleep / Wake] of TFT display*****************************
  if ((time1 - previoustime2 >= interval_on)&& SLEEP_WAKE == 1){
    digitalWrite(BACKLIGHT, HIGH); //switch backlight OFF
    SLEEP_WAKE = 0;                // Sleep mode
    tft.sleep();                   //TFT goes into sleep mode
}

//Read external temperature via Serial Link**********************************
  while (Serial1.available()){
      receivedFromMaster = Serial1.read();  //read char from serial line from Master
      //'E' is the terminator
      if (receivedFromMaster == 'E'){
        indexTemperature = receivedMessageFromMaster.indexOf(' ');
        Sensor = receivedMessageFromMaster.substring(0,indexTemperature);
        indexHumidity = receivedMessageFromMaster.indexOf(' ', indexTemperature + 1);
        Temperature = receivedMessageFromMaster.substring(indexTemperature + 1, indexHumidity);
        Humidity = receivedMessageFromMaster.substring(indexHumidity + 1, receivedMessageFromMaster.length());
        //*************************************************************
        //Serial.println("Sensor = " + Sensor);                   //Debug
        //Serial.println("Temperature = " + Temperature +" °C");  //Debug
        //Serial.println("Humidity = " + Humidity + " %");        //Debug
        //*************************************************************
        receivedMessageFromMaster = "";    //Reinitialise receivedMessageFromMaster string after allocation of measurments datas
        MESSAGE_COMPLETED = 1;             //full message received.
      }
      else {
        receivedMessageFromMaster += receivedFromMaster; //Generate the string message received from the Master
      }
  }

//***************************************************************
//Associate temperature, humidity, solar value to the right variable.
// WinterGarten (WGarten is Sensor Nb. 2 (Val2))
// Aussen (aussen is Sensor Nb. 3 (Val3))
// solar (current is Sensor Nb. 4 (Val 4))
//***************************************************************
  if (MESSAGE_COMPLETED == 1){                      //full message received previously
   if (Sensor == "Val2"){
     tempsens1 = Temperature.toFloat();
     humidsens1 = Humidity.toFloat();
     MESSAGE_COMPLETED = 0;                          //wait for next message
   }
   if (Sensor == "Val3"){
     tempsens2 = Temperature.toFloat();
     humidsens2 = Humidity.toFloat();
     MESSAGE_COMPLETED = 0;                          //wait for next message
   }
   if (Sensor == "Val4"){                            //Current message from ESP current
     current[0] = Temperature.toFloat();
     solar_stat = Humidity.toInt();
     MESSAGE_COMPLETED = 0;                         //wait for next message
   }
  }  
  
//Check Touschreen
  digitalWrite(TCHSCR, HIGH);
  touchPosition = ts.getPoint();
  digitalWrite(TCHSCR, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  touchPosition_Store = touchPosition.x;
  touchPosition.x = map(touchPosition.y, TS_MINY, TS_MAXY, TFT_WIDTH, 0);          //for TFT_rotation = 1 map(value, fromLow, fromHigh, toLow, toHigh)
  touchPosition.y = map(touchPosition_Store, TS_MINY, TS_MAXY, TFT_HEIGHT, 0);     //for TFT_rotation = 1 map(value, fromLow, fromHigh, toLow, toHigh)
  if (touchPosition.z > MINPRESSURE && touchPosition.z < MAXPRESSURE) {

      //Check power mode, if sleep then wake up the TFT display.
      if (SLEEP_WAKE == 0){     // If TFT was in Sleep, wake up.
        SLEEP_WAKE = 1;         // Wakeup and switch ON TFT
        tft.wake();             // Wakeup display
        digitalWrite(BACKLIGHT, LOW); //Switch Backlight ON
        }
      previoustime2 = time1; //as TFT was touched initialize power management time to actual time = start again with power on cycle.
            
      //************************************************************************
      //DEBUG information
      //Serial.println(touchPosition.x);
      //Serial.println(touchPosition.y);
      //************************************************************************
                                                                        
        if ((touchPosition.y > 200) && (touchPosition.y < 232)){     //touch in menu zone on page
          switch (MENU){                      //0=weather page; 1=humidity calibration page; 2=temperature calibration page; 3=solar page; 
            case 0:                           //Weather page
              for (i=0 ; i<number_column; i++){
                if ((touchPosition.x >(10+intervale_i*i)) && (touchPosition.x <(96+intervale_i*i))){
                  tft.fillRect(10+intervale_i*i, 200, 86, 32, RED);                     //Change Menu Button color
                  tft.setCursor(20+intervale_i*i,208);
                  tft.setTextColor(WHITE);
                  tft.println(text_menu[i]);            //text_menu[] = {"SOLAR","CAL_T","CAL_H",xxxxxxx};
                  delay(200);
                  MENU = 3-i;                           //0=weather datas; 1=humidity calibration; 2=temperature calibration; 3=solar
                  MENU_PAGE_CHANGE = 1;                 //Menu page should be changed
                }
              }
            break;
            case 1:                         //Humidity calibration page
              if ((touchPosition.x > 222) && (touchPosition.x < 308)){                    // "SAVE" Button 
                tft.fillRect(222, 200, 86, 32, RED);                                      //Change Menu Button color
                tft.setCursor(232, 208);
                tft.setTextColor(WHITE);
                tft.println(text_menu[4]);                                                //text_menu[] = {"SOLAR","CAL_T","CAL_H","RESET", "SAVE"};
                delay(200);
                MENU = 0;                                                                 //Back to Menu weather page
                MENU_PAGE_CHANGE = 1;
              }
            break;
            case 2:                         //Temperature calibration page
              if ((touchPosition.x > 222) && (touchPosition.x < 308)){                    // "SAVE" Button 
                tft.fillRect(222, 200, 86, 32, RED);                                      //Change Menu Button color
                tft.setCursor(232, 208);
                tft.setTextColor(WHITE);
                tft.println(text_menu[4]);                                                //text_menu[] = {"SOLAR","CAL_T","CAL_H","RESET", "SAVE"};
                delay(200);
                MENU = 0;                                                                 //Back to Menu weather page
                MENU_PAGE_CHANGE = 1;
              }
            break; 
            case 3:                       //Solar page
             if ((touchPosition.x > 222) && (touchPosition.x < 308)){             // "SAVE" Button and sent threshold to Master
                tft.fillRect(222, 200, 86, 32, RED);                              //Change Menu Button color
                tft.setCursor(232, 208);
                tft.setTextColor(WHITE);
                tft.println(text_menu[4]);                                        //text_menu[] = {"SOLAR","CAL_T","CAL_H","RESET", "SAVE"};
                delay(200);
                MENU = 0;                                                         //Back to Menu weather page
                MENU_PAGE_CHANGE = 1;
                //***********************Sent new threshold = current[1] to Master-ESP Node**********************************
                sentMessageToMaster = "Val4 "+String(current[1], 0)+"EE";  //create information for sensor current (Val4) containing the new threshold value (no decimal value)
                packetSize = sentMessageToMaster.length();
                sentMessageToMaster.toCharArray(packetBuffer, packetSize);
                packetBuffer[packetSize]=0;
                Serial1.print(packetBuffer);
             }
             if ((touchPosition.x >116) && (touchPosition.x <202)){               //"RESET" Button in SOLAR Menu. 
                tft.fillRect(116, 200, 86, 32, RED);                              //Change Menu Button color
                tft.setCursor(126, 208);
                tft.setTextColor(WHITE);
                tft.println(text_menu[3]);                                       //text_menu[] = {"SOLAR","CAL_T","CAL_H","RESET", "SAVE"};
                delay(200);
                on_time = 0;
                current[2] = 0;                                                  //current[2] = on_time
                CAL_VALUE_CHANGE = 1;
                tft.fillRect(116, 200, 86, 32, WHITE);                           //Change Menu Button color
                tft.setCursor(126, 208);
                tft.setTextColor(RED);
                tft.println(text_menu[3]);                                      //text_menu[] = {"SOLAR","CAL_T","CAL_H","RESET", "SAVE"};
             }
            break; 
           }
        }
        else {
          if ((MENU == 1)||(MENU == 2)){                                            //on page cal_temp or cal_humid
            for(j=1;j<(number_line-1);j++){
              if((touchPosition.y > 13+intervale_j*j) && (touchPosition.y < 34+intervale_j*j)){   //touch is on "+" or "-" line
                if((touchPosition.x > 234) && (touchPosition.x < 255)){               //Button "+"
                  tft.setTextSize(3);
                  tft.fillRect(234, 13+intervale_j*j, 21, 21, RED);
                  tft.setCursor(237, 14+intervale_j*j);
                  tft.setTextColor(WHITE);  
                  tft.println("+");
                  if (MENU == 1) {
                    cal_humid[j-1] = cal_humid[j-1] + 1;
                    CAL_VALUE_CHANGE = 1;
                    }
                  if (MENU == 2) {
                    cal_temp[j-1] = cal_temp[j-1] + 0.1;
                    CAL_VALUE_CHANGE =1;
                    } 
                  delay(200);
                  tft.fillRect(234, 13+intervale_j*j, 21, 21, WHITE);
                  tft.setCursor(237, 14+intervale_j*j);
                  tft.setTextColor(RED);
                  tft.println("+"); 
                }
                if((touchPosition.x > 276) && (touchPosition.x < 297)){             //Button "-"
                  tft.setTextSize(3);
                  tft.fillRect(276, 13+intervale_j*j, 21, 21, RED);
                  tft.setCursor(279, 14+intervale_j*j);
                  tft.setTextColor(WHITE);  
                  tft.println("-");
                  if (MENU == 1) {
                    cal_humid[j-1] = cal_humid[j-1] - 1;
                    CAL_VALUE_CHANGE = 1;
                    }
                  if (MENU == 2) {
                    cal_temp[j-1] = cal_temp[j-1] - 0.1;
                    CAL_VALUE_CHANGE = 1;
                    } 
                  delay(200);
                  tft.fillRect(276, 13+intervale_j*j, 21, 21, WHITE);
                  tft.setCursor(279, 14+intervale_j*j);
                  tft.setTextColor(RED);
                  tft.println("-"); 
                }
              }
            }
          }
          if(MENU == 3){                            //on page solar
            if((touchPosition.y > 13+intervale_j*2) && (touchPosition.y < 34+intervale_j*2)){   //"+" or "-" on line 3
              if((touchPosition.x > 234) && (touchPosition.x < 255)){               //Button "+"
                tft.setTextSize(3);
                tft.fillRect(234, 13+intervale_j*2, 21, 21, RED);
                tft.setCursor(237, 14+intervale_j*2);
                tft.setTextColor(WHITE);  
                tft.println("+");
                current[1] = current[1] + 1; //increase current limit
                CAL_VALUE_CHANGE = 1;
                delay(200);
                tft.fillRect(234, 13+intervale_j*2, 21, 21, WHITE);
                tft.setCursor(237, 14+intervale_j*2);
                tft.setTextColor(RED);
                tft.println("+"); 
              }
              if((touchPosition.x > 276) && (touchPosition.x < 297)){             //Button "-"
                tft.setTextSize(3);
                tft.fillRect(276, 13+intervale_j*2, 21, 21, RED);
                tft.setCursor(279, 14+intervale_j*2);
                tft.setTextColor(WHITE);  
                tft.println("-");
                current[1] = current[1] - 1; //decrease current limit
                CAL_VALUE_CHANGE = 1;
                delay(200);
                tft.fillRect(276, 13+intervale_j*2, 21, 21, WHITE);
                tft.setCursor(279, 14+intervale_j*2);
                tft.setTextColor(RED);
                tft.println("-"); 
              }
            }
          }
       }   
   } // End of touch check function
   
//Update Menu pages
//At regular interval time OR at calibration value / page change
  if ((time1 - previoustime1 >= interval_read) || (CAL_VALUE_CHANGE == 1) || (MENU_PAGE_CHANGE == 1)){
    
    if ((solar_stat == 100)&&(time1 - previoustime1 >= interval_read)){           //if load is switched ON calculate duration of ON and timer has elapsed.
      on_time = on_time + interval_read;                   //Increase on_time by sampling [ms]
      current[2] = on_time/1000/60/60;                     //calculate on_time in hours
      }
    
    if(time1 - previoustime1 >= interval_read) {   //if update due to intervale time then reinitialise time and read new internal temperature and humidity.
      tempsens0 = dht.readTemperature(false);
      humidsens0 = dht.readHumidity();
      previoustime1 = time1;
      }   
    
    if ((MENU ==0)&&(SLEEP_WAKE == 1)){                    //in Weather page not in sleep
    //Update weather value with correction
    temp[0] = tempsens0 + cal_temp[0];
    temp[1] = tempsens1 + cal_temp[1];
    temp[2] = tempsens2 + cal_temp[2];
    humid[0] = humidsens0 + cal_humid[0];
    humid[1] = humidsens1 + cal_humid[1];
    humid[2] = humidsens2 + cal_humid[2];
    if(MENU_PAGE_CHANGE == 1){     //check if page was different before if yes change to page weather
     Weather_Menu();
     MENU_PAGE_CHANGE = 0;
    }
    Weather_Value();               //Update value in weather page
  }
  
  if (((MENU==1)||(MENU==2))&&(SLEEP_WAKE == 1)){             //in calibration menu not in sleep 
      if(MENU_PAGE_CHANGE == 1){                              //check if page was different before if yes change to page calibration
        Cal_Menu(header_weather[2-MENU]);                     //header_weather[] = {"TEMP.";"HUMID."};0=weather datas; 1=humidity calibration; 2=temperature calibration; 3=solar
        MENU_PAGE_CHANGE = 0;
      }
      if (MENU == 1){                                         // in calibration menu humidity 
        humid[0] = humidsens0 + cal_humid[0];
        humid[1] = humidsens1 + cal_humid[1];
        humid[2] = humidsens2 + cal_humid[2];
        } 
      if (MENU == 2){                                          // in calibration menu temperature
        temp[0] = tempsens0 + cal_temp[0];
        temp[1] = tempsens1 + cal_temp[1];
        temp[2] = tempsens2 + cal_temp[2];
      }
      Cal_Value(MENU);
      CAL_VALUE_CHANGE = 0;
    }  
     
  if ((MENU == 3)&&(SLEEP_WAKE == 1)){                      //in solar menu not in sleep    
    if(MENU_PAGE_CHANGE == 1){                              //check if page was different before if yes change to page calibration
        Solar_Menu();                                        //Page solar
        MENU_PAGE_CHANGE = 0;
      }
    Solar_Value();
    CAL_VALUE_CHANGE = 0;  
    } 
  }
}
