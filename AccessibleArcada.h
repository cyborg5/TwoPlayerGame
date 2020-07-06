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
 * This file implements an alternate way to move the joystick and press buttons
 * using single character commands typed into the serial monitor. It allowed me to
 * test the system without needing to physically operate buttons which I cannot do
 * because of my disability. Whoever came up with the "variantReadButtons(void)"
 * concept did a great thing making this very easy to extend the basic functions
 * of the Arcada_Library. See ACCESSIBLE_INPUT the Battleship.h to turn off and on.
 */

class AccessibleArcada : public Adafruit_Arcada {
  public:
    uint32_t variantReadButtons(void); 
};

uint32_t AccessibleArcada::variantReadButtons(void) {
  switch(toupper(Serial.read())) {
    case 'A': return ARCADA_BUTTONMASK_A;
    case 'B': return ARCADA_BUTTONMASK_B;
    case 'E': return ARCADA_BUTTONMASK_SELECT;//"E" for enter
    case 'S': return ARCADA_BUTTONMASK_START;
    case 'U': return ARCADA_BUTTONMASK_UP;
    case 'D': return ARCADA_BUTTONMASK_DOWN;
    case 'L': return ARCADA_BUTTONMASK_LEFT;
    case 'R': return ARCADA_BUTTONMASK_RIGHT;
  }
  return 0;
}
