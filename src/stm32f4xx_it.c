/**
*****************************************************************************
**Megszakítás kezelõ függvények
**
**
*****************************************************************************
*/

#include "stm32f4xx_it.h"
#include "7seg.h"
#include "misc.h"

/*
 * A külsõ globális változók
 */
extern uint32_t counter;    //stopper ideje
extern uint32_t counter_b;  //visszaszámláló ideje
extern short int sel;       //sel választja ki a kijelzendõ digitet
extern FunctionalState parttime;  //kifagyasztás
extern FunctionalState setting;   //digitek egyenkénti beállítása

//Saját változók
uint32_t counter_part;              //részidõ értéke, ez mindig egy konkrét szám, nem növeli timer
FunctionalState enabletimer=DISABLE;   //azt jelzi, hogy fut vagy áll az óra (mindkét üzemmódban) 4.zöld led világít, ha áll az óra  reset->áll az óra
FunctionalState mode=ENABLE;          //mode válaszja ki az üzemmódot: mode=ENABLE->stopper  reset->stopper funkció



/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/



/*
 * Timer4 megszakításkezelõ, ami a counter globális változóban tárolt értéket
 * idõmultiplexáltan megjeleníti a hétszegmenses kijelzõkön. Egy-egy magszakítási ciklus
 * alkalmával egy-egy kijelzõre kell kiírni a hozzá tartozó számértéket.
 */



void TIM4_IRQHandler(void)
{
	/* Lekérdezzük a megszakítás okát – több is lehet */
			 if(TIM_GetITStatus(TIM4, TIM_IT_Update))
			 {  uint32_t i;        //egy idõ értéket fog kapni
			   uint8_t j;          //for segédváltozója
			   if(parttime) i=counter_part;       //ha részidõ üzemmód van, a kifagyasztott idõt mutassa
			   else {                            //egyébként a futó idõt mutassa
				   if(mode) i=counter;           //stopper üzemmód(mode=ENABLE)
				   else i=counter_b;             //visszaszámláló
			   }


			 	 for(j=0;j<5-sel;j++){           //sel változóval meghatározzuk i megfelelõ számjegyét(0...5), amit egy db kijelzõn jelzünk majd ki
			 		 i=i/10;                     //addig osztjuk le 10-zel, amíg az egyes helyiértéken lesz a kívánt számjegy
			 	 }
                i%=10;                           //10-es maradék-> i értéke:0...9
                if(sel==1||sel==3) DisplayDigit(sel,i,1); //ha a 1. és 3. kijelzõnél tartunk kell tizedespont(perc.másodperc.századmásodperc)
                else DisplayDigit(sel,i,0);               //Displaydigit jeleníti meg az adatot(i), a kiválasztott kijelzõre(sel); 7seg.c-ben van megírva

                if(sel==5) sel=0;                        //utolsó(5.) kijelzõ után a 0. jön
                 else sel++;                             //idõmultiplexelt kijelzés, sel értéke timer4 frekvenciájával(420Hz) növekszik, így váltogatva az egyes kijelzõket, de az emberi szem ezt nem tudja követni, minden kijelzõn lát számot

				 /* Töröljük a megszakítás jelzõ flag-et */
					 TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
			 }



}


/*
 * Timer6 megszakításkezelõje, ami a counter változó növelését és 0..595999
 * értékek között tartását végzi
 */
void TIM6_DAC_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM6, TIM_IT_Update)){
		if(counter==595999) counter=0;                                 //határérték elérése, nullázás
		else { if((counter%10000)==5999) counter=counter-5999+10000;    //percváltás xx.59.99->xy.00.00  y=x+1
		       else counter++;}                                       //sima századmásodperces lépés
	TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
	}

}

//Timer7
void TIM7_IRQHandler(void)  //ms késleltetés, wait_ms függvényhez(misc.c) kell,tim7_i változót ms-onként növeli
{
	if(TIM_GetITStatus(TIM7,TIM_IT_Update)){
		tim7_i++;
		TIM_ClearITPendingBit(TIM7,TIM_IT_Update);
	}
}


