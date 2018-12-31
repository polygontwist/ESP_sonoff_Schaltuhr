/*
  getestet mit: Sonoff, WIFI Smart Socket, horsky
  115200 Baud

  ESP8266, DOUT, 115200, 1M(64k SPIFFS), 80MHz
  VID: 10C4
  PID: EA60
*/

/*
  TODO: 
  -PowerOnState ->per Datei?
  -wlan/ssid-Setup per AP-Mode ->siehe esp32-32x32rgb-matrix/setINI (SSL ?)
  
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <time.h>

//https://github.com/xoseperez/hlw8012
//or CSE7759
#include <HLW8012.h>


#include <JeVe_EasyOTA.h>  // https://github.com/jeroenvermeulen/JeVe_EasyOTA/blob/master/JeVe_EasyOTA.h
#include "FS.h"

#include "myNTP.h"
myNTP oNtp;

#include "data.h" //index.htm

const char* progversion  = "WLan-Timer V2.5";//ota fs ntp ti getpin HLW8012

//-----------------------------default------------------------------------------------------
#define ARDUINO_HOSTNAME  "Schalter"
#define buttMode INPUT     
#define pin_ledinvert true   //false=on      
#define pin_buttoninvert true  
#define pin_relaisinvert false      //true=on   

#define pin_led2 -1
#define pin_led2invert true   //false=on      
#define pin_ledWLAN -1
#define pin_ledWLANinvert true   //false=on      

   
//für HLW8012 current sensor Sonoff POW
#define SEL_PIN   -1     //-= kein
#define CF1_PIN   -1
#define CF_PIN    -1

//hlw8012 / CSE7759 setup
#define CURRENT_MODE HIGH
#define CURRENT_RESISTOR                0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 5 * 470000 ) // Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 ) // Real 1.009k

#define hlw_current_multiplier   144844.93
#define hlw_voltage_multiplier   408636.51
#define hlw_power_multiplier   10343611.74


//----------------------------------------------------------------------------------------------
/*
#define ARDUINO_HOSTNAME  "sonoffpow" //http://sonoffpow.wg
#define pin_relais 12 //red+relais   true=on
#define pin_led 15    //blue          
#define pin_Button 0  //Button        LOW=down 
#define pin_ledinvert false    //      true=on      
//https://github.com/erniberni/ESP_pow/blob/master/ESP_pow_with_OTA.ino
//https://github.com/xoseperez/hlw8012
#define SEL_PIN    5
#define CF1_PIN   13
#define CF_PIN    14
*/

/*
#define ARDUINO_HOSTNAME  "sonoffs20" //V2.5  http://sonoffs20.wg ->anlage
//#define ARDUINO_HOSTNAME  "dose2" //V2.5 http://dose2.wg 32x32
//#define ARDUINO_HOSTNAME  "lampe" //V2.5  http://lampe.wg (dose3)
#define pin_relais 12               //blue+relais   true=on
#define pin_led 13                  //green         false=on      
#define pin_Button 0                //Button        LOW=down  
*/

/*
#define ARDUINO_HOSTNAME  "horsky"// V2.5 ->usb-Strom
#define pin_relais 5              //red+relais   true=on
#define pin_led 4                 //blue         false=on
#define pin_Button 13             //Button     LOW=down
#define buttMode INPUT_PULLUP     //!
*/
/*
#define ARDUINO_HOSTNAME  "luminea"//pearl  V2.5
#define pin_relais 15             //relais  
#define pin_led 2                 //green
#define pin_Button 13             //Button
*/

/**/
#define ARDUINO_HOSTNAME  "oxaoxe"// V2.5
#define pin_relais 12             //relais  
#define pin_led 0                 //LED rot
#define pin_Button 13             //Button
#define pin_led2 15               //LED blau
#define pin_led2invert true       //false=on      
#define pin_ledWLAN 4             //LED blau WLAN
#define pin_ledWLANinvert true    //false=on      
#define SEL_PIN    3              //strom/voltsensor
#define CF1_PIN   14
#define CF_PIN    5
#define hlw_current_multiplier     555.35     //571.90 ->7.73A   x->8.130A(8)                   A=W/V
#define hlw_voltage_multiplier   17066.45     //16101.83 -> 218V   x->231  x=17066.45           V=W/A
#define hlw_power_multiplier    129437.79     //122724.46 -> 1879W   x->1849.1  x=129437,79     W=V*A



//WLAN LED: io4
//LED rot: io0
//SW: io13
//rx + :io3 *
//???:io14 *
//???:io2
//???:io5 *
//Rlay: io12
//LED blau:io15

/*
 "typ":"hlw8012",
  "currentmultiplier":566.00,
  "voltagemultiplier":15208.52,
  "powermultiplier":122724.46,
  "activepower":1753,
  "voltage":230,
  "current":7.26,
  "apparentpower":1759,
  "powerfactor":94
*/

