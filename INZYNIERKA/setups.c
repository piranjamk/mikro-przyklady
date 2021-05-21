#include "stm32f1xx.h"
#include "setups.h"
/*
 * konfiguracja poszczegolnych modulow
 */

// BEGIN - UPORZADKOWAC PO TESTACH - POWTORZENIE Z MAINA
#define LSM_TEMP_OUT			0x05
#define LSM_STATUS_M			0x07
#define LSM_OUT_X_M				0x08
#define LSM_OUT_Y_M				0x0a
#define LSM_OUT_Z_M				0x0c
#define LSM_WHO_AM_I			0x0f
#define LSM_CTRL0				0x1f
#define LSM_CTRL1				0x20
#define LSM_CTRL2				0x21
#define LSM_CTRL3				0x22
#define LSM_CTRL4				0x23
#define LSM_CTRL5				0x24
#define LSM_CTRL6				0x25
#define LSM_CTRL7				0x26
#define LSM_STATUS				0x27
#define LSM_OUT_X_A				0x28
#define LSM_OUT_Y_A				0x2a
#define LSM_OUT_Z_A				0x2c

#define LSM_ADDR			0x3a




#define CZAS_JAZDY_DO_PRZODU 400
#define CZAS_JAZDY_DO_TYLU	400
#define WSPOLCZYNNIK_KORYGUJACY 0.1
#define CZAS_OBROTU_W_LEWO 600
#define CZAS_OBROTU_W_PRAWO 900
#define JAZDA_DO_PRZODU GPIO_PIN_RESET
#define JAZDA_DO_TYLU GPIO_PIN_SET
#define KANAL_SILNIKA_LEWEGO TIM_CHANNEL_4
#define KANAL_SILNIKA_PRAWEGO TIM_CHANNEL_3
#define PIN_LEWY GPIO_PIN_1
#define PIN_PRAWY GPIO_PIN_0
#define WARTOSC_WYPELNIENIA_LEWA 500//589
#define WARTOSC_WYPELNIENIA_PRAWA 500//600


extern TIM_HandleTypeDef tim4;
extern TIM_HandleTypeDef tim3_servo;
extern UART_HandleTypeDef uart1;//DRUKOWANIE PO BLUOTOOTH
extern UART_HandleTypeDef uart2;//DRUKOWANIE PRZEZ COM VIA USB-LINK
extern I2C_HandleTypeDef i2c;
// END - UPORZADKOWAC PO TESTACH - POWTORZENIE Z MAINA




void set_piny_kontrolne_mostka(void) {
	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pin = GPIO_PIN_1 | GPIO_PIN_0;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &gpio);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);
}

void set_piny_pwm_mostka() {
	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &gpio);
}
void set_przycisk(void) {
	//uzbrojenie robota
	GPIO_InitTypeDef gpio;
	gpio.Pin = GPIO_PIN_13; 	// konfigurujemy pin 13
	gpio.Mode = GPIO_MODE_INPUT; 	// jako wejście
	gpio.Pull = GPIO_PULLUP;		// włączamy rezystor podciągający
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &gpio);	// port GPIOC
}

void set_pwm(void) {
	tim4.Instance = TIM4;
	tim4.Init.Period = 1000 - 1;
	tim4.Init.Prescaler = 8 - 1; //8000 - 1;
	tim4.Init.ClockDivision = 0;
	tim4.Init.CounterMode = TIM_COUNTERMODE_UP;
	tim4.Init.RepetitionCounter = 0;
	tim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_PWM_Init(&tim4);

	TIM_OC_InitTypeDef oc;
	oc.OCMode = TIM_OCMODE_PWM1;
	oc.Pulse = WARTOSC_WYPELNIENIA_LEWA; //(int)(WARTOSC_WYPELNIENIA - WARTOSC_WYPELNIENIA * WSPOLCZYNNIK_KORYGUJACY);
	oc.OCPolarity = TIM_OCPOLARITY_HIGH;
	oc.OCNPolarity = TIM_OCNPOLARITY_LOW;
	oc.OCFastMode = TIM_OCFAST_ENABLE;
	oc.OCIdleState = TIM_OCIDLESTATE_SET;
	oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	HAL_TIM_PWM_ConfigChannel(&tim4, &oc, KANAL_SILNIKA_LEWEGO);

	oc.Pulse = WARTOSC_WYPELNIENIA_PRAWA; //WARTOSC_WYPELNIENIA;
	HAL_TIM_PWM_ConfigChannel(&tim4, &oc, KANAL_SILNIKA_PRAWEGO);
}

