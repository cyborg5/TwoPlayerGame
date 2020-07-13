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
 * This demonstration program implements a classic Battleship game using a 10 x 10 grid.
 */
#include "Adafruit_Arcada.h"

//Set this to true for an alternate assistive technology input using serial monitor
//imput in addition to the joystick and buttons.
#define ACCESSIBLE_INPUT false

//Self-test mode automatically sends a move back and forth for demonstration and
//debugging purposes. Automatically terminates after the specified number of moves.
//Set to zero to disable self-test.
#define SELF_TEST 0

//initial state of sound effects. Can be toggled using "B" button during any move
#define USE_AUDIO true

#if(ACCESSIBLE_INPUT)
  #include <AccessibleArcada.h>   //alternate input system for assistive technology
  AccessibleArcada Device;
#else
  Adafruit_Arcada Device;
#endif
#include <TwoPlayerGame_wave.h>       //Everything for audio playback

//Font used in opening splash screen
#include <Fonts/FreeSans12pt7b.h>

/************************************************************************************
 * Various global variables not really part of the game engine object. Need to be able to
 * access these from a variety of locations so we made them global.
 *************************************************************************************/
//Data structure for location of the ship and all its info
struct ship_t {
  int8_t index;     //Location of the ship. If -1 then the ship has not yet been placed.
  uint8_t hits;     //The number of hits it has received
  bool vertical;    //Orientation vertical or horizontal
  bool sunk;        //Has it been sunk?
  const uint8_t length;   //Length of the ship
  const char name[14];    //Text name of the ship
  const char wav[18];     //Name of the wave file declaring it sunk
};

//Initial condition of the ships.
//The initialization for index, hits, vertical, and sunk are to do a quick debug
//test. They get overwritten if you use random or manual ship placements.
ship_t Ships[5]= { 
  {70,5,false, true,5,"Carrier", "carrier.wav"}, 
  {20,4,true, true,4,"Battleship", "battleship.wav"}, 
  {42,3,false, true,3,"Cruiser", "cruiser.wav"}, 
  {46,3,true, true,3,"Submarine", "submarine.wav"},
  {0,0,false, false,2, "Patrol Boat", "patrol.wav"}
  };
  
//Which enemy ships we have destroyed
bool EnemyShips[5];
uint8_t EnemyHits;  //when this reaches 17 opponent wins
//Put all the colors in one place to make it easier to tweak
uint16_t shipColor=Device.display->color565(30, 30, 30);
uint16_t seaColor=Device.display->color565(50, 50, 255);
uint16_t radarColor=Device.display->color565(30,200, 0);
uint16_t hitColor=Device.display->color565(255, 0, 0);
//Possible contents of a grid
//Once a ship has been hit or sunk we don't care which ship it is anymore.
//So we don't need separate GRID_SHIP_HIT_x values for each ship.
enum grid_t {GRID_EMPTY, GRID_MISS, GRID_HIT, GRID_SHIP_HIT, GRID_CURSOR, 
            GRID_SHIP_0, GRID_SHIP_1, GRID_SHIP_2, GRID_SHIP_3, GRID_SHIP_4};
//The radar board shows you your enemy's area and the sea board shows you your area
enum board_t {SEA_BOARD, RADAR_BOARD};

char message[40];   //buffer to be used with sprintf
grid_t radar[100];  //our version of the opponent's board
grid_t sea[100];    //The sea where our ships are located

//"Center" of the board in pixels. Slightly higher than the actual center of the 
// screen to make room for the bottom message area. Initialized in BShip_Game::setup()
uint16_t centerX;     
uint16_t centerY;

#define SIZE_OF_SQR 12
/************************************************************************************
 * Various global functions not really part of the game engine object. Need to be able to
 * access these from a variety of locations so we made them global.
 *********************************************************/
/*
 * Displays a text message across the bottom of the screen in the default font
 * Comes in three varieties for normal, numerical, and text.
 */