//----------------------------------------------------------------------------------------------

//#define WIFI_SSID         ""
//#define WIFI_PASSWORD     ""
#include "wifisetup.h"    //fest eingebunden

//----------------------------------------------------------------------------------------------

#define TIMERtxt "/timer.txt"

unsigned long butt_zeitchecker= 120;//ms min presstime
unsigned long butt_previousMillis=0;
unsigned long buttpresstime=0;

unsigned long tim_zeitchecker= 15*1000;//alle 15sec Timer checken
unsigned long tim_previousMillis=0;
byte last_minute;

#define UPDATE_TIMEHLW8012 2000       //alle 2 Sekunden messen
unsigned long UPDATE_TIMEHLW8012_previousMillis=0;

#define check_wlanasclient 30000      //alle 30 Sekunden*2 gucken ob noch verbunden, wenn nicht neuer Versuch
                                      //zwischen 30 und 60 Sekunden
unsigned long check_wlanasclient_previousMillis=0;
#define anzahlVersuche 10             //nach 10 Versuchen im AP-Modus bleiben
#define keinAPModus true              //true=immer wieder versuchen ins WLAN zu kommen

#define actionheader "HTTP/1.1 303 OK\r\nLocation:/index.htm\r\nCache-Control: no-cache\r\n\r\n"

uint8_t MAC_array[6];
char MAC_char[18];
String macadresse="";


EasyOTA OTA;
ESP8266WebServer server(80);
File fsUploadFile;                      //Haelt den aktuellen Upload



HLW8012 hlw8012;

bool isAPmode=false;
int anzahlVerbindungsversuche=0;


//---------------------------------------------
//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

//---------------------------------------------
void setLED(bool an){
  if(pin_ledinvert)an=!an;
  digitalWrite(pin_led, an);
}
void toogleLED(){
  digitalWrite(pin_led, !digitalRead(pin_led));
}
bool getLED(){
  if(pin_ledinvert){
     return digitalRead(pin_led)==LOW;
    }
    else{
     return digitalRead(pin_led)==HIGH;
    }
}

void setLED2(bool an){
  if(pin_led2>-1){
    if(pin_led2invert)an=!an;
    digitalWrite(pin_led2, an);
  }
}
void toogleLED2(){
  if(pin_led2>-1)
      digitalWrite(pin_led2, !digitalRead(pin_led2));
}
bool getLED2(){
  if(pin_led2invert){
     return digitalRead(pin_led2)==LOW;
    }
    else{
     return digitalRead(pin_led2)==HIGH;
    }
}


void setLEDWLAN(bool an){
  if(pin_ledWLAN>-1){
    if(pin_ledWLANinvert)an=!an;
    digitalWrite(pin_ledWLAN, an);
  }
}
void toogleLEDWLAN(){
  if(pin_ledWLAN>-1)
    digitalWrite(pin_ledWLAN, !digitalRead(pin_ledWLAN));
}
bool getLEDWLAN(){
  if(pin_ledWLANinvert){
     return digitalRead(pin_ledWLAN)==LOW;
    }
    else{
     return digitalRead(pin_ledWLAN)==HIGH;
    }
}



void setRelais(bool an){
  if(pin_relaisinvert)an=!an;
  digitalWrite(pin_relais, an);
  setLED2(an);
}
void toogleRelais(){
  digitalWrite(pin_relais, !digitalRead(pin_relais));
  setLED2(digitalRead(pin_relais));
}

bool getRelay(){
  if(pin_relaisinvert){
     return digitalRead(pin_relais)==LOW;
    }
    else{
     return digitalRead(pin_relais)==HIGH;
    }
}
bool getButton(){
  if(pin_buttoninvert){
     return digitalRead(pin_Button)==LOW;
    }
    else{
     return digitalRead(pin_Button)==HIGH;
    }
}


//---------------------------------------------
void connectWLAN(){
   setLED(true);
   setLED2(false);
   setLEDWLAN(false);
        
   anzahlVerbindungsversuche++;
   OTA.setup(WIFI_SSID, WIFI_PASSWORD, ARDUINO_HOSTNAME);//connect to WLAN
   isAPmode=!(WiFi.status() == WL_CONNECTED);


   Serial.print("mode: ");
   if(isAPmode)
      Serial.println("AP");
      else
      Serial.println("client");

  macadresse="";
  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i) {
    if(i>0) macadresse+=":";
    macadresse+= String(MAC_array[i], HEX);
  }
  Serial.print("MAC: ");
  Serial.println(macadresse);

  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 

  if(isAPmode){
      setLED(true);
      setLEDWLAN(true);
      delay(500);
      setLED(false);
      setLEDWLAN(false);
      delay(250);
      setLED(true);
      setLEDWLAN(true);
      delay(500);
      setLED(false);
      setLEDWLAN(false);
      delay(250);
      setLED(true);
      setLEDWLAN(true);
      delay(500);
      setLED(false);
      setLEDWLAN(false);
      delay(250);
    }
    else{
      anzahlVerbindungsversuche=0;//erfolgreich verbunden, Zaehler auf 0 setzen
      setLED(false);
      setLED2(false);
      setLEDWLAN(false);
   }
}


