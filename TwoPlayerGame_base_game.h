/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 **********************************************************/
#ifndef _TwoPlayerGame_base_game_h_
#define _TwoPlayerGame_base_game_h_
/*
 * Base class definition for a game. You will create a derived class that has your game specific
 * data and methods. You will also have to create a derived Radio class, Move class, and
 * Results class which will be explained later. You should then create a single instance of each of 
 * these derived classes. The game engine class takes care of everything else. A typical program 
 * might look as follows:
 * 
 *    #include <TwoPlayerGame.h>
 *    #include "myGame.h" //all of your game specific code here
 *    #define IS_PLAYER_1 true; //change to false on one of the two devices
 *    
 *    myRadio Radio;      //Your custom Radio object derived from baseRadio
 *    myMove Move;        //Your custom Move object derived from base Move
 *    myResults Results;  //Your custom Results object derived from base Results
 *    //Create an instance of my game
 *    myGame Gamee(&Move, &Results, &Radio, IS_PLAYER_1);
 *    
 *    void setup() {
 *      Game.setup(); //One time initialization of everything
 *    }
 *    
 *    void loop() {
 *      Game.loopContents();
 *    }
 *    
 *  You should compile and upload this code to your device and then change IS_PLAYER_1 false
 *  and recompile and upload it to the other device. This is the ONLY difference in
 *  the software between your two devices.
 */
/*
 * Enumerate various states in which the game engine can be. Define strings for debug purposes.
 */
enum gameState_t {
  OFFERING_GAME, SEEKING_GAME, MY_TURN, OPPONENTS_TURN,  GAME_OVER
};
extern const char* gameStateStr[5];

