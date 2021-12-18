/**
 * This sketch writes to the EEPROM used for program storage.
 */

#include <eeprom-prog.h>

#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13

EEPROM_PROG eeprom(SHIFT_DATA, SHIFT_CLK, SHIFT_LATCH, EEPROM_D0, EEPROM_D7, WRITE_EN);

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

bool writeFlag = false; // Indicates a write was performed on the EEPROM

// OpCodes
#define NOP 0x00
#define LDA 0x01  // Load A from ROM (By Adr)
#define LDI 0x02  // Load A from ROM (Dir Data)
#define ADD 0x03  // Add to A from ROM (By Adr)
#define ADI 0x04  // Add to A from ROM (Dir Data)
#define SUB 0x05  // Sub from A from ROM (By Adr)
#define SUI 0x06  // Sub from A from ROM (Dir Data)
#define STA 0x07  // Store A into RAM adr
#define JMP 0x08  // Uncond Jump
#define JC  0x09  // Jump if carry set
#define JZ  0x0a  // Jump if zero set
#define OUT 0x0b  // Output from A Reg

#define DC  0x0c  // Disp Cmd
#define DD  0x0d  // Disp Data
#define DCA 0x0e  // Disp Cmd from A
#define DDA 0x0f  // Disp Data from A

#define DCO 0x10  // Disp Cmd + Out
#define DDO 0x11  // Disp Data + Out
#define CAO 0x12  // Disp Cmd from A Reg + Out
#define DAO 0x13  // Disp Data from A Reg + Out

#define LDR 0x14  // Load A from RAM
#define ADR 0x15  // Add to A from RAM
#define LKR 0x16  // Load A from ROM, using ptr in RAM
#define OTR 0x17  // Output from RAM
#define OTM 0x18  // Output from ROM, Direct Data
#define LBI 0x19  // Load B from ROM, Direct Data
#define OUA 0x1a  // Load A, B, OUT from ROM, Direct Data
#define SRA 0x1b  // Set RAM Address
#define OAL 0x1c  // Load, A, B, OUT, and LCD Data from ROM, Direct Data

#define HLT 0x3f

// ===== LCD [I]nstructions and [P]arameters =====
#define I_CLR           0b00000001
#define I_RET_HOME      0b00000010

#define I_ENT_MODE_SET  0b00000100
#define P_INCR          0b00000010
#define P_DECR          0b00000000
#define P_SHIFT_MD      0b00000001
#define P_NO_SHIFT      0b00000000

#define I_DISP_CTL      0b00001000
#define P_DISP_ON       0b00000100
#define P_DISP_OFF      0b00000000
#define P_CURS_ON       0b00000010
#define P_CURS_OFF      0b00000000
#define P_BLINK_ON      0b00000001
#define P_BLINK_OFF     0b00000000

#define I_SHIFT_OP      0b00010000  // Shift display or move cursor
#define P_SHIFT_DISP    0b00001000
#define P_MOVE_CURS     0b00000000
#define P_RIGHT         0b00000100
#define P_LEFT          0b00000000

#define I_FUNC_SET      0b00100000
#define P_8BIT_MODE     0b00010000
#define P_4BIT_MODE     0b00000000
#define P_2LINE         0b00001000
#define P_1LINE         0b00000000
#define P_5X10_FONT     0b00000100
#define P_5X8_FONT      0b00000000

#define I_CGRAM_ADR     0b01000000  // 6 Address bits

#define I_DDRAM_ADR     0b10000000  // 7 Address bits

// Read instructions omitted

// [S]tandard LCD Configs
#define S_FSET   I_FUNC_SET|P_8BIT_MODE|P_2LINE|P_5X8_FONT

#define S_DCTL   I_DISP_CTL|P_DISP_ON|P_CURS_ON

#define S_ESET   I_ENT_MODE_SET|P_INCR



// ===== 32 Programs of 256 bytes each =====


// ===== Hello World Looper =====

#define var_p 0xf // ROM Pointer stored in RAM

// Labels
#define lbl_init 0
#define lbl_loop 12
#define lbl_hlt 27
#define lbl_strStart 28

