/**
*****************************************************************************
**Stopper
**
*****************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f4xx.h"


void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/*
 * A feladatban haszn�lt megszak�t�skezel� f�ggv�nyek
 */
void TIM4_IRQHandler(void);
void TIM6_DAC_IRQHandler(void);
void EXTI15_10_IRQHandler(void);
void TIM7_IRQHandler(void);
void TIM8_BRK_TIM12_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */
