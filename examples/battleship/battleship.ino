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
 * Compile and upload to one device using IS_PLAYER_1 true and compile and upload
 * to the other device using IS_PLAYER_1 false. This is the only difference between 
 * the code on the two different machines.
 */
/*
 * Two player Battleship game with audio sound effects. Load the wav files onto
 * an SD card on each machine in a folder called "/wav"
 */
#define IS_PLAYER_1 1
//Basic game engine code
#include <TwoPlayerGame.h>

//Code for RF69HCW packet radios
#include <TwoPlayerGame_RF69HCW.h>

//All of my code is here
#include "Battleship.h"

/*
   The Game object requires pointers to Move, Results, and Radio objects. Create
   an instance of each of these and pass their address to the game constructor.
*/
RF69Radio Radio;
BShip_Move Move;
BShip_Results Results;
BShip_Game Game(&Move, &Results, &Radio, IS_PLAYER_1);


void setup() {
  Game.setup();           //Initializes everything
}
void loop() {
  Game.loopContents();    //Does everything
}