const PROGMEM byte PROG0[] = 
{  
  // lbl_init:
  LDI, lbl_strStart,
  STA, var_p,

  DC, S_FSET,
  DC, S_DCTL,
  DC, S_ESET,

  DC, I_CLR,

  // lbl_loop:
  LKR, var_p,
  ADI, 0,
  JZ, lbl_init,

  DAO,

  // Increment ptr
  LDR, var_p,
  ADI, 1,
  STA, var_p,
  JMP, lbl_loop,

  // lbl_hlt:
  HLT,

  // lbl_str_start:
  'H','e','l','l','o',',',' ','w','o','r','l','d','!','\0'


};


// ===== Character Set Scroller =====

// Variables in RAM
#define var_rowCount  0b1000
#define var_charCount 0b0001
// Labels
#define lbl_print 0
#define lbl_init 47
#define lbl_loop 61
#define lbl_firstRow 88
#define lbl_secondRow 96

const PROGMEM byte PROG1[] = 
{
  // lbl_print:
  DC, S_FSET,
  DC, S_DCTL,
  DC, S_ESET,
  DC, I_CLR,

  //====CHAR=SET====
  //====SCROLLER====
  
  DC, I_DDRAM_ADR|0x04,
  DD, 'C',
  DD, 'h',
  DD, 'a',
  DD, 'r',
  DD, ' ',
  DD, 'S',
  DD, 'e',
  DD, 't',
  DC, I_DDRAM_ADR|0x44,
  DD, 'S',
  DD, 'c',
  DD, 'r',
  DD, 'o',
  DD, 'l',
  DD, 'l',
  DD, 'e',
  DD, 'r',

  // Hold Text on screen
  NOP,
  NOP,
  NOP,

  // lbl_init:
  DC, S_FSET,
  DC, S_DCTL,
  DC, S_ESET,
  DC, I_CLR,
  LDI, 0,
  STA, var_charCount,
  STA, var_rowCount,
 
  // lbl_loop:
  LDR, var_charCount,
  DAO,
  ADI, 1,
  JC, lbl_init, // Reset after 255
  STA, var_charCount, // Inc charCount

  LDR, var_rowCount,
  ADI, 1,
  STA, var_rowCount, // Inc rowCount

  SUI, 16,
  JZ, lbl_secondRow, // Check rowCount == 16

  LDR, var_rowCount,
  SUI, 32,
  JZ, lbl_firstRow, // Check rowCount == 32

  JMP, lbl_loop,

  // lbl_firstRow:
  DC, I_DDRAM_ADR|0x00,
  LDI, 0,
  STA, var_rowCount, // Reset rowCount
  JMP, lbl_loop,

  // lbl_secondRow:
  DC, I_DDRAM_ADR|0x40,
  JMP, lbl_loop
  


};

// ===== Counter =====

// Variables in RAM
#define var_c 0b1000 // Count increment
#define var_s 0b0001 // Sum

// Labels
#define lbl_print 0
#define lbl_init 24
#define lbl_loop 32
#define lbl_inc 43

const PROGMEM byte PROG2[] = 
{
  // lbl_print:
  DC, S_FSET,
  DC, S_DCTL,
  DC, S_ESET,
  DC, I_CLR,

  //====COUNTER=====
  //================
  
  DC, I_DDRAM_ADR|0x04,
  DD, 'C',
  DD, 'o',
  DD, 'u',
  DD, 'n',
  DD, 't',
  DD, 'e',
  DD, 'r',

  // lbl_init:
  LDI, 1,
  STA, var_c, // c = 1;
  LDI, 0,
  STA, var_s, // s = 0;

  // lbl_loop:
  LDR, var_s,
  OUT,
  ADR, var_c, 
  JC, lbl_inc, // s + c > 255?
  STA, var_s, // s += c;
  JMP, lbl_loop,


  // lbl_inc:
  LDR, var_c,
  ADI, 1,
  STA, var_c, // c++
  SUI, 129,
  JZ, lbl_init,  // c == 129?

  LDI, 0,
  STA, var_s, // s = 0;
  JMP, lbl_loop,

};

// ===== Fibonacci Counter =====

// Variables in RAM
#define var_x 0b1000
#define var_y 0b0110
#define var_z 0b0001
// Labels
#define lbl_print 0
#define lbl_init 44
#define lbl_loop 56

