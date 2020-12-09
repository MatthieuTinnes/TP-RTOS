#include "main.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
#include "rng.h"

#define FLAG_BOUTON_BLEU 1
#define MSG_SIZE 50
void SystemClock_Config(void);
extern UART_HandleTypeDef huart2;

extern RNG_HandleTypeDef hrng;
osMessageQueueId_t queue;

osMessageQueueAttr_t queue_attr = {.name = "Queue"};
//---------------------------
volatile uint8_t Nombre_Aleatoire_Present=0;
osThreadId_t ID_Thread_1;

osThreadAttr_t Config_Clignotement_Led_Thread = { .name =
		"Clignotement_Led_Thread", .priority = osPriorityLow, .stack_size = 128
		* 4 };

osThreadAttr_t Config_Send_Message_Thread = { .name = "Send_Message_Thread",
		.priority = osPriorityLow, .stack_size = 128 * 4 };

osThreadAttr_t Config_T1_Thread = { .name = "T1_Thread", .priority =
		osPriorityNormal, .stack_size = 128 * 4 };

osThreadAttr_t Config_T2_Thread = { .name = "T2_Thread", .priority =
		osPriorityLow, .stack_size = 128 * 4 };

osThreadAttr_t Config_T3_Thread = { .name = "T3_Thread", .priority =
		osPriorityNormal, .stack_size = 128 * 4 };

void Clignotement_Led_Thread(void *P_Info) {
	while (1) {
		HAL_GPIO_TogglePin(LED_VERTE_GPIO_Port, LED_VERTE_Pin);
		osDelay(500);
	}
	osThreadTerminate(NULL);
}

void Send_Message_queue_Thread(void *P_Info) {
	char Message[MSG_SIZE];
	while (1) {
		osMessageQueueGet(queue,Message,0,osWaitForever);
		printf("%s\r\n", Message);
	}
	osThreadTerminate(NULL);
}

void Generate_Message_random_Thread(void *P_Info) {
	HAL_RNG_GenerateRandomNumber_IT(&hrng);
	char Message[MSG_SIZE];
	uint32_t Valeur_Random;
	while(1){
		if (Nombre_Aleatoire_Present) {
			Valeur_Random = HAL_RNG_ReadLastRandomNumber(&hrng);
			sprintf(Message,"Random : %ld",Valeur_Random);
			osMessageQueuePut(queue,Message,0,osWaitForever);
			Nombre_Aleatoire_Present=0;
			HAL_RNG_GenerateRandomNumber_IT(&hrng);
		}
		HAL_Delay(500);
	}
	osThreadTerminate(NULL);
}

void Generate_Message_button_Thread(void *P_Info) {
	char Message[MSG_SIZE];
	while(1){
		osThreadFlagsWait( FLAG_BOUTON_BLEU, osFlagsWaitAny, HAL_MAX_DELAY);
		uint32_t time = osKernelSysTick();
		sprintf(Message,"Bouton : %ld",time);
		osMessageQueuePut(queue,Message,0,osWaitForever);
	}
	osThreadTerminate(NULL);
}

void HAL_GPIO_EXTI_Callback(uint16_t P_Numero_GPIO)
{
    if (P_Numero_GPIO == BOUTON_BLEU_Pin) {
    	HAL_GPIO_TogglePin(LED_VERTE_GPIO_Port, LED_VERTE_Pin);
    	osThreadFlagsSet(ID_Thread_1,FLAG_BOUTON_BLEU);
    }
}
void HAL_RNG_ReadyDataCallback(RNG_HandleTypeDef *P_Handle, uint32_t P_Valeur)
{
	UNUSED(P_Valeur);
	if (P_Handle== &hrng) {
	Nombre_Aleatoire_Present=1;
	}
}

int main() {
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_RNG_Init();
	osKernelInitialize();
	queue = osMessageQueueNew(50,MSG_SIZE,&queue_attr);

	osThreadNew(Generate_Message_random_Thread, NULL, &Config_T1_Thread);
	osThreadNew(Send_Message_queue_Thread, NULL, &Config_T2_Thread);
	ID_Thread_1 = osThreadNew(Generate_Message_button_Thread, NULL, &Config_T3_Thread);
	printf("Start OS...\r\n");
	osKernelStart();
	while (1)
		;
}

int _write(int P_Flux, char *P_Message, int P_Taille) {
	HAL_StatusTypeDef Etat;
	Etat = HAL_UART_Transmit(&huart2, (uint8_t*) P_Message, P_Taille,
			HAL_MAX_DELAY);
	if (Etat == HAL_OK)
		return P_Taille;
	else
		return -1;
}
