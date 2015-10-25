/**
*****************************************************************************
**Megszak�t�s kezel� f�ggv�nyek
**
**
*****************************************************************************
*/

#include "stm32f4xx_it.h"
#include "7seg.h"
#include "misc.h"

/*
 * A k�ls� glob�lis v�ltoz�k
 */
extern uint32_t counter;    //stopper ideje
extern uint32_t counter_b;  //visszasz�ml�l� ideje
extern short int sel;       //sel v�lasztja ki a kijelzend� digitet
extern FunctionalState parttime;  //kifagyaszt�s
extern FunctionalState setting;   //digitek egyenk�nti be�ll�t�sa

//Saj�t v�ltoz�k
uint32_t counter_part;              //r�szid� �rt�ke, ez mindig egy konkr�t sz�m, nem n�veli timer
FunctionalState enabletimer=DISABLE;   //azt jelzi, hogy fut vagy �ll az �ra (mindk�t �zemm�dban) 4.z�ld led vil�g�t, ha �ll az �ra  reset->�ll az �ra
FunctionalState mode=ENABLE;          //mode v�laszja ki az �zemm�dot: mode=ENABLE->stopper  reset->stopper funkci�



/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/



/*
 * Timer4 megszak�t�skezel�, ami a counter glob�lis v�ltoz�ban t�rolt �rt�ket
 * id�multiplex�ltan megjelen�ti a h�tszegmenses kijelz�k�n. Egy-egy magszak�t�si ciklus
 * alkalm�val egy-egy kijelz�re kell ki�rni a hozz� tartoz� sz�m�rt�ket.
 */



void TIM4_IRQHandler(void)
{
	/* Lek�rdezz�k a megszak�t�s ok�t � t�bb is lehet */
			 if(TIM_GetITStatus(TIM4, TIM_IT_Update))
			 {  uint32_t i;        //egy id� �rt�ket fog kapni
			   uint8_t j;          //for seg�dv�ltoz�ja
			   if(parttime) i=counter_part;       //ha r�szid� �zemm�d van, a kifagyasztott id�t mutassa
			   else {                            //egy�bk�nt a fut� id�t mutassa
				   if(mode) i=counter;           //stopper �zemm�d(mode=ENABLE)
				   else i=counter_b;             //visszasz�ml�l�
			   }


			 	 for(j=0;j<5-sel;j++){           //sel v�ltoz�val meghat�rozzuk i megfelel� sz�mjegy�t(0...5), amit egy db kijelz�n jelz�nk majd ki
			 		 i=i/10;                     //addig osztjuk le 10-zel, am�g az egyes helyi�rt�ken lesz a k�v�nt sz�mjegy
			 	 }
                i%=10;                           //10-es marad�k-> i �rt�ke:0...9
                if(sel==1||sel==3) DisplayDigit(sel,i,1); //ha a 1. �s 3. kijelz�n�l tartunk kell tizedespont(perc.m�sodperc.sz�zadm�sodperc)
                else DisplayDigit(sel,i,0);               //Displaydigit jelen�ti meg az adatot(i), a kiv�lasztott kijelz�re(sel); 7seg.c-ben van meg�rva

                if(sel==5) sel=0;                        //utols�(5.) kijelz� ut�n a 0. j�n
                 else sel++;                             //id�multiplexelt kijelz�s, sel �rt�ke timer4 frekvenci�j�val(420Hz) n�vekszik, �gy v�ltogatva az egyes kijelz�ket, de az emberi szem ezt nem tudja k�vetni, minden kijelz�n l�t sz�mot

				 /* T�r�lj�k a megszak�t�s jelz� flag-et */
					 TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
			 }



}


/*
 * Timer6 megszak�t�skezel�je, ami a counter v�ltoz� n�vel�s�t �s 0..595999
 * �rt�kek k�z�tt tart�s�t v�gzi
 */
void TIM6_DAC_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM6, TIM_IT_Update)){
		if(counter==595999) counter=0;                                 //hat�r�rt�k el�r�se, null�z�s
		else { if((counter%10000)==5999) counter=counter-5999+10000;    //percv�lt�s xx.59.99->xy.00.00  y=x+1
		       else counter++;}                                       //sima sz�zadm�sodperces l�p�s
	TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
	}

}

//Timer7
void TIM7_IRQHandler(void)  //ms k�sleltet�s, wait_ms f�ggv�nyhez(misc.c) kell,tim7_i v�ltoz�t ms-onk�nt n�veli
{
	if(TIM_GetITStatus(TIM7,TIM_IT_Update)){
		tim7_i++;
		TIM_ClearITPendingBit(TIM7,TIM_IT_Update);
	}
}