const PROGMEM byte PROG3[] = 
{
  // lbl_print:
  DC, S_FSET,
  DC, S_DCTL,
  DC, S_ESET,
  DC, I_CLR,

  //===FIBONACCI====
  //====COUNTER=====
  
  DC, I_DDRAM_ADR|0x03,
  DD, 'F',
  DD, 'i',
  DD, 'b',
  DD, 'o',
  DD, 'n',
  DD, 'a',
  DD, 'c',
  DD, 'c',
  DD, 'i',
  DC, I_DDRAM_ADR|0x44,
  DD, 'C',
  DD, 'o',
  DD, 'u',
  DD, 'n',
  DD, 't',
  DD, 'e',
  DD, 'r',

  // lbl_init:
  LDI, 0,
  OUT,
  STA, var_x,
  STA, var_z,
  LDI, 1,
  OUT,
  STA, var_y,    // x = 0, y = 1, z = 0

  // lbl_loop:
  LDR, var_x,
  ADR, var_y,
  JC, lbl_init,
  OUT,
  STA, var_z,    // z = x + y

  LDR, var_y,
  STA, var_x,
  LDR, var_z,
  STA, var_y,    // x = y, y = z

  JMP, lbl_loop,
};

/*
  // 0: Arms Down
  DD, 0b00100,
  DD, 0b01010,
  DD, 0b00100,
  DD, 0b00100,
  DD, 0b01110,
  DD, 0b10101,
  DD, 0b00100,
  DD, 0b01010,
  
  // 1: Arms Up
  DD, 0b00100,
  DD, 0b01010,
  DD, 0b00100,
  DD, 0b10101,
  DD, 0b01110,
  DD, 0b00100,
  DD, 0b00100,
  DD, 0b01010,

  // 2: Heart
  DD, 0b00000,
  DD, 0b01010,
  DD, 0b11111,
  DD, 0b11111,
  DD, 0b11111,
  DD, 0b01110,
  DD, 0b00100,
  DD, 0b00000,
*/

// Random char placements
/*
  DD, 7,
  DD, 4,
  DD, 3,
  DD, 6,
  DD, 2,
  DD, 1,
  DD, 2,
  DD, 3,
  DD, 1,
  DD, 7,
  DD, 3,
  DD, 0,
  DD, 2,
  DD, 6,
  DD, 5,
  DD, 4,
  DC, I_DDRAM_ADR|40,
  DD, 1,
  DD, 2,
  DD, 4,
  DD, 3,
  DD, 0,
  DD, 6,
  DD, 7,
  DD, 4,
  DD, 0,
  DD, 3,
  DD, 4,
  DD, 0,
  DD, 2,
  DD, 6,
  DD, 4,
  DD, 7,
*/

// ===== Animator 1 =====

// Variables in RAM
// Labels
#define lbl_init 0
#define lbl_loop 110

