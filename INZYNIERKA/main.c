/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/




/*
 * todo lista
 *
 * ogarnac definicje jak normalny czlowiek i usunac magic numbers z definicji w jezdzie po testach
 *
 *
 * wykrywanie glebszych zalaman-prawa str. i poprawny skret w prawo (granica 40cm?)!!!
 * poprawic glowna petle obslugujaca skrety lewo/prawo/jedzprosto
 * 		implementacja: wykrywanie szybkich roznic w odczycie np. abs(poprzedni - obecny) > 20
 * 			(sprawdzic dla kata pochylenia granicznego)
 *
 * zbic do pliku zapis i odczyt LSMki
 * pomyslec nad danymi z HCSR04 (global-local array && all-one && [zwiekszyc taktowanie w pllce?- przy braku dokladnosci po testach])
 * 		np.funkcja voidowa dla globalnego arraya i update wszystkiego(szybki obrot)       albo funkcja jednoargumentowa z kierunkiem updatu
 * uporzadkowac potem wszystkie arraye z odczytami
 * dopracowac poruszanie sie i przeniesc do pliku
 *
 *
 * do pomiarow:
 * kierunki i przechodzenie przy skretach
 * update do pomiaru
 * pomiar ciagly/dyskretny   dyskretyzacja dla +/- 20cm
 *
 */


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
#include "stm32f1xx.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "setups.h"
TIM_HandleTypeDef tim4;
TIM_HandleTypeDef tim3_servo;
UART_HandleTypeDef uart1;//DRUKOWANIE PO BLUETOOTH
UART_HandleTypeDef uart2;//DRUKOWANIE PRZEZ COM VIA USB-LINK
I2C_HandleTypeDef i2c;
float kierunek_pierwotny;
void send_char(char c){
	HAL_UART_Transmit(&uart1, (uint8_t*)&c, 1, 999);
	HAL_UART_Transmit(&uart2, (uint8_t*)&c, 1, 999);
}

int __io_putchar(int ch){
	if (ch == '\n')
		send_char('\r');
	send_char(ch);
	return ch;
}


//i2c module ----------------------------------------
void lsm_write_reg(uint8_t reg, uint8_t value){
	HAL_I2C_Mem_Write(&i2c, LSM_ADDR, reg, 1, &value, sizeof(value), HAL_MAX_DELAY);
}

uint8_t lsm_read_reg(uint8_t reg){
	uint8_t value = 0;
	HAL_I2C_Mem_Read(&i2c, LSM_ADDR, reg, 1, &value, sizeof(value), HAL_MAX_DELAY);
	return value;
}

int16_t lsm_read_value(uint8_t reg){
	int16_t value = 0;
	HAL_I2C_Mem_Read(&i2c, LSM_ADDR, reg | 0x80, 1, (uint8_t*)&value, sizeof(value), HAL_MAX_DELAY);
	return value;
}


float get_heading_degrees(){
	int16_t m_x = (int16_t) lsm_read_reg(0x09) << 8 | lsm_read_reg(0x08);
	int16_t m_y = (int16_t) lsm_read_reg(0x0B) << 8 | lsm_read_reg(0x0A);

	float heading = atan2(m_y, m_x);
	if (heading < 0)
		heading += 2 * M_PI;
	return heading *180/M_PI;
}


void set_i2c(void) {
	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_AF_OD;
	gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;		// SCL, SDA
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &gpio);

	i2c.Instance = I2C1;
	i2c.Init.ClockSpeed = 100000;
	i2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
	i2c.Init.OwnAddress1 = 0xff;
	i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	i2c.Init.OwnAddress2 = 0xff;
	i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	HAL_I2C_Init(&i2c);

	//konfiguracja modulu
	lsm_write_reg(LSM_CTRL5, 0x64); // z moich ustawien -magnetometr na gicie
	lsm_write_reg(LSM_CTRL6, 0x20); // z moich ustawien -magnetometr na gicie
	lsm_write_reg(LSM_CTRL7, 0x0); // z moich ustawien -magnetometr na gicie
	HAL_Delay(100);
	//lsm_write_reg(LSM_CTRL1, 0x40|0x07); // AODR2 (25Hz) | AXEN | AYEN | AZEN
	//lsm_write_reg(LSM_CTRL5, 0x80|0x10); // TEMP_EN | M_ODR2 (50Hz)
	//lsm_write_reg(LSM_CTRL6, 0x0);
	//lsm_write_reg(LSM_CTRL7, 0x0);
	/*
	 lsm_write_reg(LSM_CTRL5, 0x64);
	 lsm_write_reg(LSM_CTRL6, 0x20);
	 lsm_write_reg(LSM_CTRL7, 0x0);
	 HAL_Delay(100);
	 */
	//konfiguracja modulu -end
}



