/**
    This sketch is specifically for programming the EEPROM used in the 8-bit
    decimal display decoder described in https://youtu.be/dLh1n2dErzE
*/
#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13

bool writeFlag = false; // Indicates a write was performed on the EEPROM



/*
    Output the address bits and outputEnable signal using shift registers.
    Take in a 16-bit int, up to 15 bits of address
*/
void setAddress(int address, bool outputEnable) {
  // OE is the MSB of the top shift register
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80));
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}


/*
    Read a byte from the EEPROM at the specified address.
*/
byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin++) {
    pinMode(pin, INPUT);
  }
  setAddress(address, /*outputEnable*/ true);

  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin--) {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}

/*
    Write a byte to the EEPROM at the specified address.
*/
void writeEEPROM(int address, byte data) {
  // Get MSB for !DATA polling later
  byte MSB = data & 0x80;

  setAddress(address, /*outputEnable*/ false);
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin++) {
    pinMode(pin, OUTPUT);
  }

  // Write LSB first
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin++) {
    digitalWrite(pin, data & 1); // Extract lowest bit
    data = data >> 1;
  }

  // Generate low write pulse on PB5

  noInterrupts();
  PORTB &= B11011111;
  // 8 NOPs ~ 500 nSec. One clock cycle is 62.5 nSec
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
  byte pollBusy = readEEPROM(address) & 0x80; // Top bit of read
  while (pollBusy != MSB)
  {
    delayMicroseconds(200); // Limit poll rate
    pollBusy = readEEPROM(address) & 0x80;
  }

  // A write has been performed
  writeFlag = true;
}


/*
    Write a byte to the EEPROM at the specified address, if the data is not already present.
    EEPROMs have a limited number of write cycles per memory location.
*/
void updateEEPROM(int address, byte writeData) {
  byte readData = readEEPROM(address);

  if (readData == writeData)
  {
    return;
  }
  else
  {
    writeEEPROM(address, writeData);
  }
}

/*
    Read the contents of the EEPROM and print them to the serial monitor.
*/
void printContents() {
  for (int base = 0; base < 8192; base += 16) {
    byte data[16];
    for (int offset = 0; offset < 16; offset++) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}

// Bit patterns for the digits 0..9 - Common Cathode 7-Seg Displays
//byte digits[] = { 0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b };

// Hex Digits 0..F - Common Anode
byte digits[] = { 0x81, 0xcf, 0x92, 0x86, 0xcc, 0xa4, 0xa0, 0x8f, 0x80, 0x84, 0x88, 0xe0, 0xb1, 0xc2, 0xb0, 0xb8 };
// 2 Bit Binary Display 0..3 - Common Anode
byte binDigits[] = { 0xff, 0xef, 0xfb, 0xeb };
// Misc characters (h, q, blank, neg sign) - Common Anode
byte chars[] = { 0xe8, 0x8c, 0xff, 0xfe };

// DP-A-B-C-D-E-F-G
// 0x7e = B01111110
// 0x30 = B00110000

void setup() {
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);
  Serial.begin(57600);

  writeFlag = false;




  Serial.println("Programming unsigned decimal mode");
  // Ones Place
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value, digits[value % 10]);
  }
  // Tens Place
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 256, digits[(value / 10) % 10]); // Shift right by one place and take last digit
  }
  // Hundreds Place
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 512, digits[(value / 100) % 10]); // Shift right by two places and take last digit
  }
  // Sign
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 768, chars[2]); // Blank char
  }


  Serial.println("Programming signed decimal mode");
  // Ones Place
  for (int value = -128; value <= 127; value += 1) {
    updateEEPROM((byte)value + 1024, digits[abs(value) % 10]);
  }
  // Tens Place
  for (int value = -128; value <= 127; value += 1) {
    updateEEPROM((byte)value + 1280, digits[abs(value / 10) % 10]);
  }
  // Hundreds Place
  for (int value = -128; value <= 127; value += 1) {
    updateEEPROM((byte)value + 1536, digits[abs(value / 100) % 10]);
  }
  // Sign
  for (int value = -128; value <= 127; value += 1) {
    if (value < 0) {
      updateEEPROM((byte)value + 1792, chars[3]); // Neg Sign
    } else {
      updateEEPROM((byte)value + 1792, chars[2]); // Blank char
    }
  }

  
  Serial.println("Programming hexadecimal mode");
  // Digit 0 : h
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 2048, chars[0]);
  }
  // Digit 1
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 2304, digits[(value >> 0) & 0x0f]);
  }
  // Digit 2
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 2560, digits[(value >> 4) & 0x0f]);
  }
  // Digit 3 : Zero
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 2816, digits[0]);
  }

  // + 2304
  // + 2560
  // + 2816
  // + 3072
  // + 3328
  // + 3584
  // + 3840
  // + 4096
  // + 4352
  // + 4608
  // + 4864

  // 7168
  
  Serial.println("Programming binary mode");
    // Digit 0
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 3072, binDigits[(value >> 0) & 0x03]); // Extract Lower 2 Bits, index into binary digits.
  }
  // Digit 1
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 3328, binDigits[(value >> 2) & 0x03]); // Increment address by 256
  }
  // Digit 2
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 3584, binDigits[(value >> 4) & 0x03]);
  }
  // Digit 3
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 3840, binDigits[(value >> 6) & 0x03]);
  }

  Serial.println("Programming octal mode");
  // Digit 0
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 4096, digits[(value >> 0) & 0x07]); // Extract Lower 3 Bits, index into digits. Will give 0 -> 7
  }
  // Digit 1
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 4352, digits[(value >> 3) & 0x07]);
  }
  // Digit 2
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 4608, digits[(value >> 6) & 0x07]);
  }
  // Digit 3 : q
  for (int value = 0; value < 256; value += 1) {
    updateEEPROM(value + 4864, chars[1]); // q char
  }  
  

  Serial.println("Programming off mode");
  // Display nothing for the rest of the address space.
  for (int address = 5120; address < 8192; address += 1) {
    updateEEPROM(address, chars[2]); // Blank char
  }


  if (writeFlag) {
    Serial.println("Data written");
  }
  else {
    Serial.println("No Data written");
  }
  // Read and print out the contents of the EEPROM
  Serial.println("Reading EEPROM");
  printContents();
}


void loop() {
  // put your main code here, to run repeatedly:

}
