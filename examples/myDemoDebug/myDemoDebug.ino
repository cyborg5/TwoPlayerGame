/*
 * Two Player Game Main Program
 * Compile and upload to one device using IS_PLAYER_1 true and compile and upload
 * to the other device using IS_PLAYER_1 false. This is the only difference between 
 * the code on the two different machines.
 */
#define IS_PLAYER_1 0

//Basic game engine code
#include <TwoPlayerGame.h>

//Code for RF69HCW packet radios
#include <TwoPlayerGame_RF69HCW.h>

//All of my code is here
#include "myDemoDebug.h"

/*
 * The Game object requires pointers to Move, Results, and Radio objects. Create
 * an instance of each of these and pass their address to the game constructor.
 */
RF69Radio Radio;
myDemoMove Move;
myDemoResults Results;
myDemoGame Game(&Move, &Results, &Radio, IS_PLAYER_1);

void setup() {
  Game.setup();           //Initializes everything
}
void loop() {
  Game.loopContents();    //Does everything
}
