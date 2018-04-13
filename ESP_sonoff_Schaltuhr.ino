/*
  Sonoff
  b4:e6:2d:25:42:7c
  115200 Baud

  ESP8266, DOUT, 115200, 1M(64k SPIFFS), 80MHz
  VID: 10C4
  PID: EA60
*/

/*
  TODO: 
  -PowerOnState ->per Datei?
  
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <time.h>

#include <JeVe_EasyOTA.h>  // https://github.com/jeroenvermeulen/JeVe_EasyOTA/blob/master/JeVe_EasyOTA.h
#include "FS.h"

#include "myNTP.h"
myNTP oNtp;

#include "data.h" //index.htm

const char* progversion  = "WLan-Timer V1.7";//ota fs ntp ti getpin

//----------------------------------------------------------------------------------------------

/*
#define ARDUINO_HOSTNAME  "sonoffpow" //http://sonoffpow.wg
#define pin_relais 12 //blue+relais   true=on
#define pin_led 13    //green         false=on      
#define pin_Button 0  //Button        LOW=down 
// =mess
#define buttMode INPUT
*/

/*
#define ARDUINO_HOSTNAME  "sonoffs20" //http://sonoffs20.wg
//#define ARDUINO_HOSTNAME  "sonoffpow" //http://sonoffpow.wg
#define pin_relais 12 //blue+relais   true=on
#define pin_led 13    //green         false=on      
#define pin_Button 0  //Button        LOW=down  
#define buttMode INPUT
*/


#define ARDUINO_HOSTNAME  "horsky"//
#define pin_relais 5              //red+relais   true=on
#define pin_led 4                 //blue         false=on
#define pin_Button 13             //Button     LOW=down
#define buttMode INPUT_PULLUP     //!

//----------------------------------------------------------------------------------------------

//#define WIFI_SSID         ""
//#define WIFI_PASSWORD     ""
#include "wifisetup.h"

//----------------------------------------------------------------------------------------------

#define TIMERtxt "/timer.txt"

unsigned long butt_zeitchecker= 120;//ms min presstime
unsigned long butt_previousMillis=0;
unsigned long buttpresstime=0;

unsigned long tim_zeitchecker= 15*1000;//alle 15sec Timer checken
unsigned long tim_previousMillis=0;
byte last_minute;

#define actionheader "HTTP/1.1 303 OK\r\nLocation:/index.htm\r\nCache-Control: no-cache\r\n\r\n"

uint8_t MAC_array[6];
char MAC_char[18];
String macadresse="";


EasyOTA OTA;
ESP8266WebServer server(80);
File fsUploadFile;                      //Hält den aktuellen Upload

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

  //Power ON:
  digitalWrite(pin_relais,false);//relay off
  digitalWrite(pin_led, false);  //LED on

  //OTA
  OTA.onMessage([](char *message, int line) {
    digitalWrite(pin_led, !digitalRead(pin_led));//Staus LED blinken
    Serial.println(message);
  });
  OTA.setup(WIFI_SSID, WIFI_PASSWORD, ARDUINO_HOSTNAME);//connect to WLAN

  //get MAC
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
  digitalWrite(pin_led, true);
  Serial.println("ready.");

  //NTP start
  oNtp.begin();
}



void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  OTA.loop();
  oNtp.update();
  
  unsigned long currentMillis = millis();//milliseconds since the Arduino board began running
  
  //Button
  if( currentMillis - butt_previousMillis > butt_zeitchecker){//alle 120ms
     butt_previousMillis=currentMillis;
     int buttonstate=digitalRead(pin_Button);
     
     if (buttonstate == HIGH) {//up
        if( buttpresstime>0){
           digitalWrite(pin_led, true);//LED off
           Serial.println(buttpresstime);
           if(buttpresstime>6000){//ca. 6 sec
              Serial.println("Restart ESP");
              //restart
              digitalWrite(pin_led, false);
              delay(500);
              digitalWrite(pin_led, true);
              delay(500);
              digitalWrite(pin_led, false);
              delay(500);
              digitalWrite(pin_led, true);
              delay(500);
              digitalWrite(pin_led, false);
              delay(500);
              digitalWrite(pin_led, true);
              delay(500);
              ESP.restart();
           }
           else
           {
            //toogle Relais
            digitalWrite(pin_relais, !digitalRead(pin_relais));
            Serial.println("toogle Relay");
           }
           buttpresstime=0;
        }
      }
      else
      {//down
        digitalWrite(pin_led, false);//LED an
        buttpresstime+=butt_zeitchecker;//Zeit merken +=120ms
        //digitalWrite(pin_Button, false);
       }
  }
 
  
  if(oNtp.hatTime() && currentMillis - tim_previousMillis > tim_zeitchecker){//Timer checken
      tim_previousMillis = currentMillis;
      if(last_minute!=oNtp.getminute()){//nur 1x pro min
        checktimer();
        last_minute=oNtp.getminute();
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
            /*Serial.print("check ");
            Serial.print(t_st);
            Serial.print(":");
            Serial.print(t_min);
            Serial.print(" ");
            Serial.print(tage, BIN);
            Serial.print(" "+befehl+" "+id);
*/
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
                  if(befehl=="ON"){//Serial.println(befehl);  
                      handleAktion(1, 1);
                    }
                    else
                  if(befehl=="OFF"){//Serial.println(befehl);  
                      handleAktion(2, 1);
                    }
                    else
                  if(befehl=="LEDON"){//Serial.println(befehl);  
                      handleAktion(3, 1);
                    }
                    else
                  if(befehl=="LEDOFF"){//Serial.println(befehl);  
                      handleAktion(4, 1);
                    }
                   /* else{
                      //Serial.print(befehl); 
                      //Serial.println("  Befehl ungültig  "); 
                     }*/
               }/*else{
                  Serial.println(" but not time.");
                }*/
            }
           /* else
            Serial.println(" is not the day.");*/
          }
      }
    }
    file.close();
  }  
}



