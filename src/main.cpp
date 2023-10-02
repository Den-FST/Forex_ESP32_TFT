//+------------------------------------------------------------------+
//|                                          Forex data ESP32 TFT    |
//|                                                     2023, FST    |
//|                                                      Denis Sk    |
//+------------------------------------------------------------------+

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

#include <TFT_Touch.h>

// These are the pins used to interface between the 2046 touch controller and Arduino Pro
#define DOUT 39 /* Data out pin (T_DO) of touch screen */
#define DIN 32  /* Data in pin (T_DIN) of touch screen */
#define DCS 33  /* Chip select pin (T_CS) of touch screen */
#define DCLK 25 /* Clock pin (T_CLK) of touch screen */

/* Create an instance of the touch screen library */
TFT_Touch touch = TFT_Touch(DCS, DCLK, DIN, DOUT);

#define devboard

#define GFXFF 1
#define FF18 &FreeMono9pt7b

TFT_eSPI tft = TFT_eSPI(); // Initialize TFT display object

// #define DEBUG_SERIAL
int DEBUG_SERIAL = 0;

const int buzzerPin = 27;  // Use the actual pin number where you connected the buzzer
int melody[] = { 262, 294, 330, 349, 392, 440, 494, 523 };
int noteDuration = 300;

// Variables to store the current firmware file size
int count = 0;
// int DWflag = 9;
int y = 0;
bool sent = false;
bool connected_server = false;
bool endmsg = false;
int lastInx = 0;
int nextInx = 0;
unsigned long previousMillis = 0;     // Stores the previous time
const unsigned long delayTime = 5000; // Delay time in milliseconds

String response;

String serialData;

WiFiClient client;

String lastSubstring;
String profit;
String todayProfit;
String debug;
String srv_HrsMins;
String srv_dayOfWeek;

char fx_server[40];
char fx_port[6] = "5000";

char fx_server2[40];
char fx_port2[6] = "5001";

char fx_server_con[40];
char fx_port_con[6];

#ifdef devboard
#define TRIGGER_PIN 22
#define CHANGE_SRV_PIN 0
#else
#define TRIGGER_PIN 32
#define CHANGE_SRV_PIN 22
#endif
int srv = 1;

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


void buzz(int note, int Duration) {
    tone(buzzerPin, note);
    delay(Duration);
    noTone(buzzerPin);
}

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len)
{
  // WebSerial.println("Received Data...");
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
  }
  else if (d == "monoff")
  {
    // Turn off the display
    digitalWrite(TFT_BL, LOW);
  }
  else if (d == "ledtest")
  {

    ledctrl(0, 1, 1);
    delay(1000);
    ledctrl(1, 0, 1);
    delay(1000);
    ledctrl(1, 1, 0);
    delay(1000);
    ledctrl(1, 1, 1);
  }
  else if (d == "update")
  {

    WebSerial.print("Current firmware version: ");
    WebSerial.println(currentVersion);
    updateFirmware();
  }
  else if (d == "buzz")
  {
    WebSerial.print("Buzzer Test: ");

    for (int i = 0; i < 8; i++) {
      buzz(melody[i], noteDuration);
      delay(100);  // Short pause between notes
    }
  }
  else if (d == "cmd")
  {

    WebSerial.print("-- For DEBUG messages On : ");
    WebSerial.println("serial=1");
    WebSerial.print("-- For DEBUG messages Off : ");
    WebSerial.println("serial=0");
    WebSerial.print("-- Get time from NTP server : ");
    WebSerial.println("gettime");
    WebSerial.print("-- Get time from NTP and sync with local time : ");
    WebSerial.println("settime");
    WebSerial.print("-- Get system time : ");
    WebSerial.println("systime");
    WebSerial.print("-- Show the time from server : ");
    WebSerial.println("time");
    WebSerial.print("-- Screen On : ");
    WebSerial.println("monon");
    WebSerial.print("-- Screen Off : ");
    WebSerial.println("monoff");
    WebSerial.print("-- Test LEDs RGB : ");
    WebSerial.println("ledtest");
    WebSerial.print("-- Update firmware from server : ");
    WebSerial.println("update");
  }
}


