/**
*****************************************************************************
**Muszka Bal�zs (B5XOTO)
**Mikrokontroller labor h�zi,A24 feladat
**Stopper,visszasz�ml�l� programk�dja
**
*****************************************************************************
*/

#include "stm32f4xx.h"
#include "7seg.h"

/*
 * A glob�lis v�ltoz�k
 */
uint32_t counter =-1;    //stopper pillanatnyi �rt�ke:00.00.00->59.59.99 (perc.m�sodperc.sz�zadm�sodperc)
uint32_t counter_b=11;   //viszzasz�ml�l� pill �rt�ke:99.59.59->00.00.00 (�ra.perc.m�sodperc)
short int sel=0;        //kijelz�k kiv�laszt�sa, 6 k�z�l �ppen melyik jelez sel={7sel2,7sel1,7sel0} -> kapcsol�si rajzon
uint16_t tim7_i=0;      //perg�smentes�t�shez(wait_ms f�ggv�nyhez) kell, timer7 megszak�t�skezel�s�ben inkrement�l�dik ms-onk�nt
FunctionalState parttime=DISABLE;   //2 m�sodperces kifagyaszt�s(r�szid�) seg�dv�ltoz�ja  kezdeti �rt�k: kikapcsolva
FunctionalState setting=DISABLE;    //visszasz�ml�l� be�ll�t�s m�dj�nak seg�dv�ltoz�ja  kezdeti �rt�k: kikapcsolva



int main(void){

  int i = 0;

  GPIO_InitTypeDef portinit;
  TIM_TimeBaseInitTypeDef timstruct;
  NVIC_InitTypeDef nvicstruct;
  EXTI_InitTypeDef extistruct;

/*
 * A h�tszegmenses kijelz�k vez�rl�s�hez �s a BTN0-3 gombokhoz sz�ks�ges
 * GPIO be�ll�t�sok
 */

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOB,ENABLE);

    portinit.GPIO_Pin=GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
    portinit.GPIO_Mode=GPIO_Mode_OUT;            //DB0...DB7(PE8...PE15),#7sen_en(PE7)
    portinit.GPIO_OType=GPIO_OType_PP;           //h�tszegmenses kijelz� adatbitek,enged�lyez� jel->kimenetek
    portinit.GPIO_PuPd=GPIO_PuPd_NOPULL;
    portinit.GPIO_Speed=GPIO_Speed_2MHz;

    GPIO_Init(GPIOE,&portinit);
    GPIO_SetBits(GPIOE,GPIO_Pin_7);          //E port

    portinit.GPIO_Pin=GPIO_Pin_8|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;     //7sel0,7sel1,7sel2(PB13...PB15),7seg_clk(PB8)
    portinit.GPIO_Mode=GPIO_Mode_OUT;                                 //h�tszegmenses kijelz� kiv�laszt� bitek �s �rajel->kimenetek
    portinit.GPIO_OType=GPIO_OType_PP;
    portinit.GPIO_PuPd=GPIO_PuPd_NOPULL;
    portinit.GPIO_Speed=GPIO_Speed_2MHz;

    GPIO_Init(GPIOB,&portinit);       //B port




    portinit.GPIO_Pin=GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;
    portinit.GPIO_Mode=GPIO_Mode_OUT;                             //ledek(PD8...PD11)->kimenetek
    portinit.GPIO_OType=GPIO_OType_PP;
    portinit.GPIO_PuPd=GPIO_PuPd_NOPULL;

    GPIO_Init(GPIOD,&portinit);   //D port



    portinit.GPIO_Pin=GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
    portinit.GPIO_Mode=GPIO_Mode_IN;
    portinit.GPIO_OType=GPIO_OType_PP;                            //gombok(PD0..PD3)->bemenetek felh�z� ellen�ll�ssal
    portinit.GPIO_PuPd=GPIO_PuPd_UP;
    portinit.GPIO_Speed=GPIO_Speed_2MHz;

    GPIO_Init(GPIOD,&portinit);  //D port