void waiting(int value)
{
	while(--value/2)
		continue;
}

void jedz_do_przodu(){
	static int licznik = 0;
	if (licznik == 0){
	HAL_GPIO_WritePin(GPIOC, PIN_PRAWY, JAZDA_DO_PRZODU);
	HAL_GPIO_WritePin(GPIOC, PIN_LEWY, JAZDA_DO_PRZODU);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_LEWEGO);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_PRAWEGO);
	HAL_Delay(CZAS_JAZDY_DO_PRZODU);
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_PRAWEGO);
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_LEWEGO);
	licznik = 1;
	} else {
		HAL_GPIO_WritePin(GPIOC, PIN_LEWY, JAZDA_DO_PRZODU);
		HAL_GPIO_WritePin(GPIOC, PIN_PRAWY, JAZDA_DO_PRZODU);
		HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_PRAWEGO);
		HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_LEWEGO);
		HAL_Delay(CZAS_JAZDY_DO_PRZODU);
		HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_LEWEGO);
		HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_PRAWEGO);
		licznik = 0;
	}
}



void update_ultrasound_distances(float *distances_array) {			//ultrasounds - prototyp blokowy - przetestowany
	//static float distances[1];
	int reflection_time;
	int pins[4] = { GPIO_PIN_7 };

	//printf("Wchodze\n");
	{
		reflection_time = 0;
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
		//waiting(2);  // wait for 2 us
		HAL_Delay(2);

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
		//waiting(800);  // wait for 10 us
		//HAL_Delay(10);
		//waiting(640);
		//HAL_Delay(800);
		waiting(800);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);

		// read the time for which the pin is high

		while (!(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7)))
			continue;  // wait for the ECHO pin to go high
		while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7))
		{
			reflection_time++;
			//if(reflection_time > 20000)
			//break;
			//waiting(1);
			//HAL_Delay(1);
		}
		//printf("Time: %d |||", reflection_time);
		float distance = reflection_time * .034 / 2 * 10 * 2 / 3;
		//printf("Distance: %.3f   ", distance);
		distances_array[0] = distance;
	}
	HAL_Delay(100);

	{
		reflection_time = 0;
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
		//waiting(2);  // wait for 2 us
		HAL_Delay(2);

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
		//waiting(800);  // wait for 10 us
		//HAL_Delay(10);
		waiting(800);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);

		// read the time for which the pin is high

		while (!(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9)))
			continue;  // wait for the ECHO pin to go high
		while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9))
		{
			reflection_time++;
			//if(reflection_time > 20000)
			//break;
			//waiting(1);
			//HAL_Delay(1);
		}
		//printf("Time: %d |||", reflection_time);
		float distance = reflection_time * .034 / 2 * 10 * 2 / 3;
		//printf("Distance: %.3f   ", distance);
		distances_array[1] = distance;

	}
	{
		reflection_time = 0;
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
		//waiting(2);  // wait for 2 us
		HAL_Delay(2);

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
		//waiting(800);  // wait for 10 us
		//HAL_Delay(10);
		waiting(800);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

		// read the time for which the pin is high

		while (!(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_11)))
			continue;  // wait for the ECHO pin to go high
		while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_11))
		{
			reflection_time++;
			//if(reflection_time > 20000)
			//break;
			//waiting(1);
			//HAL_Delay(1);
		}
		//printf("Time: %d |||", reflection_time);
		float distance = reflection_time * .034 / 2 * 10 * 2 / 3;
		//printf("Distance: %.3f   ", distance);
		distances_array[2] = distance;
	}
	HAL_Delay(1);
	{
		reflection_time = 0;
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
		//waiting(2);  // wait for 2 us
		HAL_Delay(2);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
		//waiting(800);  // wait for 10 us
		//HAL_Delay(10);
		waiting(800);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

		// read the time for which the pin is high

		while (!(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_12)))
			continue;  // wait for the ECHO pin to go high
		while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_12))
		{
			reflection_time++;
			//if(reflection_time > 20000)
			//break;
			//waiting(1);
			//HAL_Delay(1);
		}
		//printf("Time: %d |||", reflection_time);
		float distance = reflection_time * .034 / 2 * 10 * 2 / 3;
		//printf("Distance: %.3f   ", distance);
		distances_array[3] = distance;
		//printf("\n");
	}
}


void zatrzymaj_silniki(){
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_LEWEGO);
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_PRAWEGO);
}

