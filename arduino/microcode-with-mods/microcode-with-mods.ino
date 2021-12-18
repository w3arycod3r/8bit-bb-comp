/**
 * This sketch programs the microcode EEPROMs for the 8-bit breadboard computer
 * It includes support for a flags register with carry and zero flags
 * See this video for more: https://youtu.be/Zg1NdPKoosU
 */

#include <eeprom-prog.h>
#include "Arduino.h"

#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13

EEPROM_PROG eeprom(SHIFT_DATA, SHIFT_CLK, SHIFT_LATCH, EEPROM_D0, EEPROM_D7, WRITE_EN);

bool writeFlag = false; // Indicates a write was performed on the EEPROM

// 3 EEPROMs = 24 usable control bits, stored in uint32_t
#define HLT 0b100000000000000000000000  // Halt clock
#define M1I 0b010000000000000000000000  // Memory address register 1 in (RAM)
#define RI  0b001000000000000000000000  // RAM data in
#define RO  0b000100000000000000000000  // RAM data out
#define IO  0b000010000000000000000000  // Instruction register out
#define II  0b000001000000000000000000  // Instruction register in
#define AI  0b000000100000000000000000  // A register in
#define AO  0b000000010000000000000000  // A register out
#define EO  0b000000001000000000000000  // ALU out
#define SU  0b000000000100000000000000  // ALU subtract
#define BI  0b000000000010000000000000  // B register in
#define BO  0b000000000001000000000000  // B register out
#define OI  0b000000000000100000000000  // Output register in
#define CE  0b000000000000010000000000  // Program counter enable
#define CO  0b000000000000001000000000  // Program counter out
#define J   0b000000000000000100000000  // Jump (program counter in)
#define FI  0b000000000000000010000000  // Flags in
#define M2I 0b000000000000000001000000  // Memory address register 2 in (ROM)
#define RMO 0b000000000000000000100000  // ROM data out
#define LRS 0b000000000000000000010000  // LCD Register Select (Cmd/Data)
#define LCI 0b000000000000000000001000  // LCD in

#define FLAGS_Z0C0 0
#define FLAGS_Z0C1 1
#define FLAGS_Z1C0 2
#define FLAGS_Z1C1 3

#define JC  0b001001
#define JZ  0b001010


// Control Word matrix
// Row x Col = OpCode (Instruction) x Step
// 64 OpCodes with 8 steps possible for each.
const PROGMEM uint32_t UCODE[64][8] = {
  { CO|M2I,  RMO|II|CE,  0,              0,                         0,             0,            0, 0 },   // 0x00 - NOP
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|M2I|CE,                RMO|AI,        0,            0, 0 },   // 0x01 - LDA (Load A from ROM, Indirect Adr)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|AI|CE,                 0,             0,            0, 0 },   // 0x02 - LDI (Load A from ROM, Direct Data)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|M2I|CE,                RMO|BI,        EO|AI|FI,     0, 0 },   // 0x03 - ADD
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|BI|CE,                 EO|AI|FI,      0,            0, 0 },   // 0x04 - ADI
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|M2I|CE,                RMO|BI,        SU|EO|AI|FI,  0, 0 },   // 0x05 - SUB
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|BI|CE,                 SU|EO|AI|FI,   0,            0, 0 },   // 0x06 - SUI
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|M1I|CE,                AO|RI,         0,            0, 0 },   // 0x07 - STA (Store A into RAM)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|J,                     0,             0,            0, 0 },   // 0x08 - JMP
  { CO|M2I,  RMO|II|CE,  CE,             0,                         0,             0,            0, 0 },   // 0x09 - JC
  { CO|M2I,  RMO|II|CE,  CE,             0,                         0,             0,            0, 0 },   // 0x0a - JZ
  { CO|M2I,  RMO|II|CE,  AO|OI,          0,                         0,             0,            0, 0 },   // 0x0b - OUT
      
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|LCI|CE,                0,             0,            0, 0 },   // 0x0c - DC (Disp Cmd)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|LRS|LCI|CE,            0,             0,            0, 0 },   // 0x0d - DD (Disp Data)
  { CO|M2I,  RMO|II|CE,  AO|LCI,         0,                         0,             0,            0, 0 },   // 0x0e - DCA (Disp Cmd from A Reg)
  { CO|M2I,  RMO|II|CE,  AO|LRS|LCI,     0,                         0,             0,            0, 0 },   // 0x0f - DDA (Disp Data from A Reg)
      
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|LCI|OI|CE,             0,             0,            0, 0 },   // 0x10 - DCO (Disp Cmd + Out)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|LRS|LCI|OI|CE,         0,             0,            0, 0 },   // 0x11 - DDO (Disp Data + Out)
  { CO|M2I,  RMO|II|CE,  AO|LCI|OI,      0,                         0,             0,            0, 0 },   // 0x12 - CAO (Disp Cmd from A Reg + Out)
  { CO|M2I,  RMO|II|CE,  AO|LRS|LCI|OI,  0,                         0,             0,            0, 0 },   // 0x13 - DAO (Disp Data from A Reg + Out)
      
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|M1I|CE,                RO|AI,         0,            0, 0 },   // 0x14 - LDR (Load A from RAM)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|M1I|CE,                RO|BI,         EO|AI|FI,     0, 0 },   // 0x15 - ADR (Add to A from RAM)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|M1I|CE,                RO|M2I,        RMO|AI,       0, 0 },   // 0x16 - LKR (Load A from ROM, using ptr in RAM)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|M1I|CE,                RO|OI,         0,            0, 0 },   // 0x17 - OTR (Output from RAM)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|OI|CE,                 0,             0,            0, 0 },   // 0x18 - OTM (Output from ROM, Direct Data)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|BI|CE,                 0,             0,            0, 0 },   // 0x19 - LBI (Load B from ROM, Direct Data)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|AI|BI|OI|CE,           0,             0,            0, 0 },   // 0x1a - OUA (Load A, B, OUT from ROM, Direct Data)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|M1I|CE,                0,             0,            0, 0 },   // 0x1b - SRA (Set RAM Address)
  { CO|M2I,  RMO|II|CE,  CO|M2I,         RMO|AI|BI|OI|LRS|LCI|CE,   0,             0,            0, 0 },   // 0x1c - OAL (Load, A, B, OUT, and LCD Data from ROM, Direct Data)
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x1d - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x1e - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x1f - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x20 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x21 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x22 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x23 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x24 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x25 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x26 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x27 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x28 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x29 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x2a - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x2b - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x2c - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x2d - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x2e - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x2f - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x30 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x31 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x32 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x33 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x34 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x35 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x36 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x37 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x38 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x39 - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x3a - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x3b - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x3c - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x3d - NOP
  { CO|M2I,  RMO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0x3e - NOP
  { CO|M2I,  RMO|II|CE,  HLT,    HLT,    0,           0, 0, 0 },   // 0x3f - HLT
};


