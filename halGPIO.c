#include  "../header/halGPIO.h"     // private library - HAL layer
#include  "../header/flash.h"     // private library - FLASH layer
#include "stdio.h"
#include "stdint.h"
#include "string.h"

#define PI_FIXED 3.14159265358979323846

// Global Variables
int j=0;
char *ptr1, *ptr2, *ptr3;
short MSBIFG = 1;
short dataIFG = 1; // 0-state changed -> send state(pb pressed)
short calibrateIFG = 1;  //0-calibrate changed -> send stop(pb pressed)
int rotateIFG = 1; /////Flag to rotate by clicking
unsigned int delay_time = 500;
const unsigned int timer_half_sec = 65535;
unsigned int i = 0;
unsigned int tx_index;
char counter_str[4];
short Vr[] = {0, 0}; //Vr[0]=Vry , Vr[1]=Vrx//The volt of the joystick
const short state_changed[] = {1000, 1000}; // send if button pressed - state changed
char stringFromPC[80];
char file_content[80];
int ExecuteFlag = 0;
int FlashBurnIFG = 0;
int SendFlag = 0;
int counter = 513; //After testing it the amount for a complete cycle
char step_str[4];
char finish_str[3] = "FIN";
int curr_counter = 0;
short finishIFG = 0;
//--------------------------------------------------------------------
//             System Configuration  
//--------------------------------------------------------------------
void sysConfig(void){ 
	GPIOconfig();
	ADCconfig();
	StopAllTimers();
	lcd_init();
	lcd_clear();
	UART_init();
}
//--------------------------------------------------------------------
//              Send FINISH to PC // Finish current task
//--------------------------------------------------------------------
void send_finish_to_PC(){
    finishIFG = 1;
    tx_index = 0;
    UCA0TXBUF = finish_str[tx_index++];
    IE2 |= UCA0TXIE;                        // Enable USCI_A0 TX interrupt
    __bis_SR_register(LPM0_bits + GIE); // Sleep
    START_TIMERA0(10000);
    finishIFG = 0;
}

//--------------------------------------------------------------------
//              Send degree to PC
//--------------------------------------------------------------------
void send_degree_to_PC(){
    tx_index = 0;
    UCA0TXBUF = step_str[tx_index++];
    IE2 |= UCA0TXIE;                        // Enable USCI_A0 TX interrupt
    __bis_SR_register(LPM0_bits + GIE); // Sleep
    START_TIMERA0(10000);
}

