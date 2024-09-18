
#include  "../header/api.h"    		// private library - API layer
#include  "../header/halGPIO.h"     // private library - HAL layer
#include  "../header/flash.h"     // private library - FLASH layer
#include <stdio.h>
#include <math.h>
#include  "string.h"
#include "stdint.h"

unsigned int count_up = 0;
char count_up_str[5];
unsigned int* count_up_address = &count_up;

unsigned int count_down = 0;
char count_down_str[5];
unsigned int* count_down_address = &count_down;

int flag_script = 1;
int16_t Vrx = 0;
int16_t Vry = 0;
//-------------------------------------------------------------
//                Stepper Motor Calibration
//-------------------------------------------------------------
void calibrate(){
    int2str(counter_str, counter);
    tx_index = 0;
    UCA0TXBUF = counter_str[tx_index++];
    IE2 |= UCA0TXIE;                        // Enable USCI_A0 TX interrupt
    __bis_SR_register(LPM0_bits + GIE); // Sleep
    curr_counter = 0;
}

//-------------------------------------------------------------
//                Stepper clockwise
//-------------------------------------------------------------
void Stepper_clockwise(){
    int speed_clk;
    // 1 step clockwise of stepper - 150Hz
//    speed_clk = 131072/speed_Hz;
    speed_clk = 873; //(2^20/8)*(1/150[Hz]) = 873// Cycle time per step=1/150
    StepmotorPortOUT = 0x01; // out = 0001
    START_TIMERA0(speed_clk); // (2^20/8)*(1/150[Hz]) = 873
    StepmotorPortOUT = 0x08; // out = 1000
    START_TIMERA0(speed_clk); // (2^20/8)*(1/150[Hz]) = 873
    StepmotorPortOUT = 0x04; // out = 0100
    START_TIMERA0(speed_clk); // (2^20/8)*(1/150[Hz]) = 873
    StepmotorPortOUT = 0x02; // out = 0010
}

//-------------------------------------------------------------
//                Stepper counter-clockwise
//-------------------------------------------------------------
void Stepper_counter_clockwise(){
    int speed_clk;
    // 1 step counter-clockwise of stepper - 150Hz
    //  speed_clk = 131072/speed_Hz;
    speed_clk = 873; //(2^20/8)*(1/150[Hz]) = 873// Cycle time per step=1/150
    StepmotorPortOUT = 0x08; // out = 1000
    START_TIMERA0(speed_clk); //
    StepmotorPortOUT = 0x01; // out = 0001
    START_TIMERA0(speed_clk); //
    StepmotorPortOUT = 0x02; // out = 0010
    START_TIMERA0(speed_clk); //
    StepmotorPortOUT = 0x04; // out = 0100
}