void setup() {
  //serial
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  Serial.println(progversion);
  
  //SPIFFS
  SPIFFS.begin();
  

  //Pins initialisieren 
  pinMode(pin_relais, OUTPUT);
  pinMode(pin_led, OUTPUT);
  pinMode(pin_Button, buttMode); 

 //oxaoxe
 if(pin_led2>-1){
    pinMode(pin_led2, OUTPUT);
 }
 if(pin_ledWLAN>-1){
    pinMode(pin_ledWLAN, OUTPUT);
 }
 
  //Power ON:
  setRelais(false);//relay off
  
  setLED(true);  //LED on
  setLED2(false);
  setLEDWLAN(false);

  //Initialize HLW8012/ CSE7759
  if(SEL_PIN>-1){
    hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, false, 500000);  //500000microseconds
    
    hlw8012.setCurrentMultiplier(hlw_current_multiplier);
    hlw8012.setVoltageMultiplier(hlw_voltage_multiplier);
    hlw8012.setPowerMultiplier(hlw_power_multiplier);
 
    //hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);
                          //0.001,( 5 * 470000 ),( 1000 ) 
      //_current_multiplier = 144844.93
      //_voltage_multiplier = 408636.51
      //_power_multiplier =  10343611.74

  }


  //OTA
  OTA.onMessage([](char *message, int line) {
    toogleLED();
    setLED2(!getLED());
    toogleLEDWLAN();
    Serial.println(message);
  });

  connectWLAN();
    
  server.on("/action", handleAction);//daten&befehle
  
  server.on("/",handleIndex);
  server.on("/index.htm", handleIndex);
  server.on("/index.html", handleIndex);

  server.on("/data.json", handleData);//aktueller Status+Dateiliste
  
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);             //Dateiupload
  
  server.onNotFound(handleNotFound);//Datei oder 404

  server.begin();
  Serial.println("HTTP server started");
  
  //info-LED aus
  setLED(false);
  setLED2(false);
  setLEDWLAN(false);
      
  //digitalWrite(pin_led, true);
  Serial.println("ready.");

  //NTP start
  oNtp.begin();
}



void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  OTA.loop();
  oNtp.update();
  isAPmode=!(WiFi.status() == WL_CONNECTED);
  
  unsigned long currentMillis = millis();//milliseconds since the Arduino board began running
  
  //Button
  if( currentMillis - butt_previousMillis > butt_zeitchecker){//alle 120ms
     butt_previousMillis=currentMillis;
    
     if (!getButton()) {//up
        if( buttpresstime>0){
           setLED(false);
           //digitalWrite(pin_led, true);//LED off
           Serial.println(buttpresstime);
           if(buttpresstime>6000){//ca. 6 sec
              Serial.println("Restart ESP");
              //restart
              setLED(true);
              delay(500);
              setLED(false);
              delay(500);
              setLED(true);
              delay(500);
              setLED(false);
              delay(500);
              setLED(true);
              delay(500);
              setLED(false);
              delay(500);
              ESP.restart();
           }
           else
           {
            //toogle Relais
            
            toogleRelais();
            Serial.println("toogle Relay");
           }
           buttpresstime=0;
        }
      }
      else
      {//down
        buttpresstime+=butt_zeitchecker;//Zeit merken +=120ms
        if(buttpresstime>6000)
              toogleLED();
            else
              setLED(true);//LED an
       }
  }
 
  
  if(oNtp.hatTime() && currentMillis - tim_previousMillis > tim_zeitchecker){//Timer checken
      tim_previousMillis = currentMillis;
      if(last_minute!=oNtp.getminute()){//nur 1x pro min
        checktimer();
        last_minute=oNtp.getminute();
     }
   }

 //HLW8012  
  if(SEL_PIN>-1){
    if(currentMillis - UPDATE_TIMEHLW8012_previousMillis > UPDATE_TIMEHLW8012){
      UPDATE_TIMEHLW8012_previousMillis = currentMillis;
      /*
      Serial.print("[HLW] Active Power (W)    : "); Serial.println(hlw8012.getActivePower());
      Serial.print("[HLW] Voltage (V)         : "); Serial.println(hlw8012.getVoltage());
      Serial.print("[HLW] Current (A)         : "); Serial.println(hlw8012.getCurrent());
      Serial.print("[HLW] Apparent Power (VA) : "); Serial.println(hlw8012.getApparentPower());
      Serial.print("[HLW] Power Factor (%)    : "); Serial.println((int) (100 * hlw8012.getPowerFactor()));
      Serial.println();
      
      "typ":"hlw8012",
      "currentmultiplier":14484.49,
      "voltagemultiplier":408636.51,      //
      "powermultiplier":10343611.74,
      "activepower":147765,
      "voltage":5238,                     226V
      "current":185.70,
      "apparentpower":972689,
      "powerfactor":13
      
      
      */
      // When not using interrupts we have to manually switch to current or voltage monitor
       hlw8012.toggleMode();
    }
    
 }

 //WLAN-ceck
 unsigned long cwl=random(check_wlanasclient, check_wlanasclient+check_wlanasclient);//x..x+15sec sonst zu viele Anfragen am AP
 if(currentMillis - check_wlanasclient_previousMillis > cwl){
      //zeit abgelaufen
      check_wlanasclient_previousMillis = currentMillis;
       if(isAPmode){//apmode
        //neuer Verbindengsaufbauversuch
         if(anzahlVerbindungsversuche<anzahlVersuche || keinAPModus){//nur x-mal, dann im AP-Mode bleiben
               connectWLAN();
         }
      }
 }

