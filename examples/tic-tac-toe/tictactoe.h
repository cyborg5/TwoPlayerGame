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
 * This demonstration program implements a simple game of tic-tac-toe.
 */
#include <Adafruit_Arcada.h>

//Set this to true for an alternate assistive technology input using serial monitor
//imput in addition to the joystick and buttons.
#define ACCESSIBLE_INPUT false

#if(ACCESSIBLE_INPUT)
  #include <AccessibleArcada.h>   //alternate input system for assistive technology
  AccessibleArcada Device;
#else
  Adafruit_Arcada Device;
#endif

//Font used in opening splash screen
#include <Fonts/FreeSans12pt7b.h>

//Possible contents of a square
enum squares_t {SQUARE_EMPTY, SQUARE_X, SQUARE_O};

//Ways to win
enum win_t {NO_WIN, TOP_ROW, MIDDLE_ROW, BOTTOM_ROW, LEFT_COLUMN, MIDDLE_COLUMN, RIGHT_COLUMN,
            DESCENDING_DIAGONAL, ASCENDING_DIAGONAL, TIE};

/************************************************************************************
 * Various global variables not really part of the game engine object. Need to be able to
 * access these from a variety of locations so we made them global.
 *************************************************************************************/
char message[40];     //buffer to be used with sprintf

squares_t board[9];   //The squares of the board

//"Center" of the tic-tac-toe board in pixels. Slightly higher than the actual center
//  of the screen to make room for the bottom message area. Initialized in TTT_Game::setup()
uint16_t centerX;     
uint16_t centerY;

//Player #1 is always SQUARE_X and Player #2 is always SQUARE_O
// Initialized in TTT_Game::setup()
squares_t mySymbol;
squares_t opponentsSymbol;

#define SIZE_OF_SQR 40
#define X_SIZE (SIZE_OF_SQR/2-8)
/************************************************************************************
 * Various global functions not really part of the game engine object. Need to be able to
 * access these from a variety of locations so we made them global.
 *********************************************************/
/*
 * Displays a text message across the bottom of the screen in the default font
 */
void bottomMessage(const char*text) {
  Device.display->fillRect(0,Device.display->height()-8, Device.display->width(),8, ARCADA_BLACK);
  Device.display->setFont();
  Device.display->setTextSize(1);
  Device.display->setTextColor(ARCADA_WHITE);
  Device.display->setCursor(0,Device.display->height()-8);
  Device.display->print(text);
}

/*
 * Draws an X or O or blank at index "i" of the specified color
 */
void drawSquare(squares_t Type, uint8_t i,uint16_t color) {
  uint16_t sx=centerX + ((i % 3) -1)*SIZE_OF_SQR;
  uint16_t sy=centerY + ((i / 3) -1)*SIZE_OF_SQR;
  switch(Type) {
    case SQUARE_EMPTY:
      Device.display->fillRect(sx-SIZE_OF_SQR/2+3,sy-SIZE_OF_SQR/2+3, SIZE_OF_SQR-6, SIZE_OF_SQR-6, ARCADA_BLACK);
      break;
    case SQUARE_X:
      for(int8_t j=-3;j<3;j++) {
        Device.display->drawLine(sx-X_SIZE+j,sy-X_SIZE,sx+X_SIZE+j,sy+X_SIZE,color); 
        Device.display->drawLine(sx+X_SIZE+j,sy-X_SIZE,sx-X_SIZE+j,sy+X_SIZE,color);           
      } 
      break;
    case SQUARE_O:
      Device.display->fillCircle(sx,sy, SIZE_OF_SQR/2-6, color); 
      Device.display->fillCircle(sx,sy, SIZE_OF_SQR/2-11, ARCADA_BLACK); 
      break;
  }
}
/*
 * Draws the tic-tac-toe board lines and then fills in the symbols
 */
void drawBoard(void) {
  Device.display->fillScreen(ARCADA_BLACK);
  Device.display->fillRect(centerX-SIZE_OF_SQR/2-3,centerY-SIZE_OF_SQR*1.5+3,6,SIZE_OF_SQR*3-6, ARCADA_WHITE);   
  Device.display->fillRect(centerX+SIZE_OF_SQR/2-3,centerY-SIZE_OF_SQR*1.5+3,6,SIZE_OF_SQR*3-6, ARCADA_WHITE);   
  Device.display->fillRect(centerX-SIZE_OF_SQR*1.5+3,centerY-SIZE_OF_SQR/2-3,SIZE_OF_SQR*3-3,6, ARCADA_WHITE);   
  Device.display->fillRect(centerX-SIZE_OF_SQR*1.5+3,centerY+SIZE_OF_SQR/2-3,SIZE_OF_SQR*3-3,6, ARCADA_WHITE);   
  //if we see this message, it means we should have written a different message somewhere
  bottomMessage("testing 123 this is a test");
  for(uint8_t i=0;i<9;i++) {
    drawSquare(board[i],i, ARCADA_WHITE);
  }
}
/*
 * Draws horizontal, vertical or diagonal lines depending on the type of game win
 */