/*
 * Timer4 �s NVIC be�ll�t�sa
 */

//Timer4 perif�ria �rajel�nek enged�lyez�se
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);

//Timer4 be�ll�t�sa: 420/6=70 Hz-zel villogtatja az egyes kijelz�ket, id�multiplexelt kijelz�s

   TIM_TimeBaseStructInit(&timstruct);
   timstruct.TIM_Period = 199;  //Peri�dus
   timstruct.TIM_Prescaler = 999;  //El�oszt�   420Hz-es timer
   TIM_TimeBaseInit(TIM4, &timstruct);
   TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
   TIM_Cmd(TIM4,ENABLE);             //kezdetekt�l fogva fut

//IT vez�rl� be�ll�t�sa

         //priorit�s csoportok
         NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //2-es priorit�s csoport, 4 szint preemption, 4 szint subpriority

         nvicstruct.NVIC_IRQChannel = TIM4_IRQn;
         nvicstruct.NVIC_IRQChannelCmd = ENABLE;
         nvicstruct.NVIC_IRQChannelPreemptionPriority = 0;  //0 priorit�s, legfontosabb, mindenkit megszak�that
         nvicstruct.NVIC_IRQChannelSubPriority = 1;
         NVIC_Init(&nvicstruct);



/*
 * Timer6 (sz�ml�l�s sz�zadm�sodperccel a feladata) �s NVIC be�ll�t�sa
*/

//Timer6 perif�ria �rajel�nek enged�lyez�se
       RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6,ENABLE);

//Timer6 be�ll�t�sa   100Hz (0.01s) counter sz�ml�l�sa sz�zadm�sodperccel (stopper funkci�)
       TIM_TimeBaseStructInit(&timstruct);
       timstruct.TIM_Period = 8399;  //Peri�dus
       timstruct.TIM_Prescaler = 99;  //El�oszt�
       TIM_TimeBaseInit(TIM6, &timstruct);
       TIM_Cmd(TIM6,DISABLE);                      //kikapcsolva, resetkor az �ra �ll, counter �rt�ke nem v�ltozik
       TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);



//IT vez�rl� be�ll�t�sa
   nvicstruct.NVIC_IRQChannel = TIM6_DAC_IRQn;
   nvicstruct.NVIC_IRQChannelCmd = ENABLE;
   nvicstruct.NVIC_IRQChannelPreemptionPriority =0 ;  //0 priorit�s, legfontosabb, mindenkit megszak�that
   nvicstruct.NVIC_IRQChannelSubPriority = 0;
   NVIC_Init(&nvicstruct);


 //Timer7 perif�ria �rajel�nek enged�lyez�se, Timer7->wait_ms f�ggv�ny id�z�t�se,1ms=>1kHz
          RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7,ENABLE);

 //Timer7 be�ll�t�sa  84MHz=>1kHz
          TIM_TimeBaseStructInit(&timstruct);
          timstruct.TIM_Period = 839;  //Peri�dus
          timstruct.TIM_Prescaler =99;  //El�oszt�
          TIM_TimeBaseInit(TIM7, &timstruct);
          TIM_Cmd(TIM7,DISABLE);              //kikapcsolva, csak a k�slelt� f�ggv�ny(wait_ms) kapcsolja be
          TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

 //IT vez�rl� be�ll�t�sa
   nvicstruct.NVIC_IRQChannel = TIM7_IRQn;
   nvicstruct.NVIC_IRQChannelCmd = ENABLE;
   nvicstruct.NVIC_IRQChannelPreemptionPriority =1;  //kev�sb� fontos, csak a perg�smentes�t�shez kell,
   nvicstruct.NVIC_IRQChannelSubPriority =0;        //megszak�thatja a kijelz� vez�rl�s��rt felel�s timer4, �s a sz�ml�l� timer6
   NVIC_Init(&nvicstruct);



   //Timer12 perif�ria �rajel�nek enged�lyez�se, visszasz�ml�l� �zemm�d, m�sodperc alap�->1Hz
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12,ENABLE);

   //Timer12 be�ll�t�sa  84MHz=>1Hz a visszasz�ml�l� alapegys�ge m�sodperc
            TIM_TimeBaseStructInit(&timstruct);
            timstruct.TIM_Period = 8399;  //Peri�dus
            timstruct.TIM_Prescaler =9999;  //El�oszt�
            TIM_TimeBaseInit(TIM12, &timstruct);
            TIM_Cmd(TIM12,DISABLE);                      //kezdetben a sz�ml�l�k �llnak, nem fut a visszasz�ml�l�s, timer12 �ll
            TIM_ITConfig(TIM12, TIM_IT_Update, ENABLE);

   //IT vez�rl� be�ll�t�sa
     nvicstruct.NVIC_IRQChannel = TIM8_BRK_TIM12_IRQn;
     nvicstruct.NVIC_IRQChannelCmd = ENABLE;
     nvicstruct.NVIC_IRQChannelPreemptionPriority =0;    //0 priorit�s, legfontosabb
     nvicstruct.NVIC_IRQChannelSubPriority =0;
     NVIC_Init(&nvicstruct);



