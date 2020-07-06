/*********************************************************
 *    Two Player Game Engine
 *      by Chris Young
 * Allows you to create a two player game using Adafruit PyGamer, PyBadge and other similar 
 * boards connected by a packet radio or other communication systems.
 * Open source under GPL 3.0. See LICENSE.TXT for details.
 **********************************************************/
/*
 * This is the source code for the RF69Radio class. See the header fileincluded below
 * for details.
 */
#include "TwoPlayerGame_RF69HCW.h"

//Create the driver object
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Object to manage packet delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69);

/*
 * There is only one non-inline method in this class. 
 * This is called one time by baseGame::setup from your main program setup() function.
 * It is passed the player number for you and your opponent. These player numbers are used as
 * device addresses. Returns true if radio was successfully initialized.
 */
bool RF69Radio::setup(uint8_t myD,uint8_t otherD) {
  myPlayerNum=myD;
  otherPlayerNum=otherD;
  rf69_manager.setThisAddress(myPlayerNum);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  digitalWrite(RFM69_RST, HIGH);//do a reset
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  if (!rf69_manager.init()) {
    //RFM69 radio init failed
    return false;
  }
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    //setFrequency failed
    return false;
  }
  // If you are using a high power RF69 eg RFM69HCW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW
  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
  return  true;
}