void set_usart_printowanie(void) {

	GPIO_InitTypeDef gpio_usart1;				//UART - BLUETOOTH
	gpio_usart1.Pin = GPIO_PIN_9;
	gpio_usart1.Mode = GPIO_MODE_AF_PP;
	gpio_usart1.Pull = GPIO_NOPULL;
	gpio_usart1.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &gpio_usart1);

	uart1.Instance = USART1;
	uart1.Init.BaudRate = 9600;
	uart1.Init.WordLength = UART_WORDLENGTH_8B;
	uart1.Init.Parity = UART_PARITY_NONE;
	uart1.Init.StopBits = UART_STOPBITS_1;
	uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	uart1.Init.OverSampling = UART_OVERSAMPLING_16;
	uart1.Init.Mode = UART_MODE_TX_RX;
	HAL_UART_Init(&uart1);

	GPIO_InitTypeDef gpio_usart2;				//UART - ST-LINK
	gpio_usart2.Mode = GPIO_MODE_AF_PP;
	gpio_usart2.Pin = GPIO_PIN_2;
	gpio_usart2.Pull = GPIO_NOPULL;
	gpio_usart2.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &gpio_usart2);

	gpio_usart2.Mode = GPIO_MODE_AF_INPUT;
	gpio_usart2.Pin = GPIO_PIN_3;
	HAL_GPIO_Init(GPIOA, &gpio_usart2);

	uart2.Instance = USART2;
	uart2.Init.BaudRate = 9600;
	uart2.Init.WordLength = UART_WORDLENGTH_8B;
	uart2.Init.Parity = UART_PARITY_NONE;
	uart2.Init.StopBits = UART_STOPBITS_1;
	uart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	uart2.Init.OverSampling = UART_OVERSAMPLING_16;
	uart2.Init.Mode = UART_MODE_TX_RX;
	HAL_UART_Init(&uart2);
}

void init_servo() {
	GPIO_InitTypeDef gpio_servo;			//SERVO 1ms - 2ms
	gpio_servo.Pin = GPIO_PIN_6;
	gpio_servo.Pull = GPIO_NOPULL;
	gpio_servo.Mode = GPIO_MODE_AF_PP;
	gpio_servo.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &gpio_servo);

	tim3_servo.Instance = TIM3;
	tim3_servo.Init.Prescaler = 160 - 1;	//4 - 1;
	tim3_servo.Init.CounterMode = TIM_COUNTERMODE_UP;
	tim3_servo.Init.Period = 1000 - 1;	//20000 - 1;
	tim3_servo.Init.ClockDivision = 0;
	tim3_servo.Init.RepetitionCounter = 0;
	tim3_servo.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_PWM_Init(&tim3_servo);
	printf("Start\n");

	TIM_OC_InitTypeDef oc;
	oc.OCMode = TIM_OCMODE_PWM2;
	oc.Pulse = 120; // OD 26 DO 120     27-69-120		     stare:26-70-120
	oc.OCPolarity = TIM_OCPOLARITY_LOW;
	oc.OCNPolarity = TIM_OCNPOLARITY_LOW;
	oc.OCFastMode = TIM_OCFAST_ENABLE;
	oc.OCIdleState = TIM_OCIDLESTATE_SET;
	oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	HAL_TIM_OC_ConfigChannel(&tim3_servo, &oc, TIM_CHANNEL_1);

	HAL_TIM_PWM_Start(&tim3_servo, TIM_CHANNEL_1);
	//tim3_servo.Instance->CCR1 = 50;
	HAL_Delay(500);
	//HAL_TIM_PWM_Stop(&tim3_servo, TIM_CHANNEL_1);
}

void set_ultrasound_pins() {				//5V PINS!!!
	GPIO_InitTypeDef gpio_ultrasound_pin;
	//BEGIN TRIGGERS
	gpio_ultrasound_pin.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_ultrasound_pin.Pin = GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_10;
	gpio_ultrasound_pin.Pull = GPIO_NOPULL;
	gpio_ultrasound_pin.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &gpio_ultrasound_pin);
	gpio_ultrasound_pin.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_ultrasound_pin.Pin = GPIO_PIN_12;
	gpio_ultrasound_pin.Pull = GPIO_NOPULL;
	gpio_ultrasound_pin.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &gpio_ultrasound_pin);
	//END TRIGGERS

	//BEGIN ECHOS
	gpio_ultrasound_pin.Mode = GPIO_MODE_INPUT;
	gpio_ultrasound_pin.Pin = GPIO_PIN_7;
	HAL_GPIO_Init(GPIOC, &gpio_ultrasound_pin);
	gpio_ultrasound_pin.Mode = GPIO_MODE_INPUT;
	gpio_ultrasound_pin.Pin = GPIO_PIN_9;
	HAL_GPIO_Init(GPIOC, &gpio_ultrasound_pin);
	gpio_ultrasound_pin.Mode = GPIO_MODE_INPUT;
	gpio_ultrasound_pin.Pin = GPIO_PIN_11;
	HAL_GPIO_Init(GPIOC, &gpio_ultrasound_pin);
	gpio_ultrasound_pin.Mode = GPIO_MODE_INPUT;
	gpio_ultrasound_pin.Pin = GPIO_PIN_12;
	HAL_GPIO_Init(GPIOC, &gpio_ultrasound_pin);
	//END ECHOS
}