/*Timer12(timer8 és timer12 megszakításkezelõje közös, de mi csak timer12-t használjuk
 * timer12: visszaszámláló idejét csökkenti másodperces alappal f=1Hz
*/
void TIM8_BRK_TIM12_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM12, TIM_IT_Update)){
		if(counter_b==0){                                               //határérték elérése, számlálás leáálítása
		  TIM_Cmd(TIM12,DISABLE);
		  GPIO_SetBits(GPIOD,GPIO_Pin_8);                               //4. zöld led jelzi, ha áll az óra
		  enabletimer=DISABLE;                                          //enabletimer tárolja, hogy megállt az óra
		}
		else { if((counter_b%10000)==0) counter_b=counter_b+5959-10000;    //óraváltás:xy.00.00.->x(y-1).59.59
		       else {
		    	   if((counter_b%100)==0) counter_b=counter_b-100+59;      //percváltás:xy.zw.00->xy.z(w-1).59
		    	   else counter_b--;}                                  //másodperces lefele lépés
		       }
	TIM_ClearITPendingBit(TIM12, TIM_IT_Update);
	}

}


/*
 * BTN0..3 gombokhoz tartozó megszakításkezelõk
 */


//panelen az 1. gomb
void EXTI0_IRQHandler(void)            //gombok megszakításkezelése: 0gomb-> üzemmód választás
{
	if(EXTI_GetITStatus(EXTI_Line0)){
		if(!setting){                   //ha nincs digit beállítás, csak akkor szabad üzemmódot váltani
		mode=!mode;                     //funkkció váltás: stopper<->visszaszámlálás (mode változóban tároljuk, értéke:ENABLE vagy DISABLE)
		enabletimer=DISABLE;            //futás közben is lehet funkciót váltani, de ekkor le kell állítani a számlálásokat
		GPIO_SetBits(GPIOD,GPIO_Pin_8); //4. zöld led világít, jelezve h áll az óra
		if(mode){                       //ha stopper üzemmódba léptünk
			TIM_Cmd(TIM12,DISABLE);     //visszaszámláló timer-ét leállítjuk
			GPIO_SetBits(GPIOD,GPIO_Pin_11); //1. zöld ledet fölkapcsoljuk, ez jelzi a stopper funkciót
			GPIO_ResetBits(GPIOD,GPIO_Pin_10); //2. piros ledet lekapcsoljuk, nincs már visszaszámlálás
		}
		else {                            //ha visszaszámláló üzemódba léptünk
			TIM_Cmd(TIM6,DISABLE);        //timer6 leállítása (stopperé)
			GPIO_SetBits(GPIOD,GPIO_Pin_10); //ledek beállítása: 2.piros ég, 1.zöld nem
		    GPIO_ResetBits(GPIOD,GPIO_Pin_11);
		}

		}
        wait_ms(300);                                  //pergésmentesítés 300ms késleltetetéssel
        EXTI_ClearITPendingBit(EXTI_Line0);


	}
}
//panelen a 3. gomb
void EXTI1_IRQHandler(void)            //gombok megszakításkezelése stopper:2 másodperces kifagyasztás
                                                        //visszaszámlálóban: kijelzõ kiválasztása beállításra
{
	if(EXTI_GetITStatus(EXTI_Line1)){
		if(mode){                      //stopper(mode=1)->kifagyasztás
		parttime=ENABLE;              //jelezzük h részidõ mutatás üzemmódban vagyunk
		counter_part=counter;         //elmentjük a pillanatnyi idõt, counter_part tárolja a kifagyasztott idõ értéket, de counter értéke fut tovább
		if(parttime) GPIO_SetBits(GPIOD,GPIO_Pin_9); //bekapcsoljuk a 3. zöld ledet, jelezve, hogy éppen a részidõt látjuk
		}

		if(!mode && !enabletimer){  //ha visszaszámláló üzemód és áll az óra(azért mert csak ekkor szabad az egyes digitek értékén változtatni)
			if(!setting){           //ha még nem léptünk be a beállításba, csak áll az idõ, és minden digit jelez
			 TIM_Cmd(TIM4,DISABLE); //timer4 leállítása-> nincs idõmultiplexelt kijelzés, innentõl csak egy digit fog látszódni egyszerre
			 setting=ENABLE;        //jelezzük h beállítás üzemódban vagyunk
             sel=-1;
			}
			if(setting){
				if(sel==5){        //ha az utolsó digitnél tartunk, kilépünk a beállításból, elinditjuk timer4-t, újra egyszerre látszódik az összes digit
					TIM_Cmd(TIM4,ENABLE);
				    setting=DISABLE;
				}
				else sel++;       //egy gombnyomásra ugorjon a következõ digitre, amit be lehet állítani
			}

		}

    wait_ms(300);                                  //pergésmentesítés 300ms késleltetetéssel
	EXTI_ClearITPendingBit(EXTI_Line1);
	}
}
//panelen a 2. gomb
void EXTI2_IRQHandler(void)            //gombok megszakításkezelése: start-stop funkció
{
	if(EXTI_GetITStatus(EXTI_Line2)){
		if(!setting){                    //beáálítás funkcióban nem szabad indítani
		enabletimer=!enabletimer;        //start<->stop váltás (enabletimer: ENABLE vagy DISABLE)
		if(mode) TIM_Cmd(TIM6, enabletimer); //ha stopper->timer6 indítása vagy leállítása
		else TIM_Cmd(TIM12,enabletimer);     //v.számláló->timer12
		if(!enabletimer) GPIO_SetBits(GPIOD,GPIO_Pin_8); //ha áll az óra, kigyullad a 4. zöld led
		else GPIO_ResetBits(GPIOD,GPIO_Pin_8);           //ha elindul, kialszik
		}

	    wait_ms(300);                                  //pergésmentesítés 300ms késleltetetéssel
		EXTI_ClearITPendingBit(EXTI_Line2);

	}

}

