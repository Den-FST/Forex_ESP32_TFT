#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <TFT_eSPI.h>

#define GFXFF 1
#define FF18 &FreeMono9pt7b

//#define tft1 //1.54
#define tft2 // 1.69

#ifdef tft1

#endif

#ifdef tft2

#endif

TFT_eSPI tft = TFT_eSPI(); // Initialize TFT display object

int y = 0;
bool sent = false;
bool connected_server = false;
bool endmsg = false;

String response;

const char *ssid = "Xiaomi";
const char *password = "926810D20D";
String myString = "";
String profit ;

WiFiClient client;

const char *serverAddress = "192.168.1.199";
const int serverPort = 5000;
const int serverPort2 = 5001;

void connectToServer()
{
  if (!client.connect(serverAddress, serverPort))
  {
    Serial.println("Connection failed");
    return;
  }
  Serial.println("Connected to server");
}

void sendData()
{
  if (client.connected())
  {
    client.println("b");
    delay(100);

    if (sent == false)
    {
      tft.setCursor(0, 80);
      tft.println("Send data FXServer..");
      tft.setCursor(205, 80);
      tft.setTextColor(TFT_GREEN);
      tft.println("OK");
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(0, 100);
      tft.println("Waiting response..");
      sent = true;
    }

    while (client.available())
    {
      response = client.readStringUntil('#');

      char delimiter = ','; // Delimiter character
      int startIndex = 0;   // Starting index for substring extraction
      int endIndex;         // Ending index for substring extraction

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
        Serial.print("StartIndex: ");
        Serial.println(startIndex);

        if (startIndex == 0)
        {
          // tft.fillRect(0, y, 240, 50, TFT_BLACK);
          y += 18;

          tft.fillRect(0, y - 15, 280, 50, TFT_BLACK);
          tft.setTextColor(TFT_WHITE);

          #ifdef tft1
          tft.setCursor(0, y);
          #endif
          #ifdef tft2
          tft.setCursor(20, y);
          #endif  

          
          tft.print(substring);
          delay(5);
        }
        else if (startIndex == 7)
        {
          #ifdef tft1
          tft.setCursor(70, y);
          #endif
          #ifdef tft2
          tft.setCursor(90, y);
          #endif          
          if (substring == "Sell")
          {
            tft.setTextColor(TFT_BLUE);
            tft.print("SL");
          }
          else
          {
            tft.setTextColor(TFT_YELLOW);
            tft.print("BY");
          }
          delay(5);
        }
        else if (startIndex == 12)
        {
          tft.setTextColor(TFT_WHITE);
          #ifdef tft1
          tft.setCursor(100, y);
          #endif
          #ifdef tft2
          tft.setCursor(120, y);
          #endif  
          tft.print(substring);
          delay(5);
        }
        else if (startIndex == 17)
        {

          int col = substring.toInt();
          if (col <= 0)
          {
            tft.setTextColor(TFT_RED);
          }
          else
          {
            tft.setTextColor(TFT_GREEN);
          }
          #ifdef tft1
          tft.setCursor(150, y);
          #endif
          #ifdef tft2
          tft.setCursor(180, y);
          #endif  
          tft.print(substring);
          delay(5);
        }       
        else if (startIndex == 24)     {
            Serial.print("Profit: ");
            profit = substring.substring(0, 6);
            Serial.println(profit);


      }

        Serial.println(substring);  // Print the extracted substring

        startIndex = endIndex + 1; // Update the starting index for the next substring
      } 


      Serial.println("---------------------------------------");
    }
    if (endmsg == true) {
      tft.drawLine(0, y + 5, TFT_HEIGHT, y + 5, TFT_YELLOW);
          #ifdef tft1
          tft.setCursor(100, y + 20);
          tft.setTextColor(TFT_CYAN);
          tft.print("Eur: ");
          delay(5);
          tft.print(profit);
          #endif
          #ifdef tft2
          tft.setCursor(90, y + 20);

          int col = profit.toInt();
          if (col < 0) tft.setTextColor(TFT_GOLD);
          else {
            tft.setTextColor(TFT_GREEN);
          }

          tft.print("Eur: ");
          delay(5);
          tft.print(profit);
          #endif  
          endmsg = false;
    }
    client.stop();
    y = -8;
  }
}
void setup()
{
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1); // Set display rotation (adjust to match your TFT display)

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    // tft.setTextDatum(TL_DATUM);
    // tft.loadFont(AA_FONT_MONO);
    tft.setFreeFont(FF18);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 40);
    tft.println("Connecting WiFi..");
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  tft.setCursor(205, 40);
  tft.setTextColor(TFT_GREEN);
  tft.println("OK");

  Serial.println("Connected to WiFi");
  // Set text size and color
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
}

void loop()
{
  if (!client.connected())
  {

    connectToServer();
  }

  if (client.connected())
  {
    if (connected_server == false)
    {
      tft.setCursor(0, 60);
      tft.println("Connecting Server..");
      tft.setCursor(205, 60);
      tft.setTextColor(TFT_GREEN);
      tft.println("OK");
      tft.setTextColor(TFT_WHITE);
      connected_server = true;
    }
    sendData();
  }

  delay(5000);
}
