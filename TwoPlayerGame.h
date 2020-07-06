/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 **********************************************************/
#ifndef TPG_DEBUG
  #define TPG_DEBUG 0
#endif

#if (TPG_DEBUG)
  #define DEBUG(s) Serial.print(s)
  #define DEBUGLN(s) Serial.println(s)
  #define SETUP_DEBUG {Serial.begin(115200); while (!Serial) { delay(1); } }
  #define DEBUG_PRINT print()
#else
  #define DEBUG(s) {}
  #define DEBUGLN(s) {}
  #define SETUP_DEBUG  {}
  #define DEBUG_PRINT {}
#endif

#include "TwoPlayerGame_base_radio.h"
#include "TwoPlayerGame_base_packet.h"
#include "TwoPlayerGame_base_game.h"