const PROGMEM byte PROG4[] = 
{
  // lbl_init:
  
  // Init RAM
  LDI, 0,

  STA, 0,
  STA, 1,
  STA, 2,
  STA, 3,
  STA, 4,
  STA, 5,
  STA, 6,
  STA, 7,
  STA, 8,
  STA, 9,
  STA, 10,
  STA, 11,
  STA, 12,
  STA, 13,
  STA, 14,
  STA, 15,

  DC, S_FSET,
  DC, I_DISP_CTL|P_DISP_ON|P_CURS_OFF,
  DC, S_ESET,
  DC, I_CLR,

  // "Pillar" arrangement of custom chars
  DD, 0,
  DD, 2,
  DD, 4,
  DD, 6,
  DD, 0,
  DD, 2,
  DD, 4,
  DD, 6,
  DD, 0,
  DD, 2,
  DD, 4,
  DD, 6,
  DD, 0,
  DD, 2,
  DD, 4,
  DD, 6,
  DC, I_DDRAM_ADR|0x40,
  DD, 1,
  DD, 3,
  DD, 5,
  DD, 7,
  DD, 1,
  DD, 3,
  DD, 5,
  DD, 7,
  DD, 1,
  DD, 3,
  DD, 5,
  DD, 7,
  DD, 1,
  DD, 3,
  DD, 5,
  DD, 7,


  DC, I_CGRAM_ADR|0, // Put the following patterns in CGRAM


  // Animation Sequence
  // lbl_loop:

  OAL, 0b10000000,
  OAL, 0b01000000,
    SRA, 0b1000,
  OAL, 0b00100000,
  OAL, 0b00010000,
    SRA, 0b0100,
  OAL, 0b00001000,
  OAL, 0b00000100,
    SRA, 0b0010,
  OAL, 0b00000010,
  OAL, 0b00000001,
    SRA, 0b0001,
  OAL, 0b00000010,
    SRA, 0b0010,
  OAL, 0b00000100,
    SRA, 0b0100,
  OAL, 0b00001000,
  OAL, 0b00010000,
    SRA, 0b1000,
  OAL, 0b00100000,
  OAL, 0b01000000,
    SRA, 0b1100,
  OAL, 0b10000000,
  OAL, 0b11000000,
    SRA, 0b1110,
  OAL, 0b11100000,
  OAL, 0b11110000,
    SRA, 0b1111,
  OAL, 0b11111000,
    SRA, 0b0111,
  OAL, 0b11111100,
    SRA, 0b0011,
  OAL, 0b11111110,
  OAL, 0b11111111,
    SRA, 0b0001,
  OAL, 0b01111111,
  OAL, 0b00111111,
    SRA, 0b0010,
  OAL, 0b00011111,
  OAL, 0b00001111,
    SRA, 0b0100,
  OAL, 0b00000111,
  OAL, 0b00000011,
    SRA, 0b1000,
  OAL, 0b00000001,
    SRA, 0b0100,
  OAL, 0b00000011,
    SRA, 0b0010,
  OAL, 0b00000111,
  OAL, 0b00001111,
    SRA, 0b0001,
  OAL, 0b00011111,
  OAL, 0b00111111,
    SRA, 0b0011,
  OAL, 0b01111111,
  OAL, 0b11111111,
    SRA, 0b0111,
  OAL, 0b11111110,
  OAL, 0b11111100,
    SRA, 0b1111,
  OAL, 0b11111000,
    SRA, 0b1110,
  OAL, 0b11110000,
    SRA, 0b1100,
  OAL, 0b11100000,
  OAL, 0b11000000,
    SRA, 0b1000,
  OAL, 0b10000000,



  JMP, lbl_loop,
  //HLT,

};

// ===== TBD: Animator 2 =====

// Variables in RAM
// Labels
#define lbl_init 0
#define lbl_loop 0x494

const PROGMEM byte PROG5[] =
{

};


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

void writeProgram(byte* byteArray, size_t numElems, int startAdr)
{
  for (int i = 0; i < numElems; i++)
  {
    int adr = startAdr + i;
    byte lookup = pgm_read_byte_near(&byteArray[i]);
    
    byte writeStatus = eeprom.update(adr, lookup);
    handleErrors(writeStatus, adr, lookup);

    if (adr % 512 == 0)
      Serial.print(".");
  }

  writeStaticValue(startAdr + numElems, startAdr + 256, 0);
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


// Start inclusive, end exclusive
void writeStaticValue(int startAdr, int endAdr, byte value)
{
  for (int adr = startAdr; adr < endAdr; adr++)
  {
    byte writeStatus = eeprom.update(adr, value);
    handleErrors(writeStatus, adr, value);

    if (adr % 512 == 0)
      Serial.print(".");
  }
}


void setup() {
  // put your setup code here, to run once:

  eeprom.init();
  
  Serial.begin(57600);
  while (!Serial) { };

  writeFlag = false;

  // Program data bytes
  Serial.print("Programming code EEPROM");

  
  writeProgram(PROG0, NELEMS(PROG0), 0);
  writeProgram(PROG1, NELEMS(PROG1), 256);
  writeProgram(PROG2, NELEMS(PROG2), 512);
  writeProgram(PROG3, NELEMS(PROG3), 768);
  writeProgram(PROG4, NELEMS(PROG4), 1024);
  writeStaticValue(1280, 8192, 0);


  //writeStaticValue(0, 8192, 55);
  


  Serial.println(" done");
  // Read and print out the contents of the EEPROM
  Serial.println("Reading EEPROM");
  printContents(0, 8192);

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