//---------------------------------------------------------------------
//            General Function
//---------------------------------------------------------------------
void int2str(char *str, unsigned int num){
    int strSize = 0;
    long tmp = num, len = 0;
    int j;
    if (tmp == 0){
        str[strSize] = '0';
        return;
    }
    // Find the size of the intPart by repeatedly dividing by 10
    while(tmp){
        len++;
        tmp /= 10;
    }

    // Print out the numbers in reverse
    for(j = len - 1; j >= 0; j--){
        str[j] = (num % 10) + '0';
        num /= 10;
    }
    strSize += len;
    str[strSize] = '\0';
}
//-----------------------------------------------------------------------
//                          hex2int
//-----------------------------------------------------------------------
uint32_t hex2int(char *hex) {
    uint32_t val = 0;
    int o;
    for(o=0; o<2; o++) {
        // get current character then increment
        uint8_t byte = *hex++;
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}
//-----------------------------------------------------------------------
//                         print_deg_to_lcd
//-----------------------------------------------------------------------
void print_deg_to_lcd(float phi) {
    int ipart = (int)phi;
    float fpart = phi - (float)ipart;
    int afterdigit = (int) (fpart * 1000);
    char afterDigit_str[4];
    char beforeDigit_str[1];
    int2str(beforeDigit_str, ipart);
    int2str(afterDigit_str, afterdigit);
    lcd_clear();
    lcd_home();
    if (ipart == 0) lcd_puts("0");
    else lcd_puts(beforeDigit_str);
    lcd_puts(".");
    lcd_puts(afterDigit_str);
    lcd_puts(" [deg]");
    START_TIMERA0(65535);

}
//----------------------Count Timer Calls---------------------------------
void timer_call_counter(){

    unsigned int num_of_halfSec;
    unsigned int res;
    num_of_halfSec = (int) delay_time / half_sec;
    res = delay_time % half_sec;
    res = res * clk_tmp;

    for (i=0; i < num_of_halfSec; i++){
        TIMER_A0_config(timer_half_sec);
        __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ int until Byte RXed
    }
    if (res > 1000){
        TIMER_A0_config(res);
        __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ int until Byte RXed
    }
}

//---------------------------------------------------------------------
//            Start Timer With counter
//---------------------------------------------------------------------
void START_TIMERA0(unsigned int counter){
    TIMER_A0_config(counter);
    __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ interrupt
}

//---------------------------------------------------------------------------------
//         Fixed point - returns degrees
//----------------------------------------------------------------------------------
int16_t atan2_fp(int16_t y_fp, int16_t x_fp)
{
    int32_t coeff_1 = 45;
    int32_t coeff_1b = -56; // 56.24;
    int32_t coeff_1c = 11;  // 11.25
    int16_t coeff_2 = 135;
    int16_t angle = 0;
    int32_t r;
    int32_t r3;
    int16_t y_abs_fp = y_fp;

    if (y_abs_fp < 0)y_abs_fp = -y_abs_fp;
    if (y_fp == 0)
    {
        if (x_fp >= 0)
        {angle = 0;}
        else
        {angle = 180;}
    }
    else if (x_fp >= 0)
    {
        r = (((int32_t)(x_fp - y_abs_fp)) << MULTIPLY_FP_RESOLUTION_BITS) /((int32_t)(x_fp + y_abs_fp));
        r3 = r * r;
        r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
        r3 *= r;
        r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
        r3 *= coeff_1c;
        angle = (int16_t) (coeff_1 + ((coeff_1b * r + r3) >>MULTIPLY_FP_RESOLUTION_BITS)   );
    }
    else
    {
        r = (((int32_t)(x_fp + y_abs_fp)) << MULTIPLY_FP_RESOLUTION_BITS) /((int32_t)(y_abs_fp - x_fp));
        r3 = r * r;
        r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
        r3 *= r;
        r3 =  r3 >> MULTIPLY_FP_RESOLUTION_BITS;
        r3 *= coeff_1c;
        angle = coeff_2 + ((int16_t)(((coeff_1b * r + r3) >>MULTIPLY_FP_RESOLUTION_BITS)));
    }

    if (y_fp < 0)
        return (360-angle);     // negate if in quad III or IV
    else
        return (angle);
}

//---------------**************************----------------------------
//               Interrupt Services Routines
//---------------**************************----------------------------
//*********************************************************************
//                        TIMER A0 ISR
//*********************************************************************
#pragma vector = TIMER0_A0_VECTOR // For delay
__interrupt void TimerA_ISR (void)
{
    StopAllTimers();
    LPM0_EXIT;
}

//*********************************************************************
//                        TIMER A ISR
//*********************************************************************
#pragma vector = TIMER1_A0_VECTOR // For delay
__interrupt void Timer1_A0_ISR (void)
{
    if(!TAIFG) {
    StopAllTimers();
    LPM0_EXIT;
    }
}

//*********************************************************************
//                         ADC10 ISR
//*********************************************************************
#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
   LPM0_EXIT;
}
//***********************************************************************************
//                              TX ISR
//***********************************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCI0TX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    if(state == state3 && finishIFG == 1){  // For script
        UCA0TXBUF = finish_str[tx_index++];                 // TX next character

        if (tx_index == sizeof step_str - 1) {   // TX over? send FIN
            tx_index=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            stateStepp = stateDefault;
            LPM0_EXIT;
        }
    }

    if (state == state3 && finishIFG == 0){  // For script
        UCA0TXBUF = step_str[tx_index++];                 // TX next character send step cunter

        if (tx_index == sizeof step_str - 1) {   // TX over?
            tx_index=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            stateStepp = stateDefault;
            LPM0_EXIT;
        }
    }
    else if (state==state2 && stateStepp==stateStopRotate){
        UCA0TXBUF = counter_str[tx_index++];                 // TX next character

        if (tx_index == sizeof counter_str - 1) {   // TX over?
            tx_index=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            stateStepp = stateDefault;
            LPM0_EXIT;
        }
    }
    else if (!dataIFG && state == state1){  // Send Push Button state
        if(MSBIFG) UCA0TXBUF = (state_changed[i]>>8) & 0xFF;// send msb first
        else UCA0TXBUF = (state_changed[i++]) & 0xFF;   //send lsb after
        MSBIFG ^= 1;

        if (i == 2) {  // TX over?
            i=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            START_TIMERA0(10000);
            dataIFG = 1;
            LPM0_EXIT;
        }
    }
    else if(dataIFG && state == state1){ //send data for painter!!
        if(MSBIFG) UCA0TXBUF = (Vr[i]>>8) & 0xFF;   // send msb first
        else UCA0TXBUF = (Vr[i++]) & 0xFF;          //send lsb after
        MSBIFG ^= 1;

        if (i == 2) {  // TX over?
            i=0;
            IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
            START_TIMERA0(10000);
            LPM0_EXIT;
        }
    }
}
//***********************************************************************************
//                              RX ISR
//***********************************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    stringFromPC[j] = UCA0RXBUF;  // Get Whole string from PC
    j++;
    // This if to get the file data. Added 'Z' to the end of the data in the python file, acts like ack
    if (stringFromPC[j-1] == 'Z'){
        j = 0;
        SendFlag = 0;
        strcpy(file_content, stringFromPC);
     //   ExecuteFlag = 1;
    }
    // This if to get the file name
    if (!SendFlag && stringFromPC[j-1] == '\x0a'){
        for (i=0; i < j; i++){
            file.file_name[i] = stringFromPC[i];
        }
        SendFlag = 1;
        j = 0;
    }
    if (stringFromPC[j-1] == 'W'){ //pointer for 1st selected file
        FlashBurnIFG = 1;
        ptr1 = (char*) 0x1000;
        file.file_ptr[0]=ptr1;
        file.num_of_files = 1;
        j = 0;
    }
    if (stringFromPC[j-1] == 'X'){ //pointer for 2nd selected file
        FlashBurnIFG = 1;
        ptr2 = (char*) 0x1040;
        file.file_ptr[1]=ptr2;
        file.num_of_files = 2;
        j = 0;
    }
    if (stringFromPC[j-1] == 'Y'){ //pointer for 3rd selected file
        FlashBurnIFG = 1;
        ptr3 = (char*) 0x1080;
        file.file_ptr[2]=ptr3;
        file.num_of_files = 3;
        j = 0; //
    }

    if (stringFromPC[j-1] == 'T'){ //index of executed list
        ExecuteFlag = 1;
        j = 0; //
        file.num_of_files = 1;
    }
    if (stringFromPC[j-1] == 'U'){
        ExecuteFlag = 1;
        j = 0; //
        file.num_of_files = 2;
    }
    if (stringFromPC[j-1] == 'V'){
        ExecuteFlag = 1;
        j = 0; //
        file.num_of_files = 3;
    }


    // If's for states
    if (stringFromPC[0] == 'm') {state = state0; stateStepp=stateDefault; rotateIFG = 0; j = 0;}        //Manual control
    else if (stringFromPC[0] == 'P') { state = state1; stateStepp=stateDefault; rotateIFG = 0; j = 0;}  //Paint
    else if (stringFromPC[0] == 'C') { state = state2; stateStepp=stateDefault; rotateIFG = 0; j = 0;}  //Calibrate
    else if (stringFromPC[0] == 'S') { state = state3; stateStepp=stateDefault; rotateIFG = 0; j = 0;}  //Script

    else if (stringFromPC[0] == 'A'){ stateStepp = stateAutoRotate; rotateIFG = 1; j = 0;}// Auto Rotate
    else if (stringFromPC[0] == 'M'){ stateStepp = stateStopRotate; rotateIFG = 0; j = 0;}// Stop Rotate
    else if (stringFromPC[0] == 'J'){ stateStepp = stateJSRotate; j = 0;}// JoyStick Rotatefixed pmsp430


    LPM0_EXIT;
}