//panelen a 4. gomb
void EXTI3_IRQHandler(void)            //gombok megszakításkezelése   visszaszámláló üzemmódban,
                                       //ha beállítás van(setting), egy darab kijelzõ értékének megnövelése
{                                      // ha stopper, nullázzuk a kijelzõt
	if(EXTI_GetITStatus(EXTI_Line3)){
		if(setting){                 //beállítás üzemmód
			uint32_t i;
            uint8_t j;
	        i=counter_b;            //i: pillanatnyi leállított idõ, értéke:00.00.00->99.59.59
		    for(j=0;j<5-sel;j++){   //sel-lel kiválasztjuk a megfelelõ számjeget
				 i=i/10;
			 }
            i%=10;              //i értéke:0...9 vagy 0...5
            if(sel==2 || sel==4){   //ha a i a másodperc magasabb helyiértéke vagy a perc magasabb helyiértéke, csak 0 és 5 közötti értéket adhatunk neki
            	if(i==5){           //ha i=5, egy gombnyomás után az értéke 0 lesz(ezt le kell menteni counter_b-be is
            	            for(j=0;j<5-sel;j++){
            	            	i*=10;            //felszorozzuk a megfelelõ helyiértékre
            	            }
            	            counter_b-=i;        //levonjuk counter_b-bõl; így azt kaptuk, hogy 5 helyett 0 jelez ki a 3. vagy 5. kijelzõn
            	            }
            	else{           //ha i!=5 csak megnöveljük eggyel counter_b megfelelõ számjegyét
                	i=1;
                	for(j=0;j<5-sel;j++){
                		i*=10;
                	}
                	counter_b+=i;
                }
            }
            else{              //ha i értéke 0 és 9 között változhat
            if(i==9){         // ha i=9, 0 legyen az érték (counter_b-bõl levonjuk x*10*i-t(x:0..5)
            for(j=0;j<5-sel;j++){
            	i*=10;
            }
            counter_b-=i;
            }
            else{            //i!=9, a megfelelõ helyiértéket növeljük meg counter_b-ben
            	i=1;
            	for(j=0;j<5-sel;j++){
            		i*=10;
            	}
            	counter_b+=i;
            }
            }


		}
		if(mode) counter=0;         //stopper nullázása(mode=ENABLE)


	    wait_ms(300);                                  //pergésmentesítés 300ms késleltetetéssel
		EXTI_ClearITPendingBit(EXTI_Line3);
	}
}