/*Timer12(timer8 �s timer12 megszak�t�skezel�je k�z�s, de mi csak timer12-t haszn�ljuk
 * timer12: visszasz�ml�l� idej�t cs�kkenti m�sodperces alappal f=1Hz
*/
void TIM8_BRK_TIM12_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM12, TIM_IT_Update)){
		if(counter_b==0){                                               //hat�r�rt�k el�r�se, sz�ml�l�s le��l�t�sa
		  TIM_Cmd(TIM12,DISABLE);
		  GPIO_SetBits(GPIOD,GPIO_Pin_8);                               //4. z�ld led jelzi, ha �ll az �ra
		  enabletimer=DISABLE;                                          //enabletimer t�rolja, hogy meg�llt az �ra
		}
		else { if((counter_b%10000)==0) counter_b=counter_b+5959-10000;    //�rav�lt�s:xy.00.00.->x(y-1).59.59
		       else {
		    	   if((counter_b%100)==0) counter_b=counter_b-100+59;      //percv�lt�s:xy.zw.00->xy.z(w-1).59
		    	   else counter_b--;}                                  //m�sodperces lefele l�p�s
		       }
	TIM_ClearITPendingBit(TIM12, TIM_IT_Update);
	}

}


/*
 * BTN0..3 gombokhoz tartoz� megszak�t�skezel�k
 */


//panelen az 1. gomb
void EXTI0_IRQHandler(void)            //gombok megszak�t�skezel�se: 0gomb-> �zemm�d v�laszt�s
{
	if(EXTI_GetITStatus(EXTI_Line0)){
		if(!setting){                   //ha nincs digit be�ll�t�s, csak akkor szabad �zemm�dot v�ltani
		mode=!mode;                     //funkkci� v�lt�s: stopper<->visszasz�ml�l�s (mode v�ltoz�ban t�roljuk, �rt�ke:ENABLE vagy DISABLE)
		enabletimer=DISABLE;            //fut�s k�zben is lehet funkci�t v�ltani, de ekkor le kell �ll�tani a sz�ml�l�sokat
		GPIO_SetBits(GPIOD,GPIO_Pin_8); //4. z�ld led vil�g�t, jelezve h �ll az �ra
		if(mode){                       //ha stopper �zemm�dba l�pt�nk
			TIM_Cmd(TIM12,DISABLE);     //visszasz�ml�l� timer-�t le�ll�tjuk
			GPIO_SetBits(GPIOD,GPIO_Pin_11); //1. z�ld ledet f�lkapcsoljuk, ez jelzi a stopper funkci�t
			GPIO_ResetBits(GPIOD,GPIO_Pin_10); //2. piros ledet lekapcsoljuk, nincs m�r visszasz�ml�l�s
		}
		else {                            //ha visszasz�ml�l� �zem�dba l�pt�nk
			TIM_Cmd(TIM6,DISABLE);        //timer6 le�ll�t�sa (stopper�)
			GPIO_SetBits(GPIOD,GPIO_Pin_10); //ledek be�ll�t�sa: 2.piros �g, 1.z�ld nem
		    GPIO_ResetBits(GPIOD,GPIO_Pin_11);
		}

		}
        wait_ms(300);                                  //perg�smentes�t�s 300ms k�sleltetet�ssel
        EXTI_ClearITPendingBit(EXTI_Line0);


	}
}
//panelen a 3. gomb
void EXTI1_IRQHandler(void)            //gombok megszak�t�skezel�se stopper:2 m�sodperces kifagyaszt�s
                                                        //visszasz�ml�l�ban: kijelz� kiv�laszt�sa be�ll�t�sra
{
	if(EXTI_GetITStatus(EXTI_Line1)){
		if(mode){                      //stopper(mode=1)->kifagyaszt�s
		parttime=ENABLE;              //jelezz�k h r�szid� mutat�s �zemm�dban vagyunk
		counter_part=counter;         //elmentj�k a pillanatnyi id�t, counter_part t�rolja a kifagyasztott id� �rt�ket, de counter �rt�ke fut tov�bb
		if(parttime) GPIO_SetBits(GPIOD,GPIO_Pin_9); //bekapcsoljuk a 3. z�ld ledet, jelezve, hogy �ppen a r�szid�t l�tjuk
		}

		if(!mode && !enabletimer){  //ha visszasz�ml�l� �zem�d �s �ll az �ra(az�rt mert csak ekkor szabad az egyes digitek �rt�k�n v�ltoztatni)
			if(!setting){           //ha m�g nem l�pt�nk be a be�ll�t�sba, csak �ll az id�, �s minden digit jelez
			 TIM_Cmd(TIM4,DISABLE); //timer4 le�ll�t�sa-> nincs id�multiplexelt kijelz�s, innent�l csak egy digit fog l�tsz�dni egyszerre
			 setting=ENABLE;        //jelezz�k h be�ll�t�s �zem�dban vagyunk
             sel=-1;
			}
			if(setting){
				if(sel==5){        //ha az utols� digitn�l tartunk, kil�p�nk a be�ll�t�sb�l, elinditjuk timer4-t, �jra egyszerre l�tsz�dik az �sszes digit
					TIM_Cmd(TIM4,ENABLE);
				    setting=DISABLE;
				}
				else sel++;       //egy gombnyom�sra ugorjon a k�vetkez� digitre, amit be lehet �ll�tani
			}

		}

    wait_ms(300);                                  //perg�smentes�t�s 300ms k�sleltetet�ssel
	EXTI_ClearITPendingBit(EXTI_Line1);
	}
}
//panelen a 2. gomb
void EXTI2_IRQHandler(void)            //gombok megszak�t�skezel�se: start-stop funkci�
{
	if(EXTI_GetITStatus(EXTI_Line2)){
		if(!setting){                    //be��l�t�s funkci�ban nem szabad ind�tani
		enabletimer=!enabletimer;        //start<->stop v�lt�s (enabletimer: ENABLE vagy DISABLE)
		if(mode) TIM_Cmd(TIM6, enabletimer); //ha stopper->timer6 ind�t�sa vagy le�ll�t�sa
		else TIM_Cmd(TIM12,enabletimer);     //v.sz�ml�l�->timer12
		if(!enabletimer) GPIO_SetBits(GPIOD,GPIO_Pin_8); //ha �ll az �ra, kigyullad a 4. z�ld led
		else GPIO_ResetBits(GPIOD,GPIO_Pin_8);           //ha elindul, kialszik
		}

	    wait_ms(300);                                  //perg�smentes�t�s 300ms k�sleltetet�ssel
		EXTI_ClearITPendingBit(EXTI_Line2);

	}

}