/*//TODO ?
 if ( WiFi.status() != WL_CONNECTED ){
        //Serial.println("no connect");
       OTA.setup(WIFI_SSID, WIFI_PASSWORD, ARDUINO_HOSTNAME);//re-connect to WLAN
     }
*/
}

//-------------------Timer---------------
const int line_BUFFER_SIZE = 128;
char line_buffer[line_BUFFER_SIZE];

void checktimer(){ 
  if (SPIFFS.exists(TIMERtxt)) {
    // time
    File file = SPIFFS.open(TIMERtxt, "r");
    String zeile;
    char tmp[line_BUFFER_SIZE];
    int tmpcounter;
    bool onoff;
    char zeit[5];
    byte t_st=0;
    byte t_min=0;
    byte tage=0;
    String befehl="";
    String id="";
    int anz=0;
    int i;
    int t;
    int sepcounter;
    
    if(file){
      //Serial.println("opend timer.txt");
      //Zeilenweise einlesen
      //on/off|hh:mm|mo-so als bit|befehl|id
      //on|07:05|31|ON|t1
      while (file.available()){
          zeile=file.readStringUntil('\n');
          //Serial.println(zeile);
          //Zeile zerlegen
          anz= zeile.length();
          zeile.toCharArray(line_buffer,anz);
          tmpcounter=0;
          sepcounter=0;
          onoff=false;
          for(i=0;i<anz;i++){
            if(line_buffer[i]=='|'){
              tmp[tmpcounter]='\0';              
              if(sepcounter==0){//on/off
                onoff=( tmp[0]=='o' && tmp[1]=='n');
              }
              if(sepcounter==1){//hh:mm
                zeit[0]=tmp[0];
                zeit[1]=tmp[1];
                zeit[2]='\0';
                t_st=String(zeit).toInt();
                zeit[0]=tmp[3];
                zeit[1]=tmp[4];
                t_min=String(zeit).toInt();
              }
              if(sepcounter==2){
                tage=String(tmp).toInt();
              }
              if(sepcounter==3){
                befehl=String(tmp);
              }
              if(sepcounter==4){
                id=String(tmp);
              }
              sepcounter++;
              tmpcounter=0;
            }
            else{
             tmp[tmpcounter]=line_buffer[i];
             tmpcounter++;
            }
          }
          //Zeilendaten auswerten
          if(onoff){            
        byte maske=0;//uint8_t
        byte ntp_wochentag=oNtp.getwochentag();
        byte ntp_stunde=oNtp.getstunde();
        byte ntp_minute=oNtp.getminute();
        
        if(ntp_wochentag==0)maske=1;//Serial.print(" Mo");
        if(ntp_wochentag==1)maske=2;//Serial.print(" Di");
        if(ntp_wochentag==2)maske=4;//Serial.print(" Mi");
        if(ntp_wochentag==3)maske=8;//Serial.print(" Do");
        if(ntp_wochentag==4)maske=16;//Serial.print(" Fr");
        if(ntp_wochentag==5)maske=32;//Serial.print(" Sa");
        if(ntp_wochentag==6)maske=64;//Serial.print(" So");
        
        if(tage & maske ){//Serial.print(" isday, ");
              if(ntp_stunde==t_st && ntp_minute==t_min){
                  if(befehl=="ON")handleAktion(1, 1);
                  else
                  if(befehl=="OFF")handleAktion(2, 1);
                  else
                  if(befehl=="LEDON")handleAktion(3, 1);
                  else
                  if(befehl=="LEDOFF")handleAktion(4, 1);
                  else
                  if(befehl=="LED2ON")handleAktion(5, 1);
                  else
                  if(befehl=="LED2OFF")handleAktion(6, 1);
                  else
                  if(befehl=="LEDWLANON")handleAktion(7, 1);
                  else
                  if(befehl=="LEDWLANOFF")handleAktion(8, 1);
            }
          }
      }
      
    }
  }
  
    file.close();
  }  
}

