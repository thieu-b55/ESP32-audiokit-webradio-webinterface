/*
 * Arduino Board Settings:
 * ESP32 WROVER Module
 * Partition Scheme:
 *  Huge APP(3MB No OTA/1MB SPISS)
 * 
 * Audio librarie
 * https://github.com/schreibfaul1/ESP32-audioI2S
 * 
 * es8388 librarie:
 * https://github.com/maditnerd/es8388
 * 
 *  
 * zender_data.csv
 * geen header
 * kolom 1  >>  zendernaam
 * kolom2   >>  zender url
*/

#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
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

/*
 * Settings voor ESP32-A1S v2.2 (ES8388)
 * switch 2 & 3 ON
 *        1  4  5 OFF
 */
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
int volgend;
int totaalmp3;
int eerste;
int tweede;
int songindex;
int row;
int volume_keuze;
int volume_gekozen;
int laag_keuze;
int laag_gekozen;
int midden_keuze;
int midden_gekozen;
int hoog_gekozen;
int hoog_keuze;
int mp3_per_songlijst;
int array_index = MAX_AANTAL_KANALEN - 1;
int songlijst_index_vorig;
int songlijst_index;
int mp3_folder_teller;
int teller = 0;
int mp3_aantal;
int gn_keuze = 0;
unsigned long wacht_op_netwerk;
unsigned long inlezen_begin;
unsigned long inlezen_nu;
unsigned long wachttijd = millis();
bool kiezen = false;
bool lijst_maken = false;
bool speel_mp3 = false;
bool webradio = false;
bool schrijf_csv = false;
bool netwerk;
bool nog_mp3;
bool mp3_ok;
bool mp3_lijst_maken = false;
bool ssid_ingevuld = false;
bool pswd_ingevuld = false;
bool songlijsten = false;
char songfile[200];
char mp3file[200];
char song[200];
char datastring[200];
char password[40];
char ssid[40];
char charZenderFile[12];
char speler[20];
char gn_actie[20];
char gn_selectie[20];
char zendernaam[40];
char charUrlFile[12];
char url[100];
char mp3_dir[10];
char folder_mp3[10];
char aantal_mp3[10];
char songlijst_dir[12];
char totaal_mp3[15];
char mp3_lijst_folder[10];
char mp3_lijst_aantal[5];
char leeg[0];
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
const char* MIN_INPUT = "min";
const char* PLUS_INPUT = "plus";
const char* BEVESTIG_MP3 ="bevestig_mp3";
String zenderarray[MAX_AANTAL_KANALEN];
String urlarray[MAX_AANTAL_KANALEN];
String songstring =      "                                                                                                                                                                                                        ";
String inputString =     "                                                                                                                                                                                                        ";
String mp3titel =        "                                                                                                                                                                                                        ";      
String zenderFile =      "           ";
String urlFile =         "           ";
String maxurl =          "           ";
String totaal =          "           ";
String streamsong =      "                                                                                                                                                                                                        ";
String mp3_folder =      "                   ";
String songlijst_folder = "                   ";
String mp3test = "mp3";



void readFile(fs::FS &fs, const char * path){
    File file = fs.open(path);
    if(!file){
      Serial.println("Kan file niet openen om te lezen : ");
      Serial.println(path);
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
    Serial.println("Kan file niet openen om te schrijven : ");
      Serial.println(path);
      return;
  }
  file.print(message);
  file.close();
}

void createDir(fs::FS &fs, const char * path){
  File root = fs.open(path);
  if(root){
    songlijsten = true;
    lijst_maken = false;
    while(1){
      yield();
    }
  }
  else{
    fs.mkdir(path);
  }
}

void audio_showstreamtitle(const char *info){
  if(kiezen == false){
    streamsong = info;
  }
}

void audio_eof_mp3(const char *info){
  mp3_volgend();
  streamsong = mp3titel.substring(0, (mp3titel.length() - 4));
}

