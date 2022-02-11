/*
 * ESP32 WROVER Module
 * Partition Scheme:
 *  Huge APP(3MB No OTA/1MB SPISS)
 *  
 *  
 * 
 * 
 * zender_data.csv
 * geen header
 * kolom 1  >>  zendernaam
 * kolom2   >>  zender url
 * 
 * 
 * Switch S1: 1-OFF, 2-ON, 3-ON, 4-OFF, 5-OFF
*/

#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <Preferences.h>
#include "FS.h"
#include "SD.h"
#include <CSV_Parser.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "Wire.h"
#include "ES8388.h"

static ES8388 dac;                                 
int volume = 80;

Audio audio;
Preferences pref;
AsyncWebServer server(80);

#define SD_CS               13
#define SPI_MOSI            15
#define SPI_MISO             2
#define SPI_SCK             14

#define I2S_DSIN            26
#define I2S_BCLK            27
#define I2S_LRC             25
#define I2S_MCLK             0
#define I2S_DOUT            35

// I2C GPIOs
#define IIC_CLK             32
#define IIC_DATA            33

#define PA_EN               21

#define MAX_AANTAL_KANALEN  75

int gekozen = 1;
int keuze = 1;
int row;
int volume_keuze;
int volume_gekozen;
int laag_keuze;
int laag_gekozen;
int midden_keuze;
int midden_gekozen;
int hoog_gekozen;
int hoog_keuze;
int array_index = (MAX_AANTAL_KANALEN - 1);
int teller = 0;
unsigned long wacht_op_netwerk;
unsigned long wachttijd = millis();
bool kiezen = false;
bool webradio = false;
bool schrijf_csv = false;
bool netwerk;
char password[40];
char ssid[40];
char charZenderFile[12];
char speler[20];
char zendernaam[40];
char charUrlFile[12];
char url[100];
const char* KEUZEMIN_INPUT = "minKeuze";
const char* KEUZEPLUS_INPUT = "plusKeuze";
const char* BEVESTIGKEUZE_INPUT ="bevestigKeuze";
const char* LAAG = "laag_keuze";
const char* MIDDEN = "midden_keuze";
const char* HOOG = "hoog_keuze";
const char* VOLUME = "volume_keuze";
const char* VOLUME_BEVESTIG = "bevestig_volume";
const char* APssid = "ESP32webradio";
const char* APpswd = "ESP32pswd";
const char* STA_SSID = "ssid";
const char* STA_PSWD = "pswd";
const char* ZENDER = "zender";
const char* URL = "url";
const char* ARRAY_MIN = "array_index_min";
const char* ARRAY_PLUS = "array_index_plus";
const char* BEVESTIG_ZENDER = "bevestig_zender";
String zenderarray[MAX_AANTAL_KANALEN];
String urlarray[MAX_AANTAL_KANALEN];
String inputString =     "                                                                                                                                                                                                        ";
String zenderFile =      "           ";
String urlFile =         "           ";
String streamsong =      "                                                                                                                                                                                                        ";


/*
 * volgende 5 functies komen van de SD library
 */
void readFile(fs::FS &fs, const char * path){
  File file = fs.open(path);
    if(!file){
      return;
    }
  teller = 0;
  inputString = "";
  while(file.available()){
    inputString += char(file.read());
    teller++;
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char * path){
  fs.remove(path);
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  File file = fs.open(path, FILE_APPEND);
  file.print(message);
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    return;
  }
  file.print(message);
  file.close();
}

void audio_showstreamtitle(const char *info){
  if(kiezen == false){
    streamsong = info;
  }
}

/*
 * verbinding maket met het gekozen radiostation
 */
void radio_gekozen(){
  urlarray[keuze].toCharArray(url, urlarray[keuze].length() + 1);
  zenderarray[keuze].toCharArray(zendernaam, zenderarray[keuze].length() + 1);
  audio.stopSong();
  delay(100);
  audio.connecttohost(url);
  kiezen = false;
}

/*
 * de via de webpagina aangebrachte veranderingen in de zender en url array
 * wegschrijven naar de /zender_data.csv file 
 */
void schrijf_naar_csv(){
  char terminator = char(0x0a);
  String datastring = "                                                                                                                                                             ";
  char datastr[150];
  deleteFile(SD, "/zender_data.csv");
  for(int x = 0; x < 75; x++){
    datastring = zenderarray[x] + "," + urlarray[x] + String(terminator);
    datastring.toCharArray(datastr, (datastring.length() + 1));
    appendFile(SD, "/zender_data.csv", datastr);
  }
  lees_CSV();
}

