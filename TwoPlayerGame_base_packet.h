/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 **********************************************************/
#ifndef _TwoPlayerGame_base_packet_h_
#define _TwoPlayerGame_base_packet_h_
#include "TwoPlayerGame_base_radio.h"
/*
 * The game engine communicates by sending packets of data back and forth between devices. There are three
 * types of packets.
 *      1. A base packet which just indicates things like I'm offering a game or I'm accepting a game offer.
 *          You need not extend the base packet with your own class. You will however create your own
 *          Move packet and Results packet classes as explained below.
 *      2. Move packets carry the data indicating what move a player has made. It is derived from the base 
 *          packet. You will create a derived move class consisting of data and methods to suit your needs.
 *      3. Results packets send data back to the person who moved to give them the results of their actions.
 *          It is also derived from the base packet class. You will create a derived results class 
 *          consisting of data and methods to meet your needs. Not all games will need this feature but you 
 *          will have to implement it anyway. See the discussion below.
 *  The game engine creates an instance of a base packet as needed. It also contains a pointer to a
 *  Move packet and a pointer to a Results packet. You will create an instance of your Move object and 
 *  your Results object and pass the addresses to the game object constructor when you create it.
 *  Packet data is transmitted and received using the Radio object. The pointer to the Radio is a required
 *  parameter for the base packet constructor.  The game constructor will set the pointer to the Radio
 *  for Move and Results objects in the game constructor.
 */


/************************************************************************************
 * Base packet class stuff
 ************************************************************************************/
/*
 * Enumerate the different types and subtypes of packets and define string names for 
 * them for debug purposes.
 */
enum packetType_t {
  NO_PACKET_TYPE, OFFERING_GAME_PACKET, ACCEPTING_GAME_PACKET, MOVE_PACKET, RESULTS_PACKET, 
  FOUND_GAME_PACKET,COIN_FLIP_PACKET
};
enum packetSubType_t {
  NO_SUBTYPE, NORMAL_MOVE, PASS_MOVE, QUIT_MOVE, NORMAL_RESULTS, HIT_RESULTS, MISS_RESULTS, WIN_RESULTS, 
	LOSE_RESULTS, TIE_RESULTS, FLIP_TRUE, FLIP_FALSE
};
extern const char* packetTypeStr[7];
extern const char* packetSubTypeStr[10];

/*
 * We want to transmit data from a packet object on this device to an identical packet object on the
 * other device. If we start at address "this" it would include the pointer to the function table. Although
 * we are compiling identical code on identical devices we can't be 100% sure those addresses would be 
 * the same. Similarly the first data item in a packet is a pointer to the radio object which we don't 
 * want to overwrite either. So we have to offset everything by "2*sizeof(void*)". We know that 
 * "sizeof(void*)" is 4 but who knows... we might someday port this to a 64-bit platform. Therefore 
 * using "sizeof(void*)" ensures we get it right. 
 * 
 * WARNING: when implementing your derived Move and Results classes is recommended you DO NOT make use of
 * pointers to data unless you create a mechanism to send the pointed to data in a separate packet and then
 * reassemble it on the other side.
 * 
 * In addition to needing to know where in the object we want to start transmitting data, we also need to 
 * know the size of the derived packet object. The virtual method "my_size()" correctly tells a base method 
 * the size of the actual object. Therefore the data we will transmit in each packet starts at 
 * "this+(2*sizeof(void))" and the length of the data is "my_size()-(2*sizeof(void))". NOTE: the my_size() 
 * gives the size of the object but object sizes are typically rounded off to multiples of 4. So if you have 
 * an odd amount of data, there will be some garbage data at the end to pad it out to a multiple of 4. Make 
 * sure you compile the code for both devices using the same compiler and the same settings to ensure that 
 * such padding occurs the same on both devices.
 * 
 * Below we define PACKET_OFFSET to account for the function table pointer and the pointer to the radio object.
 */
#define PACKET_OFFSET (2*sizeof(void*))

