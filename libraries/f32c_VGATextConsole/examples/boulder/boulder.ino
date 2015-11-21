/************************************************************************************************
 *
 *  File:           vga_game_main.c
 *
 *  Compilers:      gcc (f32c 4.8.x-MIPS or 4.9.x-RISC-V)
 *                  
 *  Programmer:     Timofeev Victor
 *                  osa@pic24.ru, testerplus@mail.ru
 *
 *  Description:    Main file
 *
 *  History:        15.05.2009 - File created
 *                  20.11.2015 - FPGArduino F32C adaptation (EMARD)
 *
 ************************************************************************************************/


//******************************************************************************
//  
//      Includes
// 
//******************************************************************************

extern "C"
{
#include "osa.h"

#include "const.h"
#include "game.h"
#include "music.h"
#include "sprites.h"
}

//******************************************************************************
//  
//      Configuration
// 
//******************************************************************************

#define BRAM 1

#if BRAM
volatile uint16_t *text_ram = (volatile uint16_t *) 0x40000000;
volatile uint8_t *font_ram = (volatile uint8_t *) 0x40001800;
#else
volatile uint16_t *text_ram = (volatile uint16_t *) 0x80010000;
volatile uint8_t *font_ram = (volatile uint8_t *) 0x40001800;
#endif
#define text_addr (*(volatile uint32_t *)0xfffffb90)
#define cntrl_reg (*(volatile uint8_t *)0xfffffb83)

#define LED 13

//******************************************************************************
//  
//      Constants
// 
//******************************************************************************


const char SINUS[] =            // Quarter of sin wave
{
/* sqrt(sin)*/
   0,   19,   28,   34,   39,   44,   48,   52,   56,   59,   62,   65,   68,   71,   73,   76, 
  78,   80,   83,   85,   87,   89,   91,   92,   94,   96,   97,   99,  101,  102,  104,  105, 
 106,  108,  109,  110,  111,  112,  113,  114,  115,  116,  117,  118,  119,  120,  120,  121, 
 122,  122,  123,  123,  124,  124,  125,  125,  125,  126,  126,  126,  126,  126,  126,  126

/* sin *
   0,    3,    6,    9,   12,   15,   18,   21,   24,   27,   30,   33,   36,   39,   42,   45, 
  48,   51,   54,   57,   59,   62,   65,   67,   70,   73,   75,   78,   80,   82,   85,   87, 
  89,   91,   94,   96,   98,  100,  101,  103,  105,  107,  108,  110,  111,  113,  114,  116, 
 117,  118,  119,  120,  121,  122,  123,  123,  124,  125,  125,  126,  126,  126,  126,  126/**/
};


//******************************************************************************
//  
//      Variables
// 
//******************************************************************************


near unsigned   char    i_cLines;
near unsigned   char    i_cPos;
near            char    i_cFrameCounter;
near            char    i_cStep;
near unsigned   int     i_wRandom;
     signed     char    sinus[256];
                char    m_cButtons;     // Buttons state (updated in interrupt)
                char    m_cCurMap;

//******************************************************************************
//  
//      Function prototypes
// 
//******************************************************************************

signed char isin (char x);
void Init (void);

//******************************************************************************
//  
//      Functions
// 
//******************************************************************************

void show_map()
{
  int x, y;
  uint16_t *cur_line = text_ram;
  volatile uint16_t *cur_char;
  uint8_t object;
  uint8_t character;
  uint16_t color;

  for(y = 0; y < MAP_SIZE_Y; y++)
  {
    cur_char = cur_line;
    for(x = 0; x < MAP_SIZE_X; x++)
    {
      object = m_Map[y][x];
      character = 0x80 | ((object << 1) & 0x3f);
      color = m_Colors[object & 0x1f] << 8;
      *(cur_char++) = color | character;
      *(cur_char++) = color | character | 1;
    }
    cur_line += 80;
  }
}

void joystick ()
{
  // button definitions in const.h
  static uint8_t button = 0;
  m_cButtons = 
     (digitalRead(pin_BUTTON_UP)    ? 0x01 : 0)
   | (digitalRead(pin_BUTTON_DOWN)  ? 0x02 : 0)
   | (digitalRead(pin_BUTTON_LEFT)  ? 0x04 : 0)
   | (digitalRead(pin_BUTTON_RIGHT) ? 0x08 : 0)
   | (digitalRead(pin_BUTTON_FIRE)  ? 0x10 : 0);
  digitalWrite(LED, button & 1);
}

void setup ()
{
    char n;
    Init();

    OS_Init();              //Operating system init

    m_cCurMap = 0;
    InitMap(m_cCurMap);

    #if 0
    sinus[0] = 0;
    for (n = 1; n; n++) sinus[n] = (isin(n)*6 + isin(n*2)*3 + isin(n*3)*1) / 10;
    #endif

    pinMode(LED, OUTPUT);   
}

void loop ()
{
  //digitalWrite(LED, LOW);
  joystick();
  Task_Game();
  Task_Animate();
  show_map();
  //Task_Music();
  delay(100);
  //digitalWrite(LED, HIGH);
  Task_Animate();
  show_map();
  //Task_Music();
  delay(100);
}


/********************************************************************************
 *
 *  Function:       Init
 *
 *------------------------------------------------------------------------------
 *
 *  description:    Init periphery
 *
 ********************************************************************************/

void Init (void)
{
  text_addr = text_ram; // video text base address
  cntrl_reg = 0xC0; // enable text mode, no bitmap
  volatile uint8_t *font_ptr = font_ram; 

  // copy sprites to character ram
  for(int i = 0; i < 32; i++)
  {
    for(int j = 0; j < 16; j++)
    {
      font_ptr[j+16]  = (SPRITES[j][i]) & 0xff; 
      font_ptr[j+0] = (SPRITES[j][i] >> 8) & 0xff;
    }
    font_ptr += 32;
  }
}



//******************************************************************************
//  
//  Get sin(x) x = 0..255 from table [0..63]
// 
//******************************************************************************

signed char isin (char x)
{
    char temp;
    temp = x;
    if (x >= 64) temp = 127-x;
    if (x >= 128) temp = x - 128;
    if (x >= 192) temp = 255 - x;
    temp = SINUS[temp];
    if (x & 0x80) temp = -temp;
    return temp;
}    


//******************************************************************************
// 
//      THE END (vga_game_main.c)
// 
//******************************************************************************

