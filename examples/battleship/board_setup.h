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
 * This file contains all the code to either randomly place your ships or allow you to
 * place them yourself.
 */

/*
 * Tests to see if Ships[i] is in a legal position not off the edge or bottom of the board
 * and not overlapping another ship. Returns true if there is a conflict.
 */
bool testLoc(uint8_t i) {
  uint8_t j,k;
  if(Ships[i].vertical) { 
    if( (Ships[i].index + 10*(Ships[i].length-1)) > 100) {//off the bottom of the board
      return true;
    }
  } else {  //see if horizontal ship goes off the right edge
    //Test if the row number of the index is different from the row number of index + length
    if( (Ships[i].index/10) < ((Ships[i].index+Ships[i].length-1)/10) ){
      return true;    //there was a conflict so we keep looking
    }
  }
  for(j=0;j<i;j++) {  //See if it conflicts with another ship.
    if(i!=j) {    //Don't compare with yourself
      for(k=0;k<Ships[i].length;k++) { //Iterate the length of Ships[i]
        uint8_t Testing = Ships[i].index + ((Ships[i].vertical)?k*10:k);
        if(sea[Testing] != GRID_EMPTY) { //Conflict with another ship
          return true;  //there was a conflict so we keep looking
        }
      }
    }
  }
  return false; //no conflict found
}
/*
 * Once we have determined a ship is in a legal position, we place it on the board.
 */
void placeShip(uint8_t i) {
  //our chosen location was okay so we fill in the grid.
  for(uint8_t j=0;j<Ships[i].length;j++) { //Iterate the length of Ships[i]
    sea[ Ships[i].index + ((Ships[i].vertical)?j*10:j) ]= (grid_t) (((int) GRID_SHIP_0)+i);
  }
}

//menu for choosing the type of ship placement
const char* selection[3]= {"Random Placement", "Manual Placement", "Fixed debug pattern"};
/*
 * Places all five ships on the sea board. gives you a menu choice of random, manual,
 * or a fixed predefined debugging pattern.
 */
void placeShips(void) {
  uint8_t i, j, choice;
  uint32_t Buttons;
  bool Looking;
  #if(SELF_TEST)
//    return;
  #endif
  choice=Device.menu(selection,3,ARCADA_WHITE, ARCADA_BLACK);
  Device.display->fillScreen(ARCADA_GREEN);
  if(choice<2) {
    for(i=0;i<5;i++) {  //Erase the default debug locations
      Ships[i].index=-1;  //indicates not placed
      Ships[i].vertical= Ships[i].sunk=false;
      Ships[i].hits=0;
    }
  }
  switch(choice) {
    case 0:   //automatic random placement
      for(i=0;i<5;i++) {
        Looking=true;
        while(Looking) {
          Ships[i].vertical=random(2);  //random number less than two i.e. 0 or 1.
          Ships[i].index=random(100); 
          drawBoard(SEA_BOARD);
          bottomMessage("Placing %s",Ships[i].name);
          Looking=testLoc(i);
          myDelay(1000);    //Makes for interesting animation
        }
        placeShip(i); //chosen location was okay
      }
      break;
    case 1:
      Device.infoBox("Use joystick to position your ship. Press \"B\" to toggle orientation. Press \"Select\" to place the ship");
      for(i=0;i<5;i++) { 
        //put the cursor at the first empty grid location. 
        Ships[i].index=0;
        while( (Ships[i].index<100) && (sea[Ships[i].index] != GRID_EMPTY) ) {
          Ships[i].index++;
        }
        drawBoard(SEA_BOARD);
        bottomMessage("Placing %s",Ships[i].name);
        drawGridLoc(GRID_CURSOR, Ships[i].index, ARCADA_YELLOW);//the cursor
        Looking=true;
        while(Looking) {
          if (Buttons=Device.readButtons()) {//not an error
            Device.readButtons();//flush out or de-bounce
            drawGridLoc(radar[Ships[i].index], Ships[i].index, shipColor);//erase cursor from the current position
            switch(Buttons) {
              case ARCADA_BUTTONMASK_UP:    Ships[i].index = (Ships[i].index+(100-10)) % 100; break;    
              case ARCADA_BUTTONMASK_DOWN:  Ships[i].index = (Ships[i].index+10) % 100; break;    
              case ARCADA_BUTTONMASK_LEFT:  Ships[i].index = (Ships[i].index+(100-1)) % 100; break;    
              case ARCADA_BUTTONMASK_RIGHT: Ships[i].index = (Ships[i].index+1) % 100; break;    
              case ARCADA_BUTTONMASK_SELECT: 
                //We made our selection. 
                if(Looking=testLoc(i)) { //not a mistake
                  bottomMessage("Illegal position.");
                  Device.warnBox("Illegal position.",0);
                  playWave("afraid.wav");
                } else {  //No conflict with selection. Looking==false so move onto the next ship
                  drawBoard(SEA_BOARD);
                  bottomMessage("%s placed", Ships[i].name);
                  playWave("tada.wav");
                }
                break;
              case ARCADA_BUTTONMASK_B: //toggle orientation
                Ships[i].vertical= ! Ships[i].vertical;
                break;
              //case ARCADA_BUTTONMASK_START: //not used
              //case ARCADA_BUTTONMASK_A:     //not used
            }
            drawBoard(SEA_BOARD);
            bottomMessage("Placing %s",Ships[i].name);
            drawGridLoc(GRID_CURSOR, Ships[i].index, ARCADA_YELLOW);//the cursor at new location
          }
        }
        placeShip(i);
      }
      break;
    case 2:
      #if(SELF_TEST)  
        Serial.println("resetting index 1 and 2");
        sea[0]=GRID_SHIP_4;
        sea[1]=GRID_SHIP_4;
        for(uint8_t i=0;i<100;i++) {
          Serial.printf(" %d", sea[i]);
          if((i % 10)==9) Serial.println();
        }
      #endif
      return;
    }
    drawBoard(SEA_BOARD);
    bottomMessage("Ships Placed");
}
