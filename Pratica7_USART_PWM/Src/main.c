#include <stdint.h>
#include "stm32f1xx.h"

#define CPU_CLK 	8000000
#define BaudRate 	115200

#define SYSTICK_LOAD (CPU_CLK/1000000U)
#define SYSTICK_DELAY_CALIB (SYSTICK_LOAD >> 1)

#define DELAY_US(us)\
	do{\
		uint32_t start = SysTick->VAL;\
		uint32_t ticks = (us*SYSTICK_LOAD) - SYSTICK_DELAY_CALIB;\
		while((start - SysTick->VAL) < ticks);\
	}while(0)

#define DELAY_MS(ms)\
	do{\
		for(uint32_t i = 0; i < ms; i++){\
			DELAY_US(1000);\
		}\
	}while(0)

// menu
uint8_t msg[] = "Qual led você deseja acender?\n\r";
uint8_t error_msg[] = "Comando invalido\n\r";
uint8_t debug_msg[] = "Entrou aqui\n\r";
uint8_t debug_msg_red[] = "Você selecionou o led vermelho com intensidade: \n\r";
uint8_t debug_msg_green[] = "Você selecionou o led verde com intensidade: \n\r";
uint8_t debug_msg_blue[] = "Você selecionou o led azul com intensidade: \n\r";

// Buffer Circular para recepção de dados
uint8_t buffer[5]; // 5 Bytes
int index = 0;

// Ponteiro para o envio dos caracteres
uint8_t *ptr;

// SysTick
uint32_t ticks = 2000000;

// Variáveis de controle
int read_or_write = 1; 			// 1: escrever, 0: ler/processar
int write_or_processing = 1;	// 1: ler, 0: processar

/*Rotina de interrupção do SysTick
void SysTick_Handler(void){
	//MAIN_SM = 1;
}*/

void uart_read(void){
	buffer[index] = USART1->DR;
	index += 1;

	if(index == 3){
		buffer[3] = '\n';
		buffer[4] = '\r';

		index = 0;

		// Buffer cheio vamos encerrar o recebimento de requições
		write_or_processing = 0;

		// Habilitar a interrupção de recepção de dados
		// Bit 5 RXNEIE: RXNE interrupt enable
		// 0: Interrupt is inhibited
		// 1: A USART interrupt is generated whenever ORE=1 or RXNE=1
		// in the USART_SR register
		USART1->CR1 &= ~(1 << 5);
	}
}

void uart_write(uint8_t *ch){
	// 27.6.1 Status register (USART_SR)
	// vamos verificar se o conteúdo anterior em DR já foi enviado para
	// não perdermos caracteres no processo.
	// Para fazermos isso vamos ler o bit 7 Transmit data register empty
	// para sabermos se o registrador TDR (do qual não temos acesso)
	// enviou seu conteúdo
	// 0: Data is not transferred to the shift register
	// 1: Data is transferred to the shift register
	while(!(USART1->SR & (1 << 7))){} // Bit 7 TXE: Transmit data register empty

	// Enviar o primeiro caractere
	USART1->DR = (*ch & 0xFF);

	// Verificar se o byte foi enviado
	// 27.6.1 Status register (USART_SR)
	// Bit 6 TC: Transmission complete
	// 0: Transmission is not complete
	// 1: Transmission is complete
	while(!(USART1->SR & (1 << 6))){}

	ptr = ch + 1;

	// Vamos habilitar temporariamente a interrupção para
	// "automatizar" a transmissão
	// Bit 7 TXEIE: TXE interrupt enable
	// 1: A USART interrupt is generated whenever TXE=1 in the
	// USART_SR register
	USART1->CR1 |= (1 << 7);
}

