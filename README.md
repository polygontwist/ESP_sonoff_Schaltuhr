# ESP Schaltuhr

Alternative Firmware für WLAN-Steckdosen (Sonoff, Horskey, Lingan SWA1) mit folgenden Funktionen:
* Schalten per WLAN-Verbindung
* definieren von Timern (Schaltuhrfunktion)
* Uhr wird per NTP (Network Time Protocol) aktuell gehalten, Server ist 0.europe.pool.ntp.org (siehe myNTP.h)
* automatische Umschaltung Sommer-/Winterzeit
* update per OTA möglich (Arduino)
* Oberfläche per Javascript und CSS gestaltbar

# Hardwaresetup
ja nach Schaltdose sind die Pins zu seriellen Verbindung zu finden:
## Sonoff S20
![](https://raw.githubusercontent.com/polygontwist/ESP_sonoff_Schaltuhr/master/bilder/sonoff_platine.png)
Das Relais hängt an Port 12, die Status LED an Port 13 und der Button an Port 0.

## HORSKEY Wifi
![](https://raw.githubusercontent.com/polygontwist/ESP_sonoff_Schaltuhr/master/bilder/platine_horsky_Lingan-SWA1.png)
Das Relais hängt an Port 5, die Status LED an Port 4 und der Button an Port 13. Um den Button zu benutzen muß der Mode auf INPUT_PULLUP gesetzt werden (Eingang mit internen Pullupwiederstand).

Zur Verbindung mit dem Rechenr benutze ich ein USB-to-Seriel-Adapter. Dort ist TX,RX,GND und 3,3V gefordert.

# Quellen und Inspirationen
folgende Projekte waren für die Entwicklung hilfreich: 
* https://github.com/arendst/Sonoff-Tasmota/wiki
* http://tinkerman.cat/s20-smart-socket/#lightbox-gallery-pgrT/4/
* https://github.com/arendst/Sonoff-Tasmota/wiki/Lingan-SWA1
* https://nathan.chantrell.net/20170528/lingan-swa1-plug-in-wi-fi-socket-with-mqtt-esp8266/
* http://ganzprivat.de/2017/12/16/ligan-swa1-mit-temperatursensor/
* https://github.com/martin-ger/ESP8266-WiFi-Socket


# Installation mit Arduino IDE 1.8.5
* zuerst müssen die Zeilen #define WIFI_SSID und #define WIFI_PASSWORD aus kommentiert und befüllt werden
* die Zeile #include "wifisetup.h" ist auszukommentieren (ich habe dort meine Wifi-Setting abgelegt)
* Einstellung: Generic ESP8266 Modul, 80 MHZ, 40MHz, DOUT, 115200, 1M (64k SPIFFS)
* Um die Stecksode im Netzwerk zu finden kann man den ARDUINO_HOSTNAME definieren
* Es gibt verschiedene Vordefinitionen für verschiedene Steckdosen, die passende ist einzukommentieren

Bei upload auf die Sonoff muß der Button gedrück werden und dann Strom geschaltet werden - damit geht der ESP8266 in den Programiermodus.
Beim Horskey ist der Port IO0 mit GND zu verbinden.

Nach dem Übertragen startet der ESP8266 neu und meldet sich im angegeben Netzwerk an. Mit der Seriellen Konsole kann der Bootvorgang beobachtet werden.

Ist der Steckdose erfolgreich im heimischen Netz können weitere updates per OTA gemacht werden.
![](https://github.com/polygontwist/ESP_sonoff_Schaltuhr/blob/master/bilder/ota-port.png)

# Oberfläche nach den upload von CSS & JS
Wenn die Dose erfolgreich programmiert wurde, sind die Daten im Ordner www-data mit der upload-Funktion hochzuladen. Ist dies erfolgt, sieht das ganze etwas so aus:
![](https://github.com/polygontwist/ESP_sonoff_Schaltuhr/blob/master/bilder/screen01.png)
Hier habe ich schon Zeitpunkte definiert, diese werden in der Date timer.txt abgelegt.