/*
 * Inlezen CSV file naar zender en url arry
 */
void lees_CSV(){
  CSV_Parser cp("ss", false, ',');
  if(cp.readSDfile("/zender_data.csv")){
    char **station_naam = (char**)cp[0];  
    char **station_url = (char**)cp[1]; 
    for(row = 0; row < cp.getRowsCount(); row++){
      zenderarray[row] = station_naam[row];
      urlarray[row] = station_url[row];
    }
  }
}


/*
 * HTML bediening webradio
 */
const char index_html[] = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <iframe style="display:none" name="hidden-form"></iframe>
    <title>Internetradio bediening</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
    <h3><center> ESP32 internetradio webinterface </center></h3>
    <h5><center> %zenderNu% </center></h5>
    <p><small><center>%song%</center></small></p>
    <center>
      <input type="text" style="text-align:center;" value="%selecteren%" name="keuze" size=30>
    </center>
      <br>
    <form action="/get" target="hidden-form">
    <center>
      <input type="submit" name="minKeuze" value="   -   " onclick="bevestig()">
      &nbsp;&nbsp;&nbsp;
      <input type="submit" name="plusKeuze" value="   +   " onclick="bevestig()">
      &nbsp;&nbsp;&nbsp;
      <input type="submit" name="bevestigKeuze" value="OK" onclick="bevestig()">
    </center>
    </form>
    <br>
    <p><small><center><b>EQ -40 <-> 6 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Volume 0 <->21</b></center></small></p>
    <form action="/get" target="hidden-form">
    <small>
    <center>
      <labelfor="dummy">L :</label>
      <input type="text" style="text-align:center;" value="%laag_kiezen%"   name="laag_keuze"   size=1>
      &nbsp;&nbsp;
      <labelfor="dummy">M :</label>
      <input type="text" style="text-align:center;" value="%midden_kiezen%" name="midden_keuze" size=1>
      &nbsp;&nbsp;
      <labelfor="dummy">H :</label>
      <input type="text" style="text-align:center;" value="%hoog_kiezen%"   name="hoog_keuze"   size=1>
      &nbsp;&nbsp;
      <labelfor="dummy">V :</label>
      <input type="text" style="text-align:center;" value="%volume_kiezen%" name="volume_keuze" size=1>
    </center>
    </small>
    <br>
    <center>
      <input type="submit" name="bevestig_volume" value="OK" onclick="bevestig()">
    </center>
    </form>
    <br>
    <h5><center> Instellen zender en url : %array_index% </center></h5>
    <form action="/get" target="hidden-form">
    <center>
    <input type= "text" style="text-align:center;" value="%zender%" name="zender" size = 30>
    </center>
     <br>
    <center>
    <input type= "text" style="text-align:center;" value="%url%" name="url" size = 40>
    </center>
    <br>
    <center>
      <input type="submit" name="array_index_min" value="   -   " onclick="bevestig()">
      &nbsp;&nbsp;&nbsp;
      <input type="submit" name="array_index_plus" value="   +   " onclick="bevestig()">
      &nbsp;&nbsp;&nbsp;
      <input type="submit" name="bevestig_zender" value="OK" onclick="bevestig()">
    </center>
    </form>
    <br><br><br><br><br>
    <h6>thieu februari 2022</h6>
    <script>
      function bevestig(){
        setTimeout(function(){document.location.reload();},250);
      }
    </script>
  </body> 
</html>
)rawliteral";


/*
 * HTML voor ingeven ssid en pswd
 */
const char netwerk_html[] = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <iframe style="display:none" name="hidden-form"></iframe>
    <title>Internetradio bediening</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    
  </head>
  <body>
    <h3><center><strong>ESP32 Netwerk instellingen</strong></center></h3>
    <br>
    <form action="/get">
    <p><small>
    <labelfor="dummy">ssid :</label>
    <center><input type= "text" name="ssid"></center>
    <br>
    <labelfor="dummy">pswd :</label> 
    <center><input type= "text" name="pswd"></center>
    <br>
    <center><input type="submit" value="Bevestig" onclick="ok()"></center>
    </small></p>
    </form>
    <script>
      function ok(){
        setTimeout(function(){document.location.reload();},250);
      }
    </script>
  </body>  
