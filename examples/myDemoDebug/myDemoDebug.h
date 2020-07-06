/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 **********************************************************/
/*
 * Demonstration and debugging game for the Two Player Game System
 * 
 * This game uses no graphics, buttons, or joysticks. Everything happens on the serial monitor. 
 * It was used for development and debugging of the basic logic of the game engine and to
 * illustrate how to create derived classes from the baseMove, baseResults, and baseGame 
 * classes. 
 * 
 * Because we wanted to illustrate areas where we might display a message on a screen in a more realistic 
 * game, we created the function screenMessage(const char* s) and screenMessage(const char* s,uint32_t v)
 * to display some messages. We wanted to distinguish these from text that we use simply because
 * this is a serial monitor textbased "game".
 */
void screenMessage(const char* s) {
  Serial.print("Screen message='");
  Serial.print(s);
  Serial.println("'");
}

void screenMessage(const char* s,uint32_t v) {
  Serial.print("Screen message='");
  Serial.print(s);Serial.print(v);
  Serial.println("'");
}
/****************************************************************
 *    myDemoMove class and methods
 ****************************************************************/
/*
 * This is our derived move object. Just so that we have something to transmit, we've defined 
 * an array of four integers "int16_t Data[4] so we can pass some information with our move.
 * 
 *    size_t my_size();
 *      This method returns the size of our actual instantiated Move object. It MUST be 
 *      defined exactly as shown in order to work properly with the base classes.
 *      
 *    void decideMyMove(void);
 *      Normally a game would use joysticks, buttons, touchscreen or touchpad inputs
 *      to allow the user to enter their move. We will use commands typed into the serial monitor
 *      just for demonstration purposes.
 *     
 *    virtual void print(void); 
 *      Prints debug messages on the serial monitor. Calls baseMove::print() which in turn calls
 *      basePacket::print() to print generic parts of the move. Note it is "print" and not
 *      "println".
 */
class myDemoMove : public baseMove {
  public:
    int16_t Data[4];
    //Note the use of the "override" identifier which ensures
    //that your method is properly defined to override the base method.
    size_t my_size() override { return sizeof( *this ); };
    void decideMyMove(void)override;
    #if(TPG_DEBUG)
      void print(void)override;
    #endif
};

/*
 * We have provided a number of "commands" that can be entered via the serial monitor to issue
 * our move. A prompt is printed and then we enter one of the following:
 *    "N" followed by up to 4 integers to make a normal move. The data is passed as part of the move.
 *    "D" mmakes a default move the same as "N 1 2 3 4". I just got tired of typing in numbers to test :-)
 *    "Q" quits the game and the result is we lose.
 *    "P" issues a PASS_MOVE. While most games will use this, you might have a game in which a "pass" 
 *        is a legal move.
 *    "W" issues a WINNING_MOVE. For demonstration purposes, we defined certain moves to be a "win",
 *        "lose", or "tie". It is up to the "generateResults" method to determine if my opponent's move 
 *        is a win, lose, or tie move. See the discussion under myDemoResults::generateResults for more 
 *        info. We created the "W", "L", and "T" options so we wouldn't have to type particular
 *        sequences of numbers to test these features.
 *    "L" issues a LOSING_MOVE.
 *    "T" issues a TIE_MOVE.
 * Before we take input from the user we flush the serial input buffer just make sure there wasn't something    
 * left over from the previous month.
 */
void myDemoMove::decideMyMove(void) {
  uint8_t i;
  subType=NORMAL_MOVE;
  //Create a default move for now.
  for(i=0;i<4;i++) {
    Data[i]=i+1;
  }
  while(Serial.available()) Serial.read();//flush the input buffer
  //Create a move from the serial monitor input
  screenMessage ("My move #", moveNum);
  Serial.print("Enter my move: 'N' n,n,n,n  'Q' 'P' 'W' 'L' 'D' 'T'->");
  while(!Serial.available()) {delay(1); };
  uint8_t C= toupper(Serial.read());Serial.write(C);
  switch (C) {
    case 'N':
      for(i=0;i<4;i++) {
        Data[i]= Serial.parseInt();
        Serial.print("  "); Serial.print(Data[i]);
      }
      Serial.println(" Normal move");
      break;
    case 'Q': Serial.println(" Quitting Move"); subType=QUIT_MOVE;break;
    case 'P': Serial.println(" Pass Move"); subType=PASS_MOVE; break;
    case 'W': Data[3]=6; Serial.println(" Winning Move"); break;
    case 'L': Data[3]=-6; Serial.println(" Losing Move"); break;
    case 'T': Data[0]=Data[1]=Data[2]=Data[3]=5;Serial.println(" Tie Move"); break;
    case 'D': Serial.println(" Default Move"); break;
    default:  Serial.println("\nUnknown option. Sending a default move.");
  }
}
/*
 * Our debug print move method
 */