// TFT Printing function
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

// Server connection function.
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

//+------------------------------------------------------------------+
//|                  Socket exchange comunication                    |
//+------------------------------------------------------------------+

void sendData()
{
  if (client.connected())
  {
    client.println("b");
    delay(100);

    if (sent == false)
    {
      tft.print("Send data FXServer..");
      tft.setTextColor(TFT_GREEN);
      tft.println("OK");
      tft.setTextColor(TFT_WHITE);
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
          // If no more delimiter is found, extract the substring until the end of the string
          endIndex = response.length();
        }

        String substring = response.substring(startIndex, endIndex);

        if (DEBUG_SERIAL != 0)
        {
          debug = "StartIndex: " + String(startIndex) + " - Substring: " + substring;

          Serial.println(debug);
          // WebSerial.println(debug);
          delay(200);
        }

        if (startIndex == 0)
        { // PARSE AND SHOW ON TFT PAIR TEXT
          y += 18;
          tft.fillRect(0, y - 15, TFT_HEIGHT, 50, TFT_BLACK);
          printTFT(TFT_WIDTH / 16, y, substring, FF18, TFT_WHITE, 1, 0);
          delay(5);

          count++;
          nextInx = substring.length() + 1 + startIndex;

        } // PARSE AND SHOW ON TFT TYPE OPERATION TEXT
        if (startIndex == nextInx && count == 1)
        {
          if (substring == "Sell")
          {
            printTFT(TFT_WIDTH / 2.45, y, "SL", FF18, TFT_BLUE, 1, 0);
          }
          else
          {
            printTFT(TFT_WIDTH / 2.45, y, "BY", FF18, TFT_MAGENTA, 1, 0);
          }
          delay(5);
          count++;
          nextInx = substring.length() + 1 + startIndex;
        } // PARSE AND SHOW ON TFT LOTE TEXT

        if (startIndex == nextInx && count == 2)
        {
          printTFT(TFT_WIDTH / 1.8, y, substring, FF18, TFT_WHITE, 1, 0);
          delay(5);
          count++;

          nextInx = substring.length() + 1 + startIndex;
        }

        if (startIndex == nextInx && count == 3)
        { // PARSE AND SHOW ON TFT VALUE OF OPEN OPERATIONS TEXT

          int col = substring.toInt();
          if (col < 0.1)
          {
            printTFT(TFT_WIDTH / 1.3, y, substring, FF18, TFT_RED, 1, 0); // Negative values
            delay(5);
          }
          else if (col >= 0.1)
          {
            printTFT(TFT_WIDTH / 1.234, y, substring, FF18, TFT_GREEN, 1, 0); // Positive values
            delay(5);
          }
          count++;
          nextInx = substring.length() + 1 + startIndex;
        }

        if (startIndex == nextInx && count == 4)
        {
          // PARSE PROFIT VALUE TEXT
          profit = substring.substring(0, 6);

          if (profit.toInt() > 5.0)
          {
            buzz((profit.toInt()*50), 200);
          }
          count++;
          nextInx = substring.length() + 1 + startIndex;
        }

        if (startIndex == nextInx && count == 5)
        // Today profit of closed positions.
        {
          todayProfit = substring;
          count++;
          nextInx = substring.length() + 1 + startIndex;
        }

        if (startIndex == nextInx && count == 6) // Time passed of each opened posision.
        {
          printTFT(TFT_WIDTH + 20, y, substring, FF18, TFT_LIGHTGREY, 1, 0);
          nextInx = substring.length() + 1 + startIndex;
          count++;
        }

        if (startIndex == nextInx && count == 7)
        // Hours:Minutes of the server time.
        {

          srv_HrsMins = substring;
          nextInx = substring.length() + 1 + startIndex;
          count++;
        }

        if (startIndex == nextInx && count == 8)
        // Day of week from the server.
        {
          srv_dayOfWeek = substring;

          nextInx = substring.length() + 1 + startIndex;
          count++;
        }

        if (startIndex == nextInx && count == 9)
        // Day of week from the server.
        {

          if (substring.toInt() == 1)
          {
            tft.drawTriangle(2, y - 2, 8, y - 2, 5, y - 7, TFT_GREEN);
          }
          else
          {
            tft.drawTriangle(2, y - 7, 8, y - 7, 5, y - 2, TFT_RED);
          }
          count = 0;
        }
        startIndex = endIndex + 1; // Update the starting index for the next substring.
      }
      if (DEBUG_SERIAL != 0)
      {
        Serial.println("---------------------------------------");
      }
    }
    if (endmsg == true)
    {
      tft.drawLine(0, y + 5, TFT_HEIGHT, y + 5, TFT_YELLOW); // DRAW YELLOW LINE AND SHOW ON TFT PAIR TEXT
      tft.fillRect(0, y + 25, TFT_HEIGHT, TFT_WIDTH - (TFT_WIDTH - y), TFT_BLACK);
      printTFT(20, y + 20, srv_HrsMins, FF18, TFT_GREEN, 1, 1);
      printTFT(TFT_WIDTH / 1.8, y + 20, todayProfit, FF18, TFT_CYAN, 1, 1);

      tft.setCursor(TFT_WIDTH / 1.3, y + 20);
      int col = profit.toInt();
      if (col < 0)
        tft.setTextColor(TFT_GOLD);
      else
      {
        tft.setTextColor(TFT_GREEN);
      }

      tft.setCursor(TFT_WIDTH / 1.3, y + 20);
      tft.print(profit);

      int srv_h = srv_HrsMins.substring(0, 2).toInt();
      int srv_hm = srv_HrsMins.substring(0, 5).toInt();
      int srv_d = srv_dayOfWeek.toInt();

      if (DEBUG_SERIAL) {
        WebSerial.print("SRV_H: ");
        WebSerial.println(srv_HrsMins);
        WebSerial.print("SRV_M: ");
        WebSerial.println(srv_HrsMins.substring(3, 5).toInt());
        WebSerial.print("SRV_D: ");
        WebSerial.println(srv_d);
      }

      if (srv_h > 6 && srv_h < 23 && srv_d != 6 && srv_d != 0) // If time is betwen 6 a.m. and 11 p.m. and not weekend day, switch monitor on
      {
        digitalWrite(TFT_BL, HIGH);
        ledctrl(1, 0, 1);
      }

      else // Monitor off
      {
        digitalWrite(TFT_BL, LOW);
        ledctrl(0, 1, 1);
      }
      endmsg = false;
    }
    client.stop(); // Close connection

    y = -8;
  }
}