void files_in_mp3_0(fs::FS &fs, const char * dirname, uint8_t levels){
  File root = fs.open(dirname);
  if(!root){
    Serial.println("Geen mp3_0 folder");
    return;
  }
  File file = root.openNextFile();
  mp3_per_songlijst = 0;
  while(file){
    file = root.openNextFile();
    mp3_per_songlijst ++;
  }
  pref.putShort("files", mp3_per_songlijst);
  Serial.println("files in mp3_0");
  Serial.println(mp3_per_songlijst);
}

void maak_lijst(fs::FS &fs, const char * dirname){
  File root = fs.open(dirname);
  if(!root){
    nog_mp3 = false;
    return;
  }
  File file = root.openNextFile();
  while(file){
    songlijst_index =  mp3_aantal / mp3_per_songlijst;
    if(songlijst_index != songlijst_index_vorig){
      songlijst_index_vorig = songlijst_index;
      songlijst_folder = "/songlijst" + String(songlijst_index);
      songlijst_folder.toCharArray(songlijst_dir, (songlijst_folder.length() + 1));
      createDir(SD, songlijst_dir);
    }
    songstring = file.name();
    songlijst_folder = "/songlijst" + String(songlijst_index) + "/s" + String(mp3_aantal);
    songlijst_folder.toCharArray(songlijst_dir, (songlijst_folder.length() + 1));
    songstring = file.name();
    songstring.toCharArray(song, (songstring.length() + 1));
    writeFile(SD, songlijst_dir, song);
    file = root.openNextFile();
    mp3_aantal ++;
  }
}

void mp3_lijst_maken_gekozen(){
  inlezen_begin = millis();
  files_in_mp3_0(SD, "/mp3_0", 1);
  mp3_aantal = 0;
  nog_mp3 = true;
  mp3_folder_teller = 0;
  songlijst_index_vorig = -1;
  while(nog_mp3){
    mp3_folder = "/mp3_" + String(mp3_folder_teller);
    inlezen_nu = millis() - inlezen_begin;
    Serial.println(mp3_aantal);
    Serial.println(inlezen_nu);
    mp3_folder.toCharArray(mp3_dir, (mp3_folder.length() + 1));
    maak_lijst(SD, mp3_dir);
    mp3_folder_teller ++;
  }
  String(mp3_aantal).toCharArray(totaal_mp3, (String(mp3_aantal - 1).length() +1));
  writeFile(SD, "/totaal", totaal_mp3);
  int verstreken_tijd = (millis() - inlezen_begin) / 1000;
  Serial.println("verstreken tijd");
  Serial.println(verstreken_tijd);
  lijst_maken = false;
  keuze = -1;
  gekozen = -1;
  mp3_gekozen();
}

void mp3_gekozen(){
  readFile(SD, "/totaal");
  totaal = inputString.substring(0, teller);
  totaalmp3 = totaal.toInt();
  mp3_volgend();
  streamsong = mp3titel.substring(0, (mp3titel.length() - 4));
}

void mp3_volgend(){
  mp3_ok = false;
  while(mp3_ok == false){
    volgend = random(totaalmp3);
    songindex = volgend / mp3_per_songlijst;
    songstring = "/songlijst" + String(songindex) + "/s" + String(volgend);
    songstring.toCharArray(songfile, (songstring.length() +1));
    readFile(SD, songfile);
    inputString.toCharArray(mp3file, inputString.length() + 1);
    mp3_ok = audio.connecttoFS(SD, mp3file);
  }
  songstring = String(mp3file);
  eerste = songstring.indexOf("/");
  tweede = songstring.indexOf("/", eerste + 1);
  mp3titel = songstring.substring(tweede + 1);
}

