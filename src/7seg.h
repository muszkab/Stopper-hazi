/**
*****************************************************************************
**Stopper
**
*****************************************************************************
*/

#ifndef _7SEG_H_
#define _7SEG_H_

#include "stm32f4xx.h"

extern const uint8_t SegmentTable[16];
void DisplayDigit( uint8_t digit, uint8_t value, uint8_t dp  );


#endif /* 7SEG_H_ */