void podazaj_przod() {
	HAL_GPIO_WritePin(GPIOC, PIN_PRAWY, JAZDA_DO_PRZODU);
	HAL_GPIO_WritePin(GPIOC, PIN_LEWY, JAZDA_DO_PRZODU);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_LEWEGO);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_PRAWEGO);

	float distances[4];
	update_ultrasound_distances(distances);
	while (distances[3] > 20.0) {
		printf("%.3f\n", distances[0]);

		if (distances[0] > 21.5) {
			__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_LEWEGO, 200);
			__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_PRAWEGO, 400);
		} else if (distances[0] < 18.5) {
			__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_LEWEGO, 400);
			__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_PRAWEGO, 200);
		} else {
			__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_LEWEGO, 400);
			__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_PRAWEGO, 400);
		}
		/*
		 if (distances[0] > 20.5){
		 __HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_LEWEGO, 200);
		 __HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_PRAWEGO, 400);
		 } else {
		 __HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_LEWEGO, 400);
		 __HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_PRAWEGO, 200);
		 }
		 */
		update_ultrasound_distances(distances);
	}
	zatrzymaj_silniki();
}




void skrec_w_lewo(){
	__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_LEWEGO, 450);
	__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_PRAWEGO, 450);

	float dist_array[4];
	update_ultrasound_distances(dist_array);
	float odleglosc_przod = dist_array[3];

	HAL_GPIO_WritePin(GPIOC, PIN_PRAWY, JAZDA_DO_PRZODU);
	HAL_GPIO_WritePin(GPIOC, PIN_LEWY, JAZDA_DO_TYLU);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_LEWEGO);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_PRAWEGO);

	HAL_Delay(1500);
	update_ultrasound_distances(dist_array);
	float distance = dist_array[0];
	float previous_distance;
	while (1){
		previous_distance = distance;
		 update_ultrasound_distances(dist_array);
		distance = dist_array[0];
		if (abs(previous_distance - distance) < 8 && previous_distance < distance)
			break;
	}
	/*
	while (abs(odleglosc_przod - dist_array[0]) > 3.5)
	{
		//printf("%.3f    %.3f    %d    \n", dist_array[0], dist_array[3], abs(odleglosc_przod - dist_array[0]));
		update_ultrasound_distances(dist_array);
	}
	*/
	zatrzymaj_silniki();

/*
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_PRAWEGO);
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_LEWEGO);

	//int wartosc_wypelnienia_skorygowana = WARTOSC_WYPELNIENIA - WARTOSC_WYPELNIENIA * WSPOLCZYNNIK_KORYGUJACY;
	//__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_LEWEGO, WARTOSC_WYPELNIENIA);
	//__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_PRAWEGO, wartosc_wypelnienia_skorygowana);
	__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_LEWEGO, WARTOSC_WYPELNIENIA_LEWA);
	__HAL_TIM_SET_COMPARE(&tim4, KANAL_SILNIKA_PRAWEGO, WARTOSC_WYPELNIENIA_PRAWA);

*/

}

void skrec_w_prawo(){
	HAL_GPIO_WritePin(GPIOC, PIN_PRAWY, JAZDA_DO_TYLU);
	HAL_GPIO_WritePin(GPIOC, PIN_LEWY, JAZDA_DO_PRZODU);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_LEWEGO);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_PRAWEGO);
	HAL_Delay(CZAS_OBROTU_W_PRAWO);
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_PRAWEGO);
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_LEWEGO);
}

void jedz_do_tylu(){
	HAL_GPIO_WritePin(GPIOC, PIN_PRAWY, JAZDA_DO_TYLU);
	HAL_GPIO_WritePin(GPIOC, PIN_LEWY, JAZDA_DO_TYLU);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_LEWEGO);
	HAL_TIM_PWM_Start(&tim4, KANAL_SILNIKA_PRAWEGO);
	HAL_Delay(CZAS_JAZDY_DO_TYLU);
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_PRAWEGO);
	HAL_TIM_PWM_Stop(&tim4, KANAL_SILNIKA_LEWEGO);
}


