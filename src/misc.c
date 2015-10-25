/*
 * misc.c

 *
 *  Created on: 2014.04.14.
 *      Author: Muszka
 */

//seg�df�ggv�nyek (misc.c)


#include "stm32f4xx.h"
#include "misc.h"

//Timer7 glob�lis v�ltoz�ja
extern uint16_t tim7_i;

void wait_ms(uint16_t s){       //ms-os k�sleltet�s Timer7-vel (s: ennyi ms-t v�runk)
	tim7_i=0;
	while(tim7_i!=s) TIM_Cmd(TIM7,ENABLE);  //timer7 elind�t�sa (1kHz), ciklus am�g timer7_i �rt�ke megn�vekszik s-re
	if(tim7_i==s){                         //ha el�rt�k a k�v�nt �rt�ket(s) timer7_i-vel
		tim7_i=0;                          //v�ltoz� null�z�s
		TIM_Cmd(TIM7,DISABLE);             //timer7 kikapcsol�sa
	}
}