//------------Data IO--------------------

void handleData(){// data.json
  String message = "{\r\n";
  String aktionen = "";

  //übergabeparameter?
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
  }
  
  message +="\"hostname\":\""+String(ARDUINO_HOSTNAME)+"\",\r\n";
  message +="\"aktionen\":\""+aktionen+"\",\r\n";
  message +="\"progversion\":\""+String(progversion)+"\",\r\n";
  message +="\"cpu_freq\":\""+String(ESP.getCpuFreqMHz())+"\",\r\n";
  message +="\"chip_id\":\""+String(ESP.getChipId())+"\",\r\n";
  
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
  if(digitalRead(pin_relais)==HIGH)
         message +="true";
         else
         message +="false";
  message +=",\r\n";
  
  message +="\"button\":";
  if(digitalRead(pin_Button)==HIGH)
         message +="true";
         else
         message +="false";
  message +=",\r\n";
  
  message +="\"led\":";
  if(digitalRead(pin_led)==HIGH)
         message +="false";
         else
         message +="true";
  message +="\r\n";
   
  message +="},\r\n";
  
 
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
  
  server.send(200, "text/html", message );
  Serial.println("send data.json");
}


void handleIndex() {//Rückgabe HTML
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
      s.replace("$h1gtag", progversion);//Überscherscift=Prog-Version
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
            tmp+="<a href =\"" + fileName + "?delete=" + fileName + "\" class=\"fl_del\"> löschen </a>\n";
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


void handleAction() {//Rückgabe JSON
  /*
      /action?sonoff=ON       Relais einschalten
      /action?sonoff=OFF      Relais ausschalten
      /action?sonoff=LEDON    LED einschalten
      /action?sonoff=LEDOFF   LED ausschalten
      /action?getpin=0        aktuellen Status von Pin IO0
  */
  String message = "{\n";
  message += "\"Arguments\":[\n";

  uint8_t AktionBefehl = 0;
  uint8_t keyOK = 0;
  uint8_t aktionresult = 0;

  for (uint8_t i = 0; i < server.args(); i++) {
    if (i > 0) message += ",\n";
    message += "  {\"" + server.argName(i) + "\" : \"" + server.arg(i) + "\"";

    if (server.argName(i) == "sonoff") {
      if (server.arg(i) == "ON")   AktionBefehl = 1;
      if (server.arg(i) == "OFF")  AktionBefehl = 2;
      if (server.arg(i) == "LEDON")  AktionBefehl = 3;
      if (server.arg(i) == "LEDOFF")  AktionBefehl = 4;
    }
    
    if (server.argName(i) == "getpin"){
       message += " ,\"val\": \"";
       if(digitalRead( server.arg(i).toInt()  )==HIGH)
              message += "true";
              else
              message += "false";
       message += "\"";
    }
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


String getContentType(String filename) {              // ContentType für den Browser
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

bool handleFileRead(String path) {//Datei löschen oder übertragen
  if(server.hasArg("delete")) {
    //Serial.print("delete: ");
    //Serial.print(server.arg("delete"));
    SPIFFS.remove(server.arg("delete"));  //hier wir gelöscht
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
 digitalWrite(pin_led, false);
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
  digitalWrite(pin_led,true );
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
      digitalWrite(pin_relais, true);
      re = 1;
    }
    if (befehl == 2) {//OFF
      digitalWrite(pin_relais, false);
      re = 2;
    }
    if (befehl == 3) {//LEDON
      digitalWrite(pin_led, false);
      re = 3;
    }
    if (befehl == 4) {//LEDOFF
      digitalWrite(pin_led,true );
      re = 4;
    }
  }
  return re;
}



