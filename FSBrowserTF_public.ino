/* 
  FSWebServer - Example WebServer with SPIFFS backend for esp8266
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done
  
  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include "mlx90621.h"
#include "Adafruit_SSD1306_tf.h"
#include "termika_fotilo.h"

#define DBG_OUTPUT_PORT Serial
#define LED 2
#define key1 0
#define key2 2 
#define LEDon digitalWrite(LED, LOW)
#define LEDoff digitalWrite(LED, HIGH)
#define MLX_voltage analogRead(A0)

const char* ssid2 = "********";
const char* password2 = "********";
const char* ssid3 = "********";
const char* password3 = "********";
const char* ssid = "********";
const char* password = "********";
const char* host = "esp8266fs";
const char * ssid_ap      = "ESP_HTML_02";
const char * password_ap  = "";    // alternativ :  = "12345678";
IPAddress ip(192, 168, 10, 89);
IPAddress ip2(192, 168, 10, 90);
IPAddress ip3(192, 168, 10, 91);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);



int Aufruf_Zaehler = 0;
int Sent_Zaehler = 0;
String tempstring;
byte keyval,mlxstatus,debug;
unsigned long time1,time2,time3,time4,time5;
IPAddress myIP;

Adafruit_SSD1306 oled(255);
MLX90621 MLXtemp;                       // Objekt fuer Tempsensor erzeugen
ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload(){
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
}

void setup(void){
  uint16_t timeout=0;
  uint8_t wifistatus = 0;
  LEDon;
  pinMode(LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(key1, INPUT);
  oled.begin();
  oled.setTextColor(WHITE);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0,0);
  oled.print("setup STA..");
  oled.display();
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }
  

  //WIFI INIT
  DBG_OUTPUT_PORT.printf("Connecting to %s\n", ssid);
  oled.print(String(WiFi.SSID()));
  /*
  if (String(WiFi.SSID()) == String(ssid)) WiFi.begin(ssid, password);
  else if (String(WiFi.SSID()) == String(ssid2)) WiFi.begin(ssid2, password2);
    else if (String(WiFi.SSID()) == String(ssid3)) WiFi.begin(ssid3, password3);
      else WiFi.begin(ssid, password);
  timeout = 0;
  */
  WiFi.mode(WIFI_STA);   //  Workstation
  timeout = millis() + 12000L;
  WiFi.begin(ssid, password); wifi_waitconnect();
  if (WiFi.status() != WL_CONNECTED) {WiFi.begin(ssid2, password2); oled.print("1"); wifi_waitconnect();}
  if (WiFi.status() != WL_CONNECTED) {WiFi.begin(ssid3, password3); oled.clearDisplay(); oled.setCursor(0, 0); oled.print("2"); wifi_waitconnect();}
  if (WiFi.status() != WL_CONNECTED) {WiFi.config(ip, gateway, subnet); WiFi.begin(ssid, password);  oled.print("3"); wifi_waitconnect(); }
  if (WiFi.status() != WL_CONNECTED) {WiFi.config(ip2, gateway, subnet); WiFi.begin(ssid, password); oled.clearDisplay(); oled.setCursor(0, 0); oled.print("4");wifi_waitconnect();}
  if (WiFi.status() != WL_CONNECTED) {WiFi.config(ip3, gateway, subnet); WiFi.begin(ssid, password); oled.print("5"); wifi_waitconnect();}
  if (WiFi.status() != WL_CONNECTED) {WiFi.mode(WIFI_AP);WiFi.softAP(ssid_ap, password_ap);server.begin();myIP = WiFi.softAPIP();}
  else myIP = WiFi.localIP();
 
  DBG_OUTPUT_PORT.println("");
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(myIP);

  MDNS.begin(host);
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(host);
  DBG_OUTPUT_PORT.println(".local/edit to see the file browser");
  
  
  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);
  server.on("/debug1", HTTP_GET, [](){server.send(200, "text/plain", "debug on");debug=1;});
  server.on("/debug0", HTTP_GET, [](){server.send(200, "text/plain", "debug off");debug=0;});
  server.on("/reset", HTTP_GET, [](){server.send(200, "text/plain", "reset");ESP.restart();});
  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/mlxinit", HTTP_GET, [](){server.send(200, "text/plain", "mlxinit");MLX_init();});
  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/value", HTTP_GET, [](){ 
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"analog\":"+String(analogRead(A0));
    json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();  
  });
  
  server.on("/all", HTTP_GET, [](){
    LEDon;
    time2=millis();
    server.send(200, "text/json", tempstring);
    Sent_Zaehler++;
    if (!(keyval&1) || debug) {
    DBG_OUTPUT_PORT.print("Server sent: ");
    DBG_OUTPUT_PORT.println(Sent_Zaehler);
    DBG_OUTPUT_PORT.println(tempstring);
    }
    if (mlxstatus) GetTempField();
    time3=millis()-time2;
    LEDoff;
  });
  
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");
  oled.print("Init MLX...");
  mlxstatus = 1;
  mlxstatus = MLX_init();
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print(myIP);
  if(mlxstatus) {
    oled.println("  MLX");
    GetAmbientTemp(); 
    GetTempField();   
  }
  else GetdummyData();  
  oled.display();
  delay (1000);  
  LEDoff;
}
 