void radio_gekozen(){
  urlarray[keuze].toCharArray(url, urlarray[keuze].length() + 1);
  zenderarray[keuze].toCharArray(zendernaam, zenderarray[keuze].length() + 1);
  audio.stopSong();
  delay(100);
  audio.connecttohost(url);
  kiezen = false;
}

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
    <p><small><b><center>%tekst1%</center></b></small></p>
    <p><small><center>%tekst2%</center></small></p>
    <p><small><b><center>%tekst3%</center></b></small></p>
    <p><small><center>%tekst4%</center></small></p>
    <p><small><b><center>%tekst5%</center></b></small></p>
    <p><small><center>%tekst6%</center></small></p>
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
    <br>
    <br>
    <h6>thieu februari 2022</h6>
    <script>
      function bevestig(){
        setTimeout(function(){document.location.reload();},250);
      }
    </script>
  </body>  
</html>
)rawliteral";

const char netwerk_html[] = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <iframe style="display:none" name="hidden-form"></iframe>
    <title>Internetradio bediening</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
    <p><small><center>%song%</center></small></p>
    <center>
      <input type="text" style="text-align:center;" value="%selectie%" name="keuze" size=30>
    </center>
      <br>
    <form action="/get" target="hidden-form">
    <center>
      <input type="submit" name="min" value="   -   " onclick="ok()">
      &nbsp;&nbsp;&nbsp;
      <input type="submit" name="plus" value="   +   " onclick="ok()">
      &nbsp;&nbsp;&nbsp;
      <input type="submit" name="bevestig_mp3" value="OK" onclick="ok()">
    </center>
    </form>
    <br>
    <p><small><b><center>%tekst1%</center></b></small></p>
    <p><small><center>%tekst2%</center></small></p>
    <p><small><b><center>%tekst3%</center></b></small></p>
    <p><small><center>%tekst4%</center></small></p>
    <p><small><b><center>%tekst5%</center></b></small></p>
    <p><small><center>%tekst6%</center></small></p>
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
      <input type="submit" name="bevestig_volume" value="OK" onclick="ok()">
    </center>
    </form>
    <br><br>
    <h5><center><strong>ESP32 Netwerk instellingen</strong></center></h5>
    <form action="/get">
    <table style="width:100%;">
      <tr>
        <td style="text-align:left;"><p><small><b><labelfor="dummy">ssid :</label></b></small></p></td>
        <td style="text-align:center;"><input type= "text" name="ssid"></td>
      </tr>
      <tr>
        <td style="text-align:left;"><p><small><b><labelfor="dummy">pswd : </label></b></small></p></td>
        <td style="text-align:center;"><input type= "text" name="pswd"></td>
      </tr>
    </table>
    <center><input type="submit" value="Bevestig" onclick="ok()"></center>
    </form>
    <br>
    <script>
      function ok(){
        setTimeout(function(){document.location.reload();},250);
      }
    </script>
  </body>  
</html>
)rawliteral";