//+------------------------------------------------------------------+
//|                            SETUP ZONE                            |
//+------------------------------------------------------------------+

void setup()
{
  Serial.begin(115200);
  Serial.println();

  pinMode(buzzerPin, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(CHANGE_SRV_PIN, INPUT_PULLUP);
  pinMode(17, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);
  digitalWrite(17, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(16, HIGH);

  tft.begin();

  tft.setRotation(1); // Set display rotation (adjust to match your TFT display)
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
    SPIFFS.format();
  }


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
  touch.setCal(526, 3443, 750, 3377, 320, 240, 1);
}

//+------------------------------------------------------------------+
//|                             LOOP ZONE                            |
//+------------------------------------------------------------------+

void loop()
{
  ArduinoOTA.handle(); // Upload over air. Ota handle.

  uint16_t touchX, touchY;

  bool touched = touch.Pressed(); // tft.getTouch( &touchX, &touchY, 600 );
  int srvTouch = 0;

  if (touched)
  {
    touchX = touch.X();
    touchY = touch.Y();

    Serial.print("Data x ");
    Serial.println(touchX);

    Serial.print("Data y ");
    Serial.println(touchY);

    // ---=== MENU TEST ===---
    // tft.drawRect(touchX, touchY, 60, 40, TFT_RED);
    // tft.fillRect(touchX, touchY, 58, 38, TFT_LIGHTGREY);
    // tft.setTextColor(TFT_BLACK);
    // String item1 = "Menu 1";
    // tft.drawString(item1 , touchX, touchY, 1);
    // Serial.print("TEXT RECT");
    // Serial.print(tft.textWidth(item1));
    // Serial.print(" - ");
    // Serial.println(tft.fontHeight(1));

  }

  if ((touched || digitalRead(CHANGE_SRV_PIN) == LOW) && srv == 0) // Change to server 2
  {
    // tft.fillScreen(TFT_YELLOW);
    // printTFT(TFT_WIDTH / 4, TFT_HEIGHT / 3, "Server 1", FF18, TFT_BLACK, 2, 1); // print on TFT Screen

    strcpy(fx_server_con, fx_server);
    strcpy(fx_port_con, fx_port);

    tft.fillScreen(TFT_BLACK);

    String combinedString = String(fx_server_con) + ":" + String(fx_port_con);

    tft.setTextFont(1);
    tft.drawCentreString("FIRST - 1", TFT_WIDTH / 3, TFT_HEIGHT / 6, 2);
    tft.drawRoundRect(TFT_WIDTH / 15, TFT_HEIGHT / 4, tft.textWidth(combinedString) + 125, 40, 5, TFT_RED);
    // tft.fillRoundRect(TFT_WIDTH / 6, TFT_HEIGHT / 4, tft.textWidth(combinedString)+120 , 40, 3, TFT_LIGHTGREY);

    printTFT(TFT_WIDTH / 14, TFT_HEIGHT / 3, combinedString, FF18, TFT_LIGHTGREY, 1, 1);

    Serial.print("Server Changed to First! ");
    WebSerial.println(F("Server Changed to First!"));
    Serial.print(fx_server_con);
    Serial.println(fx_port_con);

    srv = 1;
    delay(2000);

    tft.fillScreen(TFT_BLACK);
    printTFT(0, 50, "Waiting for data...", FF18, TFT_WHITE, 1, 1);
  }

  if ((touched || digitalRead(CHANGE_SRV_PIN) == LOW) && srv == 1) // Change to server 2
  {

    strcpy(fx_server_con, fx_server2);
    strcpy(fx_port_con, fx_port2);

    tft.fillScreen(TFT_BLACK);

    String combinedString = String(fx_server_con) + ":" + String(fx_port_con);

    tft.setTextFont(1);
    tft.drawCentreString("SECOND - 2", TFT_WIDTH / 3, TFT_HEIGHT / 6, 2);
    tft.drawRoundRect(TFT_WIDTH / 15, TFT_HEIGHT / 4, tft.textWidth(combinedString) + 125, 40, 5, TFT_RED);
    // tft.fillRoundRect(TFT_WIDTH / 6, TFT_HEIGHT / 4, tft.textWidth(combinedString)+118 , 38, 3, TFT_LIGHTGREY);

    printTFT(TFT_WIDTH / 14, TFT_HEIGHT / 3, combinedString, FF18, TFT_LIGHTGREY, 1, 1);

    Serial.print("Server Changed to Second! ");
    WebSerial.println(F("Server Changed to Second!"));
    Serial.print(fx_server_con);
    Serial.println(fx_port_con);

    delay(2000);

    tft.fillScreen(TFT_BLACK);
    printTFT(0, 50, "Waiting for data...", FF18, TFT_WHITE, 1, 1);
    srv = 0;
  }

  if (digitalRead(TRIGGER_PIN) == LOW) // Hold reset trigger button for reset all.
  {
    tft.fillScreen(TFT_RED);

    // SPIFFS.format(); // Format SPIFFS and all data

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
   // Connection to server
    if (!client.connected())
    {
      connectToServer();
    }

    if (client.connected())
    {
      if (connected_server == false)
      {
        tft.print("Connecting Server..");
        tft.setTextColor(TFT_GREEN);
        tft.println("OK");
        tft.setTextColor(TFT_WHITE);
        connected_server = true;
      }
      sendData();
    }
  }
}