/*
 * EXTI �s NVIC vez�rl� be�ll�t�sa
*/

//EXTI-port-Pin �sszerendel�s
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);


//EXTI be�ll�t�sa  gomb0
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD,EXTI_PinSource0);
  extistruct.EXTI_Line = EXTI_Line0;
  extistruct.EXTI_LineCmd = ENABLE;
  extistruct.EXTI_Mode = EXTI_Mode_Interrupt;
  extistruct.EXTI_Trigger = EXTI_Trigger_Falling;       //lefut� �lre szak�t meg(f�ldre h�z a gomb,1->0)
  EXTI_Init(&extistruct);

//IT vez�rl� be�ll�t�sa
   nvicstruct.NVIC_IRQChannel = EXTI0_IRQn;
   nvicstruct.NVIC_IRQChannelCmd = ENABLE;
   nvicstruct.NVIC_IRQChannelPreemptionPriority = 2;   //gombok nem olyan fontosak, timerek megszak�thatj�k �ket,
   nvicstruct.NVIC_IRQChannelSubPriority = 0;          //id�beli nagys�grend a megszak�t�skezel�shez:n�h�ny sz�z ms.
   NVIC_Init(&nvicstruct);                             //Ez sokkal t�bb, mint a timerek id�z�t�sei.



 //---------------------------------------------------------------------------------------
   //EXTI be�ll�t�sa gomb1
     SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD,EXTI_PinSource1);
     extistruct.EXTI_Line = EXTI_Line1;
     extistruct.EXTI_LineCmd = ENABLE;
     extistruct.EXTI_Mode = EXTI_Mode_Interrupt;
     extistruct.EXTI_Trigger = EXTI_Trigger_Falling;
     EXTI_Init(&extistruct);

   //IT vez�rl� be�ll�t�sa

      nvicstruct.NVIC_IRQChannel = EXTI1_IRQn;
      nvicstruct.NVIC_IRQChannelCmd = ENABLE;
      nvicstruct.NVIC_IRQChannelPreemptionPriority = 2;
      nvicstruct.NVIC_IRQChannelSubPriority = 1;            //gomb0 jut �rv�nyre el�sz�r, ha egyszerre k�rtek megszak�t�st
      NVIC_Init(&nvicstruct);  	 	  	  	  	  	  	  	//(a t�bbi gomb subpriority-je mindig eggyel nagyobb az el�z�n�l)
      	  	  	  	  	  	  	  	  	  	  	  	  	    //Teh�t a gomb3 hajt�dik v�gre utolj�ra, ha egyszerre �rkezett az �sszes megszak�t�s.


