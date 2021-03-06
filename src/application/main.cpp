/**
 ******************************************************************************
 * @file    Projects/Multi/Examples/IKS01A2/DataLogExtended/Src/main.c
 * @author  CL
 * @version V4.0.0
 * @date    1-May-2017
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include <string.h> // strlen
#include <stdio.h>  // snprintf
#include <math.h>   // trunc

#include "service/serial_protocol.h"

#include "rtt/SEGGER_RTT.h"

/** @addtogroup X_NUCLEO_IKS01A2_Examples
 * @{
 */

/** @addtogroup DATALOG_EXTENDED
 * @{
 */

typedef struct displayFloatToInt_s {
	int8_t sign; /* 0 means positive, 1 means negative*/
	uint32_t out_int;
	uint32_t out_dec;
} displayFloatToInt_t;

#define MAX_BUF_SIZE 256

/* Extern variables ----------------------------------------------------------*/
extern volatile uint8_t DataLoggerActive; /*!< DataLogger Flag */
extern UART_HandleTypeDef UartHandle; /*!< UART HANDLE */
extern int use_LSI;
int RTC_SYNCH_PREDIV;

/* Private variables ---------------------------------------------------------*/
char dataOut[MAX_BUF_SIZE]; /*!< DataOut Frame */
RTC_HandleTypeDef RtcHandle; /*!< RTC HANDLE */
volatile uint32_t Sensors_Enabled = 0xFF; /*!< Enable Sensor Flag */
uint32_t Previous_Sensors_Enabled = 0; /*!< Previously Stored Enable Sensor Flag */
volatile uint32_t DataTxPeriod = 1; /*!< TX DATA Period */
volatile uint8_t AutoInit = 1; /*!< Auto Init */
SensorAxes_t ACC_Value; /*!< Acceleration Value */
SensorAxes_t GYR_Value; /*!< Gyroscope Value */
SensorAxes_t MAG_Value; /*!< Magnetometer Value */
float PRESSURE_Value; /*!< Pressure Value */
float HUMIDITY_Value; /*!< Humidity Value */
float TEMPERATURE_Value; /*!< Temperature Value */
volatile uint32_t Int_Current_Time1 = 0; /*!< Int_Current_Time1 Value */
volatile uint32_t Int_Current_Time2 = 0; /*!< Int_Current_Time2 Value */
void *ACCELERO_handle = NULL;
void *GYRO_handle = NULL;
void *MAGNETO_handle = NULL;
void *HUMIDITY_handle = NULL;
void *TEMPERATURE_handle = NULL;
void *PRESSURE_handle = NULL;

uint32_t last_tick = 0; /* create variable to hold previous time */
uint8_t new_data = 0;
uint8_t new_data_flags = 0;

/* Private function prototypes -----------------------------------------------*/
static void RTC_Config(void);
static void RTC_TimeStampConfig(void);

static void initializeAllSensors(void);
static void enableDisableSensors(void);
static void floatToInt(float in, displayFloatToInt_t *out_value,
		int32_t dec_prec);
static void RTC_Handler(TMsg *Msg);
static void Accelero_Sensor_Handler(TMsg *Msg);
static void Gyro_Sensor_Handler(TMsg *Msg);
static void Magneto_Sensor_Handler(TMsg *Msg);
static void Pressure_Sensor_Handler(TMsg *Msg);
static void Humidity_Sensor_Handler(TMsg *Msg);
static void Temperature_Sensor_Handler(TMsg *Msg);

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Main function is to show how to use X_NUCLEO_IKS01A2 expansion board to send data from a Nucleo board
 *         using UART to a connected PC or Desktop and display it on generic applications like
 *         TeraTerm and specific application like Unicleo GUI, which is developed by STMicroelectronics
 *         and provided with a separated package.
 *
 *         After connection has been established:
 *         - the user can view the data from various on-board environment sensors like Temperature, Humidity, and Pressure
 *         - the user can also view data from various on-board MEMS sensors as well like Accelerometer, Gyrometer, and Magnetometer
 *         - the user can also visualize this data as graphs using Sensors_DataLog application provided with this package
 * @param  None
 * @retval Integer
 */