//--------------Power--------------------
void unblockingDelay(unsigned long mseconds) {
    unsigned long timeout = millis();
    while ((millis() - timeout) < mseconds) delay(1);
}
void calibrate(float watt, float volt) {

   //setResistors & _calculateDefaultMultipliers
   
   //hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);
                          //0.001,( 5 * 470000 ),( 1000 ) 
      //_current_multiplier = 144844.93  // A
      //_voltage_multiplier = 408636.51  // V
      //_power_multiplier =  10343611.74 // W

    /*
          0.001       ->_current_multiplier = ( 1000000.0 * 512 * V_REF / _current_resistor / 24.0 / F_OSC );
          5 * 470000 
          1000

          _current_resistor = 0.001;
          _voltage_resistor = (5 * 470000 + 1000) / 1000; -> 2351


          _current_multiplier = ( 1000000.0 * 512 * 2.43 / _current_resistor / 24.0 / 3579000 );                          ->144844.93
          _voltage_multiplier = ( 1000000.0 * 512 * 2.43 * _voltage_resistor / 2.0 / 3579000 );                           ->408636.51
          _power_multiplier = ( 1000000.0 * 128 * 2.43 * 2.43 * _voltage_resistor / _current_resistor / 48.0 / 3579000 ); ->10343611.74
          
    */
    // Let's first read power, current and voltage
    // with an interval in between to allow the signal to stabilise:

    hlw8012.getActivePower();
   
    hlw8012.setMode(MODE_CURRENT);
    unblockingDelay(2000);
    hlw8012.getCurrent();

    hlw8012.setMode(MODE_VOLTAGE);
    unblockingDelay(2000);
    hlw8012.getVoltage();

    // Calibrate using a 60W bulb (pure resistive) on a 230V line
    hlw8012.expectedActivePower(watt);  //   60.0W
    hlw8012.expectedVoltage(volt);      //  230.0V
    hlw8012.expectedCurrent(watt / volt);// 60.0/230  -> 0.26 A

}



//------------Data IO--------------------

void handleData(){// data.json
  String message = "{\r\n";
  String aktionen = "";

  //uebergabeparameter?
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "settimekorr") {
       oNtp.setTimeDiff(server.arg(i).toInt());
       aktionen +="set_timekorr ";
    }
    if (server.argName(i) == "led") {
       if (server.arg(i) == "on" ){
            handleAktion(3,1);
             aktionen +="LED_ON ";
        }
       if (server.arg(i) == "off"){
            handleAktion(4,1);
            aktionen +="LED_OFF ";
       }
    }
    if (server.argName(i) == "relais") {
       if (server.arg(i) == "on" ){
             handleAktion(1,1);
             aktionen +="Relais_ON ";
        }
       if (server.arg(i) == "off"){
             handleAktion(2,1);
             aktionen +="Relais_OFF ";
       }
    }

    if (server.argName(i) == "led2") {
       if (server.arg(i) == "on" ){
            handleAktion(5,1);
            aktionen +="LED2_ON ";
        }
       if (server.arg(i) == "off"){
            handleAktion(6,1);
            aktionen +="LED2_OFF ";
       }
    }

    if (server.argName(i) == "ledwlan") {
       if (server.arg(i) == "on" ){
            handleAktion(7,1);
            aktionen +="LEDWLAN_ON ";
        }
       if (server.arg(i) == "off"){
            handleAktion(8,1);
            aktionen +="LEDWLAN_OFF ";
       }
    }
    
  }
  
  message +="\"hostname\":\""+String(ARDUINO_HOSTNAME)+"\",\r\n";
  message +="\"aktionen\":\""+aktionen+"\",\r\n";
  message +="\"progversion\":\""+String(progversion)+"\",\r\n";
  message +="\"cpu_freq\":\""+String(ESP.getCpuFreqMHz())+"\",\r\n";
  message +="\"chip_id\":\""+String(ESP.getChipId())+"\",\r\n";
  message +="\"flashchiprealsize\":\""+String(ESP.getFlashChipRealSize())+"\",\r\n";

  message +="\"isAPmode\":\"";
  if(isAPmode)
     message +="true";
    else
     message +="false";
  message +="\",\r\n";

  
  byte ntp_stunde   =oNtp.getstunde();
  byte ntp_minute   =oNtp.getminute();
  byte ntp_secunde  =oNtp.getsecunde();
 

