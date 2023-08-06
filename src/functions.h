#define FUNCTIONS

#ifdef FUNCTIONS

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <sys/time.h>
#include <Timezone.h>
#include <WebSerial.h>

#include <HTTPClient.h>
#include <Update.h>

#include <esp_task_wdt.h>


HTTPClient client2;
#define FWURL "http://www.fst.pt/upload/firmware_v"
#define FWversion 1

int currentVersion = FWversion;

const char* ntpServer = "pool.ntp.org"; //Time server
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

TimeChangeRule myDST = {"DST", Last, Sun, Mar, 2, 60}; // Daylight Saving Time
TimeChangeRule mySTD = {"STD", Last, Sun, Oct, 3, 0};  // Standard Time

Timezone myTZ(myDST, mySTD);


unsigned int currentFileSize = 0;
uint8_t* clientContent = nullptr;

void time_sync() {
    while (!timeClient.update()) {
    timeClient.forceUpdate();
    delay(500);
  }
}

void set_time() {

      time_sync();
    // Get the current UNIX timestamp from the time client
    unsigned long epochTime = timeClient.getEpochTime();

    // Set the system time using the UNIX timestamp
    struct timeval tv;
    tv.tv_sec = epochTime;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
    WebSerial.println("TimeSet Ok");
}

String sendTime() {
  String timeString = "";
  // Get the current datetime
  time_t t = time(nullptr);

    // Convert to local time
  TimeChangeRule* tcr;
  time_t localTime = myTZ.toLocal(t, &tcr);

  struct tm *timeinfo;
  timeinfo = localtime(&localTime);

  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  // int currentSecond = timeinfo->tm_sec;

  // Format the time string
  if (currentHour < 10) {
    timeString += "0";
  }
  timeString += String(currentHour);
  
  timeString += ":";
  
  if (currentMinute < 10) {
    timeString += "0";
  }
  timeString += String(currentMinute);
  
  // timeString += ":";
  
  // if (currentSecond < 10) {
  //   timeString += "0";
  // }
  // timeString += String(currentSecond);
  
  return timeString;
}

void printDateTime() {
  // Get the current datetime
  time_t t = time(nullptr);

    // Convert to local time
  TimeChangeRule* tcr;
  time_t localTime = myTZ.toLocal(t, &tcr);

  struct tm *timeinfo;
  timeinfo = localtime(&localTime);

  // Print the datetime
  WebSerial.print("Current datetime: ");
  WebSerial.print(timeinfo->tm_year + 1900);
  WebSerial.print("-");
  WebSerial.print(timeinfo->tm_mon + 1);
  WebSerial.print("-");
  WebSerial.print(timeinfo->tm_mday);
  WebSerial.print(" ");
  WebSerial.print(timeinfo->tm_hour);
  WebSerial.print(":");
  WebSerial.print(timeinfo->tm_min);
  WebSerial.print(":");
  WebSerial.print(timeinfo->tm_sec);

    int dayOfWeek = timeinfo->tm_wday;
  switch (dayOfWeek) {
    case 0:
      WebSerial.println(" - Sunday");
      break;
    case 1:
      WebSerial.println(" - Monday");
      break;
    case 2:
      WebSerial.println(" - Tuesday");
      break;
    case 3:
      WebSerial.println(" - Wednesday");
      break;
    case 4:
      WebSerial.println(" - Thursday");
      break;
    case 5:
      WebSerial.println(" - Friday");
      break;
    case 6:
      WebSerial.println(" - Saturday");
      break;
    default:
      WebSerial.println(" - Unknown day of the week");
      break;
  }

}


void ledctrl(int r, int g, int b) {
  digitalWrite(4, r);
  digitalWrite(16, g);
  digitalWrite(17, b);
}


int CheckDayWeek() {
    // Get the current datetime
  time_t t = time(nullptr);

    // Convert to local time
  TimeChangeRule* tcr;
  time_t localTime = myTZ.toLocal(t, &tcr);

  struct tm *timeinfo;
  timeinfo = localtime(&localTime);

  int dayOfWeek = timeinfo->tm_wday;

  return dayOfWeek;
}


bool checkFileSize(int fileSize) {
  // Get the current firmware size
  int currentFileSize = ESP.getSketchSize();
  Serial.println(currentFileSize);
  // Compare file sizes
  if (fileSize > currentFileSize) {
    WebSerial.println("New firmware available");
    Serial.println("New firmware available");
    return true;
  } else {
    WebSerial.println("Firmware is up to date");
    Serial.println("Firmware is up to date");
    return false;
  }
}

void updateFirmware() {
  esp_task_wdt_init(30, false);

  // Connect to the web server
  String fwVersionURL = FWURL + String(currentVersion + 1) + ".bin";
  Serial.print("Checking for firmware at URL: ");
  Serial.println(fwVersionURL);

  client2.begin(fwVersionURL);
  int httpCode = client2.GET();

  // Check if the server returned a valid response
  if (httpCode == HTTP_CODE_OK) {
    int contentLength = client2.getSize();
    Serial.println(contentLength);
    if (checkFileSize(contentLength)) {
      Serial.print("Updating firmware to version ");
      WebSerial.print("Updating firmware to version ");
      Serial.println(currentVersion + 1);
      delay(500);
      // Start the firmware update process
      if (Update.begin(contentLength, U_FLASH)) {
        // WebSerial.println("Firmware update started!");
        // Receive and write firmware data to the ESP32 flash memory
        WiFiClient* stream = client2.getStreamPtr();
        uint8_t buff[1024];
        int bytesRead = 0;

        while (client2.connected() && (bytesRead < contentLength)) {
          size_t size = stream->available();
          if (size) {
            int len = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            Update.write(buff, len);
            bytesRead += len;
          }
          delay(1);
        }

        // Finish the firmware update
        if (Update.end()) {
          if (Update.isFinished()) {
            Serial.println("Firmware update successful");
            WebSerial.println("Firmware update successful");
            currentVersion++;
            ESP.restart();
          } else {
            Serial.println("Firmware update failed");
            WebSerial.println("Firmware update failed");
          }
        } else {
          Serial.println("Firmware update end failed");
          WebSerial.println("Firmware update end failed");
        }
      } else {
        Serial.println("Firmware update begin failed");
        WebSerial.println("Firmware update begin failed");
      }
    }
  } else {
    Serial.println("Firmware download failed");
    WebSerial.println("Firmware download failed");
  }
  
  client2.end();
}

#endif

// Function to extract a substring between two characters
String getSubstringBetweenChars(String input, char startChar, char endChar) {
  int startIndex = input.indexOf(startChar);
  int endIndex = input.indexOf(endChar, startIndex + 1);

  if (startIndex >= 0 && endIndex > startIndex) {
    return input.substring(startIndex + 1, endIndex);
  }

  return ""; // Return an empty string if the start and end characters are not found.
}