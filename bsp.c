#include  "../header/bsp.h"    // private library - BSP layer

//-----------------------------------------------------------------------------  
//           GPIO configuration
//-----------------------------------------------------------------------------
void GPIOconfig(void){
  WDTCTL = WDTHOLD | WDTPW;     // Stop WDT

  // JoyStick Configuration  P1.3 - Vrx; P1.4 - Vry; P1.5 - PB
  // P1.3-P1.4 - X(Don't care) for Sel, Dir According the dataSheet For A3,A4 input Select For ADC
  JoyStickPortDIR &= ~(BIT3 + BIT4);// for adc

  JoyStickPortSEL &= ~BIT5;  // P1.5 Sel = '0'
  JoyStickPortDIR &= ~BIT5;  // P1.5 input = '0'
  JoyStickPortOUT &= ~BIT5;  //
  JoyStickIntEN |= BIT5; // P1.5 PB_interrupt = '1'
  JoyStickIntEdgeSel |= BIT5; // P1.5 PB pull-up - '1'
  JoyStickIntPend &= ~BIT5; // Reset Int IFG - '0'

  // Stepmotor Configuration
  StepmotorPortSEL &= ~(BIT0+BIT1+BIT2+BIT3);  // P2.0-P2.3 Sel = '0'
  StepmotorPortDIR |= BIT0+BIT1+BIT2+BIT3;  // P2.0-P2.3 output = '1'

  // LCD configuration
  LCD_DATA_WRITE &= ~0xFF;
  LCD_DATA_DIR |= 0xF0;    // P2.4-P2.7 To Output ('1')
  LCD_DATA_SEL &= ~0xF0;   // Bit clear P2.4-P2.7
  LCD_CTL_SEL &= ~0xC1;    // Bit clear P1.0, P1.6, P1.7




  _BIS_SR(GIE);                     // enable interrupts globally
}
//-------------------------------------------------------------------------------------
//            Stop All Timers
//-------------------------------------------------------------------------------------
void StopAllTimers(void){
    TACTL = MC_0; // halt timer A
}
//-------------------------------------------------------------------------------------
//            Timer A  configuration
//-------------------------------------------------------------------------------------
void TIMER_A0_config(unsigned int counter){
    TACCR0 = counter; // (2^20/8)*345m = 45219 -> 0xB0A3
    TACCTL0 = CCIE;
    TA0CTL = TASSEL_2 + MC_1 + ID_3;  //  select: 2 - SMCLK ; control: 1 - Up ; divider: 3 - /8
    TA0CTL |= TACLR;
}
//-------------------------------------------------------------------------------------
//                              UART init
//-------------------------------------------------------------------------------------
void UART_init(void){
    if (CALBC1_1MHZ==0xFF)                  // If calibration constant erased
      {
        while(1);                               // do not load, trap CPU!!
      }
    DCOCTL = 0;                               // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
    DCOCTL = CALDCO_1MHZ;

 //   P2DIR = 0xFF;                             // All P2.x outputs
 //   P2OUT = 0;                                // All P2.x reset
    P1SEL |= BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
    P1SEL2 |= BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
   // P1DIR |= RXLED + TXLED;
    P1OUT &= 0x00;

    UCA0CTL1 |= UCSSEL_2;                     // CLK = SMCLK
  //  UCA0BR0 = 52;  // For 8Mhz
    UCA0BR0 = 104;
    UCA0BR1 = 0x00;                           //
    UCA0MCTL = UCBRS0;               //
  //  UCA0MCTL = 0x10|UCOS16; //UCBRFx=1,UCBRSx=0, UCOS16=1
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
}


//-------------------------------------------------------------------------------------
//            ADC configuration
//-------------------------------------------------------------------------------------
void ADCconfig(void){
    ADC10CTL1 = INCH_4 + CONSEQ_1 + ADC10SSEL_3;            // A4/A3 highest highest channel for a sequence of conversions
    ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON + ADC10IE;
    ADC10DTC1 = 0x02;                         // 2 conversions
    ADC10AE0 |= 0x18;                         // P1.3-4 ADC10 option select

}