//  ntp_stunde
  message +="\"lokalzeit\":\"";
  if(ntp_stunde<10)message+="0";
  message+=String(ntp_stunde)+":";
  if(ntp_minute<10)message+="0";
  message+= String(ntp_minute)+":";
  if(ntp_secunde<10)message+="0";
  message+=String(ntp_secunde);
  message +="\",\r\n";
  
  message +="\"datum\":{\r\n";
  message +=" \"tag\":"+String(oNtp.getwochentag())+",\r\n";
  message +=" \"year\":"+String(oNtp.getyear())+",\r\n";
  message +=" \"month\":"+String(oNtp.getmonth())+",\r\n";
  message +=" \"day\":"+String(oNtp.getday())+",\r\n";
  message +=" \"timekorr\":"+String(oNtp.getUTCtimediff())+",\r\n";
  if(oNtp.isSummertime())
    message +=" \"summertime\":true\r\n";
  else
    message +=" \"summertime\":false\r\n";
  message +="},\r\n";

//led/relais-status
  
  message +="\"portstatus\":{\r\n";
  
  message +="\"relais\":";
  if(getRelay())
         message +="true";
         else
         message +="false";
  message +=",\r\n";
  
  /*message +="\"button\":"; nicht für www relevant, da taster - nur lokale Auswertung
  if(getButton())
         message +="true";
         else
         message +="false";
  message +=",\r\n";*/
  
  message +="\"led\":";
 
  if(getLED())
         message +="true";
         else
         message +="false";

    //oxaoxe
     if(pin_led2>-1){
         message +=",\r\n\"led2\":";
         if(getLED2())
              message += "true";
              else
              message += "false";
     }
     if(pin_ledWLAN>-1){
         message +=",\r\n\"ledWLAN\":";
         if(getLEDWLAN())
              message += "true";
              else
              message += "false";
     }
   
  message +="\r\n";
  message +="},\r\n";//Portstatus


  if(SEL_PIN>-1){
      message +="\"power\":{\r\n";
      
       message +="  \"typ\":\"hlw8012\",\r\n";
       
       message +="  \"currentmultiplier\":";
       message +=String(hlw8012.getCurrentMultiplier());
       message +=",\r\n";
       
       message +="  \"voltagemultiplier\":";
       message +=String(hlw8012.getVoltageMultiplier());
       message +=",\r\n";
       
       message +="  \"powermultiplier\":";
       message +=String(hlw8012.getPowerMultiplier());
       message +=",\r\n";
       
       message +="  \"activepower\":";
       message +=String(hlw8012.getActivePower());//W
       message +=",\r\n";
       
       message +="  \"voltage\":";
       message +=String(hlw8012.getVoltage());//V
       message +=",\r\n";
       
       message +="  \"current\":";
       message +=String(hlw8012.getCurrent());//A
       message +=",\r\n";
       
       message +="  \"apparentpower\":";
       message +=String(hlw8012.getApparentPower());//VA
       message +=",\r\n";
       
       message +="  \"powerfactor\":";
       message +=String((int) (100 * hlw8012.getPowerFactor()));//%
       message +=",\r\n";

       
       message +="  \"reactivepower\":";
       message +=String((int) (hlw8012.getReactivePower()));
       message +=",\r\n";
       
       message +="  \"energy\":";
       message +=String(hlw8012.getEnergy());//in Ws
       message +="\r\n";
       
/*      Serial.print("[HLW] Active Power (W)    : "); Serial.println(hlw8012.getActivePower());
      Serial.print("[HLW] Voltage (V)         : "); Serial.println(hlw8012.getVoltage());
      Serial.print("[HLW] Current (A)         : "); Serial.println(hlw8012.getCurrent());
      Serial.print("[HLW] Apparent Power (VA) : "); Serial.println(hlw8012.getApparentPower());
      Serial.print("[HLW] Power Factor (%)    : "); Serial.println((int) (100 * hlw8012.getPowerFactor()));
      Serial.println();
*/
  
      message +="},\r\n";
  }


 
  message +="\"macadresse\":\""+macadresse+"\",\r\n";

  FSInfo fs_info;
  if (SPIFFS.info(fs_info)) {
      message +="\"fstotalBytes\":"+String(fs_info.totalBytes)+",\r\n";
      message +="\"fsusedBytes\":"+String(fs_info.usedBytes)+",\r\n";

      message +="\"fsused\":\"";
      message +=float(int(100.0/fs_info.totalBytes*fs_info.usedBytes*100.0)/100.0);
      message +="%\",\r\n";
   }
  //files
  message +="\"files\":[\r\n";
  String fileName;
  Dir dir = SPIFFS.openDir("/");
  uint8_t counter=0;
  while (dir.next()) {
      fileName = dir.fileName(); 
      if(counter>0)  message +=",\r\n";
      message +=" {";
      message +="\"fileName\":\""+fileName+"\", ";
      message +="\"fileSize\":"+String(dir.fileSize());
      message +="}";
     counter++;
  };
  message +="\r\n]\r\n";