int main(void) {
	TMsg Msg;

	/* STM32F4xx HAL library initialization:
	 - Configure the Flash prefetch, instruction and Data caches
	 - Configure the Systick to generate an interrupt each 1 msec
	 - Set NVIC Group Priority to 4
	 - Global MSP (MCU Support Package) initialization
	 */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize LEDs */
	BSP_LED_Init(LED2);
	BSP_LED_Off(LED2);

	/* Initialize Button */
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);

	/* Initialize RTC */
	RTC_Config();
	RTC_TimeStampConfig();
	initializeAllSensors();

	while (1) {

		if (Previous_Sensors_Enabled != Sensors_Enabled) {
			Previous_Sensors_Enabled = Sensors_Enabled;
			enableDisableSensors();
		}

		RTC_Handler(&Msg);

		if (Sensors_Enabled & PRESSURE_SENSOR) {
			Pressure_Sensor_Handler(&Msg);
		}
		if (Sensors_Enabled & HUMIDITY_SENSOR) {
			Humidity_Sensor_Handler(&Msg);
		}
		if (Sensors_Enabled & TEMPERATURE_SENSOR) {
			Temperature_Sensor_Handler(&Msg);
		}
		if (Sensors_Enabled & ACCELEROMETER_SENSOR) {
			Accelero_Sensor_Handler(&Msg);
		}
		if (Sensors_Enabled & GYROSCOPE_SENSOR) {
			Gyro_Sensor_Handler(&Msg);
		}
		if (Sensors_Enabled & MAGNETIC_SENSOR) {
			Magneto_Sensor_Handler(&Msg);
		}

		if (AutoInit) {
			BSP_LED_Toggle(LED2);
		} else {
			BSP_LED_Off(LED2);
		}

		if (AutoInit) {
			HAL_Delay(500);
		}
	}
}

/**
 * @brief  Initialize all sensors
 * @param  None
 * @retval None
 */
static void initializeAllSensors(void) {
	/* Try to use automatic discovery. By default use LSM6DSL on board */
	BSP_ACCELERO_Init(ACCELERO_SENSORS_AUTO, &ACCELERO_handle);
	/* Try to use automatic discovery. By default use LSM6DSL on board */
	BSP_GYRO_Init(GYRO_SENSORS_AUTO, &GYRO_handle);
	/* Try to use automatic discovery. By default use LSM303AGR on board */
	BSP_MAGNETO_Init(MAGNETO_SENSORS_AUTO, &MAGNETO_handle);
	/* Try to use automatic discovery. By default use HTS221 on board */
	BSP_HUMIDITY_Init(HUMIDITY_SENSORS_AUTO, &HUMIDITY_handle);
	/* Try to use automatic discovery. By default use HTS221 on board */
	BSP_TEMPERATURE_Init(TEMPERATURE_SENSORS_AUTO, &TEMPERATURE_handle);
	/* Try to use automatic discovery. By default use LPS22HB on board */
	BSP_PRESSURE_Init(PRESSURE_SENSORS_AUTO, &PRESSURE_handle);
}

/**
 * @brief  Enable/disable desired sensors
 * @param  None
 * @retval None
 */
static void enableDisableSensors(void) {
	if (Sensors_Enabled & PRESSURE_SENSOR) {
		BSP_PRESSURE_Sensor_Enable(PRESSURE_handle);
	} else {
		BSP_PRESSURE_Sensor_Disable(PRESSURE_handle);
	}

	if (Sensors_Enabled & HUMIDITY_SENSOR) {
		BSP_HUMIDITY_Sensor_Enable(HUMIDITY_handle);
	} else {
		BSP_HUMIDITY_Sensor_Disable(HUMIDITY_handle);
	}

	if (Sensors_Enabled & TEMPERATURE_SENSOR) {
		BSP_TEMPERATURE_Sensor_Enable(TEMPERATURE_handle);
	} else {
		BSP_TEMPERATURE_Sensor_Disable(TEMPERATURE_handle);
	}

	if (Sensors_Enabled & ACCELEROMETER_SENSOR) {
		BSP_ACCELERO_Sensor_Enable(ACCELERO_handle);
	} else {
		BSP_ACCELERO_Sensor_Disable(ACCELERO_handle);
	}

	if (Sensors_Enabled & GYROSCOPE_SENSOR) {
		BSP_GYRO_Sensor_Enable(GYRO_handle);
	} else {
		BSP_GYRO_Sensor_Disable(GYRO_handle);
	}

	if (Sensors_Enabled & MAGNETIC_SENSOR) {
		BSP_MAGNETO_Sensor_Enable(MAGNETO_handle);
	} else {
		BSP_MAGNETO_Sensor_Disable(MAGNETO_handle);
	}
}

