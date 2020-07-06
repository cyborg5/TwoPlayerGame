/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 * 
 * See https://learn.adafruit.com/two-player-game-system-for-pygamer-and-rfm69hcw-radio-wing/
 * for more information about this project.
 **********************************************************/
/*
 * This utility program turns your PyGamer or PyBadge into a virtual USB drive.
 * When using an external SD card on the PyGamer change the following define to true.
 * When using internal QSPI memory it should be false.
 */
#define USE_SD_CARD false

#ifndef USE_TINYUSB
  #error In the Arduino IDE change to Tools->USB Stack->TinyUSB
#endif  

#include <Adafruit_Arcada.h>

Adafruit_Arcada Device;

void fatal(const char *message, uint16_t blinkDelay) {
  Serial.println(message);
  for (bool ledState = HIGH;; ledState = !ledState) {
    digitalWrite(LED_BUILTIN, ledState);
    delay(blinkDelay);
  }
}

void setup(void) {
  Serial.begin(115200);
//  while (!Serial) yield();
  delay(10000);
  Serial.println("Opens your PyGamer or PyBadge as a USB drive. Copy your wave files into the /wav folder.");
#if(USE_SD_CARD)
  #define FILESYSTEM ARCADA_FILESYS_SD
  Serial.println("Using SD card");
#else
  #define FILESYSTEM ARCADA_FILESYS_QSPI
  Serial.println("Using internal QSPI memory");
#endif
  if (!Device.arcadaBegin())     fatal("Arcada init fail!", 100);
  if (!Device.filesysBeginMSD(FILESYSTEM)) fatal("No filesystem found!", 250);
  Serial.println("Success");
}

void loop (void) {
};