int main(void)
{
	//SystemClock_Config();
	SystemCoreClock = 8000000; // sys tick, hal_delay w ms
	HAL_Init();

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_TIM4_CLK_ENABLE();
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();


	set_piny_kontrolne_mostka();
	set_piny_pwm_mostka();
	set_przycisk();
	set_pwm();
	set_usart_printowanie();
	set_i2c();
	init_servo();
	set_ultrasound_pins();
	HAL_Delay(300);


	/*

	 int16_t fam_x = (int16_t) lsm_read_reg(0x09) <<8 | lsm_read_reg(0x08);
	 int16_t fam_y = (int16_t) lsm_read_reg(0x0B) <<8 | lsm_read_reg(0x0A);
	 HAL_Delay(500);
	 printf("Sprawdzenie poprawnosci podlaczenia akcelerometru\n");




	 int16_t pierwotne_m_x = (int16_t) lsm_read_reg(0x09) <<8 | lsm_read_reg(0x08);
	 int16_t pierwotne_m_y = (int16_t) lsm_read_reg(0x0B) <<8 | lsm_read_reg(0x0A);


	 float  pierwotne_x = (float)pierwotne_m_x;
	 float pierwotne_y = (float)pierwotne_m_y;
	 float pierwotne_heading = atan2(pierwotne_y, pierwotne_x);
	 if (pierwotne_heading < 0)
	 pierwotne_heading += 2*M_PI;
	 //printf("%.5f    ||  ", pierwotne_heading*180/M_PI);

	 //printf("%.6f, %.6f  %.4f\n", pierwotne_x,  pierwotne_y, pierwotne_heading);


	 kierunek_pierwotny = get_heading_degrees();
	 printf("Kierunek pierwotny:  %.4f\n", kierunek_pierwotny);
	 HAL_Delay(2000);
	 int iteracja = 0;
	 kierunek_pierwotny = get_heading_degrees();
	 kierunek_pierwotny = get_heading_degrees();
	 */
int i = 0;
uint8_t received_char;

	while (1) {
		/*
		 float heading = get_heading_degrees();
		 //printf("Test  %d\n", iteracja);
		 iteracja++;

		 float wartosc = abs(heading - kierunek_pierwotny);

		 if (wartosc >= 0.0 && wartosc < 1.0) {
		 printf("W pozycji!!!!!!!!!!!!!!!!  ");
		 }
		 printf("  Kierunek:  %.2f    ||  roznica:  %.4f    [[[[]]]]]    \n", heading, wartosc);
		 HAL_Delay(300);
		 */

		//printf("Test%d\n", i);

		// 1-prawo 2-lewo 3-tyl 4-przod
		float distances[4];

		printf("Test wejscie\n");
		{ //glowny program

			while (1) {
				if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
					break;
			}
			while (1) {
				update_ultrasound_distances(distances);
				printf("%f\n", distances[0]);
				if (distances[3] > 20.0)
					podazaj_przod();
				for (int xyz = 0; xyz < 4; xyz++)
					printf("%f    ", distances[xyz]);
				printf("\n");
				skrec_w_lewo();

				/*
				 skrec_w_lewo();
				 //\\\printf("%f", distances[3]);
				 for (int xyz = 0; xyz < 4; xyz++)
				 //			printf("%f    ", distances[xyz]);
				 //		printf("\n");
				 //HAL_Delay(8000);
				 if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
				 break;
				 */
			}
			while (1) {
				update_ultrasound_distances(distances);
				//printf("%f\n", distances[3]);
				HAL_Delay(1000);
			}

		}

		/*
		 update_ultrasound_distances(distances);
		 for (int xyz = 0; xyz < 4; xyz++)
		 printf("%f    ", distances[xyz]);
		 printf("\n");
		 */
		i++;
		if (__HAL_UART_GET_FLAG(&uart1, UART_FLAG_RXNE) == SET) {

			HAL_UART_Receive(&uart1, &received_char, 1, 100);

			printf("Odebrano: %c\n", received_char);
			switch (received_char) {
			case 'w':
				jedz_do_przodu();
				printf("Jade do przodu\n");
				break;
			case 's':
				jedz_do_tylu();
				printf("Jade do tylu\n");
				break;
			case 'a':
				skrec_w_lewo();
				printf("Obracam w lewo\n");
				break;
			case 'd':
				skrec_w_prawo();
				printf("Obracam w prawo\n");
				break;
			default:
				printf("Niezydentyfikowany\n");
			}

		}
		if (__HAL_UART_GET_FLAG(&uart2, UART_FLAG_RXNE) == SET) {

			HAL_UART_Receive(&uart2, &received_char, 1, 100);

			printf("Odebrano: %c\n", received_char);
			switch (received_char) {
			case 'w':
				jedz_do_przodu();
				printf("Jade do przodu\n");
				break;
			case 's':
				jedz_do_tylu();
				printf("Jade do tylu\n");
				break;
			case 'a':
				skrec_w_lewo();
				printf("Obracam w lewo\n");
				break;
			case 'd':
				skrec_w_prawo();
				printf("Obracam w prawo\n");
				break;
			default:
				printf("Niezydentyfikowany\n");
			}
		}
		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
		{}
	}
}