/**
 * @brief  Splits a float into two integer values.
 * @param  in the float value as input
 * @param  out_value the pointer to the output integer structure
 * @param  dec_prec the decimal precision to be used
 * @retval None
 */
static void floatToInt(float in, displayFloatToInt_t *out_value,
		int32_t dec_prec) {
	if (in >= 0.0f) {
		out_value->sign = 0;
	} else {
		out_value->sign = 1;
		in = -in;
	}

	out_value->out_int = (int32_t) in;
	in = in - (float) (out_value->out_int);
	out_value->out_dec = (int32_t) trunc(in * pow(10, dec_prec));
}

/**
 * @brief  Handles the time+date getting/sending
 * @param  Msg the time+date part of the stream
 * @retval None
 */
static void RTC_Handler(TMsg *Msg) {
	uint8_t subSec = 0;
	RTC_DateTypeDef sdatestructureget;
	RTC_TimeTypeDef stimestructure;

	if (AutoInit) {
		HAL_RTC_GetTime(&RtcHandle, &stimestructure, FORMAT_BIN);
		HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, FORMAT_BIN);
		subSec = ((((((int) RTC_SYNCH_PREDIV)
				- ((int) stimestructure.SubSeconds)) * 100)
				/ (RTC_SYNCH_PREDIV + 1)) & 0xff);
	}

	if (AutoInit) {
		SEGGER_RTT_printf(0, "TimeStamp: %d:%d:%d.%d\r\n", stimestructure.Hours,
				stimestructure.Minutes, stimestructure.Seconds, subSec);
	}
}

/**
 * @brief  Handles the ACCELERO axes data getting/sending
 * @param  Msg the ACCELERO part of the stream
 * @retval None
 */
static void Accelero_Sensor_Handler(TMsg *Msg) {
	int32_t data[6];
	uint8_t status = 0;
	uint8_t drdy = 0;

	if (BSP_ACCELERO_IsInitialized(ACCELERO_handle, &status) == COMPONENT_OK
			&& status == 1) {
		BSP_ACCELERO_Get_DRDY_Status(ACCELERO_handle, &drdy);

		if (drdy != 0) {
			new_data++;
			new_data_flags |= 1;

			BSP_ACCELERO_Get_Axes(ACCELERO_handle, &ACC_Value);

			if (AutoInit) {
				data[0] = ACC_Value.AXIS_X;
				data[1] = ACC_Value.AXIS_Y;
				data[2] = ACC_Value.AXIS_Z;

				SEGGER_RTT_printf(0, "ACC_X: %d, ACC_Y: %d, ACC_Z: %d\r\n",
						(int) data[0], (int) data[1], (int) data[2]);
			}
		}
	}
}

/**
 * @brief  Handles the GYRO axes data getting/sending
 * @param  Msg the GYRO part of the stream
 * @retval None
 */
static void Gyro_Sensor_Handler(TMsg *Msg) {
	int32_t data[6];
	uint8_t status = 0;
	uint8_t drdy = 0;

	if (BSP_GYRO_IsInitialized(GYRO_handle, &status) == COMPONENT_OK
			&& status == 1) {
		BSP_GYRO_Get_DRDY_Status(GYRO_handle, &drdy);

		if (drdy != 0) {
			new_data++;
			new_data_flags |= 2;

			BSP_GYRO_Get_Axes(GYRO_handle, &GYR_Value);

			if (AutoInit) {
				data[3] = GYR_Value.AXIS_X;
				data[4] = GYR_Value.AXIS_Y;
				data[5] = GYR_Value.AXIS_Z;

				SEGGER_RTT_printf(0, "GYR_X: %d, GYR_Y: %d, GYR_Z: %d\r\n",
						(int) data[3], (int) data[4], (int) data[5]);
			}
		}
	}
}

/**
 * @brief  Handles the MAGNETO axes data getting/sending
 * @param  Msg the MAGNETO part of the stream
 * @retval None
 */
static void Magneto_Sensor_Handler(TMsg *Msg) {
	int32_t data[3];
	uint8_t status = 0;
	uint8_t drdy = 0;

	if (BSP_MAGNETO_IsInitialized(MAGNETO_handle, &status) == COMPONENT_OK
			&& status == 1) {
		BSP_MAGNETO_Get_DRDY_Status(MAGNETO_handle, &drdy);

		if (drdy != 0) {
			new_data++;
			new_data_flags |= 4;

			BSP_MAGNETO_Get_Axes(MAGNETO_handle, &MAG_Value);

			if (AutoInit) {
				data[0] = MAG_Value.AXIS_X;
				data[1] = MAG_Value.AXIS_Y;
				data[2] = MAG_Value.AXIS_Z;

				SEGGER_RTT_printf(0, "MAG_X: %d, MAG_Y: %d, MAG_Z: %d\r\n",
						(int) data[0], (int) data[1], (int) data[2]);
			}
		}
	}
}

