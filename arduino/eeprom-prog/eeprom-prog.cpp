// Arduino Library for programming EEPROMs

#include "eeprom-prog.h"

EEPROM_PROG::EEPROM_PROG(uint8_t sd, uint8_t sc, uint8_t sl, uint8_t d0, uint8_t d7, uint8_t we)
{
    _sd = sd;
    _sc = sc;
    _sl = sl;
    _d0 = d0;
    _d7 = d7;
    _we = we;

    pinMode(_sd, OUTPUT);
    pinMode(_sc, OUTPUT);
    pinMode(_sl, OUTPUT);
    digitalWrite(_we, HIGH);
    pinMode(_we, OUTPUT);

}



/*
    Output the address bits and outputEnable signal using shift registers.
    Take in a 16-bit int, up to 15 bits of address
*/
void EEPROM_PROG::setAddress(int address, bool outputEnable) {
  // OE is the MSB of the top shift register
  shiftOut(_sd, _sc, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80));
  shiftOut(_sd, _sc, MSBFIRST, address);

  digitalWrite(_sl, LOW);
  digitalWrite(_sl, HIGH);
  digitalWrite(_sl, LOW);
}


/*
 * Read a byte from the EEPROM at the specified address.
 */
byte EEPROM_PROG::read(int address) {
  for (int pin = _d0; pin <= _d7; pin++) {
    pinMode(pin, INPUT);
  }
  setAddress(address, /*outputEnable*/ true);

  byte data = 0;
  for (int pin = _d7; pin >= _d0; pin--) {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}


/*
    Write a byte to the EEPROM at the specified address.
*/
void EEPROM_PROG::write(int address, byte data) {
  // Get MSB for !DATA polling later
  byte MSB = data & 0x80;

  setAddress(address, /*outputEnable*/ false);
  for (int pin = _d0; pin <= _d7; pin++) {
    pinMode(pin, OUTPUT);
  }

  // Write LSB first
  for (int pin = _d0; pin <= _d7; pin++) {
    digitalWrite(pin, data & 1); // Extract lowest bit
    data = data >> 1;
  }

  // Generate low write pulse on PB5

  noInterrupts();
  PORTB &= B11011111;
  // 12 NOPs ~ 750 nSec. One clock cycle is 62.5 nSec
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  asm("nop\n\t");
  
  PORTB |= B00100000;
  interrupts();



  // !DATA polling to determine end of write cycle
  byte pollBusy = read(address) & 0x80; // Top bit of read
  while (pollBusy != MSB)
  {
    delayMicroseconds(200); // Limit poll rate
    pollBusy = read(address) & 0x80;
  }
}


/*
    Write a byte to the EEPROM at the specified address, if the data is not already present.
    EEPROMs have a limited number of write cycles per memory location.
*/
bool EEPROM_PROG::update(int address, byte writeData) {
  byte readData = read(address);

  if (readData == writeData)
  {
    return false;
  }
  else
  {
    write(address, writeData);
    return true; // Write has been performed
  }
}

