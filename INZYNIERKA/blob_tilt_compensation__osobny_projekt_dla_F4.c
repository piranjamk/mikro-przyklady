#include <string.h>
#include "stm32f4xx.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

//MAG+ACC - DOCSY,POLOLU,STRONKA

#define ACC_I2C_ADDRESS                      0x32
#define MAG_I2C_ADDRESS                      0x3C

#define LSM_WHO_AM_I_ADDR             0x0F  
#define LSM_CTRL_REG1_A               0x20  
#define LSM_CTRL_REG2_A               0x21  
#define LSM_CTRL_REG3_A               0x22  
#define LSM_CTRL_REG4_A               0x23  
#define LSM_CTRL_REG5_A               0x24 
#define LSM_CTRL_REG6_A               0x25  
#define LSM_REFERENCE_A               0x26  
#define LSM_STATUS_REG_A              0x27  
#define LSM_OUT_X_L_A                 0x28  
#define LSM_OUT_X_H_A                 0x29  
#define LSM_OUT_Y_L_A                 0x2A  
#define LSM_OUT_Y_H_A                 0x2B 
#define LSM_OUT_Z_L_A                 0x2C  
#define LSM_OUT_Z_H_A                 0x2D  
#define LSM_FIFO_CTRL_REG_A           0x2E  
#define LSM_FIFO_SRC_REG_A            0x2F  

#define LSM_INT1_CFG_A                0x30  
#define LSM_INT1_SOURCE_A             0x31  
#define LSM_INT1_THS_A                0x32  
#define LSM_INT1_DURATION_A           0x33  

#define LSM_INT2_CFG_A                0x34  
#define LSM_INT2_SOURCE_A             0x35  
#define LSM_INT2_THS_A                0x36  
#define LSM_INT2_DURATION_A           0x37  

#define LSM_CLICK_CFG_A               0x38  
#define LSM_CLICK_SOURCE_A            0x39  
#define LSM_CLICK_THS_A               0x3A  

#define LSM_TIME_LIMIT_A              0x3B  
#define LSM_TIME_LATENCY_A            0x3C  
#define LSM_TIME_WINDOW_A             0x3D  
//-----
#define LSM_SR_REG_M                  0x09  

#define LSM_CRA_REG_M                 0x00  
#define LSM_CRB_REG_M                 0x01  
#define LSM_MR_REG_M                  0x02  
#define LSM_OUT_X_H_M                 0x03  
#define LSM_OUT_X_L_M                 0x04  
#define LSM_OUT_Z_H_M                 0x05  
#define LSM_OUT_Z_L_M                 0x06  
#define LSM_OUT_Y_H_M                 0x07  
#define LSM_OUT_Y_L_M                 0x08  

#define LSM_SR_REG_M                  0x09  
#define LSM_IRA_REG_M                 0x0A  
#define LSM_IRB_REG_M                 0x0B  
#define LSM_IRC_REG_M                 0x0C  

#define LSM_TEMP_OUT_H_M              0x31  
#define LSM_TEMP_OUT_L_M              0x32  

//---
#define LSM_BLE_LSB                 ((uint8_t)0x00) 
#define LSM_BLE_MSB                 ((uint8_t)0x40) 

//----------------
#define LSM_M_SENSITIVITY_XY_1_3Ga     1100  
#define LSM_M_SENSITIVITY_XY_1_9Ga     855   
#define LSM_M_SENSITIVITY_XY_2_5Ga     670   
#define LSM_M_SENSITIVITY_XY_4_0Ga     450   
#define LSM_M_SENSITIVITY_XY_4_7Ga     400   
#define LSM_M_SENSITIVITY_XY_5_6Ga     330   
#define LSM_M_SENSITIVITY_XY_8_1Ga     230   
#define LSM_M_SENSITIVITY_Z_1_3Ga      980   
#define LSM_M_SENSITIVITY_Z_1_9Ga      760   
#define LSM_M_SENSITIVITY_Z_2_5Ga      600   
#define LSM_M_SENSITIVITY_Z_4Ga        400   
#define LSM_M_SENSITIVITY_Z_4_7Ga      355   
#define LSM_M_SENSITIVITY_Z_5_6Ga      295   
#define LSM_M_SENSITIVITY_Z_8_1Ga      205   
//-----------------------------------------------------------------------------------------------

#define LSM303_ACC_ADDRESS (0x19 << 1) // adres akcelerometru: 0011 001x
#define LSM303_ACC_CTRL_REG1_A 0x20 // rejestr ustawien 1
// CTRL_REG1_A = [ODR3][ODR2][ODR1][ODR0][LPEN][ZEN][YEN][XEN]
#define LSM303_ACC_Z_ENABLE 0x07 // 0000 0100
#define LSM303_ACC_100HZ 0x50 // 0101 0000
#define LSM303_ACC_CTRL_REG3_A 0x22 // rejestr ustawien 3
#define LSM303_ACC_Z_H_A 0x2D // wyzszy bajt danych osi Z