//--
 
  message +="\r\n}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");//wenn vom HTTPS-Seiten aufgerufen wird!
  server.send(200, "text/plain", message );
  Serial.println("send data.json");
}


void handleIndex() {//Rueckgabe HTML
  //$h1gtag $info
  int pos1 = 0;
  int pos2 = 0;
  String s;
  String tmp;

  String message = "";

  while (indexHTM.indexOf("\r\n", pos2) > 0) {
    pos1 = pos2;
    pos2 = indexHTM.indexOf("\r\n", pos2) + 2;
    s = indexHTM.substring(pos1, pos2);

    //Tags gegen Daten ersetzen
    if (s.indexOf("$h1gtag") != -1) {
      s.replace("$h1gtag", progversion);//Ueberscherschrift=Prog-Version
    }

    //Liste der Dateien
    if(s.indexOf("$filelist") != -1){        
        tmp="<table class=\"files\">\n";
        String fileName;
        Dir dir = SPIFFS.openDir("/");
        while (dir.next()) {
            fileName = dir.fileName(); 
            Serial.print("getfilelist: ");
            Serial.println(fileName);
            tmp+="<tr>\n";
            tmp+="\t<td><a target=\"_blank\" href =\"" + fileName + "\"" ;
            tmp+= " >" + fileName.substring(1) + "</a></td>\n\t<td class=\"size\">" + formatBytes(dir.fileSize())+"</td>\n\t<td class=\"action\">";
            tmp+="<a href =\"" + fileName + "?delete=" + fileName + "\" class=\"fl_del\"> l&ouml;schen </a>\n";
            tmp+="\t</td>\n</tr>\n";
        };

        FSInfo fs_info;
        tmp += "<tr><td colspan=\"3\">";
        if (SPIFFS.info(fs_info)) {
          tmp += formatBytes(fs_info.usedBytes).c_str(); //502
          tmp += " von ";
          tmp += formatBytes(fs_info.totalBytes).c_str(); //2949250 (2.8MB)   formatBytes(fileSize).c_str()
          tmp += " (";
          tmp += float(int(100.0/fs_info.totalBytes*fs_info.usedBytes*100.0)/100.0);
          tmp += "%)";
          /*tmp += "<br>\nblockSize:";
          tmp += fs_info.blockSize; //8192
          tmp += "<br>\npageSize:";
          tmp += fs_info.pageSize; //256
          tmp += "<br>\nmaxOpenFiles:";
          tmp += fs_info.maxOpenFiles; //5
          tmp += "<br>\nmaxPathLength:";
          tmp += fs_info.maxPathLength; //32*/
        }
        tmp+="</td></tr></table>\n";
        s.replace("$filelist", tmp);
    }

    
    message += s;
  }

  server.send(200, "text/html", message );
}


void handleAction() {//Rueckgabe JSON
  /*
      /action?sonoff=ON       Relais einschalten
      /action?sonoff=OFF      Relais ausschalten
      /action?sonoff=LEDON    LED einschalten
      /action?sonoff=LEDOFF   LED ausschalten
      /action?getpin=0        aktuellen Status von Pin IO0
      /action?calibrate=60&v=230    SonoffPow mit 60W Gluehbirne kalibrieren, auf 230V
  */
  String message = "{\n";
  message += "\"Arguments\":[\n";

  uint8_t AktionBefehl = 0;
  uint8_t keyOK = 0;
  uint8_t aktionresult = 0;
  float volt=0.0;
  float watt=0.0;

  for (uint8_t i = 0; i < server.args(); i++) {
    if (i > 0) message += ",\n";
    message += "  {\"" + server.argName(i) + "\" : \"" + server.arg(i) + "\"";

    if (server.argName(i) == "sonoff") {
      if (server.arg(i) == "ON")   AktionBefehl = 1;
      if (server.arg(i) == "OFF")  AktionBefehl = 2;
      if (server.arg(i) == "LEDON")  AktionBefehl = 3;
      if (server.arg(i) == "LEDOFF")  AktionBefehl = 4;
      if (server.arg(i) == "LED2ON")  AktionBefehl = 5;
      if (server.arg(i) == "LED2OFF")  AktionBefehl = 6;
      if (server.arg(i) == "LEDWLANON")  AktionBefehl = 7;
      if (server.arg(i) == "LEDWLANOFF")  AktionBefehl = 8;
    }
    
    if (server.argName(i) == "calibrate"){//  IP/action?calibrate=1&volt=230&watt=100
       if(SEL_PIN>-1){
        
          for (uint8_t i2 = 0; i2 < server.args(); i2++) {
              if(server.argName(i2) == "volt"){
                  volt=server.arg(i2).toFloat();
                }
             if(server.argName(i2) == "watt"){
                  watt=server.arg(i2).toFloat();
                }
          }

          if(volt>0 && watt>0 )
            calibrate(watt ,volt );
          
          message +=",\r\n ";
          
          message +="   \"watt\":";
          message +=String(watt);
          message +=",\r\n";
         
          message +="   \"volt\":";
          message +=String(volt);
          message +=",\r\n";
         
          message +="   \"currentmultiplier\":";
          message +=String(hlw8012.getCurrentMultiplier());
          message +=",\r\n";
          
          message +="  \"voltagemultiplier\":";
          message +=String(hlw8012.getVoltageMultiplier());
          message +=",\r\n";
          
          message +="  \"powermultiplier\":";
          message +=String(hlw8012.getPowerMultiplier());
          message +="\r\n";

       }
    }
    
    
    if (server.argName(i) == "getpin"){
       message += " ,\"val\": \"";
       if(digitalRead( server.arg(i).toInt())==HIGH)
              message += "true";
              else
              message += "false";
       message += "\"";
    }

    //TODO:
    //setSSID
    //setPassword

    message += "}";
  }
  
  message += "\n]";

  if(AktionBefehl>0){
      aktionresult = handleAktion(AktionBefehl, 1);
      message += ",\n\"befehl\":\"";
      if (aktionresult > 0)
        message += "OK";
      else
        message += "ERR";
      message += "\"";
  }
  
  message += "\n}";
  server.send(200, "text/plain", message );

}