//panelen a 4. gomb
void EXTI3_IRQHandler(void)            //gombok megszak�t�skezel�se   visszasz�ml�l� �zemm�dban,
                                       //ha be�ll�t�s van(setting), egy darab kijelz� �rt�k�nek megn�vel�se
{                                      // ha stopper, null�zzuk a kijelz�t
	if(EXTI_GetITStatus(EXTI_Line3)){
		if(setting){                 //be�ll�t�s �zemm�d
			uint32_t i;
            uint8_t j;
	        i=counter_b;            //i: pillanatnyi le�ll�tott id�, �rt�ke:00.00.00->99.59.59
		    for(j=0;j<5-sel;j++){   //sel-lel kiv�lasztjuk a megfelel� sz�mjeget
				 i=i/10;
			 }
            i%=10;              //i �rt�ke:0...9 vagy 0...5
            if(sel==2 || sel==4){   //ha a i a m�sodperc magasabb helyi�rt�ke vagy a perc magasabb helyi�rt�ke, csak 0 �s 5 k�z�tti �rt�ket adhatunk neki
            	if(i==5){           //ha i=5, egy gombnyom�s ut�n az �rt�ke 0 lesz(ezt le kell menteni counter_b-be is
            	            for(j=0;j<5-sel;j++){
            	            	i*=10;            //felszorozzuk a megfelel� helyi�rt�kre
            	            }
            	            counter_b-=i;        //levonjuk counter_b-b�l; �gy azt kaptuk, hogy 5 helyett 0 jelez ki a 3. vagy 5. kijelz�n
            	            }
            	else{           //ha i!=5 csak megn�velj�k eggyel counter_b megfelel� sz�mjegy�t
                	i=1;
                	for(j=0;j<5-sel;j++){
                		i*=10;
                	}
                	counter_b+=i;
                }
            }
            else{              //ha i �rt�ke 0 �s 9 k�z�tt v�ltozhat
            if(i==9){         // ha i=9, 0 legyen az �rt�k (counter_b-b�l levonjuk x*10*i-t(x:0..5)
            for(j=0;j<5-sel;j++){
            	i*=10;
            }
            counter_b-=i;
            }
            else{            //i!=9, a megfelel� helyi�rt�ket n�velj�k meg counter_b-ben
            	i=1;
            	for(j=0;j<5-sel;j++){
            		i*=10;
            	}
            	counter_b+=i;
            }
            }


		}
		if(mode) counter=0;         //stopper null�z�sa(mode=ENABLE)


	    wait_ms(300);                                  //perg�smentes�t�s 300ms k�sleltetet�ssel
		EXTI_ClearITPendingBit(EXTI_Line3);
	}
}