void drawWin(win_t w) {
  switch(w) {
    case TOP_ROW:
    case MIDDLE_ROW:
    case BOTTOM_ROW:
      Device.display->fillRect(centerX-SIZE_OF_SQR*1.5+3,centerY-SIZE_OF_SQR-3+(w-TOP_ROW)*SIZE_OF_SQR,SIZE_OF_SQR*3-3,6, ARCADA_GREEN);   
      break;
    case LEFT_COLUMN:
    case MIDDLE_COLUMN:
    case RIGHT_COLUMN:
      Device.display->fillRect(centerX-SIZE_OF_SQR-3+(w-LEFT_COLUMN)*SIZE_OF_SQR,centerY-SIZE_OF_SQR*1.5+3,6,SIZE_OF_SQR*3-6, ARCADA_GREEN);
      break;
    #define DIAGONAL_SIZE (SIZE_OF_SQR*1.25)
    case DESCENDING_DIAGONAL:
      for(int8_t i=-3;i<3;i++) {
        Device.display->drawLine(centerX-DIAGONAL_SIZE+i, centerY-DIAGONAL_SIZE, 
          centerX+DIAGONAL_SIZE+i, centerY+DIAGONAL_SIZE, ARCADA_GREEN);
      }
      break;
    case ASCENDING_DIAGONAL:
      for(int8_t i=-3;i<3;i++) {
        Device.display->drawLine(centerX+DIAGONAL_SIZE+i, centerY-DIAGONAL_SIZE, 
          centerX-DIAGONAL_SIZE+i, centerY+DIAGONAL_SIZE, ARCADA_GREEN);
      }
      break;
  }
}

/****************************************************************
 *    TTT_Move class and methods
 ****************************************************************/
/*
 * This is our derived move object. 
 * 
 *    uint8_t square;
 *      The board index of the location where we placed our X or O.
 * 
 *    size_t my_size();
 *      This method returns the size of TTT_Move object. 
 *      
 *    void decideMyMove(void);
 *      Prompts us to make our move. See details below.
 */
class TTT_Move : public baseMove {
  public:
    uint8_t square;
    size_t my_size() override { return sizeof( *this ); };
    void decideMyMove(void)override;
};

/*
 * Takes non-analog input from the joystick (UP, DOWN, LEFT, RIGHT) and "SELECT" button to 
 * place your mark on one of the squares. 
 * 
 * Draws our mark in green as a kind of cursor. If you move it over an existing mark, it turns red
 * and refuses to let you select that location. The cursor automatically begins at the first blank square.
 * When you press "SELECT" it turns WHITE and sets Move->square to the index of the selected square. 
 * 
 * If you press "START" it restarts the game. The "A" and "B" buttons do nothing. 
 */
void TTT_Move::decideMyMove(void) {
  subType=NORMAL_MOVE;
  //put the cursor at the first nonempty square. Assumes there is one.
  square=0;
  while( (square<9) && (board[square] != SQUARE_EMPTY) ) {
    square++;
  }
  uint32_t Buttons;
  drawBoard();
  drawSquare(mySymbol, square, (board[square])?ARCADA_RED: ARCADA_GREEN);//the cursor
  sprintf(message, "Your move #%d",moveNum);
  bottomMessage(message);
  while(true) {
    if (Buttons=Device.readButtons()) {//not an error
      Device.readButtons();//flush out or de-bounce
      drawSquare(SQUARE_EMPTY, square, ARCADA_WHITE);//erase cursor from the current position
      drawSquare(board[square], square, ARCADA_WHITE);//redraw its previous contents
      switch(Buttons){
        case ARCADA_BUTTONMASK_UP:    square = (square+(9-3)) % 9; break;    
        case ARCADA_BUTTONMASK_DOWN:  square = (square+3) % 9; break;    
        case ARCADA_BUTTONMASK_LEFT:  square = (square+(9-1)) % 9; break;    
        case ARCADA_BUTTONMASK_RIGHT: square = (square+1) % 9; break;    
        case ARCADA_BUTTONMASK_SELECT: 
          //We made our selection. 
          if(board[square]== SQUARE_EMPTY) {
            board[square]=mySymbol;
            drawBoard();
            return;
          } else {
            Device.warnBox("Square Already Occupied",0);
            delay(2000);
            drawBoard();
            break;
          }
        case ARCADA_BUTTONMASK_START: //quit the game
          Device.infoBox("Quitting the game",0);
          subType=QUIT_MOVE;
          return; 
      }
      drawSquare(mySymbol, square, (board[square])?ARCADA_RED: ARCADA_GREEN);//cursor at new location
    }
  }
};

