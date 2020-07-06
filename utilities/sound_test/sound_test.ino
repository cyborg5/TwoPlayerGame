/*********************************************************
 *    Two Player Game Engiune
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 * 
 * See https://learn.adafruit.com/two-player-game-system-for-pygamer-and-rfm69hcw-radio-wing/
 * for more information about this project.
 **********************************************************/
/*
 * This utility program tests your sound files on your PyGamer or PyBadge.
 * When using an external SD card on the PyGamer change the following define to true.
 * When using internal QSPI memory it should be false.
 */
#define USE_SD_CARD false
#define USE_AUDIO true
#include <Adafruit_Arcada.h>

Adafruit_Arcada Device;

#include <TwoPlayerGame_wave.h>

void fatal(const char *message, uint16_t blinkDelay) {
  Serial.println(message);
  for (bool ledState = HIGH;; ledState = !ledState) {
    digitalWrite(LED_BUILTIN, ledState);
    delay(blinkDelay);
  }
}

void setup(void) {
  Serial.begin(115200);
  //while (!Serial) yield();
  Serial.printf("Arcada wave player demo. Playing wave files in folder %s\n", WAV_PATH);
  Device.arcadaBegin();
  Device.displayBegin();
  #if(USE_SD_CARD)
    #define FILESYSTEM ARCADA_FILESYS_SD
    Serial.println("Using SD card");
  #else
    #define FILESYSTEM ARCADA_FILESYS_QSPI
    Serial.println("Using internal QSPI memory");
  #endif
  setupWave();
}


void loop(void) {
  playWave("afraid.wav");
  delay(1000);
  playWave("battleship.wav");
  delay(1000);
  playWave("carrier.wav");
  delay(1000);
  playWave("cruiser.wav");
  delay(1000);
  playWave("fire.wav");
  delay(1000);
  playWave("found.wav");
  delay(1000);
  playWave("game_over.wav");
  delay(1000);
  playWave("goodbye.wav");
  delay(1000);
  playWave("hit.wav");
  delay(1000);
  playWave("lets_play.wav");
  delay(1000);
  playWave("miss.wav");
  delay(1000);
  playWave("patrol.wav");
  delay(1000);
  playWave("shall_we.wav");
  delay(1000);
  playWave("sound_off.wav");
  delay(1000);
  playWave("sound_on.wav");
  delay(1000);
  playWave("submarine.wav");
  delay(1000);
  playWave("tada.wav");
  while (true) {yield();};
}
