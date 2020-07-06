/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 **********************************************************/
/*
 * Source code for the baseGame class. See "TwoPlayerGame_base_game.h" for details.
 */
 
#include "TwoPlayerGame.h"
//String versions of the game state for debugging or other purposes
const char* gameStateStr[5]={"Offering Game", "Seeking Game", "My Turn", "Opponent's Turn", "Game Over"};

/*
 * Constructor for the game object. 
 */
baseGame::baseGame(baseMove* move_ptr, baseResults* results_ptr, baseRadio* radio_ptr, bool isPlayer_1) {
  Move=move_ptr; 
  Results=results_ptr; 
  Radio=radio_ptr;
  //link the radio to the Move and get results objects
  Move->Radio=Radio;
  Results->Radio=Radio;
  //determine player number which is use as radio address
  myPlayerNum=2;
  otherPlayerNum=1;
  if(isPlayer_1) {
    myPlayerNum=1; otherPlayerNum=2;
  };  
};

/*
 * Called ONCE inside your main program "setup()" function.
 */
void baseGame::setup(void) {
  SETUP_DEBUG;
  Radio->setup(myPlayerNum,otherPlayerNum);
  initialize();  //game specific variables
};

/*
 * Called from within your main program "loop()" function. Processes
 * okay each of the game states.
 */
void baseGame::loopContents(void) {
  switch(gameState) {
    case OFFERING_GAME:  offeringGame();  break;
    case SEEKING_GAME:   seekingGame();   break;
    case MY_TURN:        doMyTurn();      break;
    case OPPONENTS_TURN: doOpponentsTurn(); break;
    case GAME_OVER:      gameOver();      break;
  }
}

#define OFFERING_TIMEOUT 1000 //one second
#define OFFERING_TRIES  2

/*
 * Internal routine to offer a game. Send out an "OFFERING_GAME" packet and wait for the time 
 * period specified in OFFERING_TIMEOUT. NOTE although we can't figure out why, our packet 
 * radio seems to use 1000 as a maximum value. Repeat OFFERING_TRIES times.
 * 
 * Sending this packet means I'm initiating a game and wanting you to join. This is in contrast 
 * to "SEEKING_GAME" which means I'm waiting for the other player to offer me a game. If there is 
 * no reply then change mode to SEEKING_GAME as we wait for an offer. 
 * 
 * Receiving an ack only means that the packet was received by the other device however that isn't 
 * enough because the other machine might be in the some random state and it would just be acknowledging 
 * the receipt. We must receive an "ACCEPTING_GAME" packet in order to begin the game.
 */
void baseGame::offeringGame(void) {
  basePacket p(Radio);
  currentMoveNum=1;
  for (uint8_t i=0;i<OFFERING_TRIES;i++) {//try repeatedly
    //If true, packet was received but that's not enough.
    if(p.send(OFFERING_GAME_PACKET)) {
      if(p.requireTypeTimeout(ACCEPTING_GAME_PACKET, OFFERING_TIMEOUT)) {
        p.send(FOUND_GAME_PACKET);//let them know they found us
        //The game has been accepted. Flip the coin and send the results in a COIN_FLIP_PACKET.
        //If it's true, we go first. If false other player goes first.
        if(p.subType=(packetSubType_t)coinFlip()) {//Not a mistake
          gameState=MY_TURN;
        } else {
          gameState=OPPONENTS_TURN;
        }
        p.send(COIN_FLIP_PACKET);
        return;
      }
      //we got a reply but it wasn't the right one so we keep looking
      DEBUGLN("Got reply but it wasn't \"Accepting\"");
      continue;
    }
    //OFFERING_TIMEOUT exceeded so we no reply so we send again
    DEBUGLN("No reply to offer.");
  }
  //We give up. Switch to seeking game.
  gameState=SEEKING_GAME;
}

