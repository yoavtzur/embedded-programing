#ifndef _halGPIO_H_
#define _halGPIO_H_

#include  "../header/bsp.h"         // private library - BSP layer
#include  "../header/app.h"         // private library - APP layer
#include "stdint.h"

#define MULTIPLY_FP_RESOLUTION_BITS 15 // For atan2-fixed point
#define PI 3.14159265;

extern enum FSMstate state;   // global variable
extern enum Stepperstate stateStepp;
extern enum SYSmode lpm_mode; // global variable
extern unsigned int delay_time;
extern int pushedIFG;
extern int sendIFG;
extern short MSBIFG;
extern short dataIFG;
extern short calibrateIFG;
extern const short state_changed[];
extern char stringFromPC[];
extern char file_content[];
extern int ExecuteFlag;
extern int FlashBurnIFG;
extern int SendFlag;
extern int j;
extern short Vr[];
extern int counter;
extern int rotateIFG;  ///Flag to rotate by clicking
extern char counter_str[];
extern unsigned int i;
extern unsigned int tx_index;
#define half_sec 500;
#define clk_tmp 131; // ( (2^20) / 8 )*(10^-3) to convert ms to counter value for TACCR0
extern int curr_counter; //The current number of steps
extern char step_str[4];

extern void Stepper_counter_clockwise();
extern void Stepper_clockwise();
extern void int2str(char *, unsigned int );
extern void print_deg_to_lcd(float phi);
extern void send_degree_to_PC();
extern void timer_call_counter();
extern void START_TIMERA0(unsigned int counter);
extern void  motorGoToPosition(uint32_t stepper_degrees, char script_state);

extern int16_t atan2_fp(int16_t y_fp, int16_t x_fp);// dont understen

extern void send_finish_to_PC();
extern uint32_t hex2int(char *hex);
extern void sysConfig(void);
extern void SetByteToPort(char);
extern void clrPortByte(char);
extern void delay(unsigned int);
extern void enterLPM(unsigned char);
extern void enable_interrupts();
extern void disable_interrupts();

extern __interrupt void Timer_A0(void);
extern __interrupt void Timer_A1(void);
extern __interrupt void Timer1_A0_ISR(void);
extern __interrupt void USCI0RX_ISR(void);
extern __interrupt void USCI0TX_ISR(void);
extern __interrupt void Joystick_handler(void);

#endif

// #define CHECKBUSY    1  // using this define, only if we want to read from LCD

#ifdef CHECKBUSY
    #define LCD_WAIT lcd_check_busy()
#else
    #define LCD_WAIT DelayMs(5)
#endif

/*----------------------------------------------------------
  CONFIG: change values according to your port pin selection
------------------------------------------------------------*/
#define LCD_EN(a)     (!a ? (P1OUT &= ~BIT0) : (P1OUT |= BIT0)) // P1.0 is LCD RS pin
#define LCD_EN_DIR(a) (!a ? (P1DIR &= ~BIT0) : (P1DIR |= BIT0)) // P1.0 pin direction

#define LCD_RS(a)     (!a ? (P1OUT &= ~BIT6) : (P1OUT |= BIT6)) // P1.6 is LCD Enable pin
#define LCD_RS_DIR(a) (!a ? (P1DIR &= ~BIT6) : (P1DIR |= BIT6)) // P1.6 pin direction

#define LCD_RW(a)     (!a ? (P1OUT &= ~BIT7) : (P1OUT |= BIT7)) // P1.7 is LCD RW pin
#define LCD_RW_DIR(a) (!a ? (P1DIR &= ~BIT7) : (P1DIR |= BIT7)) // P1.7 pin direction

#define LCD_DATA_OFFSET 0x04 // data pin selection offset for 4-bit mode

/*---------------------------------------------------------
  END CONFIG
-----------------------------------------------------------*/
#define FOURBIT_MODE    0x0
#define EIGHTBIT_MODE   0x1
#define LCD_MODE        FOURBIT_MODE

#define OUTPUT_PIN      1
#define INPUT_PIN       0
#define OUTPUT_DATA     (LCD_MODE ? 0xFF : (0x0F << LCD_DATA_OFFSET))
#define INPUT_DATA      0x00

#define LCD_STROBE_READ(value)  LCD_EN(1), \
                asm("Nope"), asm("Nope"), \
                value=LCD_DATA_READ, \
                LCD_EN(0)

#define lcd_cursor(x)       lcd_cmd(((x)&0x7F)|0x80)
#define lcd_clear()         lcd_cmd(0x01)
#define lcd_putchar(x)      lcd_data(x)
#define lcd_goto(x)         lcd_cmd(0x80+(x))
#define lcd_cursor_right()  lcd_cmd(0x14)
#define lcd_cursor_left()   lcd_cmd(0x10)
#define lcd_display_shift() lcd_cmd(0x1C)
#define lcd_home()          lcd_cmd(0x02)
#define cursor_off          lcd_cmd(0x0C)
#define cursor_on           lcd_cmd(0x0F)
#define lcd_function_set    lcd_cmd(0x3C) // 8bit,two lines,5x10 dots
#define lcd_new_line        lcd_cmd(0xC0)

extern void lcd_cmd(unsigned char);
extern void lcd_data(unsigned char);
extern void lcd_puts(const char * s);
extern void lcd_init();
extern void lcd_strobe();
extern void DelayMs(unsigned int);
extern void DelayUs(unsigned int);
/*
 *  Delay functions for HI-TECH C on the PIC18
 *
 *  Functions available:
 *      DelayUs(x)  Delay specified number of microseconds
 *      DelayMs(x)  Delay specified number of milliseconds
*/
