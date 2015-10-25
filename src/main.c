/**
*****************************************************************************
**Muszka Balázs (B5XOTO)
**Mikrokontroller labor házi,A24 feladat
**Stopper,visszaszámláló programkódja
**
*****************************************************************************
*/

#include "stm32f4xx.h"
#include "7seg.h"

/*
 * A globális változók
 */
uint32_t counter =-1;    //stopper pillanatnyi értéke:00.00.00->59.59.99 (perc.másodperc.századmásodperc)
uint32_t counter_b=11;   //viszzaszámláló pill értéke:99.59.59->00.00.00 (óra.perc.másodperc)
short int sel=0;        //kijelzõk kiválasztása, 6 közül éppen melyik jelez sel={7sel2,7sel1,7sel0} -> kapcsolási rajzon
uint16_t tim7_i=0;      //pergésmentesítéshez(wait_ms függvényhez) kell, timer7 megszakításkezelésében inkrementálódik ms-onként
FunctionalState parttime=DISABLE;   //2 másodperces kifagyasztás(részidõ) segédváltozója  kezdeti érték: kikapcsolva
FunctionalState setting=DISABLE;    //visszaszámláló beállítás módjának segédváltozója  kezdeti érték: kikapcsolva



int main(void){

  int i = 0;

  GPIO_InitTypeDef portinit;
  TIM_TimeBaseInitTypeDef timstruct;
  NVIC_InitTypeDef nvicstruct;
  EXTI_InitTypeDef extistruct;

/*
 * A hétszegmenses kijelzõk vezérléséhez és a BTN0-3 gombokhoz szükséges
 * GPIO beállítások
 */

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOB,ENABLE);

    portinit.GPIO_Pin=GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
    portinit.GPIO_Mode=GPIO_Mode_OUT;            //DB0...DB7(PE8...PE15),#7sen_en(PE7)
    portinit.GPIO_OType=GPIO_OType_PP;           //hétszegmenses kijelzõ adatbitek,engedélyezõ jel->kimenetek
    portinit.GPIO_PuPd=GPIO_PuPd_NOPULL;
    portinit.GPIO_Speed=GPIO_Speed_2MHz;

    GPIO_Init(GPIOE,&portinit);
    GPIO_SetBits(GPIOE,GPIO_Pin_7);          //E port

    portinit.GPIO_Pin=GPIO_Pin_8|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;     //7sel0,7sel1,7sel2(PB13...PB15),7seg_clk(PB8)
    portinit.GPIO_Mode=GPIO_Mode_OUT;                                 //hétszegmenses kijelzõ kiválasztó bitek és órajel->kimenetek
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
    portinit.GPIO_OType=GPIO_OType_PP;                            //gombok(PD0..PD3)->bemenetek felhúzó ellenállással
    portinit.GPIO_PuPd=GPIO_PuPd_UP;
    portinit.GPIO_Speed=GPIO_Speed_2MHz;

    GPIO_Init(GPIOD,&portinit);  //D port


/*
 * Timer4 és NVIC beállítása
 */

//Timer4 periféria órajelének engedélyezése
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);

//Timer4 beállítása: 420/6=70 Hz-zel villogtatja az egyes kijelzõket, idõmultiplexelt kijelzés

   TIM_TimeBaseStructInit(&timstruct);
   timstruct.TIM_Period = 199;  //Periódus
   timstruct.TIM_Prescaler = 999;  //Elõosztó   420Hz-es timer
   TIM_TimeBaseInit(TIM4, &timstruct);
   TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
   TIM_Cmd(TIM4,ENABLE);             //kezdetektõl fogva fut

//IT vezérlõ beállítása

         //prioritás csoportok
         NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //2-es prioritás csoport, 4 szint preemption, 4 szint subpriority

         nvicstruct.NVIC_IRQChannel = TIM4_IRQn;
         nvicstruct.NVIC_IRQChannelCmd = ENABLE;
         nvicstruct.NVIC_IRQChannelPreemptionPriority = 0;  //0 prioritás, legfontosabb, mindenkit megszakíthat
         nvicstruct.NVIC_IRQChannelSubPriority = 1;
         NVIC_Init(&nvicstruct);



/*
 * Timer6 (számlálás századmásodperccel a feladata) és NVIC beállítása
*/

//Timer6 periféria órajelének engedélyezése
       RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6,ENABLE);

