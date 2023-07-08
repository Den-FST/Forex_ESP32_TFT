#include <Arduino.h>

#include <FS.h> //this needs to be first, or it all crashes and burns...
#include "SPIFFS.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <AsyncTCP.h>
#endif

#include <WiFiClient.h>

#include <TFT_eSPI.h>
#include <ArduinoOTA.h>

// needed for library
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <WebSerial.h>

#include "functions.h"

#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson

#define GFXFF 1
#define FF18 &FreeMono9pt7b

TFT_eSPI tft = TFT_eSPI(); // Initialize TFT display object

// #define DEBUG_SERIAL
int DEBUG_SERIAL = 0;

// Variables to store the current firmware file size


int DWflag = 9;
int y = 0;
bool sent = false;
bool connected_server = false;
bool endmsg = false;

unsigned long previousMillis = 0;     // Stores the previous time
const unsigned long delayTime = 5000; // Delay time in milliseconds

String response;

String serialData;

WiFiClient client;

String myString = "";
String profit;

char fx_server[40];
char fx_port[6] = "5000";

char fx_server2[40];
char fx_port2[6] = "5001";

char fx_server_con[40];
char fx_port_con[6];

#define TRIGGER_PIN 22
#define CHANGE_SRV_PIN 0

int srv = 0;

// flag for saving data
bool shouldSaveConfig = false;

// callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

AsyncWebServer server(80);
DNSServer dns;



/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len)
{
  WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++)
  {
    d += char(data[i]);
  }
  WebSerial.println(d);
  if (d == "serial=1")
  {
    DEBUG_SERIAL = 1;
    WebSerial.println("DEBUG_SERIAL = 1");
  }
  else if (d == "serial=0")
  {
    DEBUG_SERIAL = 0;
    WebSerial.println("DEBUG_SERIAL = 0");
  }
  else if (d == "gettime")
  {
    time_sync();
    WebSerial.println(timeClient.getFormattedTime());
  }
  else if (d == "settime")
  {
    set_time();
  }
  else if (d == "systime")
  {
    printDateTime();
  }
  else if (d == "time")
  {
    String t = timeClient.getFormattedTime();
    WebSerial.println(t);
  }
  else if (d == "monon")
  {
    // Turn on the display
    digitalWrite(TFT_BL, HIGH);
    // tft.writecommand(0x11);  // Send the appropriate command to turn on the display
  }
  else if (d == "monoff")
  {
    // Turn off the display
    digitalWrite(TFT_BL, LOW);
    //  tft.writecommand(0x10);  // Send the appropriate command to turn off the display
  }
  else if (d == "ledtest") {

    ledctrl(0, 1, 1);
    delay(1000);
    ledctrl(1, 0, 1);
    delay(1000);
    ledctrl(1, 1, 0);
    delay(1000);
    ledctrl(1, 1, 1);
  }
  else if (d == "update") {

    WebSerial.print("Current firmware version: ");
    WebSerial.println(currentVersion);
    
    updateFirmware();

  }
}

void printTFT(int x, int y, String text, const GFXfont *font, uint16_t color, int size, int format)
{
  tft.setFreeFont(font);
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.setCursor(x, y);
  if (format != 1)
  {
    tft.print(text);
  }
  else
  {
    tft.println(text);
  }
}

void connectToServer()
{
  int port = atoi(fx_port_con);
  if (!client.connect(fx_server_con, port))
  {
    Serial.println("Connection failed");
    return;
  }
  if (DEBUG_SERIAL != 0)
  {
    Serial.println("Connected to server");
  }
}