#define LSM303_MAG_ADDRESS (0x1E << 1)
#define LSM303_ACC_X_L_A 0x28 // nizszy bajt danych osi X
#define LSM303_ACC_Y_L_A 0x2A // nizszy bajt danych osi X
#define LSM303_ACC_Z_L_A 0x2C // nizszy bajt danych osi Z

#define LSM303_MAG_X_L 0x04
#define LSM303_MAG_Y_L 0x08
#define LSM303_MAG_Z_L 0x06

#define LSM303_ACC_RESOLUTION 2.0

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;

void send_char(char c) {
	HAL_UART_Transmit(&huart1, (uint8_t*) &c, 1, 1000);
}

int __io_putchar(int ch) {
	if (ch == '\n')
		send_char('\r');
	send_char(ch);
	return ch;
}

int lsm_write_reg(char urzadzenie, uint8_t reg, uint8_t value) {
	int pierwszy = 0;
	if (urzadzenie == 'm') {
		pierwszy = HAL_I2C_Mem_Write(&hi2c1, LSM303_MAG_ADDRESS, reg, 1, &value,
				1, 100);
	} else {
		pierwszy = HAL_I2C_Mem_Write(&hi2c1, LSM303_ACC_ADDRESS, reg, 1, &value,
				1, 100);
	}
	return pierwszy;
}

int8_t lsm_read_reg(char urzadzenie, uint8_t reg) {
	uint8_t value = 0;
	if (urzadzenie == 'm') {
		int pierwszy = HAL_I2C_Mem_Read(&hi2c1, LSM303_MAG_ADDRESS, reg, 1,
				&value, 1, 100);
	} else {
		int drugi = HAL_I2C_Mem_Read(&hi2c1, LSM303_ACC_ADDRESS, reg, 1, &value,
				1, 100);
	}

	return value;
}

uint16_t lsm_read_value(char urzadzenie, uint8_t reg) {
	int16_t value = 0;
	if (urzadzenie == 'm') {
		int pierwszy = HAL_I2C_Mem_Read(&hi2c1, LSM303_MAG_ADDRESS, reg | 0x80,
				1, (uint8_t*) &value, sizeof(value), HAL_MAX_DELAY);
	} else {
		int drugi = HAL_I2C_Mem_Read(&hi2c1, LSM303_ACC_ADDRESS, reg | 0x80, 1,
				(uint8_t*) &value, sizeof(value), HAL_MAX_DELAY);
	}

	return value;
}

int main(void) {
	SystemCoreClock = 16000000;	// taktowanie 16Mhz
	HAL_Init();

	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_ADC1_CLK_ENABLE();
	__HAL_RCC_USART6_CLK_ENABLE();
	__HAL_RCC_I2C1_CLK_ENABLE();
	// UART1 -------------------------------------------------------------------------------------
	GPIO_InitTypeDef gpio;	//PB6-TX, PB7-RX
	gpio.Pin = GPIO_PIN_6;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Alternate = GPIO_AF8_USART6;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &gpio);
	//PB6-TX, PB7-RX
	gpio.Pin = GPIO_PIN_7;
	gpio.Mode = GPIO_MODE_AF_OD;
	gpio.Pull = GPIO_NOPULL;
	gpio.Alternate = GPIO_AF8_USART6;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &gpio);