void bottomMessage(const char*text) {
  Device.display->fillRect(0,Device.display->height()-8, Device.display->width(),8, ARCADA_BLACK);
  Device.display->setTextSize(1);
  Device.display->setTextColor(ARCADA_WHITE);
  Device.display->setCursor(0,Device.display->height()-8);
  Device.display->print(text);
}
void bottomMessage(const char*text,uint32_t value) {
  sprintf(message,text, value);  
  bottomMessage(message);
}
void bottomMessage(const char*text,const char*text2) {
  sprintf(message,text, text2);
  bottomMessage(message);
}

/*
 * Draws a circle showing hit or miss at a particular grid location
 */
void drawGridLoc(grid_t Type, uint8_t i, uint16_t color) {
  uint16_t sx=centerX + ((i % 10) -4.5)*SIZE_OF_SQR;
  uint16_t sy=centerY + ((i / 10) -4.5)*SIZE_OF_SQR;
  uint16_t c;
  switch(Type) {
    case GRID_EMPTY:  c=color; break;
    case GRID_MISS:   c=ARCADA_WHITE; break;
    case GRID_SHIP_HIT:
    case GRID_HIT:    c=hitColor; break;
    case GRID_CURSOR: c=color; break;
    default: //GRID_SHIP_0 through GRID_SHIP_4:
      c=shipColor; break;
  }
  Device.display->fillCircle(sx,sy,2,c);
}
/*
 * Draws the the board. Draws outlines of the ships on the SEA_BOARD.
 * Draws hits and misses.
 */
void drawBoard(board_t Type) {
  uint16_t color;
  grid_t* board;
  if(Type==SEA_BOARD) {
    board=sea;
    color=seaColor;
    Device.display->fillScreen(color);
    for(uint8_t i=0;i<5;i++) {//draw ship
      uint16_t w,h;
      if(Ships[i].index<0) {  //ship hasn't been placed yet
        continue;
      }
      if(Ships[i].vertical) {
        w=SIZE_OF_SQR-3; h=Ships[i].length*SIZE_OF_SQR-3;
      } else {
        h=SIZE_OF_SQR-3; w=Ships[i].length*SIZE_OF_SQR-3;
      }
      Device.display->fillRoundRect(
          centerX + ((Ships[i].index % 10) -4.5)*SIZE_OF_SQR-SIZE_OF_SQR/2+2,
          centerY + ((Ships[i].index / 10) -4.5)*SIZE_OF_SQR-SIZE_OF_SQR/2+2, 
          w,h,3,(Ships[i].sunk)?hitColor:shipColor);
    }
  } else {
    board=radar;
    color=radarColor;
    Device.display->fillScreen(color);
  }
  for (uint8_t i=1;i<10;i++) {
    Device.display->drawFastVLine(centerX-SIZE_OF_SQR*5+i*SIZE_OF_SQR, centerY-SIZE_OF_SQR*5,
                                  SIZE_OF_SQR*10, ARCADA_WHITE);
    Device.display->drawFastHLine(centerX-SIZE_OF_SQR*5, centerY-SIZE_OF_SQR*5+i*SIZE_OF_SQR,
                                  SIZE_OF_SQR*10, ARCADA_WHITE);
  }
  Device.display->drawRect(centerX-SIZE_OF_SQR*5,centerY-SIZE_OF_SQR*5,SIZE_OF_SQR*10+1,SIZE_OF_SQR*10+1, ARCADA_WHITE);
  //if we see this message, it means we should have written a different message somewhere
  bottomMessage("testing 123 this is a test");
  
  for(uint8_t i=0;i<100;i++) {
    drawGridLoc(board[i],i,color);
  }
}

/****************************************************************
 *    BShip_Move class and methods
 ****************************************************************/
/*
 * This is our derived move object. 
 * 
 *    uint8_t shot;
 *      The board index where we fired the shot
 * 
 *    size_t my_size();
 *      This method returns the size of BShip_Move object. 
 *      
 *    void decideMyMove(void);
 *      Prompts us to make our move. See details below.
 */
class BShip_Move : public baseMove {
  public:
    uint8_t shot;
    size_t my_size() override { return sizeof( *this ); };
    void decideMyMove(void)override;
};