/**
 * @brief  Handles the PRESSURE sensor data getting/sending
 * @param  Msg the PRESSURE part of the stream
 * @retval None
 */
static void Pressure_Sensor_Handler(TMsg *Msg) {
	uint8_t status = 0;
	uint8_t drdy = 0;

	if (BSP_PRESSURE_IsInitialized(PRESSURE_handle, &status) == COMPONENT_OK
			&& status == 1) {
		BSP_PRESSURE_Get_DRDY_Status(PRESSURE_handle, &drdy);

		if (drdy != 0) {
			new_data++;
			new_data_flags |= 8;

			BSP_PRESSURE_Get_Press(PRESSURE_handle, &PRESSURE_Value);

			if (AutoInit) {
				displayFloatToInt_t out_value;
				floatToInt(PRESSURE_Value, &out_value, 2);

				SEGGER_RTT_printf(0, "PRESS: %d.%02d\r\n",
						(int) out_value.out_int, (int) out_value.out_dec);
			}
		}
	}
}

/**
 * @brief  Handles the HUMIDITY sensor data getting/sending
 * @param  Msg the HUMIDITY part of the stream
 * @retval None
 */
static void Humidity_Sensor_Handler(TMsg *Msg) {
	uint8_t status = 0;
	uint8_t drdy = 0;

	if (BSP_HUMIDITY_IsInitialized(HUMIDITY_handle, &status) == COMPONENT_OK
			&& status == 1) {
		BSP_HUMIDITY_Get_DRDY_Status(HUMIDITY_handle, &drdy);

		if (drdy != 0) {
			new_data++;
			new_data_flags |= 16;

			BSP_HUMIDITY_Get_Hum(HUMIDITY_handle, &HUMIDITY_Value);

			if (AutoInit) {
				displayFloatToInt_t out_value;
				floatToInt(HUMIDITY_Value, &out_value, 2);

				SEGGER_RTT_printf(0, "HUM: %d.%02d\r\n",
						(int) out_value.out_int, (int) out_value.out_dec);
			}
		}
	}
}

/**
 * @brief  Handles the TEMPERATURE sensor data getting/sending
 * @param  Msg the TEMPERATURE part of the stream
 * @retval None
 */
static void Temperature_Sensor_Handler(TMsg *Msg) {
	uint8_t status = 0;
	uint8_t drdy = 0;

	if (BSP_TEMPERATURE_IsInitialized(TEMPERATURE_handle, &status)
			== COMPONENT_OK && status == 1) {
		BSP_TEMPERATURE_Get_DRDY_Status(TEMPERATURE_handle, &drdy);

		if (drdy != 0) {
			new_data++;
			new_data_flags |= 32;

			BSP_TEMPERATURE_Get_Temp(TEMPERATURE_handle, &TEMPERATURE_Value);

			if (AutoInit) {
				displayFloatToInt_t out_value;
				floatToInt(TEMPERATURE_Value, &out_value, 2);

				SEGGER_RTT_printf(0, "TEMP: %c%d.%02d\r\n",
						((out_value.sign) ? '-' : '+'), (int) out_value.out_int,
						(int) out_value.out_dec);
			}
		}
	}
}

/**
 * @brief  Configures the RTC
 * @param  None
 * @retval None
 */
static void RTC_Config(void) {
	/*##-1- Configure the RTC peripheral #######################################*/
	/* Check if LSE can be used */
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/*##-1- Configue LSE as RTC clock soucre ###################################*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI
			| RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		/* LSE not available, we use LSI */
		use_LSI = 1;
		RtcHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV_LSI;
		RtcHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV_LSI;
		RTC_SYNCH_PREDIV = RTC_SYNCH_PREDIV_LSI;
	} else {
		/* We use LSE */
		use_LSI = 0;
		RtcHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV_LSE;
		RtcHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV_LSE;
		RTC_SYNCH_PREDIV = RTC_SYNCH_PREDIV_LSE;
	}
	RtcHandle.Instance = RTC;

	/* Configure RTC prescaler and RTC data registers */
	/* RTC configured as follow:
	 - Hour Format    = Format 12
	 - Asynch Prediv  = Value according to source clock
	 - Synch Prediv   = Value according to source clock
	 - OutPut         = Output Disable
	 - OutPutPolarity = High Polarity
	 - OutPutType     = Open Drain */
	RtcHandle.Init.HourFormat = RTC_HOURFORMAT_12;
	RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
	RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

	if (HAL_RTC_Init(&RtcHandle) != HAL_OK) {
		/* Initialization Error */
		Error_Handler();
	}
}

