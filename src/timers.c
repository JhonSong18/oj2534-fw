/* Common code for timer resources */
//XXX todo : (auto)calculate prescaler values
#include <stddef.h>

#include <stm32f0xx.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_rcc.h>
#include "timers.h"

#include "iso.h"	//iso tx worker

//partial timer inits :  enable periph clocks + set up prescalers
void timers_init(void) {
	TIM_TimeBaseInitTypeDef tbi;

	/* frclock, complete init + enable*/
	NVIC_DisableIRQ(IRQN_FRCLOCK);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	tbi.TIM_ClockDivision = TIM_CKD_DIV1;	//XXX a calcular
	tbi.TIM_CounterMode = TIM_CounterMode_Up;
	tbi.TIM_Period = -1;
	tbi.TIM_Prescaler = 0;	//XXX a calcular
	//tbi.TIM_RepetitionCounter	//irrelevant on TIM2
	TIM_TimeBaseInit(FRCLOCK_TMR, &tbi);
	FRCLOCK_TMR->CNT = 0;
	TIM_Cmd(FRCLOCK_TMR, ENABLE);

	/* TXworker */
	NVIC_DisableIRQ(IRQN_TXWORK);
	RCC_APB1PeriphClockCmd(TXWORK_APBC, ENABLE);
	tbi.TIM_ClockDivision = TIM_CKD_DIV1;	//XXX calc
	tbi.TIM_CounterMode = TIM_CounterMode_Up;
	tbi.TIM_Period = -1;
	tbi.TIM_Prescaler = 0;	//XXX a calcular
	//tbi.TIM_RepetitionCounter	//irrelevant on TIM3
	TIM_TimeBaseInit(TXWORK_TMR, &tbi);
	TXWORK_TMR->CNT = 0;

	/* PMSG */
	NVIC_DisableIRQ(IRQN_PMSG);
	RCC_APB2PeriphClockCmd(PMSG_APBC, ENABLE);
	tbi.TIM_ClockDivision = TIM_CKD_DIV1;	//XXX calc
	tbi.TIM_CounterMode = TIM_CounterMode_Down;
	tbi.TIM_Period = -1;
	tbi.TIM_Prescaler = 0;	//XXX a calcular
	tbi.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(PMSG_TMR, &tbi);
	TXWORK_TMR->CNT = 0;
	return;
}

//Interrupt handler for tx worker interrupts
void TXWORK_TMR_IRQH(void) {
	if (TXWORK_TMR->SR & ISO_TMR_CCIF) {
		TIM_ITConfig(TXWORK_TMR, ISO_TMR_CCIF, DISABLE);
		TIM_ClearFlag(TXWORK_TMR, ISO_TMR_CCIF);
		isotx_work();
	} else {
		DBGM("bad TXW int", TXWORK_TMR->SR);
	}
	return;

}

// set specified CCRx to match in (ms) millisecs.
// Caution: TIM_ITconfig does an unlocked R-M-W on DIER, so this should only be called from the TMR ISR !
void txwork_setint(u16 ms, volatile u32 * CCR) {
	u32 now;
	assert((ms > 0) && (CCR != NULL));

	now = TXWORK_TMR->CNT;

	TIM_ClearFlag(TXWORK_TMR, ISO_TMR_CCIF);
	*CCR = now + ms;

	TIM_ITConfig(TXWORK_TMR, ISO_TMR_CCIF, ENABLE);
	return;
}