</html>
)rawliteral";


String processor(const String& var){
  if(var == "zenderNu"){
    return(zenderarray[gekozen]);
    }
  if(var == "song"){
    return(streamsong);
  }
  else if(var == "selecteren"){
    return(zenderarray[keuze]);
  }
  if(var == "laag_kiezen"){
    return(String(laag_gekozen));
  }
  if(var == "midden_kiezen"){
    return(String(midden_gekozen));
  }
  if(var == "hoog_kiezen"){
    return(String(hoog_gekozen));
  }
  if(var == "volume_kiezen"){
    return(String(volume_gekozen));
  }
  if(var == "array_index"){
    return(String(array_index));
  }
  else if(var == "zender"){
    return(zenderarray[array_index]);
  }
  else if(var == "url"){
    return(urlarray[array_index]);
  }
  return String();
}


void html_input(){
  server.begin();
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
  if(netwerk == true){
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, processor);
    });
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
      String http_zender = "                         ";
      String http_url = "                                                                             ";
      char terminator = char(0x0a);
      if(request->hasParam(KEUZEMIN_INPUT)){
        keuze--;
        while((urlarray[keuze].length() < 5) && (keuze != -3)){
          keuze --;
        }
        if(keuze < 0){
          keuze = MAX_AANTAL_KANALEN - 1;
            while((urlarray[keuze].length() < 5) && (keuze > -1)){
              keuze --;
            }
        }
      wachttijd = millis();
      kiezen = true;
      }
      if(request->hasParam(KEUZEPLUS_INPUT)){
        keuze++;
        while((urlarray[keuze].length() < 5) && (keuze < MAX_AANTAL_KANALEN)){
          keuze ++;
        }
        if(keuze > MAX_AANTAL_KANALEN - 1){
          keuze = 0;
        }
      }
      wachttijd = millis();
      kiezen = true;
      if((request->hasParam(BEVESTIGKEUZE_INPUT)) && (kiezen == true)){
        kiezen = false;
        gekozen = keuze;
        pref.putShort("station", gekozen);
        webradio = true;
      }
      if(request->hasParam(LAAG)){
        laag_keuze = ((request->getParam(LAAG)->value()) + String(terminator)).toInt();
        
      }
      if(request->hasParam(MIDDEN)){
        midden_keuze = ((request->getParam(MIDDEN)->value()) + String(terminator)).toInt();
      }
      if(request->hasParam(HOOG)){
        hoog_keuze = ((request->getParam(HOOG)->value()) + String(terminator)).toInt();
      }
      if(request->hasParam(VOLUME)){
        volume_keuze = ((request->getParam(VOLUME)->value()) + String(terminator)).toInt();
      }
      if(request->hasParam(VOLUME_BEVESTIG)){
        if((laag_keuze > -41) && (laag_keuze < 7)){
          laag_gekozen = laag_keuze;
        }
        if((midden_keuze > -41) && (midden_keuze < 7)){
          midden_gekozen = midden_keuze;
        }
        if((laag_keuze > -41) && (laag_keuze < 7)){
          hoog_gekozen = hoog_keuze;
        }
        if((volume_keuze > -1) && (laag_keuze < 22)){
          volume_gekozen = volume_keuze;
        }
        audio.setVolume(volume_gekozen);
        audio.setTone(laag_gekozen, midden_gekozen, hoog_gekozen);
        pref.putShort("laag", laag_gekozen);
        pref.putShort("midden", midden_gekozen);
        pref.putShort("hoog", hoog_gekozen);
        pref.putShort("volume", volume_gekozen);
      }
      if(request->hasParam(ARRAY_MIN)){
        array_index -= 1;
        if(array_index < 0){
          array_index = MAX_AANTAL_KANALEN - 1;
        }
      }
      if(request->hasParam(ARRAY_PLUS)){
        array_index += 1;
        if(array_index > MAX_AANTAL_KANALEN - 1){
          array_index = 0;
        }
      }
      if(request->hasParam(ZENDER)){
        http_zender = (request->getParam(ZENDER)->value());
      }
      if(request->hasParam(URL)){
        http_url = (request->getParam(URL)->value());
      }
      if(request->hasParam(BEVESTIG_ZENDER)){
        zenderarray[array_index] = http_zender;
        urlarray[array_index] = http_url; 
        schrijf_csv = true; 
      }
    });
  }
  else{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", netwerk_html, processor);
    });
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
      String netwerk = "                         ";
      String paswoord = "                          ";
      char terminator = char(0x0a);
      if(request->hasParam(STA_SSID)){
        netwerk = (request->getParam(STA_SSID)->value());
        netwerk = netwerk + String(terminator);
        netwerk.toCharArray(ssid, (netwerk.length() +1));
        writeFile(SD, "/ssid", ssid);
      }
      if(request->hasParam(STA_PSWD)){
        paswoord = (request->getParam(STA_PSWD)->value());
        paswoord = paswoord + String(terminator);
        paswoord.toCharArray(password, (paswoord.length() + 1));
        writeFile(SD, "/pswd", password);
      }
    });
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  pinMode(PA_EN, OUTPUT);
  digitalWrite(PA_EN, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  if(!SD.begin(SD_CS)){
    Serial.println("check SD kaart");
  }
  while (not dac.begin(IIC_DATA, IIC_CLK)){
    Serial.println("dac verbindinding mislukt");
    delay(1000);
  }
  audio.i2s_mclk_pin_select(I2S_MCLK);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DSIN);
  /*
   * zendertabel inlzen
   */
  lees_CSV();
  /*
   * Voorinstellingen inlezen
   */
  pref.begin("WebRadio", false); 
  if(pref.getShort("station", 1000) == 1000){
    pref.putShort("station", 1);
  }
  else{
    gekozen = pref.getShort("station");
  }
  if(pref.getShort("volume", 1000) == 1000){
    pref.putShort("volume", 10);
  }
  else{
    volume_gekozen = pref.getShort("volume");
    volume_keuze = volume_gekozen;
  }
  if(pref.getShort("laag", 1000) == 1000){
    pref.putShort("laag", 0);
  }
  else{
    laag_gekozen = pref.getShort("laag");
    laag_keuze = laag_gekozen;
  }
  if(pref.getShort("midden", 1000) == 1000){
    pref.putShort("midden", 0);
  }
  else{
    midden_gekozen = pref.getShort("midden");
    midden_keuze = midden_gekozen;
  }
  if(pref.getShort("hoog", 1000) == 1000){
    pref.putShort("hoog", 0);
  }
  else{
    hoog_gekozen = pref.getShort("hoog");
    hoog_keuze = hoog_gekozen;
  }
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(volume_gekozen);
  audio.setTone(laag_gekozen, midden_gekozen, hoog_gekozen);
  keuze = gekozen;
  /*
   * WiFi opstarten
   */
  readFile(SD, "/ssid");
  inputString.toCharArray(ssid, teller);
  readFile(SD, "/pswd");
  inputString.toCharArray(password, teller);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.softAP(APssid, APpswd);
  netwerk = true;
  wacht_op_netwerk = millis();
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    if(millis() - wacht_op_netwerk > 15000){
      netwerk = false;
      Serial.println("Geen internet verbinding");
      break;
    }
  }
  if(netwerk){  
    IPAddress subnet(WiFi.subnetMask());
    IPAddress gateway(WiFi.gatewayIP());
    IPAddress dns(WiFi.dnsIP(0));
    IPAddress static_ip(192,168,1,177);
    WiFi.disconnect();
    if (WiFi.config(static_ip, gateway, subnet, dns, dns) == false) {
      Serial.println("WiFi configuratie mislukt");
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    wacht_op_netwerk = millis();
    while(WiFi.status() != WL_CONNECTED){
      delay(500);
      if(millis() - wacht_op_netwerk > 15000){
        netwerk = false;
        Serial.println("Geen internet verbinding vast IP");
        break;
      }
    }
    html_input();
  }
  if(!netwerk){
    WiFi.mode(WIFI_AP);
    WiFi.softAP(APssid, APpswd);
    html_input();
    while(!netwerk){
      yield();
    }
  }
  radio_gekozen();
}


void loop(){
  if(schrijf_csv == true){
    schrijf_csv = false;
    schrijf_naar_csv();
  }
  if(webradio == true){
    webradio = false;
    gekozen = keuze;
    pref.putShort("station", gekozen);
    radio_gekozen();
  }
  if(((millis() - wachttijd) > 5000) && (kiezen == true)){
    kiezen = false;
    keuze = gekozen;
  }
  audio.loop();
}
