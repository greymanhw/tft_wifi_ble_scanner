#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "FS.h"

#include <TFT_eSPI.h>      // Hardware-specific library
#include <SPI.h>

#include "WiFi.h"


/* 
 * Touch screen configuration
 */
TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The SPIFFS file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData1"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

/*
 * WiFi Configuration
 */

 /*
  * BLE Configuration
  */
uint32_t scanTime = 100; //In 10ms (1000ms)
BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    tft.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
  }
};

void output(String message, bool line=false) {
  if (line == true) {
    Serial.println(message);
    tft.println(message);
  } else {
    Serial.print(message);
    tft.print(message);
  }
};

int scanWiFiNetworks() {
    Serial.println("WiFi network scanning starting");
    int n = WiFi.scanNetworks();
    Serial.println("WiFi network scanning complete");
    return n;
};

void drawWiFiScreen(int n=0) {
    // clear screen
    tft.fillScreen(TFT_BLACK);
    // x, y, font
    tft.setCursor(0, 0, 2);
    // set text color and size
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);

    if (n == 0) {
        output("no networks found");
    } else {
        output(String(n));
        output(" networks found", true);
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI (Received Signal Strength Indicator) for each network found
            output(String(i + 1));
            output(": ");
            output(WiFi.SSID(i));
            output(" (");
            output(String(WiFi.RSSI(i)));
            output(")");
            output((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*", true);
            delay(10); // example had a delay here, not sure why
        }
    }
    output("", true);
};

void scanBLENetworks() {
    Serial.println("BLE network scanning starting");
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value

    BLEScanResults foundDevices = pBLEScan->start(100, false);
    Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    Serial.println("BLE network scanning complete");
};

void drawBLEScreen() {
};

/*
 * App variables
 */
 String screens[5] = { "switcher", "wifi", "ble", "web", "settings" };
 int current_screen = 2;

void setup() {
  // Use serial port
  Serial.begin(115200);

  // Initialise the TFT screen
  tft.init();

  // Set the rotation before we calibrate
  tft.setRotation(2);

  // Calibrate the touch screen and retrieve the scaling factors
  touch_calibrate();

  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  // set WiFi station mode
  WiFi.mode(WIFI_STA);
}

void loop() {
    // WiFi.scanNetworks will return the number of networks found
    int n = scanWiFiNetworks();

    switch(current_screen) {
      case 1:
        drawWiFiScreen(n);
        break;
      case 2:
        // scanBLENetworks loads the variable 
        scanBLENetworks();
        drawBLEScreen();
        break;
      default:
        drawBLEScreen(); //TODO: replace with switcher
        break;
    }


    // Wait a bit before scanning again
    delay(5000);
}


//------------------------------------------------------------------------------------------

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    // Serial.println(F("Formating file system"));
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println(F("Touch corners as indicated"));

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println(F("Set REPEAT_CAL to false to stop this running again!"));
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    // tft.println(F("Calibration complete!"));

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