#if(TPG_DEBUG)
  void myDemoMove::print(void) {
    baseMove::print();    //Prints info about basePacket and baseMove
    Serial.print(" Data=(");
    for(uint8_t i=0;i<4;i++) {
      Serial.print (Data[i]);
      if(i<3)Serial.print(",");
    }
    Serial.print(") ");//Don't in with a println. 
  };
#endif


/****************************************************************
 *    myDemoResults class and methods
 ****************************************************************/
/*
 * Our results class. See the discussion in "TwoPlayerGame_base_packet.h" under baseResults
 * for a discussion of why you might or might not need "results" for your particular game.
 * We MUST issue a results packet for every move. It's also a way to inform our opponent
 * whether or not his move was a win, lose, or tie.
 * 
 * For demonstration purposes we have defined a single integer "Data" that is our result along with
 * the result type.
 * 
 *    size_t my_size();
 *      This method returns the size of our actual instantiated Results object. It MUST be 
 *      defined exactly as shown in order to work properly with the base classes.
 *    
 *    bool processResults(void)
 *      This method is called so that we can look at and deal with the Results packet received after
 *      our move. It informs us if our move into the game with a win, lose, or tie. In our 
 *      implementation we simply print out some messages based on the results we received.
 *      Returns true if the game was over as a result of our move.
 *      
 *    bool generateResults(baseMove* Move);
 *      In this method we generate the results of our opponent's move. Although the parameter is defined
 *      as a pointer to a baseMove is actually a pointer to one of our move objects myDemoMove which we 
 *      typecast back again. For demonstration purposes just so we would have something to do here,
 *      we made up the following "rules"...
 *      
 *      1. Compute this sum Data[0] + Data[1] + Data[2] and return that as our results "Data". 
 *      2. If that sum is equal to Data[3] we declare that a winning move. 
 *      3. If the sum is equal to -Data[3] then we defined that as an automatic losing move. 
 *      4. If all four data items are equal it's a tie.
 *      Sure it's ridiculous but it's just used to check out the game engine logic.
 *      
 *    virtual void print(void); 
 *      Prints debug messages on the serial monitor. Calls baseResults::print() which in turn calls
 *      basePacket::print() to print generic parts of the move. Note it is "print" and not
 *      "println".
 */
class myDemoResults : public baseResults {
  public:
    int16_t Data;
    size_t my_size() override { return sizeof( *this ); };
    bool processResults(void)override;
    bool generateResults(baseMove* Move)override;
    #if(TPG_DEBUG)
      void print(void)override;
    #endif  
};

/*
 * Process a results packet. Return true if the game is over, false otherwise.
 */
bool myDemoResults::processResults(void) {
  DEBUGLN("Processing results");
  DEBUG_PRINT; DEBUGLN();
  Serial.print("Results for #");Serial.print(resultsNum);Serial.print("\t");
  switch(subType) {
    case NORMAL_RESULTS: screenMessage("Normal results."); return false;
    case HIT_RESULTS:    screenMessage("Hooray! I got a hit."); return false;
    case MISS_RESULTS:   screenMessage("Rats I missed."); return false;
    case WIN_RESULTS:    screenMessage("Hallelujah!! I win!"); return true;
    case LOSE_RESULTS:   screenMessage("$#!+ I lost :-("); return true;
    case TIE_RESULTS:    screenMessage("It's a tie"); return true;
    default:    Serial.print("Unknown result type="); Serial.println(subType);
  }
}
/*
 * Processes your opponents move and generate a results packet which the game engine will send.
 * See the above discussion on how we generated these results.
 * Returns true if game is over.
 */
bool myDemoResults::generateResults(baseMove* M) {
  myDemoMove* Move = (myDemoMove*)M;//saves us a bunch of type casts
  screenMessage("Received Move #", Move->moveNum);
  resultsNum = Move->moveNum;
  //generate some silly results for demo purposes
  Data= Move->Data[0] + Move->Data[1] + Move->Data[2];
  switch(Move->subType) {
    case NORMAL_MOVE:
      if(Move->Data[3] == Data) {
        screenMessage("Rats! Opponent won");
        subType=WIN_RESULTS;
        return true;
      } else if (Move->Data[3] == -Data) {
        screenMessage("The idiot lost :-)");
        subType=LOSE_RESULTS;
        return true;
      } else if ((Move->Data[0]==Move->Data[1]) && (Move->Data[0]==Move->Data[2]) &&
                 (Move->Data[0]==Move->Data[3]) ) {
        screenMessage("It's a tie");
        subType=TIE_RESULTS;
        return true;
      }
      //if it's not a winning or losing move then we will randomly send one of the following types
      //NORMAL_RESULTS, HIT_RESULTS, or MISS_RESULTS
      subType=(packetSubType_t)(NORMAL_RESULTS+random(3)); //for now use random results just to test
      DEBUG("Created result of '"); DEBUG(packetSubTypeStr[subType]); DEBUG("'");
      return false;
    case PASS_MOVE:
      screenMessage("Opponent passed.");
      subType=NORMAL_RESULTS;
      return false;
    case QUIT_MOVE:
      screenMessage("Opponent quit.");
      subType=LOSE_RESULTS;
      return true;
  };
}
/*
 * Print results data for debugging
 */