//*********************************************************************
//            Port1 Interrupt Service Routine
//*********************************************************************
#pragma vector=PORT1_VECTOR
  __interrupt void Joystick_handler(void){ ///Pressing doesn't work so right now it's the pb0 key
      delay(debounceVal);
      if (state == state1){
          IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
          if(JoyStickIntPend & BIT5){ //int at P1.5
              dataIFG = 0; // send state!
              JoyStickIntPend &= ~BIT5;
          }
         IE2 |= UCA0TXIE;                       // enable USCI_A0 TX interrupt
      }

      else if(state == state2){
          if(JoyStickIntPend & BIT5){
              if(calibrateIFG){ stateStepp = stateAutoRotate; rotateIFG = 1; j = 0;calibrateIFG=0;}
              else if(!calibrateIFG){ stateStepp = stateStopRotate; rotateIFG = 0; j = 0;calibrateIFG = 1;}
              JoyStickIntPend &= ~BIT5;
          }
          LPM0_EXIT;
      }
  }
//******************************************************************
// send a command to the LCD
//******************************************************************
void lcd_cmd(unsigned char c){

  LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h

  if (LCD_MODE == FOURBIT_MODE)
  {
      LCD_DATA_WRITE &= ~OUTPUT_DATA;// clear bits before new write
      LCD_DATA_WRITE |= ((c >> 4) & 0x0F) << LCD_DATA_OFFSET;
      lcd_strobe();
      LCD_DATA_WRITE &= ~OUTPUT_DATA;
      LCD_DATA_WRITE |= (c & (0x0F)) << LCD_DATA_OFFSET;
      lcd_strobe();
  }
  else
  {
      LCD_DATA_WRITE = c;
      lcd_strobe();
  }
}
//******************************************************************
// send data to the LCD
//******************************************************************
void lcd_data(unsigned char c){

  LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h

  LCD_DATA_WRITE &= ~OUTPUT_DATA;
  LCD_RS(1);
  if (LCD_MODE == FOURBIT_MODE)
  {
          LCD_DATA_WRITE &= ~OUTPUT_DATA;
          LCD_DATA_WRITE |= ((c >> 4) & 0x0F) << LCD_DATA_OFFSET;
          lcd_strobe();
          LCD_DATA_WRITE &= (0xF0 << LCD_DATA_OFFSET) | (0xF0 >> 8 - LCD_DATA_OFFSET);
          LCD_DATA_WRITE &= ~OUTPUT_DATA;
          LCD_DATA_WRITE |= (c & 0x0F) << LCD_DATA_OFFSET;
          lcd_strobe();
  }
  else
  {
          LCD_DATA_WRITE = c;
          lcd_strobe();
  }

  LCD_RS(0);
}
//******************************************************************
// write a string of chars to the LCD
//******************************************************************
void lcd_puts(const char * s){

  while(*s)
      lcd_data(*s++);
}
//******************************************************************
// initialize the LCD
//******************************************************************
void lcd_init(){

  char init_value;

  if (LCD_MODE == FOURBIT_MODE) init_value = 0x3 << LCD_DATA_OFFSET;
  else init_value = 0x3F;

  LCD_RS_DIR(OUTPUT_PIN);
  LCD_EN_DIR(OUTPUT_PIN);
  LCD_RW_DIR(OUTPUT_PIN);
  LCD_DATA_DIR |= OUTPUT_DATA;
  LCD_RS(0);
  LCD_EN(0);
  LCD_RW(0);

  DelayMs(15);
  LCD_DATA_WRITE &= ~OUTPUT_DATA;
  LCD_DATA_WRITE |= init_value;
  lcd_strobe();
  DelayMs(5);
  LCD_DATA_WRITE &= ~OUTPUT_DATA;
  LCD_DATA_WRITE |= init_value;
  lcd_strobe();
  DelayUs(200);
  LCD_DATA_WRITE &= ~OUTPUT_DATA;
  LCD_DATA_WRITE |= init_value;
  lcd_strobe();

  if (LCD_MODE == FOURBIT_MODE){
      LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h
      LCD_DATA_WRITE &= ~OUTPUT_DATA;
      LCD_DATA_WRITE |= 0x2 << LCD_DATA_OFFSET; // Set 4-bit mode
      lcd_strobe();
      lcd_cmd(0x28); // Function Set
  }
  else lcd_cmd(0x3C); // 8bit,two lines,5x10 dots

  lcd_cmd(0xF); //Display On, Cursor On, Cursor Blink
  lcd_cmd(0x1); //Display Clear
  lcd_cmd(0x6); //Entry Mode
  lcd_cmd(0x80); //Initialize DDRAM address to zero
}
//******************************************************************
// lcd strobe functions
//******************************************************************
void lcd_strobe(){
LCD_EN(1);
asm("NOP");
// asm("NOP");
LCD_EN(0);
}
// ------------------------------------------------------------------
//                     Polling delays
//---------------------------------------------------------------------
//******************************************************************
// Delay usec functions
//******************************************************************
void DelayUs(unsigned int cnt){

  unsigned char i;
  for(i=cnt ; i>0 ; i--) asm("Nope"); // tha command asm("nop") takes raphly 1usec

}
//******************************************************************
// Delay msec functions
//******************************************************************
void DelayMs(unsigned int cnt){

  unsigned char i;
  for(i=cnt ; i>0 ; i--) DelayUs(1000); // tha command asm("nop") takes raphly 1usec

}
//******************************************************************
//            Polling based Delay function
//******************************************************************
void delay(unsigned int t){  //
  volatile unsigned int i;

  for(i=t; i>0; i--);
}
//---------------------------------------------------------------------
//            Enter from LPM0 mode
//---------------------------------------------------------------------
void enterLPM(unsigned char LPM_level){
  if (LPM_level == 0x00)
    _BIS_SR(LPM0_bits);     /* Enter Low Power Mode 0 */
      else if(LPM_level == 0x01)
    _BIS_SR(LPM1_bits);     /* Enter Low Power Mode 1 */
      else if(LPM_level == 0x02)
    _BIS_SR(LPM2_bits);     /* Enter Low Power Mode 2 */
  else if(LPM_level == 0x03)
    _BIS_SR(LPM3_bits);     /* Enter Low Power Mode 3 */
      else if(LPM_level == 0x04)
    _BIS_SR(LPM4_bits);     /* Enter Low Power Mode 4 */
}
//---------------------------------------------------------------------
//            Enable interrupts
//---------------------------------------------------------------------
void enable_interrupts(){
_BIS_SR(GIE);
}
//---------------------------------------------------------------------
//            Disable interrupts
//---------------------------------------------------------------------
void disable_interrupts(){
_BIC_SR(GIE);
}
