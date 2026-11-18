#include "stm32f1xx.h"

void ADC1_2_IRQHandler(void){
	volatile int adc_data;
	if(ADC1->SR & ADC_SR_EOC){
		adc_data = ADC1->DR;

		// Definir o brilho de cada um dos 3 leds
		volatile int led1_limit = 910;
		volatile int led2_limit = 1820;
		volatile int led3_limit = 2730;

		if(adc_data < led1_limit){
			/*led 1 (PB3)*/
			TIM2->CCR2 = (adc_data / led1_limit) * 99;
			/*led 2 (PB4)*/
			TIM3->CCR1 = 0;
			/*led 3 (PB8)*/
			TIM4->CCR3 = 0;
		}
		else{
			if((adc_data > led1_limit) && (adc_data < led2_limit)){
				/*led 1 (PB3)*/
				TIM2->CCR2 = 99;
				/*led 2 (PB4)*/
				TIM3->CCR1 = (adc_data / led2_limit) * 99;
				/*led 3 (PB8)*/
				TIM4->CCR3 = 0;
			}
			else{
				if((adc_data >= led2_limit) && (adc_data < led3_limit)){
					/*led 1 (PB3)*/
					TIM2->CCR2 = 99;
					/*led 2 (PB4)*/
					TIM3->CCR1 = 99;
					/*led 3 (PB8)*/
					TIM4->CCR3 = (adc_data / led3_limit) * 99;
				}
			}
		}
	}
}