#if(TPG_DEBUG)
  void myDemoResults::print(void) {
    baseResults::print();//Print info from basePacket and baseResults
    Serial.print(" Data="); Serial.print(Data);
    Serial.print(" ");//don't end with println, it's handled elsewhere
  }
#endif


/****************************************************************
 *    myDemoGame class and methods
 ****************************************************************/
/*
 * Most of the logic of the game engine is handled by baseGame. We extend the baseGame with our own
 * data and methods. We will create a single instance of this new class and pass it pointers to
 * a move object, a results object, and a radio object. See "TwoPlayerGame_base_game.h" for details.
 * 
 *    myDemoGame(myDemoMove* move_ptr, myDemoResults* results_ptr, RF69Radio* radio_ptr, bool isPlayer_1)
 *      : baseGame((baseMove*)move_ptr, (baseResults*)results_ptr, (baseRadio*)radio_ptr, isPlayer_1) {};
 *          Constructor. Basically just does a typecast from our Move, Results, and Radio objects
 *          into their base versions and passes them on to the base constructor. See baseGame for
 *          details about these parameters as well as the "isPlayer_1" flag.
 *          
 *    void setup(void)
 *      This method is called by the game engine ONCE during the setup() function of your main program.
 *      It MUST call baseGame::setup() before doing anything else.
 *      
 *    void initialize(void)
 *      This method is called by the game engine at the start of each new game.
 *      
 *    bool coinFlip(void)
 *      This method is called by the offering player to determine who goes first.  If it returns true
 *      then the offering player goes first. If it returns false then the accepting player
 *      goes first. The outcome of the flip is sent to the accepting player via a COIN_FLIP_PACKET.
 *      We implemented it as a random coin toss. You could implement some other method.
 *      
 *    void processGameOver(void);
 *      This method is called at the end of a game. It allows us to display messages or whatever we want.
 *      
 *    void fatalError(const char* s);
 *      This method is called in the event of an unrecoverable transmission error or a fatal programming
 *      logic error. Lets us print some sort of message of apology :-)
 *      
 *    void loopContents(void);
 *      This method is called within your main program loop() function. It allows us to do something
 *      as the gameState changes from one state to another. It MUST call baseGame::loopContents()
 *      which is the heart of the game state engine.
 *      
 *    void processFlip(bool coin){};
 *      Used by the accepting player to process an incoming COIN_FLIP_PACKET. Base method does nothing.
 *      We use it to print a message giving the outcome of the flip.
 */
class myDemoGame : public baseGame {
  public:
    myDemoGame(myDemoMove* move_ptr, myDemoResults* results_ptr, RF69Radio* radio_ptr, bool isPlayer_1)
        : baseGame((baseMove*)move_ptr, (baseResults*)results_ptr, (baseRadio*)radio_ptr, isPlayer_1) {};
    void setup(void) override;
    void initialize(void) override {randomSeed(millis());};
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
void myDemoGame::setup(void) {
  //Because we use the serial monitor, if debugging is off then we must initialize it here.
  #if (!TPG_DEBUG)
    Serial.begin(115200); while(!Serial) {delay(1);};
  #endif
  Serial.print("\n\n\n\nTwo Player Game Setup. You are player #");
  Serial.println(myPlayerNum);
  baseGame::setup();  //MUST call this 
}

/*
 * Random coin flip
 */
bool myDemoGame::coinFlip(void) {
  screenMessage("Offer accepted. Flipping a coin.");
  bool coin=random(2);//a random integer less than 2 i.e. zero or one
  if(coin) {
    screenMessage("I won the toss!");
  } else {
    screenMessage("Opponent won the toss.");
  }
  return coin;
};

/*
 * Processes ecounselingnd of game
 */
void myDemoGame::processGameOver(void) {
  Serial.println("Game over. Enter any character to restart the game.");
  Serial.flush();
  while(Serial.available()) {Serial.read();};
  while(!Serial.available()) {delay(1);};
  Serial.println("Restarting game.\n\n\n");
};

void myDemoGame::fatalError(const char* s) {
  Serial.print("FATAL error '");
  Serial.print(s);
  Serial.print("'");
  gameState= GAME_OVER;
}

void myDemoGame::loopContents(void) {
  switch(gameState) {
    case OFFERING_GAME:  screenMessage("Offering a game.");           break;
    case SEEKING_GAME:   screenMessage("No reply. Seeking a game.");  break;
    case MY_TURN:        screenMessage("It's my turn.");              break;
    case OPPONENTS_TURN: screenMessage("Waiting on opponent's move.");break;
    case GAME_OVER:      screenMessage("Game over.");                 break;
  }
  baseGame::loopContents(); //MUST call this
}
void myDemoGame::processFlip(bool coin) {
  if(coin) {
    screenMessage("Other player won the toss. Waiting for his move. ");
  } else {
    screenMessage("We won the toss.");
  }
}
void myDemoGame::foundGame(void) {
  screenMessage("Found a game. Waiting on coin flip.");
}
