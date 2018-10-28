# Projekt: Wifi Schaltuhr Version 2.3

Alternative Firmware für WLAN-Steckdosen (Sonoff, Horsky, Lingan SWA1) mit folgenden Funktionen:
* Schalten per WLAN-Verbindung
* definieren von Timern (Schaltuhrfunktion)
* Uhr wird per NTP (Network Time Protocol) aktuell gehalten, Server ist 0.europe.pool.ntp.org (siehe myNTP.h)
* automatische Umschaltung Sommer-/Winterzeit
* update per OTA möglich (Arduino)
* Oberfläche per Javascript und CSS gestaltbar

Getestet mit: Sonoff S20, HORSKY, Sononoff POW.

Schaue Dir auch das Projekt https://github.com/arendst/Sonoff-Tasmota/ evtl. passt es besser zu Deinem Setup (mit MQTT für Alexa & Co.).

# Hardwaresetup
Zur Verbindung mit dem Recher benutze ich ein USB-to-Seriel-Adapter. Dort ist TX, RX, GND und 3,3V gefordert, keinesfalls darf der ESP eine höhere Spannung als 3,3V bekommen!

Je nach Schaltdose sind die Pins zu seriellen Verbindung zu finden:

## Sonoff S20
![](https://raw.githubusercontent.com/polygontwist/ESP_sonoff_Schaltuhr/master/bilder/sonoff_platine.png)

Das Relais hängt an Port 12, die Status LED an Port 13 und der Button an Port 0.

## HORSKY Wifi
![](https://raw.githubusercontent.com/polygontwist/ESP_sonoff_Schaltuhr/master/bilder/platine_horsky_Lingan-SWA1.png)

Das Relais hängt an Port 5, die Status LED an Port 4 und der Button an Port 13. Um den Button zu benutzen muß der Mode auf INPUT_PULLUP gesetzt werden (Eingang mit internen Pullupwiederstand).
Eine Besonderheit bei der HORSKY das es kein direkten Draht zum 3.3V gibt, Vcc geht über einen Spannungsregler (AMS1117) und dann zum ESP. Ich habe die 3.3V dort angeschlossen - es hatte funktioniert. Alternativ kann man sich auch ein Kabel an den ESP löten (Pin 3v3 oben links).
Der ESP scheint ein ESP8266-9 zu sein.

## Sonoff S20 POW
Diese hat einen zusätzlichen Chip (CSE7759) zur Strom- und Spannungsmessung. Die Ansteuerung ist analog zum hlw8012.
![](https://raw.githubusercontent.com/polygontwist/ESP_sonoff_Schaltuhr/master/bilder/sonoffPOW_serial.jpg)
* pin_relais 12 //red+relais   true=on
* pin_led 15    //blue          
* pin_Button 0  //Button  LOW=down 
* SEL_PIN    5  //https://github.com/xoseperez/hlw8012
* CF1_PIN   13
* CF_PIN    14

## WIFI Smart Socket PS-16-M (10A)
Pinbelegung & Handling wie Sonoff
![](https://raw.githubusercontent.com/polygontwist/ESP_sonoff_Schaltuhr/master/bilder/wifi_smart_socket.jpg)

# Quellen und Inspirationen
folgende Bibliothecken wurden verwendet:
* JeVe_EasyOTA (Version 2.2.0) https://github.com/jeroenvermeulen/JeVe_EasyOTA/
* HLW8012 library for Arduino and ESP8266 https://github.com/xoseperez/hlw8012
* ESP8266WiFi
* WiFiClient
* ESP8266WebServer
* time
* FS (SPIFFS)

folgende Projekte waren für die Entwicklung hilfreich: 
* https://github.com/arendst/Sonoff-Tasmota/wiki
* http://tinkerman.cat/s20-smart-socket/#lightbox-gallery-pgrT/4/
* https://github.com/arendst/Sonoff-Tasmota/wiki/Lingan-SWA1
* https://nathan.chantrell.net/20170528/lingan-swa1-plug-in-wi-fi-socket-with-mqtt-esp8266/
* http://ganzprivat.de/2017/12/16/ligan-swa1-mit-temperatursensor/
* https://github.com/martin-ger/ESP8266-WiFi-Socket
* https://botland.com.pl/moduly-wifi/7453-dfrobot-modul-wifi-wt8266-s1-esp8266-9-gpio-adc-pcb-antena.html


# Installation mit Arduino IDE 1.8.5
* zuerst müssen die Zeilen #define WIFI_SSID und #define WIFI_PASSWORD aus kommentiert und befüllt werden
* die Zeile #include "wifisetup.h" ist auszukommentieren (ich habe dort meine Wifi-Einstellungen abgelegt)
* Einstellung: Generic ESP8266 Modul, 80 MHZ, 40MHz, DOUT, 115200, 1M (64k SPIFFS)
* Um die Stecksode im Netzwerk zu finden, sollte ARDUINO_HOSTNAME definiert werden
* Es gibt verschiedene Vordefinitionen für verschiedene Steckdosen, die passende ist einzukommentieren
* kann sich die Dose nicht ins Netzwerk einloggen, schaltet diese in den AP-Modus (AP-Name ist der in ARDUINO_HOSTNAME definiert wurde) und ist unter 192.168.4.1 erreichbar

Bei upload auf die Sonoff muß der Button gedrück werden und dann Strom geschaltet werden - damit geht der ESP8266 in den Programiermodus.
Beim Horsky ist der Port IO0 mit GND zu verbinden.

Nach dem Übertragen startet der ESP8266 neu und meldet sich im angegeben Netzwerk an. Mit der Seriellen Konsole kann der Bootvorgang beobachtet werden.

Ist der Steckdose erfolgreich im heimischen Netz können weitere updates per OTA gemacht werden.
![](https://github.com/polygontwist/ESP_sonoff_Schaltuhr/blob/master/bilder/ota-port.png)

# Oberfläche nach den upload von CSS & JS
Wenn die Dose erfolgreich programmiert wurde, sind die Daten im Ordner www-data mit der upload-Funktion hochzuladen. Wer möchte kann das natürlich je nach Bedarf anpassen. Ist dies erfolgt, sieht das ganze etwa so aus:

![](https://github.com/polygontwist/ESP_sonoff_Schaltuhr/blob/master/bilder/screen01.png)

Hier habe ich schon Zeitpunkte definiert, diese werden in der Datei timer.txt abgelegt.

# Web-API
folgende Befehle stehen zur Verfügung:
* *[ip]/action?sonoff=ON* Relais einschalten
* *[ip]/action?sonoff=OFF* Relais ausschalten
* *[ip]/action?sonoff=LEDON* LED einschalten
* *[ip]/action?sonoff=LEDOFF* LED ausschalten
* *[ip]/data.json* Rückgabe ein JSON-Formatierte Systeminfo (Relaystatus, LEDstatus, Dateien,...)
* *[ip]/timer.txt* (wenn vorhanden) die definierten Timer
* *[ip]/favicon.ico?delete=/favicon.ico* hiermit kann eine Datei gelöscht werden
* *[ip]/upload* Schnittstelle zum hochladen von Dateien (per upload-Formular)
  
## timer.txt
jede Zeite repräsentiert einen Timereintrage z.B. on|07:05|31|ON|t1
* der 1. Wert sagt ob der Timer aktiv ist (on=aktiv)
* der 2. Wert gibt den Zeitpunkt der Ausführung an, in Stunden:Minute (hh:mm)
* der 3. Wert gibt an an welchem Tag der Timer aktiv sein soll, als byte (1=Mo, 2=Di, 4=Mi, 8=Do, 16=Fr, 32=Sa, 64=So) z.B. 31=Montag bis Freitag
* der 4. Wert beschreibt den Befehl (ON,OFF,LEDON,LEDOFF)
* der 5. Wert ist die ID des Timers
  
## History
* Ab Version 2.0: wenn der erste Versuch nicht geklappt hat, versucht die Dose es noch 15 mal (alle 15 Sekunden). Gelingt keine Verbindung bleibt sie im AP-Modus. 
Fällt zwischendurch das WLAN aus, versucht die Dose einen reconnect (10mal, alle 15 Sek.), gelinkt keine Verbindung bleibt sie im AP-Modus.
* Version 2.1: HTML Umlaute codiert
* Version 2.2: mit "#define keinAPModus true" versucht die Dose endlos eine Neuverbindung, wenn WLan-Verbundung unterbrochen wurde.
* Version 2.3: größere (check_wlanasclient) und variablere WLAN-check, um bei Wiederverbindung AP nicht zu überlasten
  
 ## TODO / was ist noch zu tun
 * WLAN-Einstellungen dynamisch anpassbar machen (siehe esp32-32x32rgbmatrix)


## Beispiel Lampe
Wer die Daten aus dem Ordner "beispiel_lampe" hochläd, erhält folgende Features:
* Einstellungen in Tabs gruppiert
* LED schalten ausgeblendet

![](https://github.com/polygontwist/ESP_sonoff_Schaltuhr/blob/master/bilder/screenshots_lampe.png)
 
 