//Timer6 beállítása   100Hz (0.01s) counter számlálása századmásodperccel (stopper funkció)
       TIM_TimeBaseStructInit(&timstruct);
       timstruct.TIM_Period = 8399;  //Periódus
       timstruct.TIM_Prescaler = 99;  //Elõosztó
       TIM_TimeBaseInit(TIM6, &timstruct);
       TIM_Cmd(TIM6,DISABLE);                      //kikapcsolva, resetkor az óra áll, counter értéke nem változik
       TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);



//IT vezérlõ beállítása
   nvicstruct.NVIC_IRQChannel = TIM6_DAC_IRQn;
   nvicstruct.NVIC_IRQChannelCmd = ENABLE;
   nvicstruct.NVIC_IRQChannelPreemptionPriority =0 ;  //0 prioritás, legfontosabb, mindenkit megszakíthat
   nvicstruct.NVIC_IRQChannelSubPriority = 0;
   NVIC_Init(&nvicstruct);


 //Timer7 periféria órajelének engedélyezése, Timer7->wait_ms függvény idõzítése,1ms=>1kHz
          RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7,ENABLE);

 //Timer7 beállítása  84MHz=>1kHz
          TIM_TimeBaseStructInit(&timstruct);
          timstruct.TIM_Period = 839;  //Periódus
          timstruct.TIM_Prescaler =99;  //Elõosztó
          TIM_TimeBaseInit(TIM7, &timstruct);
          TIM_Cmd(TIM7,DISABLE);              //kikapcsolva, csak a késleltõ függvény(wait_ms) kapcsolja be
          TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

 //IT vezérlõ beállítása
   nvicstruct.NVIC_IRQChannel = TIM7_IRQn;
   nvicstruct.NVIC_IRQChannelCmd = ENABLE;
   nvicstruct.NVIC_IRQChannelPreemptionPriority =1;  //kevésbé fontos, csak a pergésmentesítéshez kell,
   nvicstruct.NVIC_IRQChannelSubPriority =0;        //megszakíthatja a kijelzõ vezérléséért felelõs timer4, és a számláló timer6
   NVIC_Init(&nvicstruct);



   //Timer12 periféria órajelének engedélyezése, visszaszámláló üzemmód, másodperc alapú->1Hz
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12,ENABLE);

   //Timer12 beállítása  84MHz=>1Hz a visszaszámláló alapegysége másodperc
            TIM_TimeBaseStructInit(&timstruct);
            timstruct.TIM_Period = 8399;  //Periódus
            timstruct.TIM_Prescaler =9999;  //Elõosztó
            TIM_TimeBaseInit(TIM12, &timstruct);
            TIM_Cmd(TIM12,DISABLE);                      //kezdetben a számlálók állnak, nem fut a visszaszámlálás, timer12 áll
            TIM_ITConfig(TIM12, TIM_IT_Update, ENABLE);

   //IT vezérlõ beállítása
     nvicstruct.NVIC_IRQChannel = TIM8_BRK_TIM12_IRQn;
     nvicstruct.NVIC_IRQChannelCmd = ENABLE;
     nvicstruct.NVIC_IRQChannelPreemptionPriority =0;    //0 prioritás, legfontosabb
     nvicstruct.NVIC_IRQChannelSubPriority =0;
     NVIC_Init(&nvicstruct);



/*
 * EXTI és NVIC vezérlõ beállítása
*/

//EXTI-port-Pin összerendelés
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);


//EXTI beállítása  gomb0
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD,EXTI_PinSource0);
  extistruct.EXTI_Line = EXTI_Line0;
  extistruct.EXTI_LineCmd = ENABLE;
  extistruct.EXTI_Mode = EXTI_Mode_Interrupt;
  extistruct.EXTI_Trigger = EXTI_Trigger_Falling;       //lefutó élre szakít meg(földre húz a gomb,1->0)
  EXTI_Init(&extistruct);