/**
 * @brief  Configures the current time and date
 * @param  None
 * @retval None
 */
static void RTC_TimeStampConfig(void) {
	RTC_DateTypeDef sdatestructure;
	RTC_TimeTypeDef stimestructure;

	/*##-3- Configure the Date #################################################*/
	/* Set Date: Tuesday February 18th 2014 */
	sdatestructure.Year = 0x14;
	sdatestructure.Month = RTC_MONTH_FEBRUARY;
	sdatestructure.Date = 0x18;
	sdatestructure.WeekDay = RTC_WEEKDAY_TUESDAY;

	if (HAL_RTC_SetDate(&RtcHandle, &sdatestructure, FORMAT_BCD) != HAL_OK) {
		/* Initialization Error */
		Error_Handler();
	}

	/*##-4- Configure the Time #################################################*/
	/* Set Time: 08:10:00 */
	stimestructure.Hours = 0x08;
	stimestructure.Minutes = 0x10;
	stimestructure.Seconds = 0x00;
	stimestructure.TimeFormat = RTC_HOURFORMAT12_AM;
	stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

	if (HAL_RTC_SetTime(&RtcHandle, &stimestructure, FORMAT_BCD) != HAL_OK) {
		/* Initialization Error */
		Error_Handler();
	}
}

/**
 * @brief  EXTI line detection callbacks
 * @param  GPIO_Pin the pin connected to EXTI line
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == KEY_BUTTON_PIN) {
		/* Manage software debouncing*/
		int doOperation = 0;

		if (Int_Current_Time1 == 0 && Int_Current_Time2 == 0) {
			Int_Current_Time1 = user_currentTimeGetTick();
			doOperation = 1;
		} else {
			int i2;
			Int_Current_Time2 = user_currentTimeGetTick();
			i2 = Int_Current_Time2;

			/* If I receive a button interrupt after more than 300 ms from the first one I get it, otherwise I discard it */
			if ((i2 - Int_Current_Time1) > 300) {
				Int_Current_Time1 = Int_Current_Time2;
				doOperation = 1;
			}
		}

		if (doOperation) {
			AutoInit = (AutoInit) ? 0 : 1; /* toggle on each button pressed */
		}
	}
}

/**
 * @brief  Configures the current time and date
 * @param  hh the hour value to be set
 * @param  mm the minute value to be set
 * @param  ss the second value to be set
 * @retval None
 */
void RTC_TimeRegulate(uint8_t hh, uint8_t mm, uint8_t ss) {

	RTC_TimeTypeDef stimestructure;

	stimestructure.TimeFormat = RTC_HOURFORMAT12_AM;
	stimestructure.Hours = hh;
	stimestructure.Minutes = mm;
	stimestructure.Seconds = ss;
	stimestructure.SubSeconds = 0;
	stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

	if (HAL_RTC_SetTime(&RtcHandle, &stimestructure, FORMAT_BIN) != HAL_OK) {
		/* Initialization Error */
		Error_Handler();
	}
}

/**
 * @brief  This function is executed in case of error occurrence
 * @param  None
 * @retval None
 */
void Error_Handler(void) {
	while (1) {
	}
}

/**
 * @brief  Get the current tick value in millisecond
 * @param  None
 * @retval The tick value
 */
uint32_t user_currentTimeGetTick(void) {
	return HAL_GetTick();
}

/**
 * @brief  Get the delta tick value in millisecond from Tick1 to the current tick
 * @param  Tick1 the reference tick used to compute the delta
 * @retval The delta tick value
 */
uint32_t user_currentTimeGetElapsedMS(uint32_t Tick1) {
	volatile uint32_t Delta, Tick2;

	Tick2 = HAL_GetTick();

	/* Capture computation */
	Delta = Tick2 - Tick1;
	return Delta;
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number where the assert_param error has occurred
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line) {
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1) {
	}
}
#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