/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents(int start, int length) {
  for (int base = start; base < length; base += 16) {
    byte data[16];
    for (int offset = 0; offset <= 15; offset += 1) {
      data[offset] = eeprom.read(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}

// Print appropriate error messages
void handleErrors(byte writeStatus, int adr, byte data)
{
  if (writeStatus == 0)
  {
    return;
  }
  if (writeStatus == 1)
  {
    writeFlag = true;
    String mess = "Write data " + String(data) + " passed at " + String(adr);
    Serial.println(mess);
    return;
  }
  if (writeStatus == 2)
  {
    writeFlag = true;
    String mess = "Write data " + String(data) + " failed at " + String(adr);
    Serial.println(mess);
    return;
  }
}


void setup() {
  // put your setup code here, to run once:

  eeprom.init();
  
  Serial.begin(57600);
  while (!Serial) { };

  writeFlag = false;

  // Program data bytes
  Serial.print("Programming microcode EEPROM");


  // 13 Bits of address
  for (int address = 0; address < 8192; address++) {
    int flags       = (address & 0b1100000000000) >> 11; // ZF|CF
    int byte_sel    = (address & 0b0011000000000) >> 9;
    int instruction = (address & 0b0000111111000) >> 3;
    int step        = (address & 0b0000000000111);

    // Retrieve CW from lookup table in PROGMEM
    uint32_t lookupCW = pgm_read_dword_near(&UCODE[instruction][step]);


    
    // Handle 8 JC and JZ exceptions
    if ( (instruction == JC   &&
         step == 2            &&
         flags == FLAGS_Z0C1) ||

         (instruction == JZ   &&
         step == 2            &&
         flags == FLAGS_Z1C0) ||

         (instruction == JC   &&
         step == 2            &&
         flags == FLAGS_Z1C1) ||

         (instruction == JZ   &&
         step == 2            &&
         flags == FLAGS_Z1C1) )
    {
      lookupCW = CO|M2I;
    }

    if ( (instruction == JC   &&
         step == 3            &&
         flags == FLAGS_Z0C1) ||

         (instruction == JZ   &&
         step == 3            &&
         flags == FLAGS_Z1C0) ||

         (instruction == JC   &&
         step == 3            &&
         flags == FLAGS_Z1C1) ||

         (instruction == JZ   &&
         step == 3            &&
         flags == FLAGS_Z1C1) )
    {
      lookupCW = RMO|J;
    }
        

    byte writeStatus = 0;

    switch (byte_sel) {
      case 0:
        writeStatus = eeprom.update(address, lookupCW >> 16);
        handleErrors(writeStatus, address, lookupCW >> 16);
        break;
      case 1:
        writeStatus = eeprom.update(address, lookupCW >> 8);
        handleErrors(writeStatus, address, lookupCW >> 8);
        break;
      case 2:
        writeStatus = eeprom.update(address, lookupCW >> 0);
        handleErrors(writeStatus, address, lookupCW >> 0);
        break;
      case 3:
        writeStatus = eeprom.update(address, 0);
        handleErrors(writeStatus, address, 0);
        break;
    }

    if (address % 512 == 0) {
      Serial.print(".");
    }
  }

  Serial.println(" done");

  // Read and print out the contents of the EEPROM
  //Serial.println("Reading EEPROM");
  //printContents(0, 8192);

  if (writeFlag) {
    Serial.println("New Data written");
  }
  else {
    Serial.println("All Data verified");
  }
  
}


void loop() {
  // put your main code here, to run repeatedly:

}
