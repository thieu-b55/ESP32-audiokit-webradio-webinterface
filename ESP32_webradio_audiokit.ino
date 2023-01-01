/*
* MIT License
*
* Copyright (c) 2022 thieu-b55
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
* 
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
int ip_1_int = 192;
int ip_2_int = 168;
int ip_3_int = 1;
int ip_4_int = 177;
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
bool songlijst_bestaat_bool;
char ip_char[20];
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
char zenderarray[MAX_AANTAL_KANALEN][40];
char urlarray[MAX_AANTAL_KANALEN][100];
const char* IP_1_KEUZE = "ip_1_keuze";
const char* IP_2_KEUZE = "ip_2_keuze";
const char* IP_3_KEUZE = "ip_3_keuze";
const char* IP_4_KEUZE = "ip_4_keuze";
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
const char* h_char = "h";
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
String ip_1_string = "   ";
String ip_2_string = "   ";
String ip_3_string = "   ";
String ip_4_string = "   ";
String ip_string = "                   ";

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

void readIP(fs::FS &fs, const char * path){
    int temp;
    int temp1;
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
    temp = inputString.indexOf('.');
    ip_1_int = (inputString.substring(0, temp - 1)).toInt();
    temp1 = inputString.indexOf('.', temp + 1);
    ip_2_int = (inputString.substring(temp + 1 , temp1 - 1)).toInt();
    temp = inputString.indexOf('.', temp1 + 1);
    ip_3_int = (inputString.substring(temp1 + 1, temp - 1)).toInt();
    ip_4_int = (inputString.substring(temp + 1, inputString.length() - 1)).toInt();
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

void testDir(fs::FS &fs, const char * path){
  File root = fs.open(path);
  if(root){
    songlijst_bestaat_bool = true;
  }
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
  String(mp3_aantal - 1).toCharArray(totaal_mp3, (String(mp3_aantal - 1).length() +1));
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
  memset(url, 0, sizeof(url));
  strcpy(url, urlarray[keuze]);
  memset(zendernaam, 0, sizeof(zendernaam));
  strcpy(zendernaam, zenderarray[keuze]);
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
  for(int x = 0; x < MAX_AANTAL_KANALEN; x++){
    datastring = String(zenderarray[x]) + "," + String(urlarray[x]) + String(terminator);
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
      memset(zenderarray[row], 0, sizeof(zenderarray[row]));
      strcpy(zenderarray[row], station_naam[row]);
      memset(urlarray[row], 0, sizeof(urlarray[row]));
      strcpy(urlarray[row], station_url[row]);
    }
  }
}

const char index_html[] = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <iframe style="display:none" name="hidden-form"></iframe>
    <title>Internetradio bediening</title>
    <meta name="viewport" content="width=device-width, initial-scale=.85">
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
    <meta name="viewport" content="width=device-width, initial-scale=.85">
    <style>
        div.kader {
          position: relative;
          width: 400px;
          height: 12x;
        }
          div.links{
          position: absolute;
          left : 0px;
          width; auto;
          height: 12px;
        }
        div.links_midden{
          position:absolute;
          left:  80px;
          width: auto;
          height: 12px; 
        }
        div.blanco_20{
          width: auto;
          height: 20px;
        }
        div.blanco_40{
          width: auto;
          height: 40px;
        }
    </style>
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
    <small>
    <div class="kader">
      <div class="links"><b>ssid :</b></div>
      <div class="links_midden"><input type="text" style="text-align:center;" name="ssid"></div>
    </div>
    <div class="blanco_40">&nbsp;</div>
    <div class="kader">
      <div class="links"><b>pswd :</b></div>
      <div class="links_midden"><input type="text" style="text-align:center;" name="pswd"></div>
    </div>
    <div class="blanco_20">&nbsp;</div>
    </small>
    <h5><center>Gewenst IP address (default 192.168.1.177)</center></h5>
    <div class="kader">
      <center>
        <input type="text" style="text-align:center;" value="%ip_address_1%" name="ip_1_keuze" size=1>
        &nbsp;&nbsp;
        <input type="text" style="text-align:center;" value="%ip_address_2%" name="ip_2_keuze" size=1>
        &nbsp;&nbsp;
        <input type="text" style="text-align:center;" value="%ip_address_3%" name="ip_3_keuze" size=1>
        &nbsp;&nbsp;
        <input type="text" style="text-align:center;" value="%ip_address_4%" name="ip_4_keuze" size=1>
      </center>
    </div>
    <div class="blanco_20">&nbsp;</div>
    </small>
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
  if(var == "ip_address_1"){
    return(String(ip_1_int));
  }
  if(var == "ip_address_2"){
    return(String(ip_2_int));
  }
  if(var == "ip_address_3"){
    return(String(ip_3_int));
  }
  if(var == "ip_address_4"){
    return(String(ip_4_int));
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
        while((urlarray[keuze][0] != *h_char) && (keuze > 0)){
          keuze --;
        }
        if(keuze < -2){
          keuze = MAX_AANTAL_KANALEN - 1;
            while((urlarray[keuze][0] != *h_char) && (keuze > 0)){
              keuze --;
            }
        }
      }
      if(request->hasParam(KEUZEPLUS_INPUT)){
        keuze++;
        if(keuze > MAX_AANTAL_KANALEN + 1){
          keuze = 0;
        }
        if((keuze > 0) && (keuze < MAX_AANTAL_KANALEN)){
          while((urlarray[keuze][0] != *h_char) && (keuze < MAX_AANTAL_KANALEN)){
            keuze ++;
          }
        }
        if(keuze == MAX_AANTAL_KANALEN){
          keuze = -2;
        }
        if(keuze == MAX_AANTAL_KANALEN + 1 ){
          keuze = -1;
        }
      }
      if((request->hasParam(BEVESTIGKEUZE_INPUT)) && (kiezen == true)){
        kiezen = false;
        if(keuze == -2){
          songlijst_bestaat_bool = false;
          testDir(SD, "/songlijst0");
          if(songlijst_bestaat_bool == false){
            lijst_maken = true;
          }
          else{
            keuze = pref.getShort("station");
            webradio = true;
          }
          
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
        memset(zenderarray[array_index], 0, sizeof(zenderarray[array_index]));
        http_zender.toCharArray(zenderarray[array_index], http_zender.length() + 1);
        memset(urlarray[array_index], 0, sizeof(urlarray[array_index]));
        http_url.toCharArray(urlarray[array_index], http_url.length() + 1);
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
          songlijst_bestaat_bool = false;
          testDir(SD, "/songlijst0");
          if(songlijst_bestaat_bool == false){
            lijst_maken = true;
          }
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
      if(request->hasParam(IP_1_KEUZE)){
        ip_1_string = (request->getParam(IP_1_KEUZE)->value()) +String(terminator);
      }
      if(request->hasParam(IP_2_KEUZE)){
        ip_2_string = (request->getParam(IP_2_KEUZE)->value()) +String(terminator);
      }
      if(request->hasParam(IP_3_KEUZE)){
        ip_3_string = (request->getParam(IP_3_KEUZE)->value()) +String(terminator);
      }
      if(request->hasParam(IP_4_KEUZE)){
        ip_4_string = (request->getParam(IP_4_KEUZE)->value()) +String(terminator);
      }
      if((ssid_ingevuld) && (pswd_ingevuld)){
        ssid_ingevuld = false;
        pswd_ingevuld = false;
        ip_string = ip_1_string + "." + ip_2_string + "." + ip_3_string + "." + ip_4_string;
        ip_string.toCharArray(ip_char, (ip_string.length() + 1));
        writeFile(SD, "/ip", ip_char);
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
  if(pref.getString("controle") != "dummy geladen"){
    pref.putShort("station", 1);
    pref.putShort("volume", 10);
    pref.putShort("laag", 0);
    pref.putShort("midden", 0);
    pref.putShort("hoog", 0);
    pref.putShort("files", mp3_per_songlijst);
    pref.putString("controle", "dummy geladen");
  }
  gekozen = pref.getShort("station");
  volume_gekozen = pref.getShort("volume");
  volume_keuze = volume_gekozen;
  laag_gekozen = pref.getShort("laag");
  laag_keuze = laag_gekozen;
  midden_gekozen = pref.getShort("midden");
  midden_keuze = midden_gekozen;
  hoog_gekozen = pref.getShort("hoog");
  hoog_keuze = hoog_gekozen;
  mp3_per_songlijst = pref.getShort("files");
  audio.setVolume(volume_gekozen);
  audio.setTone(laag_gekozen, midden_gekozen, hoog_gekozen);
  lees_CSV();
  readFile(SD, "/ssid");
  inputString.toCharArray(ssid, teller);
  readFile(SD, "/pswd");
  inputString.toCharArray(password, teller);
  readIP(SD, "/ip");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  //WiFi.softAP(APssid, APpswd);
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
    IPAddress static_ip(ip_1_int, ip_2_int, ip_3_int, ip_4_int);
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