int string_to_int(uint8_t *ch){
	int result = 0;
	int local_index = 0;
	int sign = 1;

	// verificar se é um número negativo ou não
	if(ch[0] == '-'){
		sign = -1;
		local_index += 1;
	}

	while(ch[local_index] != '\n'){
		// checar se o caractere é número
		if(((ch[local_index] - '0') >= 0) && ((ch[local_index] - '0') <= 9)){
			result *= 10;
			result += ch[local_index] - '0';
		}
		else{
			return 0;
		}
		local_index += 1;
	}

	result *= sign;

	return result;
}

// Rotina de interrupção do USART
void USART1_IRQHandler(void){
	// 27.4 USART interrupts

	// Table 196. USART interrupt requests
	// 27.6. 1 Status register (USART_SR)
	// Bit 5 RXNE: Read data register not empty
	// 0: Data is not received
	// 1: Received data is ready to be read.
	if((USART1->SR & (1 << 5))){
		uart_read();
	}
	else{
		// Table 196. USART interrupt requests
		// Transmit data register empty(TXE): Enable Control Bit => TXEIE
		// 27.6.1 Status register (USART_SR)
		// Essa flag indica que o registrador Data Register está vazio
		// e aceita a escrita de novo byte
		if((USART1->SR & (1 << 7))){
			if(*ptr){
				// Estamos colocando o caractere dentro de DR e a USART
				// já inicia automaticamente a transmissão
				USART1->DR = (*ptr & 0xFF);

				// Verificar se o byte foi enviado
				// 27.6.1 Status register (USART_SR)
				// Bit 6 TC: Transmission complete
				// 0: Transmission is not complete
				// 1: Transmission is complete
				while(!(USART1->SR & (1 << 6))){}

				// próximo byte a ser enviado
				ptr++;
			}
			else{
				// 27.6.4 Control register 1 (USART_CR1)
				// Aqui vamos inibir a interrupção TXE limpando o bit 7
				// (Bit 7 TXEIE: TXE interrupt enable)
				USART1->CR1 &= ~(1 << 7);
			}
		}
	}
}