void sendData()
{
  if (client.connected())
  {
    client.println("b");
    delay(100);

    if (sent == false)
    {
      // tft.setCursor(0, 80);
      tft.print("Send data FXServer..");
      // tft.setCursor(205, 80);
      tft.setTextColor(TFT_GREEN);
      tft.println("OK");
      tft.setTextColor(TFT_WHITE);
      // tft.setCursor(0, 100);
      tft.print("Waiting response..");
      sent = true;
    }

    while (client.available())
    {
      response = client.readStringUntil('#');

      char delimiter = ','; // Delimiter character
      int startIndex = 0;   // Starting index for substring extraction
      int endIndex;         // Ending index for substring extraction
      int valIndex = 0;
      int valIndex1 = 0;

      tft.setFreeFont(FF18);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);

      while (startIndex < response.length())
      {
        endIndex = response.indexOf(delimiter, startIndex); // Find the next delimiter

        if (endIndex == -1)
        {
          endmsg = true;
          // tft.fillRect(0, 20, 240, 220, TFT_BLACK);
          // If no more delimiter is found, extract the substring until the end of the string
          endIndex = response.length();
        }

        String substring = response.substring(startIndex, endIndex);
        if (DEBUG_SERIAL != 0)
        {
          Serial.print("Substring: ");
          Serial.println(substring);
          Serial.print("StartIndex: ");
          Serial.println(startIndex);
        }

        if (startIndex == 0)
        { // PARSE AND SHOW ON TFT PAIR TEXT
          y += 18;
          tft.fillRect(0, y - 15, TFT_HEIGHT, 50, TFT_BLACK);
          tft.setTextColor(TFT_WHITE);
          tft.setCursor(TFT_WIDTH / 16, y);
          tft.print(substring);
          delay(5);
        } // PARSE AND SHOW ON TFT TYPE OPERATION TEXT
        else if (startIndex == 7)
        {
          tft.setCursor(TFT_WIDTH / 2.2, y);
          if (substring == "Sell")
          {
            tft.setTextColor(TFT_BLUE);
            tft.print("SL");
            valIndex = 12;
            valIndex1 = 17;
          }
          else
          {
            tft.setTextColor(TFT_PURPLE);
            tft.print("BY");
            valIndex = 11;
            valIndex1 = 16;
          }
          delay(5);
        } // PARSE AND SHOW ON TFT LOTE TEXT
        else if (startIndex == valIndex)
        {
          printTFT(TFT_WIDTH / 1.5, y, substring, FF18, TFT_WHITE, 1, 0);
          delay(5);
        }
        else if (startIndex == valIndex1)
        { // PARSE AND SHOW ON TFT VALUE OF OPEN OPERATIONS TEXT

          int col = substring.toInt();
          if (col <= 0.0)
          {
            printTFT(TFT_WIDTH / 1.1, y, substring, FF18, TFT_RED, 1, 0);
            delay(5);
          }
          else
          {
            printTFT(TFT_WIDTH / 1.044, y, substring, FF18, TFT_GREEN, 1, 0);
            delay(5);
          }
        }
        else if (startIndex == 23)
        {
          // PARSE PROFIT VALUE TEXT
          if (DEBUG_SERIAL != 0)
          {
            Serial.print("Profit: ");
            WebSerial.print("Profit: ");
          }

          profit = substring.substring(0, 6);
          if (DEBUG_SERIAL != 0)
          {
            Serial.println(profit);
            WebSerial.println(profit);
          }
        }
        if (DEBUG_SERIAL != 0)
        {
          Serial.println(substring); // Print the extracted substring
          WebSerial.println(substring);
        }

        startIndex = endIndex + 1; // Update the starting index for the next substring
      }
      if (DEBUG_SERIAL != 0)
      {
        Serial.println("---------------------------------------");
        WebSerial.println("---------------------------------------");
      }
    }
    if (endmsg == true)
    { // DRAW YELLOW LINE AND SHOW ON TFT PAIR TEXT
      tft.drawLine(0, y + 5, TFT_HEIGHT, y + 5, TFT_YELLOW);
      tft.setCursor(TFT_WIDTH / 2.5, y + 20);
      int col = profit.toInt();
      if (col < 0)
        tft.setTextColor(TFT_GOLD);
      else
      {
        tft.setTextColor(TFT_GREEN);
      }
      tft.print("Eur: ");
      delay(5);
      tft.print(profit);
      endmsg = false;

      tft.fillRect(0, y + 25, TFT_HEIGHT, TFT_WIDTH - (TFT_WIDTH - y), TFT_BLACK);
    }
    client.stop(); // Close connection

    y = -8;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(CHANGE_SRV_PIN, INPUT_PULLUP);
  pinMode(17, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);
  digitalWrite(17, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(16, HIGH);

  // clean FS, for testing
  // SPIFFS.format();
  tft.begin();
  // delay(50);
  tft.setRotation(1); // Set display rotation (adjust to match your TFT display)
  // tft.setFreeFont(FF18);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 20);
  tft.println("Started FX Monitor");
  // read configuration from FS json
  tft.println("mounting FS...");

  if (SPIFFS.begin())
  {
    // Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      // file exists, reading and loading
      Serial.println("reading config file");
      tft.println("reading config file");
      tft.println(" ");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          Serial.println("\nparsed json");

          strcpy(fx_server, json["fx_server"]);
          strcpy(fx_port, json["fx_port"]);
          strcpy(fx_server2, json["fx_server2"]);
          strcpy(fx_port2, json["fx_port2"]);

          tft.setTextColor(TFT_YELLOW);
          tft.print("Server: ");
          tft.println(fx_server);
          tft.print("Port: ");
          tft.println(fx_port);
          tft.println(" ");
        }
        else
        {
          Serial.println("failed to load json config");
          tft.println("failed to load json config");
          SPIFFS.format();
        }
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
    tft.println("failed to mount FS");
    tft.println("Open FX_AP with pass: Future2050");
  }

#ifdef DEBUG_SERIAL
  Serial.println(fx_server);
  Serial.println(fx_port);
  Serial.println(fx_server2);
  Serial.println(fx_port2);
#endif

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  AsyncWiFiManagerParameter custom_fx_server("server", "fx server", fx_server, 40);
  AsyncWiFiManagerParameter custom_fx_port("port", "fx port", fx_port, 5);
  AsyncWiFiManagerParameter custom_fx_server2("server2", "fx2 server", fx_server2, 40);
  AsyncWiFiManagerParameter custom_fx_port2("port2", "fx2 port", fx_port2, 5);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  AsyncWiFiManager wifiManager(&server, &dns);

  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // add all your parameters here
  wifiManager.addParameter(&custom_fx_server);
  wifiManager.addParameter(&custom_fx_port);

  wifiManager.addParameter(&custom_fx_server2);
  wifiManager.addParameter(&custom_fx_port);

  // reset settings - for testing
  //  wifiManager.resetSettings();

  // sets timeout until configuration portal gets turned off
  // useful to make it all retry or go to sleep
  // in seconds
  //  wifiManager.setTimeout(120);

  // tft.println("Open FX_AP with pass: Future2050");
  // tft.println(wifiManager.getConfigPortalSSID());

  // fetches ssid and pass and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"

  // and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("FX_AP", "Future2050"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    // reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  // if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  tft.setTextColor(TFT_GREEN);
  tft.println("Connected");
  tft.setTextColor(TFT_WHITE);
  // read updated parameters
  strcpy(fx_server, custom_fx_server.getValue());
  strcpy(fx_port, custom_fx_port.getValue());
  strcpy(fx_server2, custom_fx_server2.getValue());
  strcpy(fx_port2, custom_fx_port2.getValue());

  // save the custom parameters to FS
  if (shouldSaveConfig)
  {
    Serial.println("saving config");
    tft.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["fx_server"] = fx_server;
    json["fx_port"] = fx_port;
    json["fx_server2"] = fx_server2;
    json["fx_port2"] = fx_port2;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
      Serial.println("failed to open config file for writing");
      tft.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    // end save
  }

  tft.println(wifiManager.getConfiguredSTASSID());

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());

  tft.print("Local ip: ");
  tft.setTextColor(TFT_BLUE);
  tft.println(WiFi.localIP());
  tft.setTextColor(TFT_WHITE);
  tft.println(" ");
  Serial.print("FX Server 1: ");
  Serial.println(fx_server);
  Serial.print("FX port 1: ");
  Serial.println(fx_port);
  Serial.print("FX Server 2: ");
  Serial.println(fx_server2);
  Serial.print("FX port 2: ");
  Serial.println(fx_port2);

  strcpy(fx_server_con, fx_server);
  strcpy(fx_port_con, fx_port);

  ArduinoOTA.setHostname("FX-ota");
  ArduinoOTA.begin();
  delay(500);
  // WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&server);
  /* Attach Message Callback */
  WebSerial.msgCallback(recvMsg);
  server.begin();
  delay(1000);
  timeClient.begin();
  delay(100);
  set_time();
  delay(100);
  // Get the current firmware file size
  currentFileSize = ESP.getSketchSize();
  WebSerial.print("Current firmware version: ");
  WebSerial.println(currentVersion);
  
  updateFirmware();
}






