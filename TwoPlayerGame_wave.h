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
 * This file handles all the sound effects for the Battleship game
 * Load the ".wav" files onto an SD card in a directory called "/wav"
 */
#define WAV_PATH "/wav"
bool soundEffects;
volatile bool isPlaying;

//Using traditional "delay(amount)" caused problems with packet radio and wave file playback
//Use this modified version instead.
void myDelay(uint32_t Delay) {
  uint32_t StartTime=millis();
  while((millis()-StartTime)< Delay) yield ();
}

void waveError(const char* s) {
  Device.warnBox(s);
  soundEffects=false;
}

//Called from BShip::setup() which is called from the main setup() function
void setupWave(void) {
  soundEffects=USE_AUDIO;
  isPlaying=false;
  if(!soundEffects) return;
  if(!Device.filesysBegin()) {
    waveError("Could not initialize file system."); 
    return;
  }
  if(!Device.chdir(WAV_PATH)) {
    waveError("Cannot change to wave path");
    return;
  }
}

//Callback function which plays a single sample. It is called by 
//an interrupt timer. 
void wavOutCallback(void) {
  wavStatus status = Device.WavPlayNextSample();
  if (status == WAV_EOF) {
    // End of WAV file reached, stop timer, stop audio
    Device.timerStop();
    Device.enableSpeaker(false);
    isPlaying = false;
  } else {
    isPlaying = true;
  }
}

//This function actually plays the named wave file. We tried using the built-in
//function Device.WavPlayComplete(Name) but for some reason the sound quality 
//wasn't as good.
void playWave(const char* name) {
  if(!soundEffects) return;
  myDelay(500);
  uint32_t sampleRate;
  wavStatus status;
  File my_wave_file= Device.open(name, FILE_READ);
  status = Device.WavLoad(my_wave_file, &sampleRate);
  if ((status == WAV_LOAD) || (status == WAV_EOF)) {
    Device.enableSpeaker(true);  // enable speaker output
    Device.timerCallback(sampleRate, wavOutCallback); // setup the callback to play audio
  } else {
    waveError("Could not open wave file");
    return;
  }
  do { // Repeat this loop until WAV_EOF or WAV_ERR_*
    if (Device.WavReadyForData()) {
      yield();
      status = Device.WavReadFile();
    }
    yield();
  } while ((status == WAV_OK) || (status == WAV_LOAD));
  while (isPlaying)   yield();
  my_wave_file.close();
  myDelay(500);
}
