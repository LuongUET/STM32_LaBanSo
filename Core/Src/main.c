#include "main.h"
#include "config_clock.h"
#include "i2c.h"
#include "HMC5883L.h"
#include "i2c-lcd.h"

#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_3
#define LED1_GPIO_Port GPIOA
#define LED2_Pin GPIO_PIN_4
#define LED2_GPIO_Port GPIOA
#define BTN1_Pin GPIO_PIN_0
#define BTN1_GPIO_Port GPIOB
#define BTN2_Pin GPIO_PIN_1
#define BTN2_GPIO_Port GPIOB

#define LED_STOP_INTERVAL	500		// ms
#define LED_RUN_INTERVAL	1000 // ms
#define LCD_REFRESH_INTERVAL	200
typedef enum {
	STOP = 0,
	START = 1,
} state_e;

typedef enum {
	MODE_COMPASS = 0,
	MODE_AXIS = 1,
} mode_e;

/*--------------------------- global variable -----------------------------*/
state_e g_state = STOP;
mode_e g_mode = MODE_AXIS;

QMC_t pusula_sensor;
float Compas_Value;

uint32_t ticks;
uint32_t ticks_lcd;

static void gpio_init(void){
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; // Enable IO port C Clock

	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable IO port A Clock
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // Enable IO port B Clock
	
	/* Config Led Pin PC13*/
	GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13); // clear bit
	GPIOC->CRH |= (GPIO_CRH_MODE13_1 | GPIO_CRH_MODE13_0); //output 50 MHz
	GPIOC->CRH |= GPIO_CRH_CNF13_0; // push-pull (CNF13[1:0] = 00)

	/* Config LED3 Pin PA3 */
	GPIOA->CRL &= ~(GPIO_CRL_MODE3 | GPIO_CRL_CNF3); // Clear bits for PA3
	GPIOA->CRL |= (GPIO_CRL_MODE3_1 | GPIO_CRL_MODE3_0); // Output mode, max speed 50 MHz
	GPIOA->CRL |= GPIO_CRL_CNF3_0; // General purpose output push-pull

	/* Config LED4 Pin PA4 */
	GPIOA->CRL &= ~(GPIO_CRL_MODE4 | GPIO_CRL_CNF4); // Clear bits for PA4
	GPIOA->CRL |= (GPIO_CRL_MODE4_1 | GPIO_CRL_MODE4_0); // Output mode, max speed 50 MHz
	GPIOA->CRL |= GPIO_CRL_CNF4_0; // General purpose output push-pull
	
	 /* Config PB0 and PB1 as input pull-up */
	GPIOB->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0); // Clear bits for PB0
	GPIOB->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1); // Clear bits for PB1
	GPIOB->CRL |= (GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1); // Input mode, pull-up/pull-down

	/* Enable pull-up for PB0 and PB1 */
	GPIOB->ODR |= (GPIO_ODR_ODR0 | GPIO_ODR_ODR1); // Set bits for pull-up
}

int main(void)
{
  HAL_Init();

  SystemClock_Config();
  gpio_init();

	i2c_init();
//	i2c1_scan_bus();
	
	QMC_init(&pusula_sensor, 200);
	
	lcd_init();
	HAL_Delay(100);
	lcd_goto_XY(1, 3);
	HAL_Delay(100);
	LED_GPIO_Port->BSRR = LED_Pin;
	
	LED1_GPIO_Port->BSRR = LED1_Pin; // set LED1
	LED2_GPIO_Port->BSRR = LED2_Pin; // set LED1
	GPIOA->BSRR = GPIO_PIN_9; // set LED1
	GPIOA->BSRR = GPIO_PIN_11; // set LED1
	
	lcd_send_string("press button");
	
  while (1)
  {
		if (!(GPIOB->IDR & GPIO_IDR_IDR0)) { // Kiem tra nut nhan xem co duoc nhan hay khong?
			while(!(GPIOB->IDR & GPIO_IDR_IDR0));
			g_state = !g_state;
		}
		else if(!(GPIOB->IDR & GPIO_IDR_IDR1)){
			while(!(GPIOB->IDR & GPIO_IDR_IDR1));
			g_mode = !g_mode;
			lcd_clear_display();
			lcd_goto_XY(1, 0);
			lcd_send_string("  press button");
		}
		
		switch(g_state){
			case START:
			{
				if(HAL_GetTick() - ticks_lcd >= LCD_REFRESH_INTERVAL){
					if(QMC_read(&pusula_sensor)==0){
						Compas_Value=pusula_sensor.heading;
						switch (g_mode){
							case MODE_AXIS:
								lcd_goto_XY(1, 0);
								lcd_send_string("Compass: %.2f", Compas_Value);
							break;
							case MODE_COMPASS:
								lcd_goto_XY(1, 0);
								lcd_send_string(" X-----Y-----Z ", Compas_Value);
								lcd_goto_XY(2, 0);
								lcd_send_string("%d  %d  %d", pusula_sensor.Xaxis, pusula_sensor.Yaxis, pusula_sensor.Zaxis);
							break;
						}
					}
				}

				// Blink LED
				if(HAL_GetTick() - ticks >= LED_RUN_INTERVAL){
					uint32_t odr = LED1_GPIO_Port->ODR;
					LED2_GPIO_Port->BSRR = LED2_Pin; // set LED1
					LED1_GPIO_Port->BSRR = ((odr & LED1_Pin) << 16) | (~odr & LED1_Pin);
					ticks = HAL_GetTick();
				}
				break;
			}
			case STOP:
			{
				// Blink LED
				if(HAL_GetTick() - ticks >= LED_STOP_INTERVAL){
					uint32_t odr = LED2_GPIO_Port->ODR;
					LED1_GPIO_Port->BSRR = LED1_Pin; // set LED1
					LED2_GPIO_Port->BSRR = ((odr & LED2_Pin) << 16) | (~odr & LED2_Pin);
					ticks = HAL_GetTick();
				}
				break;
			}
			default: break;
		}	
  }
}

void Error_Handler(void)
{

  __disable_irq();
  while (1)
  {
  }
}
