/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 **********************************************************/
#include "TwoPlayerGame.h"

//String versions of the type and subType enums for debugging and other purposes
const char* packetTypeStr[7]={"No packet type","Offering Game Packet", "Accepting Game Packet", "Move Packet", 
                                "Results Packet", "Found Game Packet","Coin Flip Packet"};
const char* packetSubTypeStr[10]= {"No subtype", "Normal Move", "Pass Move", "Quit Move", "Normal Results", 
                                    "Hit Results", "Miss Results", "Win Results", "Lose Results", "Tie Results",};

/*
 * Base methods for basePacket, baseMove, and baseResults. See "TwoPlayerGame_base_packet.h" for details.
 */

/************************************************************************************
 * Base packet class stuff
 ************************************************************************************/

/*
 * This is the base send packet function that actually sends the packet data. It returns true if 
 * packet is acknowledged. 
 */
bool basePacket::send(void) {
  DEBUG("BP::send "); 
  DEBUG_PRINT;
  if(Radio->send((uint8_t*)this+PACKET_OFFSET, my_size()-PACKET_OFFSET)) {
    DEBUGLN(" (ack received)");
    return true;
  }
  DEBUGLN(" (ERROR:no ack)");
  return false;
}  

/*
 * Waits specified time for a packet of the specified type. Returns true if the packet was received and the 
 * type was correct. Returns false if time ran out or if it was the wrong packet type.
 */
bool basePacket::requireTypeTimeout(packetType_t t,uint16_t timeout) {
  uint8_t len = my_size()-PACKET_OFFSET;
  if(Radio->recvTimeout((uint8_t*)this+PACKET_OFFSET,&len,timeout)) {
    DEBUG("Got timed packet. "); 
    DEBUG_PRINT;
    //we got a packet but if it's the wrong type then return false
    if(type==t){
      DEBUGLN("Was required type.");
      return true;
    } else {
      DEBUGLN("Was wrong type.");
      return false;
    }
  }
  //if we timed out return false
  return false;
}

/*
 * Waits forever for a packet of the specified type. Returns only if the packet type was correct. 
 */
void basePacket::requireType(packetType_t t) {
  uint8_t len = my_size()-PACKET_OFFSET;
  while(true) {
    if(Radio->available()) {
      if(Radio->recv((uint8_t*)this+PACKET_OFFSET,&len)) {
        DEBUG("Got packet. "); 
        DEBUG_PRINT;
        //we got a packet but only exit if it's the right type
        if(type==t){
          DEBUGLN("Was required type.");
          return;
        } else {
          DEBUGLN("Was wrong type, ignoring.");
        }
      }
    }
  }
}

#if(TPG_DEBUG)
  /*
   * Prints debug messages showing contents of a message packet. This method also gets called by derived 
   * classes such as baseMove.print() and baseResults.print() to print the packet information nonspecific to them. 
   */
  void basePacket::print(void) {
    Serial.print("BP::print 'this'=0x"); Serial.print((uint32_t)this,HEX);
    #if(0)
      uint8_t Buffer[30];
      memcpy(Buffer,this,my_size());
      Serial.print(" Dump=(");
      for(uint8_t i=0;i<my_size();i++) {
        Serial.print(Buffer[i],HEX); Serial.print (" ");
      }
      Serial.println(")");
    #endif
    Serial.print("  Type='");Serial.print(packetTypeStr[type]); 
    Serial.print("' Size=");Serial.print(my_size()); Serial.print(" ");
    if(subType != NO_SUBTYPE) {
      Serial.print("subtype='"); Serial.print(packetSubTypeStr[subType]);
      Serial.print("' ");
    }
  };
#endif  

/*************************************************************************************
 * Move class derived from Packet class
 ************************************************************************************/
/*
 * The only baseMove method we need to define here is the debug print method
 */
#if(TPG_DEBUG)
  void baseMove::print(void) {
    basePacket::print();
    Serial.print("\nMove #");
    Serial.print(moveNum);
    Serial.print(" ");
  }
#endif

/*************************************************************************************
 * Results class derived from Packet class
 ************************************************************************************/
/*
 * The only baseResults method we need to define here is the debug print method
 */
#if(TPG_DEBUG)
  void baseResults::print(void) {
    basePacket::print();
    Serial.print("\nResults #");
    Serial.print(resultsNum);
    Serial.print(" ");
  }
#endif
