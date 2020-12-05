//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_EFM8BB1_Register_Enums.h>                // SFR declarations
//-----------------------------------------------------------------------------
// SiLabs_Startup() Routine
// ----------------------------------------------------------------------------
// This function is called immediately after reset, before the initialization
// code is run in SILABS_STARTUP.A51 (which runs before main() ). This is a
// useful place to disable the watchdog timer, which is enable by default
// and may trigger before main() in some instances.
//-----------------------------------------------------------------------------
void SiLabs_Startup (void)
{
  // Disable the watchdog here
  WDTCN = 0xDE;
  WDTCN = 0xAD;
  PCA0MD &= ~0x40;
}



//-----------------------------------------------------------------------------
// main() Routine
// ----------------------------------------------------------------------------
// Note: the software watchdog timer is not disabled by default in this
// example, so a long-running program will reset periodically unless
// the timer is disabled or your program periodically writes to it.
//
// Review the "Watchdog Timer" section under the part family's datasheet
// for details. To find the datasheet, select your part in the
// Simplicity Launcher and click on "Data Sheet".
//-----------------------------------------------------------------------------

#include <stdint.h>

void PORT_init(void);

#define F_CPU 24500000
#define CPU_DIV 1
#define SYSCLK_MS ((F_CPU / CPU_DIV) / 1000.0) / 60.0
#define SUBTRACT_MS 7500.0 / 60.0 / CPU_DIV
void delay(uint16_t millis) {
  uint16_t clocks = millis * SYSCLK_MS * millis;
  uint16_t subtract = (SUBTRACT_MS * millis) + 34;
  if(clocks > subtract) clocks -= subtract;
  while(clocks-- > 0) {
      _nop_();
  }
}

//Display variables

uint16_t THEME_FG = 0xFFFF;
uint16_t THEME_BG = 0x0000;

void setReset(char val) {
  if(val) {
      P1 |= 0x40;
  } else {
      P1 &= 0xBF;
  }
}
void setDC(char val) {
  if(val) {
      P1 |= 0x20;
  } else {
      P1 &= 0xDF;
  }
}
void writeSPI(uint8_t dat) {
  SPI0DAT = dat;
  while(!SPI0CN0_SPIF);
  dat = SPI0DAT;
  SPI0CN0_SPIF = 0;
}
void writeCommand(uint8_t cmd) {
  setDC(0);
  writeSPI(cmd);
}
void writeData(uint8_t dat) {
  setDC(1);
  writeSPI(dat);
}
void writeRegister(uint8_t reg, uint16_t dat) {
  writeCommand(reg);
  writeData((dat>>8));
  writeData(dat&0xFF);
}
void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  writeRegister(0x36,x1);
  writeRegister(0x37,x0);

  writeRegister(0x38,y1);
  writeRegister(0x39,y0);

  writeRegister(0x20,x0);
  writeRegister(0x21,y0);

  writeCommand(0x00);
  writeCommand(0x22);
}


void ili9225c_drawPixel(uint8_t x,uint8_t y,uint16_t color) {
  if(x>175) return;
  if(y>219) return;
  setWindow(x,y,x,y);
  writeData(color>>8);
  writeData(color&0xFF);
}

void swap(int* a,int* b) {
    int w=*a;
    *a=*b;
    *b=w;
}

#include <math.h>
void ili9225c_line(int x1, int y1, int x2, int y2, unsigned int c) {
  // Classic Bresenham algorithm
  signed int steep = abs((signed int)(y2 - y1)) > abs((signed int)(x2 - x1));
  signed int dx, dy;
  signed int err;
  signed int ystep;
  if (steep) {
    swap(&x1, &y1);
    swap(&x2, &y2);
  }
  if (x1 > x2) {
    swap(&x1, &x2);
    swap(&y1, &y2);
  }
  dx = x2 - x1;
  dy = abs((signed int)(y2 - y1));
  err = dx / 2;
  if (y1 < y2) ystep = 1;
  else ystep = -1;
  for (; x1 <= x2; x1++) {
    if (steep) ili9225c_drawPixel(y1, x1, c);
    else       ili9225c_drawPixel(x1, y1, c);
    err -= dy;
    if (err < 0) {
      y1 += ystep;
      err += dx;
    }
  }
}

void ili9225c_fillRectangle(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint16_t color) {
  uint16_t i;
  uint16_t max = ((uint16_t)(x2-x1))*((uint16_t)(y2-y1));
  setWindow(x1, y1, x2, y2);
  setDC(0);
  for(i=0;i<max;i++) {
      writeData(color>>8);
      writeData(color&0xFF);
  }
  setDC(1);
}

const uint8_t maxX=176;
const uint8_t maxY=220;

void ili9225c_fill(uint16_t color) {
  ili9225c_fillRectangle(0,0,maxX,maxY,color);
}

void ili9225c_clear() {
  ili9225c_fill(THEME_BG);
}

#include <string.h>

#include "symbols.h"