//IT vezérlõ beállítása
   nvicstruct.NVIC_IRQChannel = EXTI0_IRQn;
   nvicstruct.NVIC_IRQChannelCmd = ENABLE;
   nvicstruct.NVIC_IRQChannelPreemptionPriority = 2;   //gombok nem olyan fontosak, timerek megszakíthatják õket,
   nvicstruct.NVIC_IRQChannelSubPriority = 0;          //idõbeli nagyságrend a megszakításkezeléshez:néhány száz ms.
   NVIC_Init(&nvicstruct);                             //Ez sokkal több, mint a timerek idõzítései.



 //---------------------------------------------------------------------------------------
   //EXTI beállítása gomb1
     SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD,EXTI_PinSource1);
     extistruct.EXTI_Line = EXTI_Line1;
     extistruct.EXTI_LineCmd = ENABLE;
     extistruct.EXTI_Mode = EXTI_Mode_Interrupt;
     extistruct.EXTI_Trigger = EXTI_Trigger_Falling;
     EXTI_Init(&extistruct);

   //IT vezérlõ beállítása

      nvicstruct.NVIC_IRQChannel = EXTI1_IRQn;
      nvicstruct.NVIC_IRQChannelCmd = ENABLE;
      nvicstruct.NVIC_IRQChannelPreemptionPriority = 2;
      nvicstruct.NVIC_IRQChannelSubPriority = 1;            //gomb0 jut érvényre elõször, ha egyszerre kértek megszakítást
      NVIC_Init(&nvicstruct);  	 	  	  	  	  	  	  	//(a többi gomb subpriority-je mindig eggyel nagyobb az elõzõnél)
      	  	  	  	  	  	  	  	  	  	  	  	  	    //Tehát a gomb3 hajtódik végre utoljára, ha egyszerre érkezett az összes megszakítás.


//--------------------------------------------------------------------------------------------

      //EXTI beállítása gomb2
        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD,EXTI_PinSource2);
        extistruct.EXTI_Line = EXTI_Line2;
        extistruct.EXTI_LineCmd = ENABLE;
        extistruct.EXTI_Mode = EXTI_Mode_Interrupt;
        extistruct.EXTI_Trigger = EXTI_Trigger_Falling;
        EXTI_Init(&extistruct);

      //IT vezérlõ beállítása

         nvicstruct.NVIC_IRQChannel = EXTI2_IRQn;
         nvicstruct.NVIC_IRQChannelCmd = ENABLE;
         nvicstruct.NVIC_IRQChannelPreemptionPriority = 2;
         nvicstruct.NVIC_IRQChannelSubPriority = 2;
         NVIC_Init(&nvicstruct);

//-----------------------------------------------------------------------------------------

         //EXTI beállítása gomb3
           SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD,EXTI_PinSource3);
           extistruct.EXTI_Line = EXTI_Line3;
           extistruct.EXTI_LineCmd = ENABLE;
           extistruct.EXTI_Mode = EXTI_Mode_Interrupt;
           extistruct.EXTI_Trigger = EXTI_Trigger_Falling;
           EXTI_Init(&extistruct);

         //IT vezérlõ beállítása

            nvicstruct.NVIC_IRQChannel = EXTI3_IRQn;
            nvicstruct.NVIC_IRQChannelCmd = ENABLE;
            nvicstruct.NVIC_IRQChannelPreemptionPriority = 2;
            nvicstruct.NVIC_IRQChannelSubPriority = 3;
            NVIC_Init(&nvicstruct);



 GPIO_SetBits(GPIOD,GPIO_Pin_8|GPIO_Pin_11);  //ledek kezdeti beállítása:1. és 4. zöld led ég(jelentés: stopper funkció, áll a stopper


  while (1)  //végtelen ciklusban feladatvégrahajtás
  {
	i++;


	if(setting){           //visszaszámláló üzemmód, az egyes digitek értékbeállítása
		 uint32_t k;       //jelenlegi idõ váltzozóját tároljuk benne, hogy az éppen beállítandó digitet kijelezzük
		 uint8_t j;       //for ciklus segédváltozója
		 k=counter_b;
		 for(j=0;j<5-sel;j++){   //a kijelzett idõ megfelelõ digitjének kiválasztása sel segítségével
		  k=k/10;
		 }
		 k%=10;         //k 0 és 1 között lévõ szám lesz
		 if(sel==1||sel==3) DisplayDigit(sel,k,1);   //van tizedespont(óra és perc után,2. és 4.kijelzõ)
	     else DisplayDigit(sel,k,0);                //nincs tizedespont
	}                                              //ebben az üzemmódban, mindig csak egy digitet jelenítünk meg: sel értéke gombnyomásra változik
	else{
		if(parttime){            //azt vizsgáljuk, kell-e 2 másodpercig részidõt mutatni(kifagyasztás)
				wait_ms(2000);   //ha igen, 2 másodpercig várunk
				parttime=DISABLE;  //utána letiltjuk a parttime változót, jelezve, hogy futhat tovább az óra
				GPIO_ResetBits(GPIOD,GPIO_Pin_9);  //a 3. zöld ledet lekapcsoljuk, jelezve, hogy véget ért a kifagyasztás (a 3. zöld led jelzi, ha éppen ki van fagyasztva az idõ)
		}
	}



  }
}