//Configure these UART pins (TX as alternate function pull-up, RX as alternate function Input).
	//UART_HandleTypeDef huart1;
	huart1.Instance = USART6;
	huart1.Init.BaudRate = 9600;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_Init(&huart1);
	//huart1->gState = HAL_UART_STATE_READY;

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 400000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLED;
	HAL_I2C_Init(&hi2c1);

	lsm_write_reg('a', LSM303_ACC_CTRL_REG1_A,
			LSM303_ACC_Z_ENABLE | LSM303_ACC_100HZ);
	//lsm_write_reg('a', LSM303_ACC_CTRL_REG1_A, 0xF7);
	HAL_Delay(1000);

	lsm_write_reg('m', 0x00, 0x1C);
	HAL_Delay(1000);
	lsm_write_reg('m', 0x01, 0xE0);
	HAL_Delay(1000);
	lsm_write_reg('m', 0x02, 0x00);
	HAL_Delay(1000);
	lsm_write_reg('m', 0x25, 0x00);
	HAL_Delay(1000);    //test sensivity
	HAL_Delay(500);
	//slm_write_reg('m', )

	float maxx = 0.0;
	float minx = 0.0;
	float maxy = 0.0;
	float miny = 0.0;
	while (1) {
		int16_t ax = lsm_read_value('a', LSM303_ACC_X_L_A);
		int16_t ay = lsm_read_value('a', LSM303_ACC_Y_L_A);
		int16_t az = lsm_read_value('a', LSM303_ACC_Z_L_A);

		//nmie -----------------------------------------------

		int16_t pnRawData[3];
		uint8_t ctrlx[2] = { 0, 0 };
		int8_t buffer[8];
		uint8_t i = 0;
		uint16_t sensitivity_xy = 1100;
		uint16_t sensitivity_z = 980;
		uint8_t ctrl = 0x00;
		float degree;
		double convertData[3];

		ctrlx[0] = lsm_read_reg('a', LSM_CTRL_REG4_A);
		ctrlx[1] = lsm_read_reg('m', LSM_CRB_REG_M);

		buffer[0] = lsm_read_reg('m', LSM_OUT_X_L_M);
		buffer[1] = lsm_read_reg('m', LSM_OUT_X_H_M);
		buffer[2] = lsm_read_reg('m', LSM_OUT_Z_L_M);
		buffer[3] = lsm_read_reg('m', LSM_OUT_Z_H_M);
		buffer[4] = lsm_read_reg('m', LSM_OUT_Y_L_M);
		buffer[5] = lsm_read_reg('m', LSM_OUT_Y_H_M);

		ctrl = lsm_read_reg('m', LSM_SR_REG_M);

		if ((ctrlx[0] & LSM_BLE_MSB)) {
			for (i = 0; i < 3; i++) {
				pnRawData[i] = ((int16_t) ((uint16_t) buffer[2 * i + 1] << 8)
						+ buffer[2 * i]);
			}
		} else {
			for (i = 0; i < 3; i++) {
				pnRawData[i] = ((int16_t) ((uint16_t) buffer[2 * i] << 8)
						+ buffer[2 * i + 1]);
			}
		}

		switch (ctrlx[1] & LSM_M_SENSITIVITY_XY_8_1Ga) {
		case LSM_M_SENSITIVITY_XY_1_3Ga:
			sensitivity_xy = 1100;
			sensitivity_z = 980;
			break;
		case LSM_M_SENSITIVITY_XY_1_9Ga:
			sensitivity_xy = 855;
			sensitivity_z = 760;
			break;
		case LSM_M_SENSITIVITY_XY_2_5Ga:
			sensitivity_xy = 670;
			sensitivity_z = 600;
			break;
		case LSM_M_SENSITIVITY_XY_4_0Ga:
			sensitivity_xy = 450;
			sensitivity_z = 400;
			break;
		case LSM_M_SENSITIVITY_XY_4_7Ga:
			sensitivity_xy = 400;
			sensitivity_z = 355;
			break;
		case LSM_M_SENSITIVITY_XY_5_6Ga:
			sensitivity_xy = 330;
			sensitivity_z = 295;
			break;
		case LSM_M_SENSITIVITY_XY_8_1Ga:
			sensitivity_xy = 230;
			sensitivity_z = 205;
			break;
		}

		convertData[0] = (float) pnRawData[0] / sensitivity_xy;
		convertData[1] = (float) pnRawData[1] / sensitivity_z;
		convertData[2] = (float) pnRawData[2] / sensitivity_xy;

		degree = atan2(convertData[2], convertData[0]) * (180 / M_PI);
		if (degree < 0) {
			degree = 360 + degree;
		}

		float accl_x = ax;
		float accl_y = ay;
		float accl_z = az;
		float magn_x = convertData[0];
		float magn_y = convertData[2];
		float magn_z = convertData[1];
		float fXa = ax;
		float fYa = ay;
		float fZa = az;
		//printf("%.4f %.4f %.4f\n", (float)ax, (float)ay, (float)az);
		float fXm = convertData[0];
		float fYm = convertData[2];
		float fZm = convertData[1];

		float roll = atan2(accl_y, accl_z);
		float pitch = atan(-accl_x / (accl_y * sin(roll) + accl_z * cos(roll)));

		float magn_fy_fs = magn_y * cos(roll) - magn_z * sin(roll); //magn_z * sin(roll) + magn_y*cos(roll);
		float magn_fx_fs = magn_x * cos(pitch) + magn_y * sin(pitch) * sin(roll)
				+ magn_z * sin(pitch) * cos(roll);

		float heading = (atan2(magn_fy_fs, magn_fx_fs) * 180.0) / M_PI;
		if (heading < 0)
			heading += 360;
		printf("pitch:%.1f  ", pitch * 180 / M_PI);
		printf("roll:%.1f   ", roll * 180 / M_PI);

		printf("x_raw:%.2f   ", magn_x);
		printf("y_raw:%.2f   ", magn_y);
		printf("z_raw:%.2f   ", magn_z);
		printf("  |||heading_raw:%.0f     H_norm:%.0f", degree, heading);
		printf("   x_norm: %.2f", magn_fx_fs);
		printf("   y_norm: %.2f", magn_fy_fs);
		printf("   z_norm: %.2f",
				-magn_x * cos(pitch) * sin(roll) + magn_y * sin(pitch)
						+ magn_z * cos(pitch) * cos(roll));
		printf("\n");
		HAL_Delay(1000);
	}
}