//-------------------------------------------------------------
//             JoyStickADC_Steppermotor
//-------------------------------------------------------------
void JoyStickADC_Steppermotor(){
        ADC10CTL0 &= ~ENC;
        while (ADC10CTL1 & ADC10BUSY);               // Wait if ADC10 core is active
        ADC10SA = &Vr;                        // Data buffer start
        ADC10CTL0 |= ENC + ADC10SC; // Sampling and conversion start
        __bis_SR_register(LPM0_bits + GIE);        // LPM0, ADC10_ISR will force exit

}
//-------------------------------------------------------------
//             StepperUsingJoyStick
//-------------------------------------------------------------
void StepperUsingJoyStick(){
    uint32_t counter_phi;
    uint32_t phi;
    uint32_t phi2;
    uint32_t temp;
    while (counter != 0 && state==state0 && stateStepp==stateJSRotate){ //counter != 0 We did Calibrate first
        JoyStickADC_Steppermotor();
        if (!( Vr[1] > 400 && Vr[1] < 600 && Vr[0] > 400 && Vr[0] < 600)){ //Assuming they  still in center the pressure weak
            Vrx = Vr[1] -473;  //Calculation to maintain a ratio that the center will be zero
            Vry = Vr[0] -479;  //Calculation to maintain a ratio that the center will be zero

            phi = atan2_fp(Vry, Vrx);
            temp = phi * counter;

            if (270 < phi) {
                counter_phi = ((counter * 7) / 4) - (temp / 360);  // ((360+270-phi)/360)*counter;
                }
            else {
                counter_phi = ((counter * 3) / 4) - (temp / 360);  // ((270-phi)/360)*counter;
                }
            if ((int)(curr_counter - counter_phi) < 0) {
                Stepper_clockwise();
                curr_counter++;
            }
            else {
                Stepper_counter_clockwise();
                curr_counter--;
            }
        }
    }

}
//-------------------------------------------------------------
//               JoyStick_Painter
//-------------------------------------------------------------
void JoyStick_Painter(){
    JoyStickIntEN &= ~BIT5; // allow interrupt only in the end of cycle
    i = 0;
    if(dataIFG) { //send data
        ADC10CTL0 &= ~ENC;
        while (ADC10CTL1 & ADC10BUSY);               // Wait if ADC10 core is active
        ADC10SA = &Vr;                               // Data buffer start
        ADC10CTL0 |= ENC + ADC10SC;                  //Sampling and conversion start
        __bis_SR_register(LPM0_bits + GIE);

        IE2 |= UCA0TXIE;
        __bis_SR_register(LPM0_bits + GIE);        //  exit when finish tx
    }

    else if (!dataIFG) { //send state

        IE2 |= UCA0TXIE;
        __bis_SR_register(LPM0_bits + GIE);        //  exit when finish tx


        START_TIMERA0(5000); // wait PC to get sync after  the debounce and interrupt delay
        JoyStickIntPend &= ~BIT5;
    }
    JoyStickIntEN |= BIT5; // allow interrupt  end of cycle
}
//---------------------------------------------------------
//          Flash
//----------------------------------------------------------
void ScriptFunc() {

    if(FlashBurnIFG){
        FlashBurnIFG=0;
        FCTL2 = FWKEY + FSSEL0 + FN1;             // MCLK/3 for Flash Timing Generator
        file.file_size[file.num_of_files - 1] = strlen(file_content) - 1; //Size save Size

        write_to_mem();
        send_finish_to_PC(); // send ACK to PC Finish Save  file in memory

        IE2 |= UCA0RXIE;
    }
    if(ExecuteFlag){
        ExecuteFlag=0;
        flag_script = 1;
        delay_time = 500;  // delay default time
        if(flag_script){ ExecuteScript();}
        flag_script=0;
        send_finish_to_PC(); // finish script
    }
    __bis_SR_register(LPM0_bits + GIE); //Waiting to run next file
}

