// Arduino Library for programming EEPROMs

#ifndef EEPROM_PROG_H
#define EEPROM_PROG_H

#include "Arduino.h"


class EEPROM_PROG
{
    public:
        EEPROM_PROG(uint8_t sd, uint8_t sc, uint8_t sl, uint8_t d0, uint8_t d7, uint8_t we);

        byte read(int address);
        bool update(int address, byte writeData);

    private:
        uint8_t _sd; // Shift Data
        uint8_t _sc; // Shift Clock
        uint8_t _sl; // Shift Latch
        uint8_t _d0; // EEPROM D0
        uint8_t _d7; // EEPROM D7
        uint8_t _we; // Write Enable

        void setAddress(int address, bool outputEnable);
        void write(int address, byte data);


};

#endif