/****************************************************************
 *    TTT_Results class and methods
 ****************************************************************/
/*
 *  This is our derived results object.
 * 
 *    win_t Win;
 *      The type of winning move is set by TTT_Results::generateResults(Move);
 *      It can be one of the following values: NO_WIN, TOP_ROW, MIDDLE_ROW, BOTTOM_ROW, 
 *      LEFT_COLUMN, MIDDLE_COLUMN, RIGHT_COLUMN, DESCENDING_DIAGONAL, ASCENDING_DIAGONAL, or TIE.  
 *      
 *    size_t my_size();
 *      This method returns the size of the TTT_Results object.
 *    
 *    bool processResults(void)
 *      Processes the results packet we received in response to our move. See below for details.
 *      
 *    bool generateResults(baseMove* Move);
 *      Generate results package in response to our opponent's move. It's up to us to decide if it was
 *      a tie or a winning move and what kind. See below for details.
 */
class TTT_Results : public baseResults {
  public:
    win_t Win;
    size_t my_size() override { return sizeof( *this ); };
    bool processResults(void)override;
    bool generateResults(baseMove* Move)override;
};

/*
 * Process a results packet. Return true if the game is over, false otherwise.
 */
bool TTT_Results::processResults(void) {
  switch(subType) {
    case NORMAL_RESULTS: return false;
    case WIN_RESULTS:   drawWin(Win); bottomMessage("Hallelujah!! I win!"); return true;
    case TIE_RESULTS:   bottomMessage("It's a draw");  return true;
    case LOSE_RESULTS:  bottomMessage("I quit"); ;return true;//if we resigned, we get lose results
    //We don't use HIT_RESULTS or MISS_RESULTS in this game.
  }
}
/*
 * Extra non-method function determines if we got a tic-tac-toe or a tie.
 */
win_t checkForWin(void) {
  uint8_t i;
  uint8_t countEmpty=0;
  //if there are no empty squares it's a tie
  for(i=0;i<9;i++) {
    if(board[i]== SQUARE_EMPTY){
      countEmpty++;
    }
  }
  if(countEmpty==0) {
    return TIE;
  }
  //If there is a "tic-tac-toe" generate WIN_RESULTS
  for(i=0;i<3;i++) {
    //check the rows
    if( (board[i*3]==opponentsSymbol) && (board[i*3]==board[i*3+1]) && (board[i*3]==board[i*3+2]) ) { 
      return (win_t)(((uint8_t)TOP_ROW)+i);
    }
    //check the columns
    if( (board[i]==opponentsSymbol) && (board[i]==board[i+3]) && (board[i]==board[i+6]) ) {
      return (win_t)(((uint8_t)LEFT_COLUMN)+i);
    }
  }
  //check the diagonals
  //upper left to lower right
  if( (board[0]==opponentsSymbol) && (board[0]==board[4]) && (board[0]==board[8]) ) {  
    return DESCENDING_DIAGONAL;
  }
  //upper right to lower left
  if( (board[2]==opponentsSymbol) && (board[2]==board[4]) && (board[2]==board[6]) ) { 
    return ASCENDING_DIAGONAL;
  }
  return NO_WIN;
}
/*
 * Looks at your opponent's move to see if it was a winning move. Sets Results->Win and Results->subType
 * appropriately. Returns true if game is over.
 */
bool TTT_Results::generateResults(baseMove* M) {
  TTT_Move* Move = (TTT_Move*)M;//saves us a bunch of type casts
  board[Move->square]=opponentsSymbol;
  drawBoard();
  resultsNum = Move->moveNum;
  switch(Move->subType) {
    case NORMAL_MOVE:
      switch(Win=checkForWin()) {
        case TIE:
          subType=TIE_RESULTS; 
          bottomMessage("Its a tie.");
          return true;
        case NO_WIN:
          subType=NORMAL_RESULTS;
          return false;
        default:
          drawWin(Win);
          subType=WIN_RESULTS;
          bottomMessage("Rats! Opponent won.");
          return true;
      }
      break;//not really necessary
    case QUIT_MOVE:
      Device.infoBox("Opponent quit.",0);
      bottomMessage("I win. Opponent quit.");
      subType=LOSE_RESULTS;
      return true;
    //case PASS_MOVE: not used in this game
  };
}