void loop()
{
  ArduinoOTA.handle();

  if (digitalRead(CHANGE_SRV_PIN) == LOW && srv == 0) // Change to server 2
  {
    tft.fillScreen(TFT_YELLOW);
    printTFT(TFT_WIDTH / 4, TFT_HEIGHT / 3, "Server 1", FF18, TFT_BLACK, 2, 1); // print on TFT Screen

    strcpy(fx_server_con, fx_server2);
    strcpy(fx_port_con, fx_port2);

    Serial.println("Server Changed to First! ");
    WebSerial.println(F("Server Changed to First!"));
    Serial.println(fx_server_con);
    Serial.println(fx_port_con);

    srv = 1;
    delay(2000);

    tft.fillScreen(TFT_BLACK);
    printTFT(0, 50, "Waiting for data...", FF18, TFT_WHITE, 1, 1);
  }

  if (digitalRead(CHANGE_SRV_PIN) == LOW && srv == 1) // Change to server 2
  {
    tft.fillScreen(TFT_GREEN);
    printTFT(TFT_WIDTH / 4, TFT_HEIGHT / 3, "Server 2", FF18, TFT_BLACK, 2, 1);

    strcpy(fx_server_con, fx_server);
    strcpy(fx_port_con, fx_port);

    Serial.println("Server Changed to Second! ");
    WebSerial.println(F("Server Changed to Second!"));
    Serial.println(fx_server_con);
    Serial.println(fx_port_con);

    delay(2000);

    tft.fillScreen(TFT_BLACK);
    printTFT(0, 50, "Waiting for data...", FF18, TFT_WHITE, 1, 1);
    srv = 0;
  }

  if (digitalRead(TRIGGER_PIN) == LOW)
  {
    tft.fillScreen(TFT_RED);

    SPIFFS.format(); // Format SPIFFS and all data

    AsyncWiFiManager wifiManager(&server, &dns); // Local intialization. Once its business is done, there is no need to keep it around
    wifiManager.resetSettings();                 // reset settings - for testing

    Serial.println("reset");
    delay(1000);
    ESP.restart();
  }

  unsigned long currentMillis = millis(); // Get the current time

  // Check if the desired delay time has passed
  if (currentMillis - previousMillis >= delayTime)
  {
    // Reset the previous time
    previousMillis = currentMillis;

    int dayweek = CheckDayWeek();
    
    if (dayweek != DWflag) {

    
      if (dayweek == 6 || dayweek == 1)
      {
        WebSerial.println("Weekeed");
        digitalWrite(TFT_BL, LOW);
        ledctrl(0, 1, 1);
      }
      else
      {
        WebSerial.println("Work day");
        digitalWrite(TFT_BL, HIGH);
        ledctrl(1, 0, 1);
      }

    DWflag = dayweek;
    }
    // Connection to server
    if (!client.connected())
    {

      connectToServer();
    }

    if (client.connected())
    {
      if (connected_server == false)
      {
        // tft.setCursor(0, 60);

        tft.print("Connecting Server..");
        // tft.setCursor(205, 60);
        tft.setTextColor(TFT_GREEN);
        tft.println("OK");
        tft.setTextColor(TFT_WHITE);
        connected_server = true;
      }
      sendData();
    }
  }
}