void ili9225c_writeChar(char c, uint8_t x, uint8_t y) {
  xdata char alphabet[] = " !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}";
  //char* alphaArray = {strtok(strdup(alphabet), "")};
  char* alphaArray = {strtok(alphabet, "")};

  const uint8_t width = 5;
  uint8_t indexX = 0;
  uint8_t indexY = 0;

  uint8_t i;
  uint8_t j;
  uint8_t bitt;

  for (i = 0; i < strlen(alphabet); i++) {
    if (c == alphaArray[i]) {
      uint8_t pixel_buffer[5];
      for (j=0;j<5;j++) {
          pixel_buffer[j]=symbols_8x5[i*5+j];
      }
      for (j = 0; j < width; j++) {
        for (bitt = 0x01; bitt; bitt <<= 1) {
          uint8_t xN = x - indexX;
          uint8_t yN = y + indexY;
          if (bitt & pixel_buffer[j]) {
            ili9225c_drawPixel(xN, yN, THEME_FG);
          } else {
            ili9225c_drawPixel(xN, yN, THEME_BG);
          }
          indexY++;
          if (indexY > 7) {
            indexX++;
            indexY = 0;
          }
        }
      }
    ili9225c_line(x-5,y,x-5,y+7,THEME_BG);
    }
  }
}
void ili9225c_writeString(const char* dat, uint8_t x, uint8_t y, uint8_t maxl, uint8_t* l, uint8_t* h) {

  //char* array = {strtok(strdup(dat), "")};
  char* array = {strtok(dat, "")};

  uint8_t indexX = 0;
  uint8_t indexY = 0;

  uint8_t i;
  for (i = 0; i < strlen(dat); i++) {
    ili9225c_writeChar(array[i], (maxX-x-3) - indexX, y + indexY);
    indexX += 6;
    *l+=6;
    if (indexX > maxl-3) {
      indexX = 0;
      *l=0;
      indexY += 8;
      *h += 8;
    }
  }
}

void ili9225c_init() {
  delay(50);
  setReset(1);
  delay(50);
  setReset(0);
  delay(50);
  setReset(1);
  delay(250);

  writeRegister(0x01, 0x031C); // set SS and NL bit
  writeRegister(0x02, 0x0100); // set 1 line inversion
  writeRegister(0x03, 0x1030); // set GRAM write direction and BGR=1.
  writeRegister(0x08, 0x0808); // set BP and FP
  writeRegister(0x0C, 0x0000); // RGB interface setting R0Ch=0x0110 for RGB 18Bit and R0Ch=0111for RGB16Bit
  writeRegister(0x0F, 0x0801); // Set frame rate
  writeRegister(0x20, 0x0000); // Set GRAM Address
  writeRegister(0x21, 0x0000); // Set GRAM Address

  delay(50); // delay 50ms
  writeRegister(0x10, 0x0A00); // Set SAP,DSTB,STB
  writeRegister(0x11, 0x1038); // Set APON,PON,AON,VCI1EN,VC
  delay(50); // delay 50ms
  writeRegister(0x12, 0x1121); // Internal reference voltage= Vci;
  writeRegister(0x13, 0x0066); // Set GVDD
  writeRegister(0x14, 0x5F60); // Set VCOMH/VCOML voltage
  //------------------------ Set GRAM area --------------------------------//
  writeRegister(0x30, 0x0000);
  writeRegister(0x31, 0x00DB);
  writeRegister(0x32, 0x0000);
  writeRegister(0x33, 0x0000);
  writeRegister(0x34, 0x00DB);
  writeRegister(0x35, 0x0000);
  writeRegister(0x36, 0x00AF);
  writeRegister(0x37, 0x0000);
  writeRegister(0x38, 0x00DB);
  writeRegister(0x39, 0x0000);
  // ----------- Adjust the Gamma Curve ----------//
  writeRegister(0x50, 0x0400);
  writeRegister(0x51, 0x060B);
  writeRegister(0x52, 0x0C0A);
  writeRegister(0x53, 0x0105);
  writeRegister(0x54, 0x0A0C);
  writeRegister(0x55, 0x0B06);
  writeRegister(0x56, 0x0004);
  writeRegister(0x57, 0x0501);
  writeRegister(0x58, 0x0E00);
  writeRegister(0x59, 0x000E);
  delay(50); // delay 50ms
  writeRegister(0x07, 0x1017);
}
/*
void ili9225c_reset() {
  setReset(1);
  delay(50);
  setReset(0);
  delay(50);
  setReset(1);
  delay(50);
  ili9225c_init();
  delay(10);
}
*/

int main (void)
{

  uint8_t length;
  uint8_t height;

  const char* testStr = "Hello, World!";

  PORT_init();

  ili9225c_init();
  ili9225c_clear();

  THEME_BG = 0x0000;
  THEME_FG = 0x2FE0;

  ili9225c_writeString(testStr, 0, 0, maxX, &length, &height);

  while(1);
}

void PORT_init(void) {

  XBR0 = 0x16; //XBAR config
  XBR1 = 0x24;
  XBR2 = 0x40;

  P1MDOUT |= 0x60; //Set GPIO 1.5 and 1.6 to outputs (push-pull)

  SPI0CN0 = 0xB;
  SPI0CFG = 0x47;
  SPI0CKR = 0x08; //Recommended clock divider for ILI9225 screen

  RSTSRC  = 0x04;                     // Enable missing clock detector
}