/****************************************************************
 *    TTT_Game class and methods
 ****************************************************************/
/*
 * Most of the logic of the game engine is handled by baseGame. We extend the baseGame with our own
 * data and methods. Some of our global functions and variables could have been put here but it was
 * easier to make them global because some needed to be referenced inside Move and Results.
 * We have therefore added no additional data or methods beyond those we extend from baseGame.
 * 
 *    TTT_Game(TTT_Move* move_ptr, TTT_Results* results_ptr, RF69Radio* radio_ptr, bool isPlayer_1)
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
class TTT_Game : public baseGame {
  public:
    TTT_Game(TTT_Move* move_ptr, TTT_Results* results_ptr, RF69Radio* radio_ptr, bool isPlayer_1)
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
void TTT_Game::setup(void) {
  #if(ACCESSIBLE_INPUT)
    //we use the serial monitor for alternate input
    Serial.begin(115200); while (!Serial) { delay(1);};
    Serial.print("\n\n\n\nTwo Player Game Setup. You are player #");
    Serial.println(myPlayerNum);
  #endif
  Device.arcadaBegin();
  Device.displayBegin();
  mySymbol=(squares_t)myPlayerNum;
  Device.setBacklight(90);
  opponentsSymbol=(squares_t)otherPlayerNum;
  centerX=Device.display->width()/2;
  centerY=Device.display->height()/2-5;
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
/*
 * Called at the beginning of each game. Prints splash screen, erase the board
 */
void TTT_Game::initialize(void) {
  randomSeed(millis());
  Device.display->fillScreen(ARCADA_GREEN);
  Device.display->setFont(&FreeSans12pt7b);
  Device.display->setTextColor(ARCADA_WHITE);
  CenterTextH("Welcome", 30);
  CenterTextH("to",55);
  CenterTextH("Tic-Tac-Toe",80);
  if(mySymbol== SQUARE_X) {
    CenterTextH("You are X",105);
  } else {
    CenterTextH("You are O",105);
  }
  for(uint8_t i=0;i<9;i++){
    board[i]=SQUARE_EMPTY;
  }
  delay(5000);
};
/*
 * Random coin flip
 */
bool TTT_Game::coinFlip(void) {
  Device.display->fillScreen(ARCADA_GREEN);
  Device.infoBox ("Offer accepted!\nFlip the coin to see who goes first.");
  Device.display->fillScreen(ARCADA_GREEN);
  bool coin=random(2);//a random integer less than 2 i.e. zero or one
  if(coin) {
    Device.infoBox ("I won the toss!",0);
    delay(2000);
  } else {
    drawBoard();
    Device.infoBox ("Opponent won the toss!",0);
  }
  return coin;
};

/*
 * Called at the end of the game. Prompts user to press "Start" to restart.
 */
void TTT_Game::processGameOver(void) {
  delay(5000);
  Device.infoBox("Press 'Start' to restart.", ARCADA_BUTTONMASK_START);
  initialize();
};
/*
 * We hope this is never needed. Terminates the game an event of fatal error
 */
void TTT_Game::fatalError(const char* s) {
  Device.errorBox("Unrecoverable error.", ARCADA_BUTTONMASK_START);
  gameState= GAME_OVER;
}
/*
 * Allows us to print prompts and messages as we go through each state of the game.
 * Calls baseGame::loopContents() so the game engine can do its job.
 */
void TTT_Game::loopContents(void) {
  switch(gameState) {
    case OFFERING_GAME: 
      Device.display->fillScreen(ARCADA_GREEN);
      Device.alertBox ("Offering a game.", ARCADA_WHITE, ARCADA_BLACK,0);
      break;
    case SEEKING_GAME:
      Device.display->fillScreen(ARCADA_GREEN);
      Device.alertBox("No response... Seeking a game.", ARCADA_WHITE, ARCADA_BLACK,0);
      break;
    case MY_TURN:      
      break;
    case OPPONENTS_TURN: 
      sprintf(message,"Waiting for move #%d", currentMoveNum);
      bottomMessage(message);
      break;
//    case GAME_OVER:      bottomMessage("Game over.");      break;
  }
  baseGame::loopContents(); //MUST call this
}
void TTT_Game::processFlip(bool coin) {
  drawBoard();
  if(coin) {
    Device.infoBox("We lost the coin flip so we wait on our opponents first move.",0);
  } else {
    Device.infoBox("We won the toss! We go first.",0);
    delay(2000);
  }
}
void TTT_Game::foundGame(void) {
  drawBoard();
  bottomMessage("Waiting on coin toss");
  Device.infoBox("Found a game. Waiting on the coin toss.",0);
}