/*
 * Takes non-analog input from the joystick (UP, DOWN, LEFT, RIGHT) and "SELECT" button to 
 * place your mark on one of the grid locations. 
 * 
 * Draws a shot in yellow as a kind of cursor. If you move it over a previous shot, it turns black
 * and refuses to let you select that location. The cursor automatically begins at the first blank grid
 * location.
 * 
 * When you press "SELECT" it turns WHITE and sets Move->shot to the index of the selected grid location. 
 * When you get the results of your move if it was a hit, it will turn red.
 * 
 * If you press "START" it restarts the game. The "B" button toggles sound effects off and on.
 * The "A" button does nothing when making a move but is used by some prompts. 
 */
void BShip_Move::decideMyMove(void) {
  if(moveNum>1) {
    delay (6000);//Allows us to see opponent's move before making ours
  }
  subType=NORMAL_MOVE;
  //put the cursor at the first empty grid location. Assumes there is one.
  shot=0;
  while( (shot<100) && (radar[shot] != GRID_EMPTY) ) {
    shot++;
  }
  drawBoard(RADAR_BOARD);
  drawGridLoc(GRID_CURSOR, shot, (radar[shot])?ARCADA_BLACK: ARCADA_YELLOW);//the cursor
  bottomMessage("Make your move #%d",moveNum);
  uint32_t Buttons;
  while(true) {
  #if(SELF_TEST)
    if (Buttons = ARCADA_BUTTONMASK_SELECT) {
    Serial.printf("Self test move= %d\n", moveNum);
  #else
    if (Buttons=Device.readButtons()) {//not an error
  #endif
      Device.readButtons();//flush out or de-bounce
      drawGridLoc(radar[shot], shot, radarColor);//erase cursor from the current position
      switch(Buttons){
        case ARCADA_BUTTONMASK_UP:    shot = (shot+(100-10)) % 100; break;    
        case ARCADA_BUTTONMASK_DOWN:  shot = (shot+10) % 100; break;    
        case ARCADA_BUTTONMASK_LEFT:  shot = (shot+(100-1)) % 100; break;    
        case ARCADA_BUTTONMASK_RIGHT: shot = (shot+1) % 100; break;    
        case ARCADA_BUTTONMASK_SELECT: 
          //We made our selection. 
          if(radar[shot]== GRID_EMPTY) {
            radar[shot]=GRID_MISS;//assume we missed until we know different
            drawBoard(RADAR_BOARD);
            bottomMessage("Firing!");
            playWave("fire.wav");
            return;
          } else {
            Device.warnBox("Square Already Occupied",0);
            playWave("afraid.wav");
            myDelay(2000);
            break;
          }
        case ARCADA_BUTTONMASK_START: //quit the game
          Device.infoBox("Quitting the game",0);
          subType=QUIT_MOVE;
          return; 
        case ARCADA_BUTTONMASK_B: //toggle sound effects
          if (soundEffects) {
            playWave("sound_off.wav");
            soundEffects=false;
            Device.infoBox("Sound effects off",0);
          } else {
            soundEffects=true;
            playWave("sound_on.wav");
            Device.infoBox("Sound effects on",0);
          }
          myDelay(2000);
          break;
      }
      drawBoard(RADAR_BOARD);
      drawGridLoc(GRID_CURSOR, shot, (radar[shot])?ARCADA_BLACK: ARCADA_YELLOW);//the cursor
      bottomMessage("Make your move #%d",moveNum);
      myDelay(500); //Slow down the cursor
    }
  }
};

/****************************************************************
 *    BShip_Results class and methods
 ****************************************************************/
/*
 *  This is our derived results object.
 * 
 *    int8_t shipDestroyed;
 *      Normally set to -1 but if we destroy a ship it tells us which ship we destroyed.
 *      Normally with a hit you don't know which ship it was until you actually destroy it.
 *      
 *    uint8_t shot;
 *      Repeating back to you the shot you just made. It makes the processResults code easier.
 *      
 *    size_t my_size();
 *      This method returns the size of the BShip_Results object.
 *    
 *    bool processResults(void)
 *      Processes the results packet we received in response to our move. See below for details.
 *      
 *    bool generateResults(baseMove* Move);
 *      Generate results package in response to our opponent's move. Reports if it was a hit
 *      or miss, did we sink a ship, and is the game over because it was the last ship.
 */
class BShip_Results : public baseResults {
  public:
    int8_t shipDestroyed;
    uint8_t shot;
    size_t my_size() override { return sizeof( *this ); };
    bool processResults(void)override;
    bool generateResults(baseMove* Move)override;
};

/*
 * Process a results packet. Return true if the game is over, false otherwise.
 */
bool BShip_Results::processResults(void) {
  switch(subType) {
    case MISS_RESULTS:  
      radar[shot]=GRID_MISS;
      drawBoard(RADAR_BOARD);
      bottomMessage("I missed");
      myDelay(4000);
      return false;
    case WIN_RESULTS:
    case HIT_RESULTS:   
      radar[shot]=GRID_HIT;  
      drawBoard(RADAR_BOARD);
      bottomMessage("I hit the enemy!");
      if(shipDestroyed>=0) {
        EnemyShips[shipDestroyed]= true;
        bottomMessage("I sank enemy %s!",Ships[shipDestroyed].name);
        Device.pixels.setPixelColor(shipDestroyed, 50,0,0);
        Device.pixels.show();
        playWave("goodbye.wav");
      }
      if(subType==WIN_RESULTS){
        bottomMessage("Hallelujah! I win");
        playWave("tada.wav");
        myDelay(6000);
        return true;
      } else {  //Hit but it wasn't destroyed and didn't win
        myDelay(3000);
        return false;
      }
    case LOSE_RESULTS:  
      bottomMessage("I quit"); 
      return true;//if we resigned, we get lose results
    //We don't use TIE_RESULTS or NORMAL_RESULTS in this game.
  }
}

/*
 * Determines if a shot was a hit or miss. Determines if it sank a ship and if it was the final ship 
 * resulting In the game being won.  Returns true if game is over.
 */
bool BShip_Results::generateResults(baseMove* M) {
  BShip_Move* Move = (BShip_Move*)M;//saves us a bunch of type casts
  resultsNum = Move->moveNum;
  shot=Move->shot;
  shipDestroyed=-1;   //default they didn't destroy anything
  uint8_t shipHit;    //which ship they hit
  switch(Move->subType) {
    case NORMAL_MOVE:
      if(sea[shot]>=GRID_SHIP_0) {//They hit a ship
        playWave("hit.wav");
        subType=HIT_RESULTS;
        shipHit=sea[shot]-GRID_SHIP_0;
        sea[shot]=GRID_HIT;
        Ships[shipHit].hits++;
        if(Ships[shipHit].length == Ships[shipHit].hits) {//ship sunk
          Ships[shipHit].sunk=true;
          shipDestroyed=shipHit;    //tell opponent which one
          drawBoard(SEA_BOARD);
          bottomMessage("Enemy sank my %s", Ships[shipHit].name);
          playWave(Ships[shipHit].wav);
        } else {  //Hit but not sunk
          drawBoard(SEA_BOARD);
          bottomMessage("Enemy hit my %s", Ships[shipHit].name);
        }
        if ((++EnemyHits)==17) {  //They win
          playWave("game_over.wav");
          subType=WIN_RESULTS;
          myDelay(3000);
          bottomMessage("Rats! The enemy won.");
          return true;
        }
      } else {    //They missed
        subType=MISS_RESULTS;
        sea[shot]=GRID_MISS;
        drawBoard(SEA_BOARD);
        bottomMessage("Ha Ha They missed");
        playWave("miss.wav");
      }
      return false;
    case QUIT_MOVE:
      Device.infoBox("Opponent quit.",0);
      bottomMessage("I win. Opponent quit.");
      subType=LOSE_RESULTS;
      return true;
    //case PASS_MOVE: not used in this game
  };
}

/****************************************************************
 *    BShip_Game class and methods
 ****************************************************************/
/*
 * Most of the logic of the game engine is handled by baseGame. We extend the baseGame with our own
 * data and methods. Some of our global functions and variables could have been put here but it was
 * easier to make them global because some needed to be referenced inside Move and Results.
 * We have therefore added no additional data or methods beyond those we extend from baseGame.
 * 
 *    BShip_Game(BShip_Move* move_ptr, BShip_Results* results_ptr, RF69Radio* radio_ptr, bool isPlayer_1)
 *      : baseGame((baseMove*)move_ptr, (baseResults*)results_ptr, (baseRadio*)radio_ptr, isPlayer_1) {};
 *          Standard constructor passes typecast pointers to the base constructor.
 *          
 *    void setup(void)
 *      Runs ONCE during the setup() function of the main program. Initializes Arcada Device and
 *      display. Initializes global variables.
 *      
 *    void initialize(void)
 *      This method is called by the game engine at the start of each new game.
 *      
 *    bool coinFlip(void)
 *      This method called by the offering player to determine who goes first using a random number.  
 *      Prompts the user to press "A" to initiate the flip. If it returns true then the offering 
 *      player goes first. If it returns false, the accepting player goes first. The game engine
 *      handles everything else. We just produce a true or false outcome of the flip.
 *      
 *    void processGameOver(void);
 *      Called at the end of the game. We display a message saying "Press 'Start' to restart."
 *      
 *    void fatalError(const char* s);
 *      Called in the event of an unrecoverable transmission error or programming logic error. 
 *      Prints a message and terminates the game.
 *      
 *    void loopContents(void);
 *      This method is called within your main program loop() function. It allows us to do something
 *      as the gameState changes from one state to another. 
 *      
 *    void processFlip(bool coin);
 *      This method is called to inform the outcome of the coin flip. Prints a message.
 *      
 *    void foundGame(void);
 *      This method is called on the seeking player when he finds a game. Prints a message.
 */
class BShip_Game : public baseGame {
  public:
    BShip_Game(BShip_Move* move_ptr, BShip_Results* results_ptr, RF69Radio* radio_ptr, bool isPlayer_1)
        : baseGame((baseMove*)move_ptr, (baseResults*)results_ptr, (baseRadio*)radio_ptr, isPlayer_1) {};
    void setup(void) override;
    void initialize(void) override;
    bool coinFlip(void) override;
    void processGameOver(void) override;
    void fatalError(const char* s) override;
    void loopContents(void) override;
    void processFlip(bool coin) override;
    void foundGame(void) override;
};

/*
 * Called once during the main program setup() function
 */
void BShip_Game::setup(void) {
  #if(ACCESSIBLE_INPUT)
    //we use the serial monitor for alternate input
    Serial.begin(115200); while (!Serial) {myDelay(1);};
    Serial.print("\n\n\n\nTwo Player Game Setup. You are player #");
    Serial.println(myPlayerNum);
  #endif
  Device.arcadaBegin();
  Device.displayBegin();
  Device.setBacklight(255);
  centerX=Device.display->width()/2;
  centerY=Device.display->height()/2-4;
  setupWave();
  baseGame::setup();  //MUST call this
}
/*
 * Non-method function centers the text for the splash screen
 */
void CenterTextH(const char* text,int16_t y) {
  int16_t x1,y1;
  uint16_t w,h;
  Device.display->getTextBounds(text,0,y,&x1,&y1,&w,&h);
  Device.display->setCursor(Device.display->width()/2-w/2,y);
  Device.display->print(text);
}
//Code to place the ships randomly or manually
#include "board_setup.h"
/*
 * Called at the beginning of each game. Prints splash screen, erase the board
 */
void BShip_Game::initialize(void) {
  randomSeed(millis());
  Device.display->fillScreen(ARCADA_GREEN);
  Device.display->setFont(&FreeSans12pt7b);
  Device.display->setTextColor(ARCADA_WHITE);
  CenterTextH("Welcome", 40);
  CenterTextH("to",65);
  CenterTextH("Battleship",90);
  myDelay(2000);
  Device.display->setFont();
  uint8_t i,j;
  for(i=0;i<100;i++){
    sea[i]=GRID_EMPTY;
    radar[i]=GRID_EMPTY;
  }
  Device.display->fillScreen(ARCADA_GREEN);
  placeShips(); //See board_setup.h
  for(i=0;i<5;i++) {
    Device.pixels.setPixelColor (i,0,50,0);
    EnemyShips[i]= false;
    //loops through the length of the ship and posts it on the board
    for(j=0;j<Ships[i].length;j++) {  
      sea[Ships[i].index + j*((Ships[i].vertical)?10:1)] =
          (Ships[i].sunk) ? GRID_HIT :(grid_t)((uint8_t)GRID_SHIP_0+i);
    }
  }
  Device.pixels.show();

  #if (SELF_TEST)
    Serial.println("kits = 15, no hits and no sink on ship 4");
    EnemyHits=15; 
    Ships[4].hits=0;
    Ships[4].sunk=false;
  #else
    EnemyHits=0;
  #endif
};

/*
 * Random coin flip
 */
bool BShip_Game::coinFlip(void) {
  drawBoard(SEA_BOARD);
  bottomMessage("Flipping coin.");
  Device.infoBox ("Offer accepted!\nFlip the coin to see who goes first.");
  playWave("lets_play.wav");
  Device.display->fillScreen(ARCADA_GREEN);
  #if (SELF_TEST)
    bool coin=true;
  #else
    bool coin=random(2);//a random integer less than 2 i.e. zero or one
  #endif
  if(coin) {
    Device.infoBox ("I won the toss!",0);
    myDelay(2000);
  } else {
    Device.infoBox ("Opponent won the toss!",0);
  }
  return coin;
};

/*
 * Called at the end of the game. Prompts user to press "Start" to restart.
 */
void BShip_Game::processGameOver(void) {
  myDelay(5000);
  Device.infoBox("Press 'Start' to restart.", ARCADA_BUTTONMASK_START);
  initialize();
};
/*
 * We hope this is never needed. Terminates the game an event of fatal error
 */
void BShip_Game::fatalError(const char* s) {
  Serial.printf("Fatal error '%s'\n",s);
  sprintf(message, "Fatal Error '%s'",s);
  Device.errorBox(message, ARCADA_BUTTONMASK_START);
  gameState= GAME_OVER;
}
/*
 * Allows us to print prompts and messages as we go through each state of the game.
 * Calls baseGame::loopContents() so the game engine can do its job.
 */
void BShip_Game::loopContents(void) {
  gameState_t saveState=gameState;  //save the gameState for postprocessing
  //This is processing we do before the game engine does a particular state
  switch(gameState) {
    case OFFERING_GAME: 
      drawBoard(SEA_BOARD);
      bottomMessage("Offering a game.");
      Device.alertBox ("Offering a game.", ARCADA_WHITE, ARCADA_BLACK,0);
      playWave("shall_we.wav");
      break;
    case SEEKING_GAME:
      drawBoard(SEA_BOARD);
      bottomMessage("Seeking a game.");
      Device.alertBox("No response... Seeking a game.", ARCADA_WHITE, ARCADA_BLACK,0);
      break;
    case MY_TURN:      
      break;
    case OPPONENTS_TURN: 
      drawBoard(SEA_BOARD);
      bottomMessage("Waiting for move #%d", currentMoveNum);
      break;
  }
  baseGame::loopContents(); //MUST call this to let the game engine do its thing
  //This is processing we do after the game engine does a particular state
  //we have to use saveState because the baseGame::loopContents() will change the state
  switch(saveState) {
    case OFFERING_GAME: 
      myDelay(3000);  //gives time for post coin flip sound effects 
      break;
    case OPPONENTS_TURN:
      if(((BShip_Results*)Results)->shipDestroyed>=0) {
        myDelay(9000);//goodbye.wav clip is extra long. This compensates.
      }
      break;
  }
  #if (SELF_TEST)
    if(currentMoveNum>SELF_TEST) {
      Serial.println("Self test completed.");
      gameState= GAME_OVER;
      myDelay(10000);
    }
  #endif
}

void BShip_Game::processFlip(bool coin) {
  playWave("lets_play.wav");
  drawBoard(SEA_BOARD);
  if(coin) {
    Device.infoBox("We lost the coin flip so we wait on our opponents first move.",0);
  } else {
    Device.infoBox("We won the toss! We go first.",0);
    myDelay(4000);
  }
}
void BShip_Game::foundGame(void) {
  drawBoard(SEA_BOARD);
  Device.infoBox("Found a game. Waiting on the coin toss.",0);
}