/*
 * The base class for handling a packet. You need not create your own derived class from basePacket.
 * 
 *    baseRadio* Radio;         
 *      Pointer to the radio object that handles transmission. See "TwoPlayerGame_base_radio.h" for details. 
 *      
 *    packetType_t type;        
 *    packetSubType_t subType;
 *      See enum definitions above for legal types
 *      
 *    basePacket(); 
 *    basePacket(baseRadio* radio_ptr); 
 *      Constructors.
 *      
 *    virtual size_t my_size() { return sizeof( *this ); }
 *      Returns the size of the actual object as instantiated. ALL CLASSES derived from this
 *      type MUST implement this method EXACTLY as shown above.
 *      
 *    virtual bool send(void);            
 *      Sends the packet.
 *      
 *    virtual bool send(packetType_t t);  
 *      Sends a simple non-data packet. Used in "baseGame::offeringGame" to send an OFFERING_GAME_PACKET and 
 *      in "baseGame::acceptingGame" to send an ACCEPTING_GAME_PACKET. Returns true if packet was acknowledged.
 *      
 *    bool requireTypeTimeout(packetType_t t,uint16_t timeout);
 *      Waits for the specified time in attempt to receive a particular type of packet from the 
 *      other device. Returns true if the proper packet was received before timeout. Returns false 
 *      if either time ran out or a received packet was the wrong type.
 *      
 *    void requireType(packetType_t t);
 *      Waits indefinitely for a packet of a particular type. Ignores any of other packets.
 *      
 *    virtual void print(void); 
 *      Prints debug messages on the serial monitor. Derived classes that have "print" methods
 *      for debugging may want to call this function first. Note it is "print" and not "println".
 */
class basePacket {
  public:
    baseRadio* Radio;
    packetType_t type;
    packetSubType_t subType;
    basePacket(void) {subType=NO_SUBTYPE;}
    basePacket(baseRadio* radio_ptr) {subType=NO_SUBTYPE; Radio=radio_ptr;};
    virtual size_t my_size() { return sizeof( *this ); }
    virtual bool send(void);
    virtual bool send(packetType_t t) {type=t; return send();};
    bool requireTypeTimeout(packetType_t t,uint16_t timeout);
    void requireType(packetType_t t);
    #if(TPG_DEBUG)
      virtual void print(void); //Prints debug messages on the serial monitor.
    #endif
};



/*************************************************************************************
 * Move class derived from Packet class
 ************************************************************************************/
/*
 * Base class for handling a move. You MUST create a derived move class because it contains
 * pure virtual functions that you will have to create. Your derived move object will
 * contain data and methods for a move in your particular game.
 * 
 * The following items are inherited from basePacket...
 *    baseRadio* Radio;
 *      The game engine initializes the Radio pointer automatically. 
 *      
 *    packetType_t type;
 *      Is always MOVE_PACKET.
 *      
 *    packetSubType_t subType;
 *      Default is NORMAL_MOVE but can also be PASS_MOVE or QUIT_MOVE.
 *      
 * The following items are specific to a move packet 
 *    uint16_t moveNum;
 *      The number of this move.
 *      
 *    baseMove(void) 
 *      Constructor.
 *      
 *    virtual size_t my_size() { return sizeof( *this ); }
 *      Returns the size of the actual object as instantiated. ALL CLASSES derived from this
 *      type MUST implement this method EXACTLY as shown above.
 *      
 *    virtual void decideMyMove(void)
 *      You MUST implement and override this pure virtual function. It will prompt the user
 *      for some input so he can specify his move using any method you want such as
 *      buttons, joysticks, or serial monitor input.
 *      
 *    void require(void)
 *      Wait forever for a MOVE_PACKET from the other device.
 *      
 *    virtual void print(void); 
 *      Prints debug messages on the serial monitor. Derived classes that have "print" methods
 *      for debugging may want to call this function first. This method calls basePacket::print();
 *      before printing its information. Note it is "print" and not "println". 
 */
