#include  "../header/api.h"    		// private library - API layer
#include  "../header/app.h"    		// private library - APP layer
#include  <stdio.h>


enum FSMstate state;
enum Stepperstate stateStepp;
enum SYSmode lpm_mode;


void main(void){
  
  state = state0;  // start in idle state on RESET
  stateStepp = stateDefault;
  lpm_mode = mode0;     // start in idle state on RESET
  sysConfig();     // Configure GPIO, Init ADC


  while(1){
	switch(state){

	case state0: //   StepperUsingJoyStick
	    IE2 |= UCA0RXIE; // Enable USCI_A0 RX interrupt
	    switch(stateStepp){
	    case stateAutoRotate:
	       //As long as there is a flag, run
	        while(rotateIFG){START_TIMERA0(600); curr_counter++; Stepper_clockwise(); }
	        break;

        case stateStopRotate:
            break;

        case stateJSRotate:
            counter=513;
            StepperUsingJoyStick();
            break;
        case stateDefault:
            __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ until Byte in RXed
            break;
	    }
	    break;

	case state1: // Paint
	    JoyStickIntEN |= BIT5;
	    while (state == state1){JoyStick_Painter();}
        JoyStickIntEN &= ~BIT5;
	    break;

	case state2: // Calibrate
        IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt

        switch(stateStepp){
        case stateDefault:
            JoyStickIntEN |= BIT5;
            __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ int until Byte RXed
            break;

        case stateAutoRotate: // start rotate
            counter = 0;
            while(rotateIFG) {START_TIMERA0(600);Stepper_clockwise(); counter++; }
            break;

        case stateStopRotate: // stop and set phi
            JoyStickIntEN &= ~BIT5;
            calibrate();
            break;
        }
	    break;

	case state3:  //Script
        IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
	    while ( state == state3){
	        ScriptFunc();
	    }
        break;
		
	}
  }
}

  
  
  
  
  
  