/*
 * This is the primary class that is the logic and game flow for the game engine. You will create
 * a derived class with your game for specific data and methods. 
 * 
 * The game logic is implemented as a state machine using gameState as follows:
 *    1. After initializing everything it goes into gameState=OFFERING_GAME. The radio will send
 *        multiple packets saying that it is offering a game and it waits for an accepting packet.
 *        If that fails after a specified amount of time it goes into gameState=SEEKING_GAME.
 *    2. While seeking a game, your device waits indefinitely to receive an OFFERING_GAME_PACKET.
 *    3. Once a game is accepted, the offering player sends a FOUND_GAME_PACKET to inform the other 
 *        player that they found a game. The offering player then performs a coin flip or other 
 *        method to determine who goes first. The outcome of the flip is sent to the accepting player.
 *        If the flip was true, offering player goes first otherwise accepting player goes first.
 *    4. After a move is sent, a "Results" packet is sent back showing the results of your move.
 *        See the discussion on "Results" in "TwoPlayerGame_base_packet.h".
 *    5. The gameState alternates between MY_TURN and OPPONENTS_TURN until a player wins, ties,
 *        or resigns at which point gameState=GAME_OVER. Depending on your game design
 *        you may then reset gameState to OFFERING_GAME however if both devices enter that state
 *        simultaneously it is possible neither one accepts the others offer. Users 
 *        will have to coordinate and stagger their restart efforts.
 *        
 * The class contains the following data and methods:
 *    uint16_t currentMoveNum;  
 *      The number of the current move
 *    
 *    baseMove* Move;           
 *      Pointer to a move object that will be transmitted between devices. Is used for both
 *      sending our move and receiving our opponents move. You will create your own move class
 *      derived from baseMove and create an instance of it. Then pass its address to the game object
 *      in its constructor. See "TwoPlayerGame_base_packet.h" for definition and documentation of
 *      baseMove and how to create your own derived version.
 *      
 *    baseResults* Results;     
 *      Pointer to a results object that will be transmitted between devices. Is used for both
 *      sending our results and receiving results from our opponent. You will create your own results class
 *      derived from baseResults and create an instance of it. Then pass its address to the game object
 *      in is constructor. See "TwoPlayerGame_base_packet.h" for defamation and documentation of
 *      baseResults and how to create your own derived version.
 *      
 *    baseRadio* Radio;         
 *      Pointer to a radio object that will do the transmitting and receiving of data. You will create
 *      your own Radio class derived from baseRadio and create an instance of it. Then pass its address
 *      to the game object in its constructor. See "TwoPlayerGame_base_radio.h" for information and
 *      documentation of baseRadio and how to create your own derived version. A derived version for the
 *      RF69HCW packet radio is available in "TwoPlayerGame_RF69HCW.h" and "TwoPlayerGame_RF69HCWcpp".
 *      
 *    uint8_t myPlayerNum;
 *      My player number either 1 or 2 computed by constructor based on isPlayer_1 parameter.    
 *      
 *    baseGame(baseMove* move_ptr, baseResults* results_ptr, baseRadio* radio_ptr, bool isPlayer_1);
 *      Constructor for the game object. See the sample program code at the top of this file.  
 *      You should create a constructor for your derived game class as follows:
 * 
 *        myGame(myMove* move_ptr, myResults* results_ptr, myRadio* radio_ptr, bool isPlayer_1)
 *            : baseGame((baseMove*)move_ptr, (baseResults*)results_ptr, (baseRadio*)radio_ptr, isPlayer_1) 
 *        {
 *          //Perform any other initialization here.
 *        };
 *      
 *    virtual void setup(void); 
 *      Call this method ONLY once in your main "setup()" function. See sample code above.
 *      If you create a virtual method in your class you MUST call baseGame::setup(); from it.
 *      Note it calls "initialize" to initialize the first game.
 *      
 *    virtual void loopContents(void);  
 *      Call this method inside your main "loop()" function. See sample code above. If you create 
 *      a virtual method in your class you MUST call baseGame::loopContents(); from it.
 *      
 *    virtual void initialize(void);
 *      This method is called any time a new game starts. If you have your own virtual
 *      method it MUST call baseGame::initialize() somewhere within it.
 *      
 *    bool coinFlip(void);
 *      You MUST implement this pure virtual method in your derived game class. It determines 
 *      who goes first. The Offering player calls this method. If it returns true then the offering 
 *      player goes first, otherwise the accepting player goes first. Outcome of the flip is sent 
 *      to the accepting player via a COIN_FLIP_PACKET.
 *      
 *    void processGameOver(void);
 *      You MUST implement this method in your derived game class. It will be called at the end
 *      of every game. You get to decide what to do.
 *      
 *    void fatalError (const char* s);
 *      You MUST implement this method in your derived game class. It would only be called in the 
 *      event of an unrecoverable error such as failure to receive acknowledgment of a data packet 
 *      or some other programming logic error.
 *      
 *    void processFlip(bool coin){};
 *      Used by accepting player to process an incoming COIN_FLIP_PACKET. Base method does nothing.
 *      You may optionally override this virtual to print a message or take other action.
 *      
 *    void foundGame(void) {};
 *      Used by accepting player to print a message that a game has been found and we are waiting
 *      on the offering player to do the coin toss.
 *      
 *    gameState_t gameState;    
 *      The internal state of the game engine. Legal values are: OFFERING_GAME, SEEKING_GAME, 
 *      MY_TURN, OPPONENTS_TURN, and GAME_OVER.
 *      
 *    void offeringGame(void);
 *    void seekingGame(void);
 *    void doMyTurn(void);
 *    void doOpponentsTurn(void);
 *    void gameOver(void); 
 *      These internal private methods handle each of the game states.
 */
class baseGame {
  public:
    uint16_t currentMoveNum; 
    baseMove* Move;          
    baseResults* Results;    
    baseRadio* Radio;
    uint8_t myPlayerNum;      //For initializing my radio
    uint8_t otherPlayerNum;   //Destination of our transmissions
    baseGame(baseMove* move_ptr, baseResults* results_ptr, baseRadio* radio_ptr, bool isPlayer_1);
    virtual void setup(void); 
    virtual void loopContents(void);
  protected:
    virtual void initialize(void){gameState=OFFERING_GAME;};
    virtual bool coinFlip(void)=0;
    virtual void processGameOver(void)=0;
    virtual void fatalError(const char* s)=0;
    virtual void processFlip(bool coin) {};
    virtual void foundGame(void) {};
    gameState_t gameState;    //The internal state of the game engine, see definitions above
  private:
    //Internal routines that handle each of the various states of the engine
    void offeringGame(void);
    void seekingGame(void);
    void doMyTurn(void);
    void doOpponentsTurn(void);
    void gameOver(void); 
};

#endif //not defined _TwoPlayerGame_base_game_h_