/*
 * Internal method to wait for a game. We tried offering a game but no one was ready so 
 * now we sit and wait forever for an offer to come in. If we get one, we accept it and 
 * the other player will perform coin flip. The outcome of the flip is received in a
 * COIN_FLIP_PACKET. If the packet subType is true, offering player goes first. Otherwise
 * we go first.
 */
void baseGame::seekingGame(void) {
  basePacket p(Radio); 
  p.requireType(OFFERING_GAME_PACKET);//wait forever
  DEBUGLN("Offer Received.");
  //we got the offer so now let's accept it
  if(!p.send(ACCEPTING_GAME_PACKET)) {
    fatalError("No ack during Accepting Game");
    return;
  }
  p.requireType(FOUND_GAME_PACKET);//When we get this we found a game
  foundGame();  //Let the derived game print a message
  p.requireType(COIN_FLIP_PACKET);
  processFlip((bool)p.subType);  //let the derived game know results of coin flip
  if(p.subType) { //If flip was true, opponent goes first, otherwise we do
    gameState=OPPONENTS_TURN;
  } else {
    gameState=MY_TURN;
  }
  currentMoveNum=1;
}

/*
 * Internal routine for handling my move. It calls Move->decideMyMove() which is a virtual method
 * you will supply in your derived move class to actually decide what your move will be. Then this
 * method transmits it and then waits indefinitely for the other device to send us a "results" message.
 * We then call Results->processResults() which is a virtual method you will supply in your derived
 * results class. See the baseResults definition and comments in "TwoPlayerGame_base_packet.h"
 * for an explanation of what is a result.
 */
void baseGame::doMyTurn(void) {
  Move->moveNum = currentMoveNum;
  Move->decideMyMove();
  if(!Move->send()) {
    fatalError("No ack from send move");
    return;
  }
  DEBUGLN("Waiting for results.");
  Results->require();
  if(Results->resultsNum != currentMoveNum) {
    DEBUG("Results.resultsNum incorrect. Value is:"); DEBUG(Results->resultsNum);
    DEBUG (" expected:"); DEBUGLN(currentMoveNum);
    fatalError("Results number mismatch error.");
    return;
  }
  if(Results->processResults()) { //returns true if the game ended
    gameState=GAME_OVER;
  } else {
    gameState=OPPONENTS_TURN;     //otherwise it's our opponents turn
  }
  //now that I've got results, the move is complete so increment the move number.
  currentMoveNum++;
}

/*
 * Internal function that handles receiving opponent's move and creating results and sending 
 * them back to him. It calls "generateResults(Move)" which is a virtual method you will provide 
 * in your derived results class. It will decide the results of your opponent's move and will 
 * generate the results packet which we will then send to our opponent. See baseResults and the 
 * comments surrounding it to see what we mean by "results" in "TwoPlayerGame_base_packet.h".
 */
void baseGame::doOpponentsTurn(void) {
  Move->require();  //wait indefinitely for your opponent's move
  //if I won the coin toss then the other player passes by sending me move #0
  //so I have to adjust appropriately.
  if(Move->moveNum==0) {
    currentMoveNum=0;
  }
  // if it's not an initial pass move and numbers don't match then warn
  if( (Move->moveNum != currentMoveNum) && (Move->moveNum>0)) {
    DEBUG("Opponents move number incorrect. Value is:"); DEBUG(Move->moveNum);
    DEBUG(" expected:"); DEBUGLN(currentMoveNum);
    fatalError("Opponents move number mismatch error.");
    return;
  }
  if(Results->generateResults(Move)) {  //returns true if the game ended as a result of your opponents move
    gameState=GAME_OVER;
  } else {
    gameState=MY_TURN;                  //otherwise now it's my turn
  };
  Results->send();
  //now that he has completed his move, I processed it and sent him his results,
  //I can now increment the move counter to count his move
  currentMoveNum++;
}

/*
 * internal routine to handle the end of game. It calls your virtual function "processGameOver"
 */
void baseGame::gameOver(void) {
  processGameOver();
  gameState=OFFERING_GAME;
}
