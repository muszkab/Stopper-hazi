/*
 * misc.c

 *
 *  Created on: 2014.04.14.
 *      Author: Muszka
 */

//segédfüggvények (misc.c)


#include "stm32f4xx.h"
#include "misc.h"

//Timer7 globális változója
extern uint16_t tim7_i;

void wait_ms(uint16_t s){       //ms-os késleltetés Timer7-vel (s: ennyi ms-t várunk)
	tim7_i=0;
	while(tim7_i!=s) TIM_Cmd(TIM7,ENABLE);  //timer7 elindítása (1kHz), ciklus amíg timer7_i értéke megnövekszik s-re
	if(tim7_i==s){                         //ha elértük a kívánt értéket(s) timer7_i-vel
		tim7_i=0;                          //változó nullázás
		TIM_Cmd(TIM7,DISABLE);             //timer7 kikapcsolása
	}
}
