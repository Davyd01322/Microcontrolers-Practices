#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

#include "stm32f1xx.h"

static volatile int led_9 = 0;

void TIM2_IRQHandler(void){
	if(TIM2->SR & TIM_SR_UIF){
		if(led_9 == 0){
			// Setar o pino 9 (acender led)
			GPIOB->BSRR = (1 << 9);
			led_9 = 1;
		} else{
			// Apagar o pino 9 (desligar o led)
			GPIOB->BSRR = (1 << 25);
			led_9 = 0;
		}

		// Limpar a flag de interrupção
		TIM2->SR = 0;
	}
}

int main(void){
	// Setar o bit 3 para habilitar o clock do gpiob
	RCC->APB2ENR |= (1 << 3);
	// Habilitar o clock do RCC
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// Tornar os pinos PB8 e PB9 como saídas
	GPIOB->CRH &= 0xFFFFFF00;
	GPIOB->CRH |= 0x00000022;

	// Resetar os pinos pb8 e pb9 para um valor conhecido
	GPIOB->ODR &= ~((1 << 9) | (1 << 8));

	// Configurações do timer2
	TIM2->CR1 |= TIM_CR1_CEN;
	TIM2->PSC = 7200-1;
	TIM2->ARR = 1000-1;
	TIM2->EGR |= TIM_EGR_UG;
	TIM2->CNT = 0;

	// Habilitar a geração de interrupções
	TIM2->DIER |= TIM_DIER_UIE;

	// Habilitar a interrupção do timer no nvic
	NVIC_EnableIRQ(TIM2_IRQn);

	while(1){
		if(TIM2->CNT > 500){
			GPIOB->BSRR = (1 << 8);
			if(TIM2->CNT > 550){
				GPIOB->BSRR = (1 << 24);
			}
		}
	}
}

