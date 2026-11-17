#include <stdint.h>
#include <stm32f10x.h>
#include <ssd1306.h>

volatile uint8_t current_digit = 0;
volatile uint8_t update_display = 0;

uint8_t bitrev(uint8_t b){
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
};

void delay_us(uint32_t us){
	__asm volatile (
		"push {r0}\r\n"
		"mov R0, %0\r\n"
		"_loop:\r\n"
			"cmp R0, #0\r\n"
			"beq _exit\r\n"
			"sub R0, R0, #1\r\n"
			"nop\r\n"
			"b _loop\r\n"
		"_exit:\r\n"
		"pop {r0}\r\n"
		:: "r"(9 * us)
	);
}

void TIM2_IRQHandler(void) 
{
    if (TIM2->SR & TIM_SR_UIF) 
    { 
        TIM2->SR &= ~TIM_SR_UIF;
        current_digit = (current_digit + 1) % 10;
        update_display = 1;
    }
}

uint32_t tim2_clk(void)
{
    uint32_t ppre1 = (RCC->CFGR >> 8) & 0x7;
    uint32_t pclk1;
    switch (ppre1) {
        case 0: case 1: case 2: case 3: pclk1 = SystemCoreClock; break;
        case 4: pclk1 = SystemCoreClock / 2;  break;
        case 5: pclk1 = SystemCoreClock / 4;  break;
        case 6: pclk1 = SystemCoreClock / 8;  break;
        default:pclk1 = SystemCoreClock / 16; break;
    }
    return (ppre1 == 0) ? pclk1 : (pclk1 * 2);   // 0 = APB1 без деления
}

void init_timer(void) 
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
	uint32_t f = tim2_clk();
	TIM2->PSC = (f / 10000U) - 1U;   
	TIM2->ARR = 10000U - 1U;         
	TIM2->DIER |= TIM_DIER_UIE; // Enable Update Interrupt
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	NVIC_EnableIRQ(TIM2_IRQn); // Enable IRQ in NVIC
	TIM2->CR1 |= TIM_CR1_CEN; // Start timer
}

int main(void) {
    SPI1_Init();
    SSD1306_Init();

	SSD1306_Clear();
	SSD1306_DrawDigit(68, 28, 0);
    SSD1306_Update();

	init_timer();
    while(1) {
		if (update_display){
			update_display = 0;
			SSD1306_Clear();
			SSD1306_DrawDigit(68, 28, current_digit);
			SSD1306_Update();
		}
    }
}