class baseMove : public basePacket {
  public:
    uint16_t moveNum;
    baseMove(void) {type=MOVE_PACKET;subType=NORMAL_MOVE;};
    virtual size_t my_size() { return sizeof( *this ); };
    virtual void decideMyMove(void)=0;
    void require(void) {requireType(MOVE_PACKET);};
    #if(TPG_DEBUG)
      virtual void print(void);
    #endif
};


/*************************************************************************************
 * Results class derived from Packet class
 ************************************************************************************/
/*
 * After a move is received, we must generate results to send back to the other player. Most games
 * do not need to generate results from your opponent's move. For example a game with an open board 
 * that everyone could see such as "tic-tac-toe", "checkers" or "chess", you know the results of your move.
 * 
 * However consider "Battleship". Your move consists of firing a shot at an X,Y coordinate and your
 * opponent has to tell you if it was a HIT_RESULTS or MISS_RESULTS. Or consider the card game "go fish".
 * Your move might be "Have you got any threes?" And the results could either be MISS_RESULTS (i.e. go fish)
 * or HIT_RESULTS and send some data saying "I'm giving you the three of spades".
 * 
 * Even if it is an open board game where you know the results of your move, the game logic is set up
 * so that the receiving player determines if the move you sent was a WIN_RESULTS, LOSE_RESULTS, or
 * TIE_RESULTS. Even if you know your move was a winner, it's officially the other player's responsibility to 
 * declare that the game is over as a result of your move.
 * 
 * The following items are inherited from basePacket...
 *    baseRadio* Radio;
 *      The game engine initializes the Radio pointer automatically. 
 *      
 *    packetType_t type;
 *      Is always RESULTS_PACKET.
 *      
 *    packetSubType_t subType;
 *      Default is NORMAL_RESULTS but can be any of the following: HIT_RESULTS, MISS_RESULTS, 
 *      WIN_RESULTS, or LOSE_RESULTS.
 *      
 * The following items are specific to a results packet...
 * 
 *    uint16_t resultsNum; 
 *      The move number to which these results refer.
 *      
 *    baseResults(void) 
 *      Constructor.
 *      
 *    virtual size_t my_size() { return sizeof( *this ); }
 *      Returns the size of the actual object as instantiated. ALL CLASSES derived from this
 *      type MUST implement this method EXACTLY as shown above.
 *      
 *    virtual void require(void) 
 *      Waits forever for a RESULTS_PACKET.
 *      
 *    virtual bool generateResults(baseMove* Move)
 *      You MUST implement a derived generateResults method in your game implementation to produce 
 *      a packet of data to send back to your opponent telling them the results of their efforts. 
 *      You will implement the rules that determine what is a WIN_RESULTS, LOSE_RESULTS, or
 *      TIE_RESULTS in addition to any other results information you may need to provide.
 *      Returns true if the results ended the game.
 *      
 *    virtual bool processResults(void)
 *      You MUST implement a derived processResults method in your game implementation to react to the 
 *      results packet you will receive after you make a move.  Returns true if the results ended the game.
 *      
 *    virtual void print(void); 
 *      Prints debug messages on the serial monitor. Derived classes that have "print" methods
 *      for debugging may want to call this function first. This method calls basePacket::print();
 *      before printing its information. Note it is "print" and not "println". 
 */
class baseResults : public basePacket {
  public:
    uint16_t resultsNum;
    baseResults(void) {type=RESULTS_PACKET; subType=NORMAL_RESULTS;};
    virtual size_t my_size() { return sizeof( *this ); };
    virtual void require(void) {requireType(RESULTS_PACKET);};
    virtual bool generateResults(baseMove* Move)=0;
    virtual bool processResults(void)=0;
    #if(TPG_DEBUG)
      virtual void print(void);//Prints debug information about the results. Calls basePacket::print
    #endif  
};
#endif //not defined TwoPlayerGame_base_packet_h