int main(void){
	// Primeiro vamos habilitar o clock do GPIOA e do GPIOB
	// 8.3 RCC Registers
	// 8.3.7 APB2 peripheral clock enable register (RCC_APB2ENR)
	RCC->APB2ENR |= (1 << 2); // GPIOA
	RCC->APB2ENR |= (1 << 3); // GPIOB

	// Habilitar o clock do alternate function
	// 8.3.7 APB2 peripheral clock enable register (RCC_APB2ENR)
	// Vamos setar o bit 0 para habilitar o clock do Alternate Function
	RCC->APB2ENR |= (1 << 0);

	// Habilitar o clock do ADC1
	// 8.3.7 APB2 peripheral clock enable register (RCC_APB2ENR)
	RCC->APB2ENR |= (1 << 9); // Bit ADC1_EN

	// Habilitar o clock dos timers 2, 3 e 4
	RCC->APB1ENR |= (1 << 0) /*timer2*/ | (1 << 1) /*timer3*/ | (1 << 2) /*timer4*/;

	// Agora vamos configurar o pino PA1 como entrada analógica
	// 9.2 GPIO Registers
	// 9.2.1 Port configuration register low (GPIOx_CRL) (x=A..G)
	GPIOA->CRL &= 0xFFFFFF0F;
	GPIOA->CRL |= 0x00000000; // CFN[1:0] = 00 e MODE[1:0]=00

	// Também vamos configurar os pinos PB8, PB3 e PB4 como saídas (Os leds)
	// 9.2 GPIO Registers
	// 9.2.1 Port configuration register low (GPIOx_CRL) (x=A..G)
	// PB4 (Timer 3 canal 1)
	GPIOB->CRL &= 0xFFF0FFFF;
	GPIOB->CRL |= 0x000B0000;
	// PB3 (Timer 2 canal 2)
	GPIOB->CRL &= 0xFFFF0FFF;
	GPIOB->CRL |= 0x0000B000;
	// 9.2.2 Port configuration register high (GPIOx_CRH) (x=A..G)
	// PB8 (Timer 4 canal 3)
	GPIOB->CRH &= 0xFFFFFFF0;
	GPIOB->CRH |= 0x0000000B;

	// Configurar o alternate function (Fazer Remap)
	// 9.4 AFIO Registers
	// 9.3.7 Timer alternate function remapping
	// Table 43. TIM4 alternate function remapping
	// Segundo ela não precisamos remapear, pois o pino PB8 já está
	// conectado ao canal 3 do timer 4.
	AFIO->MAPR |= (0 << 12);

	// Como PB3 e PB4 são reservados ao JTAG precisamos adicionar
	AFIO->MAPR |= (2 << 24);

	// Table 44. TIM3 alternate function remapping
	// Queremos conectar o pino PB4 ao timer3 canal 1 então vamos precisar
	// remapear. No caso basta um remap parcial. Escreveremos
	// TIM3_REMAP[1:0] = “10” (partial remap)
	// 9.4.2 AF remap and debug I/O configuration register (AFIO_MAPR)
	AFIO->MAPR |= (2 << 10);

	// Table 45. TIM2 alternate function remapping
	// Queremos conectar o pino PB3 ao timer2 canal 2 então vamos precisar
	// remapear. De novo um remap parcial. Escreveremos
	// TIM2_REMAP[1:0] = “01” (partial remap)
	// 9.4.2 AF remap and debug I/O configuration register (AFIO_MAPR)
	AFIO->MAPR |= (1 << 8);

	// Configurar qual evento externo irá disparar o inicio de uma
	// conversão de um grupo regular
	// 11.12 ADC Registers
	// 11.12.3 ADC control register 2 (ADC_CR2)
	ADC1->CR2 |= (7 << 17); // Bits 19:17 EXTSEL[2:0] = 111

	// Ativar o modo contíno
	// Assim depois de terminar uma conversão, o ADC já inicia outra
	ADC1->CR2 |= (1 << 1);

	// Escolher o número de canais que vamos ler e converter e
	// a ordem das leituras
	// 11.12.9 ADC regular sequence register 1 (ADC_SQR1)
	// Definir o número de canais que serão amostrados (no caso, 1)
	ADC1->SQR1 = (0 << 20);

	// 11.12.11 ADC regular sequence register 3 (ADC_SQR3)
	// Aqui estamos dizendo que o primeiro canal a ser lido é o canal 1
	ADC1->SQR3 = 1;

	// Definir o tempo de amostragem do canal (o tempo está em função
	// do número de ciclos)
	// 11.12.5 ADC sample time register 2 (ADC_SMPR2)
	ADC1->SMPR2 |= 0x00000007; // tempo de amostragem do canal 0 definido como 239.5 ciclos

	// Ligar o Conversor (Ligar a alimentação no caso)
	// 11.12.3 ADC control register 2 (ADC_CR2)
	ADC1->CR2 |= (1 << 0);

	// 15 General-purpose timers (TIM2 to TIM5)
	// 15.4.11 TIMx prescaler (TIMx_PSC)
	TIM2->PSC = 0;
	TIM3->PSC = 0;
	TIM4->PSC = 0;

	// Vamos definir como 100 a resolução de todos os PWM
	// (facilita as comparações com %)
	// 15.4.12 TIMx auto-reload register (TIMx_ARR)
	TIM2->ARR = 100;
	TIM3->ARR = 100;
	TIM4->ARR = 100;

	// Habilitar o modo PWM nos respectivos canais ligados aos pinos fisicos
	// PB4, PB3 e PB9
	// 15.4.7 TIMx capture/compare mode register 1 (TIMx_CCMR1)
	// PB4 (Timer 3 canal 1)
	TIM3->CCMR1 |= (2 << 4) | (4 << 4);//0x0060; // Bits 6:4 OC1M = 110 PWM Mode 1
	// PB3 (Timer 2 canal 2)
	TIM2->CCMR1 |= (2 << 12) | (4 << 12);//0x6000; // Bits 14:12 OC2M = 110 PWM Mode 1
	// PB9 (Timer 4 canal 3)
	TIM4->CCMR2 |= (2 << 4) | (4 << 4);//0x6000; // Bits 6:4 OC3M = 110 PWM Mode 1

	// Habitar canais
	// 15.4.9 TIMx capture/compare enable register (TIMx_CCER)
	TIM3->CCER |= (1 << 0); // Bit CC1E = 1
	TIM2->CCER |= (1 << 4); // Bit CC2E = 1
	TIM4->CCER |= (1 << 8); // Bit CC3E = 1

	// 15.4.6 TIMx event generation register (TIMx_EGR)
	TIM2->EGR |= (1 << 0);
	TIM3->EGR |= (1 << 0);
	TIM4->EGR |= (1 << 0);

	// Iniciar a contagem dos timers
	// 15.4.1 TIMx control register 1 (TIMx_CR1)
	TIM2->CR1 |= (1 << 0); // Bit CEN = 1 timer começa a contar
	TIM3->CR1 |= (1 << 0); // Bit CEN = 1 timer começa a contar
	TIM4->CR1 |= (1 << 0); // Bit CEN = 1 timer começa a contar

	// 11.12.3 ADC control register 2 (ADC_CR2)
	ADC1->CR2 |= (1 << 0);
	ADC1->CR2 |= (1 << 22);

	// Habilitar a interrupção do ADC
	// 11.12.2 ADC control register 1 (ADC_CR1)
	ADC1->CR1 |= (1 << 5); // Bit 5 EOCIE: Interrupt enable for EOC
	NVIC_EnableIRQ(ADC1_2_IRQn);

	while(1){
		/*
		for(volatile int i = 0; i < 100; i++){

			TIM2->CCR2 = i;
			TIM3->CCR1 = i;
			TIM4->CCR4 = i;
			for(int i = 0; i < 10000; i++){}
		}

		for(volatile int i = 99; i > 0; i--){
			TIM2->CCR2 = i;
			TIM3->CCR1 = i;
			TIM4->CCR4 = i;
			for(int i = 0; i < 10000; i++){}
		}
		*/
	}
}
