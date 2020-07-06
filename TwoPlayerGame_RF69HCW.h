/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 **********************************************************/
/*
 * This module defines a class "RF69Radio" for use with the Two Player Game system.
 * It is derived from the baseRadio class defined in "TwoPlayerGame_base_radio.h". See that
 * file for details.
 * 
 * It makes use of the RadioHead and RHReliableDatagram libraries available at 
 * https://github.com/adafruit/RadioHead
 * Those libraries automatically handle packet transmission and reception with "ack" acknowledgment 
 * signals in a way that is completely transparent to us. We just tell it the start address and 
 * length of data and it handles everything else. 
 * 
 * Any of the RF69HCW devices sold by Adafruit should work but we have only tested using the
 * Feather Wing RFM69HCW 900 MHz RadioFruit https://www.adafruit.com/product/3229
 * See https://learn.adafruit.com/radio-featherwing for details. It should be easy to
 * adapt this code for other types of radios but we do not have access to them.
 * 
 * You will create a single instance of this radio object and pass its address to the
 * game constructor. See "TwoPlayerGame_base_game.h" for details.
 */
#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
#include <TwoPlayerGame_base_radio.h>

// American model uses 915 MHz with a range of 850-950 MHz
// European model uses 433 MHz with a range of 400-460 MHz
#define RF69_FREQ 915.0

//Wiring for PyGamer M4 same as Feather M0
//NOTE the learning guide recommends differently but
//these are the values used in the sample code
#define RFM69_CS      10   // "B"
#define RFM69_RST     11   // "A"
#define RFM69_INT     6    // "D"

//The driver object
extern RH_RF69 rf69;

// Object to manage packet delivery and receipt, using the driver declared above
extern RHReliableDatagram rf69_manager;

/*
 * The class definition
 */
class RF69Radio : public baseRadio {
  public:
    bool setup(uint8_t myD,uint8_t otherD);
    bool send(uint8_t* packet_ptr,uint8_t len) {
      return rf69_manager.sendtoWait(packet_ptr,len, otherPlayerNum);
    };
    bool recvTimeout(uint8_t* packet_ptr,uint8_t* len_ptr,uint16_t timeout) {
      return rf69_manager.recvfromAckTimeout(packet_ptr,len_ptr,timeout);
    };
    bool recv(uint8_t* packet_ptr,uint8_t* len_ptr) {
      return rf69_manager.recvfromAck(packet_ptr,len_ptr);
    };
    bool available(void) {return rf69_manager.available();};
};

//This is the maximum legal size of the packet we can transmit. You should check it against
//your Move and Results objects to ensure that they are smaller than this number.
#define MAX_LEGAL_PACKET_SIZE RH_RF69_MAX_MESSAGE_LEN
