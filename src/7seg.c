/**
*****************************************************************************
**Stopper
**
*****************************************************************************
*/



#include "7seg.h"

/*
 * SegmentTable t�mb(16 elem�: 0,1,...,F): egy adott kilejzend� sz�mhoz milyen 8 bites adat tartozik, amit a kijelz�re kell k�ldeni, hogy az egyes ledek megfelel�en rajzol�k ki a sz�mot.
 * DisplayDigit f�ggv�ny
 */
const uint8_t SegmentTable[16] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};


void DisplayDigit( uint8_t digit, uint8_t value, uint8_t dp  )  //digit: melyik kijelz�re �rjuk az adatot, �rt�ke:0...5)
{                                                              //value:0,1,...,F kijelzend� sz�m; dp:0->nincs tizedespont 1->van
	uint16_t portval;

	uint16_t select=digit<<13;           //shiftel�s  7sel0,..,7sel2: PB13,..PB15; a 16 bites port fels� 3 bitj�t �ll�tjuk, ez�rt select �rt�ke 13-mal shiftelve van f�lfele select: abcxxxxxxxxxxxxx a,b,c az �rt�kes bit
	uint16_t data=SegmentTable[value];   //kiv�lasztjuk a t�mbb�l a kik�ldend� adatot, a kijelzend� sz�m(value) alapj�n
		     data<<=8;                           //shiftel�s 0xab00 DB0..DB7->PE8..Pe15: E port fels� 8 bitje, ez�rt shiftej�k 8-cal az �rt�ket

	 GPIO_ResetBits(GPIOB,GPIO_Pin_8);       //7seg_clk lefut� �l (D flipflophoz)

	 portval=GPIO_ReadOutputData(GPIOB);     //kiolvassuk a B port jelenlegi �rt�k�t (select itt ker�l a port l�baira
	 portval&=0x1FFF;                        //port t�bbi bitj�t ne piszk�ljuk, ez�rt a fels� 3 bitet(13,14,15) kinul�zzuk, a t�bbi �rt�k�n�l marad
	 portval|=select;                        //select �rt�k�t �tmentj�k a port fels� 3 bitj�be (select als� 13 bitje 0)
	 GPIO_Write(GPIOB,portval);              //kik�ldj�k az adatot a B portra (7sel0,7sel1,7sel2 �rt�ket kap, amik demuxon kereszt�l kiv�lasztanak egy db kijelz�t, amit meghajtunk)

	 portval=GPIO_ReadOutputData(GPIOE);    //hasonl�an az E portn�l is
	 portval&=0x00FF;                       //als� 8 bitet v�ltozatlanul kell vissza�rni
	 portval|=data;                        //data �rt�ke beker�l a port fels� 8 bitj�be
	 GPIO_Write(GPIOE,portval);            //adatki�r�s


	 if(dp) GPIO_SetBits(GPIOE,GPIO_Pin_15);   //ha kell tizedespont, �ll�tjuk a megfelel� l�bat
	 else   GPIO_ResetBits(GPIOE,GPIO_Pin_15);

	 GPIO_SetBits(GPIOB,GPIO_Pin_8);              //7seg_clk felfut� �l-> D-flipflop kimenet�n megjelenik az adat
	 GPIO_ResetBits(GPIOE,GPIO_Pin_7);           //#7sen: enged�lyezz�k a demux-t �s D-flipflop-ot (low active)

}