//--------------------------------------------------------------------------------------------

      //EXTI be�ll�t�sa gomb2
        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD,EXTI_PinSource2);
        extistruct.EXTI_Line = EXTI_Line2;
        extistruct.EXTI_LineCmd = ENABLE;
        extistruct.EXTI_Mode = EXTI_Mode_Interrupt;
        extistruct.EXTI_Trigger = EXTI_Trigger_Falling;
        EXTI_Init(&extistruct);

      //IT vez�rl� be�ll�t�sa

         nvicstruct.NVIC_IRQChannel = EXTI2_IRQn;
         nvicstruct.NVIC_IRQChannelCmd = ENABLE;
         nvicstruct.NVIC_IRQChannelPreemptionPriority = 2;
         nvicstruct.NVIC_IRQChannelSubPriority = 2;
         NVIC_Init(&nvicstruct);

//-----------------------------------------------------------------------------------------

         //EXTI be�ll�t�sa gomb3
           SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD,EXTI_PinSource3);
           extistruct.EXTI_Line = EXTI_Line3;
           extistruct.EXTI_LineCmd = ENABLE;
           extistruct.EXTI_Mode = EXTI_Mode_Interrupt;
           extistruct.EXTI_Trigger = EXTI_Trigger_Falling;
           EXTI_Init(&extistruct);

         //IT vez�rl� be�ll�t�sa

            nvicstruct.NVIC_IRQChannel = EXTI3_IRQn;
            nvicstruct.NVIC_IRQChannelCmd = ENABLE;
            nvicstruct.NVIC_IRQChannelPreemptionPriority = 2;
            nvicstruct.NVIC_IRQChannelSubPriority = 3;
            NVIC_Init(&nvicstruct);



 GPIO_SetBits(GPIOD,GPIO_Pin_8|GPIO_Pin_11);  //ledek kezdeti be�ll�t�sa:1. �s 4. z�ld led �g(jelent�s: stopper funkci�, �ll a stopper


  while (1)  //v�gtelen ciklusban feladatv�grahajt�s
  {
	i++;


	if(setting){           //visszasz�ml�l� �zemm�d, az egyes digitek �rt�kbe�ll�t�sa
		 uint32_t k;       //jelenlegi id� v�ltzoz�j�t t�roljuk benne, hogy az �ppen be�ll�tand� digitet kijelezz�k
		 uint8_t j;       //for ciklus seg�dv�ltoz�ja
		 k=counter_b;
		 for(j=0;j<5-sel;j++){   //a kijelzett id� megfelel� digitj�nek kiv�laszt�sa sel seg�ts�g�vel
		  k=k/10;
		 }
		 k%=10;         //k 0 �s 1 k�z�tt l�v� sz�m lesz
		 if(sel==1||sel==3) DisplayDigit(sel,k,1);   //van tizedespont(�ra �s perc ut�n,2. �s 4.kijelz�)
	     else DisplayDigit(sel,k,0);                //nincs tizedespont
	}                                              //ebben az �zemm�dban, mindig csak egy digitet jelen�t�nk meg: sel �rt�ke gombnyom�sra v�ltozik
	else{
		if(parttime){            //azt vizsg�ljuk, kell-e 2 m�sodpercig r�szid�t mutatni(kifagyaszt�s)
				wait_ms(2000);   //ha igen, 2 m�sodpercig v�runk
				parttime=DISABLE;  //ut�na letiltjuk a parttime v�ltoz�t, jelezve, hogy futhat tov�bb az �ra
				GPIO_ResetBits(GPIOD,GPIO_Pin_9);  //a 3. z�ld ledet lekapcsoljuk, jelezve, hogy v�get �rt a kifagyaszt�s (a 3. z�ld led jelzi, ha �ppen ki van fagyasztva az id�)
		}
	}



  }
}
