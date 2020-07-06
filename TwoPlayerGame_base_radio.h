/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 **********************************************************/
#ifndef _TwoPlayerGame_base_radio_h_
#define _TwoPlayerGame_base_radio_h_
#include <Arduino.h>
/*
 * Base class for object handling data transmission. You may use the provided derived class
 * RF69HCW provided or you may create a derived class that implements your own data transmission system.
 * If developing your own radio cries, you MUST implement each of these functions in your derived radio class.
 * 
 * You will create an instance of the Radio object and pass its address to the constructor of your game.
 * Note that the communication system need not be actual RF radio such as the RF69HWC devices that we
 * originally used in this system. You could (and we may later) implement other systems such as
 * Bluetooth or infrared transmit and receive. Making the Radio a separate class and object
 * makes it easier to drop in other communication methods without changing the base code of the game engine.
 * 
 * Each device has its own unique address. Player 1 will be a device #1 and Player 2 will be device #2
 * This device number is the ONLY difference between the software on one device versus the other.
 * EVERYTHING else is identical. See the "baseGame" constructor for details.
 *
 *    bool setup(uint8_t myPlayerNum, uint8_t otherPlayerNum)
 *      Gets called ONCE during your Game.setup call inside your main program setup(). Returns true
 *      if the setup was successful.
 *
 *    bool send(uint8_t* packet_ptr,uint8_t len)=0;
 *      Sends a packet of data starting at memory address "packet_ptr" with the number of bytes 
 *      specified by "len". Returns true if sent data was acknowledged as received.
 *        
 *    bool recvTimeout(uint8_t* packet_ptr,uint8_t* len_ptr,uint16_t timeout)
 *      Attempts to receive a packet and waits until the specified timeout. The packet_ptr
 *      is the start address of the data. The len_ptr points to an uint8_t specifying
 *      the size of our buffer. Upon return it passes back the actual number of bytes received.
 *      Returns true if data was received before the timeout.
 *      
 *    bool recv(uint8_t* packet_ptr,uint8_t* len_ptr)
 *      If data is available it receives it. Parameters are the same as the first two of 
 *      recvTimeout explained above. Returns true if successful.
 *      
 *    bool available(void)
 *      Returns true if data is available to be received.
 */

class baseRadio {
  public:
    uint8_t myPlayerNum;    //my radios address
    uint8_t otherPlayerNum; //opponent's radio address
    virtual bool setup(uint8_t myPlayerNum, uint8_t otherPlayerNum)=0;
    virtual bool send(uint8_t* packet_ptr,uint8_t len)=0;
    virtual bool recvTimeout(uint8_t* packet_ptr,uint8_t* len_ptr,uint16_t timeout)=0;
    virtual bool recv(uint8_t* packet_ptr,uint8_t* len_ptr)=0;
    virtual bool available(void)=0;
};
#endif  //not defined _TwoPlayerGame_base_radio_h_