void loop(void){
  time1=millis();
  keyval = digitalRead(key1);
  pinMode(LED, INPUT);     // Initialize the LED_BUILTIN pin as an output
  keyval |= digitalRead(key2) << 1;
  pinMode(LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  server.handleClient();
  //yield();
  // ab hier nur für debugging
  if (!(keyval&1) || debug) {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print(myIP);
    if(mlxstatus) {
      oled.println("  MLX");
      GetAmbientTemp();         
    }
    oled.print(Sent_Zaehler); 
    oled.print(" Time: ");
    oled.print(millis()-time1);
    oled.print(" ");
    oled.println(time3);
    oled.display(); 
    DBG_OUTPUT_PORT.print("time: ");
    DBG_OUTPUT_PORT.print(millis()-time1);
    DBG_OUTPUT_PORT.print(" ");
    DBG_OUTPUT_PORT.println(time3);
    }
}

void GetAmbientTemp(void)
{
  float t = MLXtemp.get_ptat();
  oled.print("Temp: ");
  oled.print(String(t,2));
  oled.print("   U:");
  oled.println(MLX_voltage);
  /*
  char puffer[10];
  TFTscreen.stroke (0, 0, 0);
  TFTscreen.fill (0, 0, 0);
  TFTscreen.rect (0, 112, 70, 128-112);          // Alte Zahl loeschen
  TFTscreen.stroke (0xFF, 0xFF, 0);
  TFTscreen.setTextSize (2);
  dtostrf (t, 6, 1, puffer);
  TFTscreen.text (puffer, 0, 112);     // 5x7 Font x2
  TFTscreen.setTextSize (1);
  TFTscreen.text ("\xF7", 74, 112);     // "°C" ASCII 0xF7 fuer Gradzeichen
  TFTscreen.text ("C", 80, 112);    
  */
}

uint8_t MLX_init(void) {
   uint8_t timeout = 0;
   while (!MLXtemp.init())  {      // MLX90621 init failed
    delay (100);
    if(timeout++ >100)  {oled.println("fail"); DBG_OUTPUT_PORT.println("fail");return 0;}
  }
  return 1;
}

void GetTempField(void) {
  float temps[16][4];
  uint8_t x, y;
  MLXtemp.read_all_irfield (temps);
  tempstring =  "{\"thfo\":[" ;
    for (y = 0; y < 4; y++) for (x = 0; x < 16; x++) {
      tempstring += String((int16_t)(temps[x][y]*10)) + ",";
    }
  tempstring += "0]}"; 
  if (!(keyval&1) || debug) {
  DBG_OUTPUT_PORT.println(tempstring);
  for (x = 0; x < 4; x++) {
  oled.setCursor(0, (x+2)*8);
  oled.print(String(temps[0][x],1));
  oled.setCursor(32, (x+2)*8);
  oled.print(String(temps[1][x],1));
  oled.setCursor(64, (x+2)*8);
  oled.print(String(temps[2][x],1));
  oled.setCursor(96, (x+2)*8);
  oled.print(String(temps[3][x],1));
  }
  oled.println();
  oled.display(); 
  }
}


void GetdummyData(void) {
  float temps[16] = {23.58,47.83,95.34,156.53,-12.58,-2.56,56.84,-23.56,28.89,67.88,212.32,87.45,109.45,-32.56,25.56,63.45};
  uint8_t x, y;
  tempstring =  "{\"thfo\":[" ;
    for (y = 0; y < 4; y++) for (x = 0; x < 16; x++) {
      tempstring += String((int16_t)(temps[random(0,15)]*10)) + ",";
    }
   tempstring += "0]}"; 
  if (!(keyval&1) || debug) {
  DBG_OUTPUT_PORT.println(tempstring);
  for (x = 0; x < 4; x++) {
  oled.setCursor(0, (x+2)*8);
  oled.print(String(temps[0+x*4],1));
  oled.setCursor(32, (x+2)*8);
  oled.print(String(temps[1+x*4],1));
  oled.setCursor(64, (x+2)*8);
  oled.print(String(temps[2+x*4],1));
  oled.setCursor(96, (x+2)*8);
  oled.print(String(temps[3+x*4],1)); 
  }
  oled.println();
  }
}

uint8_t wifi_waitconnect (void) {
  unsigned long timeout = millis() + 12000L;
  while (WiFi.status() != WL_CONNECTED && millis() < timeout) {delay(100); oled.print(".");oled.display();}
    if (WiFi.status() != WL_CONNECTED) oled.print("fail");
    else return 1;
    return 0;
  }