int main(void){
	// Primeiro vamos habilitar o clock do GPIOA e do GPIOB
	// 8.3 RCC Registers
	// 8.3.7 APB2 peripheral clock enable register (RCC_APB2ENR)
	RCC->APB2ENR |= (1 << 3); // GPIOB
	RCC->APB2ENR |= (1 << 2); // GPIOA

	// Habilitar o clock da USART
	// 8.3.7 APB2 peripheral clock enable register (RCC_APB2ENR)
	RCC->APB2ENR |= (1 << 14);

	// Habilitar o clock do alternate function
	// 8.3.7 APB2 peripheral clock enable register (RCC_APB2ENR)
	// Vamos setar o bit 0 para habilitar o clock do Alternate Function
	RCC->APB2ENR |= (1 << 0);

	// Habilitar o clock dos timers 2, 3 e 4
	RCC->APB1ENR |= (1 << 0) /*timer2*/ | (1 << 1) /*timer3*/ | (1 << 2) /*timer4*/;

	/* Pinos do USART */
	// Agora vamos configurar os pinos PA10 como entrada (RX1)
	// No caso uma entrada com pull-up/pull-down
	// 9.2 GPIO Registers
	// 9.2.2 Port configuration register low (GPIOx_CRH) (x=A..G)
	GPIOA->CRH &= 0xFFFFF0FF;
	GPIOA->CRH |= 0x00000400; // CFN[1:0] = 01 e MODE[1:0]=00

	// Agora vamos configurar os pinos PA9 como saida (TX1)
	// No caso uma saida alternate function push-pull com max speed 50MHz
	// 9.2 GPIO Registers
	// 9.2.2 Port configuration register low (GPIOx_CRH) (x=A..G)
	GPIOA->CRH &= 0xFFFFFF0F;
	GPIOA->CRH |= 0x000000B0; // CFN[1:0] = 10 e MODE[1:0]=11

	/* Pinos do PWM */
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

	/* Configuração do PWM */
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
	// PB4, PB3 e PB8
	// 15.4.7 TIMx capture/compare mode register 1 (TIMx_CCMR1)
	// PB4 (Timer 3 canal 1)
	TIM3->CCMR1 |= (2 << 4) | (4 << 4);//0x0060; // Bits 6:4 OC1M = 110 PWM Mode 1
	// PB3 (Timer 2 canal 2)
	TIM2->CCMR1 |= (2 << 12) | (4 << 12);//0x6000; // Bits 14:12 OC2M = 110 PWM Mode 1
	// PB8 (Timer 4 canal 3)
	TIM4->CCMR2 |= (2 << 4) | (4 << 4);//0x6000; // Bits 6:4 OC3M = 110 PWM Mode 1

	// Habilitar canais
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

	/* Configurar o SysTick */
	if((ticks - 1UL) <= 0xFFFFFFFFUL){
		SysTick->LOAD = (uint32_t)(ticks - 1UL);
		SysTick->VAL = 0UL;
		SysTick->CTRL |= (0 << 2) | (0 << 1) | (1 << 0);
	}

	/* Configurar o USART */
	// 27.6.4 Control register 1 (USART_CR1)
	// Bit 3 TE: Transmitter enable
	// 1: Transmitter is enabled
	USART1->CR1 |= (1 << 3);

	// Bit 2 RE: Receiver enable
	// 1: Receiver is enabled and begins searching for a start bit
	USART1->CR1 |= (1 << 2);

	// Definir o baud rate
	// 27.6.3 Baud rate register (USART_BRR)
	USART1->BRR = ((CPU_CLK + (BaudRate/2U))/BaudRate);

	// Configurar o tamanho da palavra que será transmitida
	// 27.6.4 Control register 1 (USART_CR1)
	// 0: 1 Start bit, 8 Data bits, n Stop bit
	USART1->CR1 |= (0 << 12); // Bit 12 M: Word length

	// Ligar a USART
	// 27.6.4 Control register 1 (USART_CR1)
	USART1->CR1 |= (1 << 13); // Bit 13 UE: USART enable

	// Habilitar que as interrupções da USART sejam aceitas no nvic
	NVIC_EnableIRQ(USART1_IRQn);

	int flag = 1;
	while(flag){
		if(read_or_write){
			// Mostrar a mensagem na tela
			uart_write(msg);
			read_or_write = 0;

			// Habilitar a interrupção de recepção de dados
			// Bit 5 RXNEIE: RXNE interrupt enable
			// 1: A USART interrupt is generated whenever ORE=1 or RXNE=1
			// in the USART_SR register
			USART1->CR1 |= (1 << 5);
		}
		else{
			if(write_or_processing){
				// Recebendo requisições...
			}
			else{
				uart_write(buffer);
				switch(buffer[0]){
				case 'R':
				case 'r':
					uart_write(debug_msg_red);
					// Led vermelho / PB8 / TIMER 4 CANAL 3
					TIM4->CCR3 = string_to_int(&buffer[1]);
					break;
				case 'G':
				case 'g':
					uart_write(debug_msg_green);
					// Led verde / PB4 / TIMER 3 CANAL 1
					TIM3->CCR1 = string_to_int(&buffer[1]);
					break;
				case 'B':
				case 'b':
					uart_write(debug_msg_blue);
					// Led azul / PB3 / TIMER 2 CANAL 2
					TIM2->CCR2 = string_to_int(&buffer[1]);
					break;
				case 'D':
				case 'd':
					flag = 0;
					break;
				default:
					break;
				}
				read_or_write = 1;
				write_or_processing = 1;
			}
		}
	}

	while(1){
		for(volatile int i = 0; i <= 100; i++){
			TIM2->CCR2 = i;
			TIM3->CCR1 = 100 - i;
			TIM4->CCR3 = i;
			DELAY_MS(1);
		}
		for(volatile int i = 0; i <= 100; i++){
			TIM2->CCR2 = 100 - i;
			TIM3->CCR1 = i;
			TIM4->CCR3 = 100 - i;
			DELAY_MS(1);
		}
	}
}
