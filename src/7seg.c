/**
*****************************************************************************
**Stopper
**
*****************************************************************************
*/



#include "7seg.h"

/*
 * SegmentTable tömb(16 elemû: 0,1,...,F): egy adott kilejzendõ számhoz milyen 8 bites adat tartozik, amit a kijelzõre kell küldeni, hogy az egyes ledek megfelelõen rajzolák ki a számot.
 * DisplayDigit függvény
 */
const uint8_t SegmentTable[16] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};


void DisplayDigit( uint8_t digit, uint8_t value, uint8_t dp  )  //digit: melyik kijelzõre írjuk az adatot, értéke:0...5)
{                                                              //value:0,1,...,F kijelzendõ szám; dp:0->nincs tizedespont 1->van
	uint16_t portval;

	uint16_t select=digit<<13;           //shiftelés  7sel0,..,7sel2: PB13,..PB15; a 16 bites port felsõ 3 bitjét állítjuk, ezért select értéke 13-mal shiftelve van fölfele select: abcxxxxxxxxxxxxx a,b,c az értékes bit
	uint16_t data=SegmentTable[value];   //kiválasztjuk a tömbbõl a kiküldendõ adatot, a kijelzendõ szám(value) alapján
		     data<<=8;                           //shiftelés 0xab00 DB0..DB7->PE8..Pe15: E port felsõ 8 bitje, ezért shiftejük 8-cal az értéket

	 GPIO_ResetBits(GPIOB,GPIO_Pin_8);       //7seg_clk lefutó él (D flipflophoz)

	 portval=GPIO_ReadOutputData(GPIOB);     //kiolvassuk a B port jelenlegi értékét (select itt kerül a port lábaira
	 portval&=0x1FFF;                        //port többi bitjét ne piszkáljuk, ezért a felsõ 3 bitet(13,14,15) kinulázzuk, a többi értékénél marad
	 portval|=select;                        //select értékét átmentjük a port felsõ 3 bitjébe (select alsó 13 bitje 0)
	 GPIO_Write(GPIOB,portval);              //kiküldjük az adatot a B portra (7sel0,7sel1,7sel2 értéket kap, amik demuxon keresztül kiválasztanak egy db kijelzõt, amit meghajtunk)

	 portval=GPIO_ReadOutputData(GPIOE);    //hasonlóan az E portnál is
	 portval&=0x00FF;                       //alsó 8 bitet változatlanul kell visszaírni
	 portval|=data;                        //data értéke bekerül a port felsõ 8 bitjébe
	 GPIO_Write(GPIOE,portval);            //adatkiírás


	 if(dp) GPIO_SetBits(GPIOE,GPIO_Pin_15);   //ha kell tizedespont, állítjuk a megfelelõ lábat
	 else   GPIO_ResetBits(GPIOE,GPIO_Pin_15);

	 GPIO_SetBits(GPIOB,GPIO_Pin_8);              //7seg_clk felfutó él-> D-flipflop kimenetén megjelenik az adat
	 GPIO_ResetBits(GPIOE,GPIO_Pin_7);           //#7sen: engedélyezzük a demux-t és D-flipflop-ot (low active)

}
