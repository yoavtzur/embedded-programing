#ifndef _api_H_
#define _api_H_

#include  "../header/halGPIO.h"     // private library - HAL layer

extern void Stepper_counter_clockwise();
extern void Stepper_clockwise();
extern void calibrate();
extern void ScriptFunc();
extern void ExecuteScript();
extern void JoyStick_Painter();
extern void JoyStickADC_Steppermotor();

extern void StepperUsingJoyStick(); // dont understen

extern void inc_lcd(int);
extern void dec_lcd(int);
extern void rra_lcd(char);
extern int16_t Vrx;
extern int16_t Vry;
extern char step_str[4];
#endif







