#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

#include "stm32f1xx.h"

void delay(uint32_t ms){
	for(uint32_t i = 0; i < ms; i++){
		SysTick->LOAD = 72000 - 1;
		SysTick->VAL = 0;

		SysTick->CTRL = (1 << 0) | (1 << 2);

		while((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
		SysTick->CTRL = 0;
	}
}

int main(void){
	// Configurando o systick (será nosso delay)
	SysTick->CTRL = 0;

	// Configurando o gpio
	// primeiro vamos setar o bit 3 do registrador APB2 enable register
	// para ligar o clock da parte b do gpio
	RCC->APB2ENR |= (1 << 3);

	// 9.2 GPIO Registers
	// 9.2.2 Port Configuration Register High (GPIOx_CRH)
	// Vamos configurar o pino PB11 como saída, é nele que vamos
	// conectar o led laranja
	GPIOB->CRH &= 0xFFFF0FFF;
	GPIOB->CRH |= 0x0000B000;

	// 9.2 GPIO Registers
	// 9.2.1 Port Configuration Register Low (GPIOx_CRL)
	// Vamos também configurar o pino PB5 para conectarmos o led
	// vermelho
	// Vamos configurar os pinos 23:20
	GPIOB->CRL &= 0xFF0FFFFF;
	GPIOB->CRL |= 0x00BFFFFF;

	// Vamos setar o bit 0 para habilitar o clock do Alternate Function
	RCC->APB2ENR |= (1 << 0);

	// 8.3.8 APB1 peripheral clock enable register (RCC_APB1ENR)
	// Habilitar os clocks dos timers 2 e 3
	RCC->APB1ENR |= (1 << 0) | (1 << 1);

	// Habilitar a interrupção
	TIM2->DIER |= (1 << 0);
	NVIC_EnableIRQ(TIM4_IRQn);

	// 9.3.7 Timer Alternate Function Remmapping
	// Table 45.TIM2 alternate function remapping
	// Use REMAP function to use PWM4 from TIM2 in B11
	// 9.4.2 AF remap and debug I/O configuration register (AFIO_MAPR)
	AFIO->MAPR |= (3 << 8);

	// Table 44. TIM3 alternate function remapping
	// No caso do timer3 vamos fazer um partial remap porque o encapsulamento
	// da blue pill é o LQFP48 e os pinos PC6, PC7, PC8 e PC9 não
	// estão disponíveis físicamente.
	// 9.4.2 AF remap and debug I/O configuration register (AFIO_MAPR)
	AFIO->MAPR |= (2 << 10);

	// 8.3.8 APB1 peripheral clock enable register (RCC_APB1ENR)
	// Habilitar o clock do timer2 e do timer3
	RCC->APB1ENR |= (1 << 0) | (1 << 1);

	// 15.4 TIMx registers
	// 15.4.11 TIMx prescaler (TIMx_PSC)
	// Estamos configurando o prescaler como 0 na intenção
	// de não dividirmos a frequência do clock.
	TIM2->PSC = 0;
	TIM3->PSC = 0;

	// 15.4.12 TIMx auto-reload register (TIMx_ARR)
	// É o valor máximo da contagem (1, 2, 3, ... 100)
	TIM2->ARR = 100;
	TIM3->ARR = 100;

	// 15.4.8 TIMx capture/compare mode register 2 (TIMx_CCMR2)
	// No caso do timer2 estamos setando os bits de OC4M para 110
	// com a intenção de colocarmos o canal 4 (que está conectado ao pino
	// PB11) no modo PWM mode 1.
	TIM2->CCMR2 |= (4 << 12) | (2 << 12);

	// 15.4.7 TIMx capture/compare mode register 1 (TIMx_CCMR1)
	// No timer3 fazemos semelhante, a diferença que vamos configurar os
	// bits OC2M que correspondem ao canal 2 que está ligado ao pino PB5
	TIM3->CCMR1 |= (4 << 12) | (2 << 12);

	// 15.4.9 TIMx capture/compare enable register (TIMx_CCER)
	// Habilitando a saída no canal 4
	TIM2->CCER |= (1 << 12);
	TIM3->CCER |= (1 << 4);

	// 15.4.1 TIMx control register 1 (TIMx_CR1)
	// Habilitar a contagem do timer2 e do timer3
	TIM2->CR1 |= (1 << 0);
	TIM3->CR1 |= (1 << 0);

	while(1){
		for(volatile int i = 0; i <= 100; i++){
			// 15.4.16 TIMx capture/compare register 4 (TIMx_CCR4)
			// Controlando o duty-cycle do canal 4 do timer2
			TIM2->CCR4 = i;
			// 15.4.14 TIMx capture/compare register 3 (TIMx_CCR2)
			// Controlando o duty-cycle do canal 2 do timer3
			TIM3->CCR2 = 100 - i;
			//for(volatile int j = 0; j < 10000; i++);
			delay(1);
		}
		for(volatile int i = 100; i >= 0; i--){
			// 15.4.16 TIMx capture/compare register 4 (TIMx_CCR4)
			// Controlando o duty-cycle do canal 4 do timer2
			TIM2->CCR4 = i;
			// 15.4.14 TIMx capture/compare register 3 (TIMx_CCR2)
			// Controlando o duty-cycle do canal 2 do timer3
			TIM3->CCR2 = 100 - i;
			//for(volatile int j = 0; j < 10000; i++);
			delay(1);
		}
	}
}