String processor(const String& var){
  char char_tekst1[30];
  char char_tekst2[30];
  char char_tekst3[30];
  char char_tekst4[30];
  char char_tekst5[30];
  char char_tekst6[30];
  char char_tekst7[40];
  if(var == "zenderNu"){
    if(gekozen == -2){
      String mp3_lijst = "mp3 lijst maken";
      mp3_lijst.toCharArray(speler, (mp3_lijst.length() + 1));
      return(speler);
    }
    else if(gekozen == -1){
      String mp3_speler = "mp3 speler";
      mp3_speler.toCharArray(speler, (mp3_speler.length() + 1));
      return(speler);
    }
    else{
      return(zenderarray[gekozen]);
    }
  }
  if(var == "song"){
    return(streamsong);
  }
  if(var == "selectie"){
    if(gn_keuze == 0){
      String selectie = "Stop mp3 speler";
      selectie.toCharArray(gn_selectie, (selectie.length() + 1));
      return(gn_selectie);
    }
    if(gn_keuze == 1){
      String selectie = "mp3 speler";
      selectie.toCharArray(gn_selectie, (selectie.length() + 1));
      return(gn_selectie);
    }
    if(gn_keuze == 2){
      String selectie = "Maak mp3 lijst";
      selectie.toCharArray(gn_selectie, (selectie.length() + 1));
      return(gn_selectie);
    }
  }
  if(var == "selecteren"){
    if(keuze == - 2){
      String mp3_lijst = "mp3 lijst maken";
      mp3_lijst.toCharArray(speler, (mp3_lijst.length() + 1));
      return(speler);
    }
    else if((keuze == -1) || (gn_keuze == 1)){
      String mp3_speler = "mp3 speler";
      mp3_speler.toCharArray(speler, (mp3_speler.length() + 1));
      return(speler);
    }
    else{
      return(zenderarray[keuze]);
    }
  }
  if(var == "tekst1"){
    if((!lijst_maken) && (!songlijsten)){
      return(leeg);
    }
    if(lijst_maken){
      String tekst1 = "inlezen van : ";
      tekst1.toCharArray(char_tekst1, (tekst1.length() + 1));
      return(char_tekst1);
    }
    if(songlijsten){
      String tekst7 = "EERST ALLE SONGLIJSTXX VERWIJDEREN";
      tekst7.toCharArray(char_tekst7, (tekst7.length() + 1));
      return(char_tekst7);
    }
  }
  if(var == "tekst2"){
    if(!lijst_maken){
      return(leeg);
    }
    else{
      mp3_folder.toCharArray(char_tekst2, (mp3_folder.length() + 1));
      return(char_tekst2);;
    }
  }
 if(var == "tekst3"){
    if(!lijst_maken){
      return(leeg);
    }
    else{
      String tekst3 = "aantal mp3's ingelezen : ";
      tekst3.toCharArray(char_tekst3, (tekst3.length() + 1));
      return(char_tekst3);
    }
  }
  if(var == "tekst4"){
    if(!lijst_maken){
      return(leeg);
    }
    else{
      String tekst4 = String(mp3_aantal);
      tekst4.toCharArray(char_tekst4, (tekst4.length() + 1));
      return(char_tekst4);
    }
  }
  if(var == "tekst5"){
    if(!lijst_maken){
      return(leeg);
    }
    else{
      String tekst5 = "seconden reeds bezig : ";
      tekst5.toCharArray(char_tekst5, (tekst5.length() + 1));
      return(char_tekst5);
    }
  }
  
  if(var == "tekst6"){
    if(!lijst_maken){
      return(leeg);
    }
    else{
      int seconden = (millis() - inlezen_begin) / 1000;
      String(seconden).toCharArray(char_tekst6, (String(seconden).length() + 1));
      return(char_tekst6);
    }
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
  if(var == "zender"){
    return(zenderarray[array_index]);
  }
  if(var == "url"){
    return(urlarray[array_index]);
  }
  if(var == "folder"){
    String folder = mp3_folder;
    folder.toCharArray(mp3_lijst_folder, (folder.length() + 1));
    return(mp3_lijst_folder);
  }
  if(var == "mp3"){
    String aantal = String(mp3_aantal);
    aantal.toCharArray(mp3_lijst_aantal, (aantal.length() + 1));
    return(mp3_lijst_folder);
  }
  return String();
}

void html_input(){
  server.begin();
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
  if(netwerk){
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, processor);
    });
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
      String http_zender = "                         ";
      String http_url = "                                                                             ";
      char terminator = char(0x0a);
      wachttijd = millis();
      kiezen = true;
      if(request->hasParam(KEUZEMIN_INPUT)){
        keuze--;
        while((urlarray[keuze].length() < 5) && (keuze != -3)){
          keuze --;
        }
        if(keuze < -2){
          keuze = MAX_AANTAL_KANALEN - 1;
            while((urlarray[keuze].length() < 5) && (keuze > -1)){
              keuze --;
            }
        }
      }
      if(request->hasParam(KEUZEPLUS_INPUT)){
        keuze++;
        while((urlarray[keuze].length() < 5) && (keuze < MAX_AANTAL_KANALEN)){
          keuze ++;
        }
        if(keuze > MAX_AANTAL_KANALEN - 1){
          keuze = -2;
        }
      }
      if((request->hasParam(BEVESTIGKEUZE_INPUT)) && (kiezen == true)){
        kiezen = false;
        if(keuze == -2){
          lijst_maken = true;
        }
        else if(keuze == -1){
          gekozen = keuze;
          speel_mp3 = true;
        }
        else{
          gekozen = keuze;
          pref.putShort("station", gekozen);
          webradio = true;
        }
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
          if((hoog_keuze > -41) && (hoog_keuze < 7)){
            hoog_gekozen = hoog_keuze;
          }
          if((volume_keuze > -1) && (volume_keuze < 22)){
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
  if(!netwerk){
    Serial.println("geen netwerk");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", netwerk_html, processor);
    });
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
      String netwerk = "                         ";
      String paswoord = "                          ";
      char terminator = char(0x0a);
      wachttijd = millis();
      kiezen = true;
      if(request->hasParam(MIN_INPUT)){
        gn_keuze --;
        if(gn_keuze < 0){
          gn_keuze = 2;
        }
      }
      if(request->hasParam(PLUS_INPUT)){
        gn_keuze ++;
        if(gn_keuze > 2){
          gn_keuze = 0;
        }
      }
      if(request->hasParam(BEVESTIG_MP3)){
        if(gn_keuze == 0){
          audio.stopSong();
        }
        if(gn_keuze == 1){
          speel_mp3 = true;
        }
        if(gn_keuze == 2){
          lijst_maken = true;
        }
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
      if(request->hasParam(STA_SSID)){
        netwerk = (request->getParam(STA_SSID)->value());
        netwerk = netwerk + String(terminator);
        netwerk.toCharArray(ssid, (netwerk.length() +1));
        writeFile(SD, "/ssid", ssid);
        ssid_ingevuld = true;
      }
      if(request->hasParam(STA_PSWD)){
        paswoord = (request->getParam(STA_PSWD)->value());
        paswoord = paswoord + String(terminator);
        paswoord.toCharArray(password, (paswoord.length() + 1));
        writeFile(SD, "/pswd", password);
        pswd_ingevuld = true;
      }
      if((ssid_ingevuld) && (pswd_ingevuld)){
        ssid_ingevuld = false;
        pswd_ingevuld = false;
        Serial.println("Restart over 5 seconden");
        delay(5000);
        ESP.restart();
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
  if(pref.getShort("files", 1000) == 1000){
    pref.putShort("files", mp3_per_songlijst);
  }
  else{
    mp3_per_songlijst = pref.getShort("files");
  }
  audio.setVolume(volume_gekozen);
  audio.setTone(laag_gekozen, midden_gekozen, hoog_gekozen);
  lees_CSV();
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
      break;
    }
  }
  if(netwerk == true){  
    IPAddress subnet(WiFi.subnetMask());
    IPAddress gateway(WiFi.gatewayIP());
    IPAddress dns(WiFi.dnsIP(0));
    IPAddress static_ip(192,168,1,177);
    WiFi.disconnect();
    if (WiFi.config(static_ip, gateway, subnet, dns, dns) == false) {
      Serial.println("Configuration failed.");
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    wacht_op_netwerk = millis();
    while(WiFi.status() != WL_CONNECTED){
      delay(500);
      if(millis() - wacht_op_netwerk > 15000){
        netwerk = false;
        break;
      }
    }
    keuze = gekozen;
    radio_gekozen();
    html_input();
  }
  else{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(APssid, APpswd);
    html_input();
  }
}

void loop(){
  if(schrijf_csv == true){
    schrijf_csv = false;
    schrijf_naar_csv();
  }
  if(lijst_maken == true){
    //lijst_maken = false;
    mp3_lijst_maken_gekozen();
  }
  if(speel_mp3 == true){
    speel_mp3 = false;
    mp3_gekozen();
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