//---------------Execute Script--------------------------------
void ExecuteScript(void)
{
    char *Flash_ptrscript;                         // Segment pointer
    char OPCstr[5], Operand1Flash[20], Operand2Flash[20];
    unsigned int Oper2ToInt, X,K;

    Flash_ptrscript = file.file_ptr[file.num_of_files - 1];
    for (K = 0; K < 20;K++){
        OPCstr[0] = *Flash_ptrscript++;
        OPCstr[1] = *Flash_ptrscript++;

        switch (OPCstr[1])
        {
        case '1':
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;

            Oper2ToInt = hex2int(Operand1Flash);
            inc_lcd(Oper2ToInt);    //Count up from zero
            break;

        case '2':
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;

            Oper2ToInt = hex2int(Operand1Flash);
            dec_lcd(Oper2ToInt);           //Count down from x
            break;

        case '3':
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;

            Oper2ToInt = hex2int(Operand1Flash);
            rra_lcd(Operand1Flash[1]);        //Rotate right onto LCD
            break;


        case '4':
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;

            delay_time = hex2int(Operand1Flash);//set delay
            delay_time = delay_time * 10 ; //its in unit of 10ms
            break;
        case '5':
           lcd_clear();
            break;
        case '6': //point stepper motor to degree p
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;

            X = hex2int(Operand1Flash);
            motorGoToPosition(X, OPCstr[1]);

            break;
        case '7': //scan area between angle l to r
            Operand1Flash[0] = *Flash_ptrscript++;
            Operand1Flash[1] = *Flash_ptrscript++;

            Operand2Flash[0] = *Flash_ptrscript++;
            Operand2Flash[1] = *Flash_ptrscript++;

            X = hex2int(Operand1Flash);
            motorGoToPosition(X, OPCstr[1]);           // start

            X = hex2int(Operand2Flash);
            motorGoToPosition(X, OPCstr[1]);          //  stop

            break;
        case '8': // go sleep
//            state = state0;
            break;

        }
    }
}
//****************Script Functions*****************************
//-------------------------------------------------------------
//                1. count_up_LCD
//-------------------------------------------------------------
void inc_lcd(int X){
    while(*count_up_address<=X){
        lcd_clear();
        lcd_home();
        lcd_puts("Count Up: ");
        lcd_new_line;
        int2str(count_up_str, *count_up_address);
        lcd_puts(count_up_str);
        timer_call_counter();
        *count_up_address = (*count_up_address + 1) % 65536;
    }
    count_up = 0;
    lcd_clear();
}
//-------------------------------------------------------------
//                2. count_down_LCD
//-------------------------------------------------------------
void dec_lcd(int X){
    count_down = X;
    while(*count_down_address!=0){
        lcd_clear();
        lcd_home();
        lcd_puts("Count down: ");
        lcd_new_line;
        int2str(count_down_str, *count_down_address);
        lcd_puts(count_down_str);
        timer_call_counter();
        *count_down_address = (*count_down_address - 1) % 65536;
    }
    lcd_home();
    lcd_new_line;
    lcd_putchar('0');
    timer_call_counter();
    lcd_clear();
}

//-------------------------------------------------------------
//                3. Rotate right LEDS
//-------------------------------------------------------------
void rra_lcd(char X){
    unsigned int i;
    lcd_home();
    for(i=0; i<=15;i++){
        lcd_putchar(X);
        timer_call_counter();
        lcd_cursor_left();
        lcd_putchar(' ');
    }
    lcd_new_line;
    for(i=0; i<=15;i++){
        lcd_putchar(X);
        timer_call_counter();
        lcd_cursor_left();
        lcd_putchar(' ');
    }
}
//-------------------------------------------------------------
//                6.and 7 motorGoToPosition
//-------------------------------------------------------------
void motorGoToPosition(uint32_t stepper_degrees, char script_state){
    int clicks_cnt;
    uint32_t step_counts;
    uint32_t calc_temp;
    calc_temp = stepper_degrees * counter;
    step_counts = (calc_temp / 360); // how much clicks to wanted degree
    float step_phi = (float)360 / counter;
    float curr_phi;
    //RK code
    int diff = step_counts - curr_counter;

    if(0 <= diff){ //move CW
        for (clicks_cnt = 0; clicks_cnt < diff; clicks_cnt++){
            curr_counter++;
            Stepper_clockwise();

            START_TIMERA0(10000);
            //send data only if FINISH or stepper_deg (state 6)
            if(script_state == '6'){
                int2str(step_str, curr_counter);
                send_degree_to_PC(); }
        }
        if (script_state == '7') {
            curr_phi = (step_phi*curr_counter);
            print_deg_to_lcd(curr_phi);

            int2str(step_str, curr_counter);
            send_degree_to_PC();
        }
        sprintf(step_str, "%s", "FFFF"); // add finish flag
        send_degree_to_PC();
   }
//-------------------------------------------------------------------------
    else{ // move CCW
        for (clicks_cnt = diff; clicks_cnt < 0; clicks_cnt++){
            curr_counter--;
            Stepper_counter_clockwise();
            START_TIMERA0(10000);

            if(script_state == '6'){    //send data  stepper_deg (state 6)
                int2str(step_str, curr_counter);
                send_degree_to_PC();
            }
        }
        if (script_state == '7') {
            curr_phi = (step_phi*curr_counter);
            print_deg_to_lcd(curr_phi);

            int2str(step_str, curr_counter);
            send_degree_to_PC();
        }
        sprintf(step_str, "%s", "FFFF"); // add finish flag
        send_degree_to_PC();
        }
}