String getContentType(String filename) {              // ContentType fuer den Browser
  if (filename.endsWith(".htm")) return "text/html";
  //else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  /*else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";*/
  return "text/plain";
}

void handleFileUpload() {          // Dateien ins SPIFFS schreiben
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    //Serial.print("handleFileUpload Name: ");
    if (filename.length() > 30) {
      int x = filename.length() - 30;
      filename = filename.substring(x, 30 + x);
    }
    filename = server.urlDecode(filename);
    filename = "/" + filename;
    
    fsUploadFile = SPIFFS.open(filename, "w");
    //if(!fsUploadFile) Serial.println("!! file open failed !!");
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile){
        //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
        fsUploadFile.write(upload.buf, upload.currentSize);
      }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile){
      fsUploadFile.close();
      //Serial.println("close");
    }
    yield();
    //Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
    server.sendContent(actionheader);//Seite neu laden
  }
}

bool handleFileRead(String path) {//Datei loeschen oder uebertragen
  if(server.hasArg("delete")) {
    //Serial.print("delete: ");
    //Serial.print(server.arg("delete"));
    SPIFFS.remove(server.arg("delete"));  //hier wir geloescht
       /*Serial.println(" OK");
        else
        Serial.println(" ERR");*/
    server.sendContent(actionheader);//Seite neu laden
    return true;
  }
  path = server.urlDecode(path);
  //Serial.print("GET ");
  //Serial.print(path);
  if (SPIFFS.exists(path)) {
    //Serial.println(" send");
    //   Serial.println("handleFileRead: " + path);
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, getContentType(path));
    file.close();
     return true;
  }
  //Serial.println(" 404");
  return false;
}


void handleNotFound() {
 //--check Dateien im SPIFFS--
 if(!handleFileRead(server.uri())){ 
    //--404 als JSON--
    String message = "{\n \"error\":\"File Not Found\", \n\n";
    message += " \"URI\": \"";
    message += server.uri();
    message += "\",\n \"Method\":\"";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\",\n";
    message += " \"Arguments\":[\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      if (i > 0) message += ",\n";
      message += "  {\"" + server.argName(i) + "\" : \"" + server.arg(i) + "\"}";
    }
    message += "\n ]\n}";  
    server.send(404, "text/plain", message);  
  }
}

//----------------------IO-----------------------------
uint8_t handleAktion(uint8_t befehl, uint8_t key) {
  uint8_t re = 0;
 /* Serial.print("handleAktion:");
  Serial.print(befehl);
  Serial.print(",");
  Serial.println(key);*/
  if (key == 1) {
    if (befehl == 1) {//ON
      setRelais(true);
      re = befehl;
    }
    if (befehl == 2) {//OFF
      setRelais(false);
      re = befehl;
    }
    if (befehl == 3) {//LEDON
      setLED(true);
      //digitalWrite(pin_led, false);
      re = befehl;
    }
    if (befehl == 4) {//LEDOFF
      setLED(false);
      //digitalWrite(pin_led,true );
      re = befehl;
    }


    if (befehl == 5) {//LED2ON
      setLED2(true);
      re = befehl;
    }
    if (befehl == 6) {//LED2OFF
      setLED2(false);
      re = befehl;
    }

    if (befehl == 7) {//LEDWLANON
      setLEDWLAN(true);
      re = befehl;
    }
    if (befehl == 8) {//LEDWLANOFF
      setLEDWLAN(false);
      re = befehl;
    }
  }
  return re;
}
