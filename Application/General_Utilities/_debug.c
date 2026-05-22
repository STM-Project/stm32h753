/*
 * debug.c
 *
 *  Created on: 02.04.2021
 *      Author: Elektronika RM
 */

#include <_debug.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"
#include "mini_printf.h"
#include "FreeRTOS.h"
#include "task.h"

#define RECV_BUFF_SIZE	128
#define SEND_BUFF_SIZE	1024		/* musi byc wiekszy lub rowny dlugosci wyslanej za jednym razem */
#define DEBUG_DEBUG		1

RAM_D2_ALIGN32 static char dbgRecvBuffer[RECV_BUFF_SIZE];
RAM_D2_ALIGN32 static char dbgSendBuffer[SEND_BUFF_SIZE];

/*static*/ volatile uint16_t dbg_head = 0;		/* Uzywaj volatile jesli sa modyfikowane w przerwaniach i korzystane w petli w watku */
volatile uint16_t dbg_tail = 0;
volatile uint8_t dbg_dma_busy = 0;

void DEBUG_Init(void)
{
	memset(dbgRecvBuffer,0,RECV_BUFF_SIZE);
	DEBUG_ReceiveStart((uint8_t*)dbgRecvBuffer, RECV_BUFF_SIZE);
}

void DBG_EndSendInterrupt(void)
{
	if (dbg_head != dbg_tail){
        if (dbg_head > dbg_tail){	DEBUG_SendDma( (uint8_t*)&dbgSendBuffer[dbg_tail], dbg_head-dbg_tail 	   );	 dbg_tail = dbg_head;	}	/* to nic NIE szkodzi ze w przerwaniu 'dbg_tail' modyfikuje a w watku odczyuje i na odwrot z inna zmienna */
        else					{	DEBUG_SendDma( (uint8_t*)&dbgSendBuffer[dbg_tail], SEND_BUFF_SIZE-dbg_tail );	 dbg_tail = 0;			}
    }
	else{
		dbg_dma_busy = 0;
		/* Give SemaphorFromISR (in future) */
	}
}
												/* WAZNE: Wersja ta z buforem kołowym bedzie używana tylko do szczególowego debagowania dla wybranego watku, NIGDY wlaczona na stałe aby mogly wiecej watkow naraz korzystac ze szczegolowego debagowania. Do logów z roznych wątków w przyszlosci bedzie uruchomiony nowy dedykowany temu wątek obsługujacy kolejki RTOS */
static void DbgSendDma(char *txt)				/* funkcja ta wywolywana z roznych watkow, trzeba zastosowac mutex i semafor, ktory jest zwalniany w przerwaniu przy wyjsciu a najlepiej zastosowac kolejke (dla logow), ktora jest obslugiwana w osobnym watku */
{												/* Jeśli zablokujesz Mutex, a potem uśpisz wątek semaforem oczekującym na koniec DMA, zablokujesz możliwość logowania innym wątkom na bardzo długi czas (czas transmisji UART/DMA). Logowanie stanie się operacją blokującą, co przeczy idei używania bufora kołowego i DMA */
	/* Take Mutex (in future) */
    while (*txt){								/* zapis do bufora kolowego */
        uint16_t next_head = (dbg_head + 1) % SEND_BUFF_SIZE;
        if (next_head == dbg_tail)
        	vTaskDelay(1);						/* bufor pelny, niedapisujemy, mozemy tez poczekac az zwolni sie bufor w DBG_EndSendInterrupt() */
        dbgSendBuffer[dbg_head] = *txt++;
        dbg_head = next_head;
    }

    if (!dbg_dma_busy && dbg_head != dbg_tail){		/* Ta czesc jes wykonywana gdy na 100% nie przyjdzie przerwania DBG_EndSendInterrupt() wiec mozemy modygikowac 'dbg_tail' */
        dbg_dma_busy = 1;
        if (dbg_head > dbg_tail){	DEBUG_SendDma( (uint8_t*)&dbgSendBuffer[dbg_tail], dbg_head-dbg_tail 	   );	 dbg_tail = dbg_head;	}
        else					{	DEBUG_SendDma( (uint8_t*)&dbgSendBuffer[dbg_tail], SEND_BUFF_SIZE-dbg_tail );	 dbg_tail = 0;			} 		/* Reszta danych pójdzie w callbacku */
    }
    /* Wait for Semaphor (in future) */		/* Tu zasyiam i oddaje czas innym watkom */
    /* Give Mutex 		 (in future) */
}

void DbgDma(int on, char *txt)
{
	if(on)
		DbgSendDma(txt);
}

void Dbg(int on, char *txt)
{
	if(on)
		DEBUG_Send(txt);
}

void DbgMulti(int on, char *startTxt, char *txt, char *endTxt)
{
	if(on)
	{
		DEBUG_Send(startTxt);
		DEBUG_Send(txt);
		DEBUG_Send(endTxt);
	}
}

void DbgMultiDma(int on, char *startTxt, char *txt, char *endTxt)
{
	if(on)
	{
		DbgSendDma(startTxt);
		DbgSendDma(txt);
		DbgSendDma(endTxt);
	}
}

void DbgVar(int on, unsigned int buffLen, const char *fmt, ...)
{
	if(on)
	{
		char *temp = (char*)pvPortMalloc(buffLen);
		va_list va;
		va_start(va, fmt);
		mini_vsnprintf(temp, buffLen, fmt, va);
		va_end(va);
		DEBUG_Send(temp);
		vPortFree(temp);
	}
}

void DbgVarDma(int on, unsigned int buffLen, const char *fmt, ...)
{
	if(on)
	{
		char *temp = (char*)pvPortMalloc(buffLen);
		va_list va;
		va_start(va, fmt);
		mini_vsnprintf(temp, buffLen, fmt, va);
		va_end(va);
		DbgSendDma(temp);
		vPortFree(temp);
	}
}

void DbgVar2(int on, unsigned int buffLen, const char *fmt, ...)
{
	if(on)
	{
		char *temp = (char*)pvPortMalloc(buffLen);
		va_list va;
		va_start(va, fmt);
		vsnprintf(temp,buffLen, fmt, va);			/* W niektórych implementacjach standardowej biblioteki C (szczególnie pełnej newlib), vsnprintf może pod spodem niejawnie wywołać malloc(). W systemie wielowątkowym klasyczny malloc bez odpowiednich wrapperów nitkowych (np. malloc_r lub mutexów) nie jest bezpieczny i prowadzi do uszkodzenia pamięci heap */
		va_end(va);
		DEBUG_Send(temp);
		vPortFree(temp);
	}
}

void DbgVarDma2(int on, unsigned int buffLen, const char *fmt, ...)
{
	if(on)
	{
		char *temp = (char*)pvPortMalloc(buffLen);
		va_list va;
		va_start(va, fmt);
		vsnprintf(temp,buffLen, fmt, va);
		va_end(va);
		DbgSendDma(temp);
		vPortFree(temp);
	}
}

static int DEBUG_IsAnythingReceive(void)
{
	if(dbgRecvBuffer[0]>0)
		return 1;
	else
		return 0;
}

void DEBUG_InvalidateDCache(void){
	SCB_InvalidateDCache_by_Addr((uint32_t*)dbgRecvBuffer, RECV_BUFF_SIZE);
}

int DEBUG_IsTxtReceive(char *txt)
{
	if(strstr(dbgRecvBuffer,txt))
	{
		DEBUG_ReceiveStop();
		DEBUG_ReceiveStart((uint8_t*)dbgRecvBuffer, RECV_BUFF_SIZE);
		return 1;
	}
	return 0;
}

void DEBUG_RxFullBuffService(void)
{
	Dbg(DEBUG_DEBUG,"\r\n -----  DEBUG_RxFullBuffService -------  ");
	DEBUG_ReceiveStop();
	DEBUG_ReceiveStart((uint8_t*)dbgRecvBuffer, RECV_BUFF_SIZE);
}

int DEBUG_RcvStr(char *txt)
{
	if(DEBUG_IsAnythingReceive())
	{
	  if(DEBUG_IsTxtReceive(txt))
		  return 1;
	  else
		  return 0;
	}
	else
		return 0;
}

char* _Col(FONT_BKG_COLOR background, uint8_t red, uint8_t green, uint8_t blue)
{
	#define	SIZE_TAB			MAX_SIZE_TXT * 10
	#define	MAX_SIZE_TXT	20

	static char tab[SIZE_TAB]={0};
	static int i=0;
	uint8_t fontBkg;
	int i_copy;

	if(i + MAX_SIZE_TXT >= SIZE_TAB-1)
		i=0;
	i_copy = i;

	switch(background){
		case font:
			fontBkg=38;
			break;
		default:
			fontBkg=48;
			break;
	}
	i += mini_snprintf(&tab[i],MAX_SIZE_TXT,"\x1b[%d;2;%d;%d;%dm",fontBkg,red,green,blue);
	tab[i++]=0;
	return &tab[i_copy];
}

void* DEBUG_TestFunction(void *a, DATA_TYPE dataType, DATA_ACTION dataAction, void *step, void *min, void *max, char *descr, VOID_FUNCTION_TEST xfunc, VOID_FUNCTION_TEST xfunc2){

	#define  _OPERAT(type)\
		if(_Incr==dataAction) INCR( *((type*)a), *((type*)step), *((type*)max));\
		if(_Decr==dataAction) DECR( *((type*)a), *((type*)step), *((type*)max));\
		if(_Wrap==dataAction) INCR_WRAP( *((type*)a), *((type*)step), *((type*)min), *((type*)max));\
		if(_float==dataType||_double==dataType) DbgVar(1,50,"\r\n%s: %s ",descr,Float2Str(*((type*)a),' ',1,Sign_none,1));\
		else 												 DbgVar(1,50,"\r\n%s: %d ",descr, *((type*)a));\
		if(xfunc !=NULL) xfunc (NULL,NULL);	/* in future use */
		if(xfunc2!=NULL) xfunc2(NULL,NULL);

	switch((int)dataType){
		case _int:		_OPERAT(int) 	  break;
		case _int8:		_OPERAT(int8_t)   break;
		case _int16:	_OPERAT(int16_t)  break;
		case _int32:	_OPERAT(int32_t)  break;
		case _uint8:	_OPERAT(uint8_t)  break;
		case _uint16:	_OPERAT(uint16_t) break;
		case _uint32:	_OPERAT(uint32_t) break;
		case _float:	_OPERAT(float)    break;
		case _double:	_OPERAT(double)   break;
	}
	return (void*)(0);

	#undef _OPERAT
}

/* --- Najbezpiecznaiejsza wersja logów BEZ sekcji krytycznych  --- */
/*
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define LOG_QUEUE_SIZE 20
QueueHandle_t xLogQueue = NULL;
TaskHandle_t xLogTaskHandle = NULL; // Uchwyt wątku, potrzebny dla przerwania

// 1. Funkcja wywoływana z dowolnego wątku (Thread-Safe)
void DbgSend(const char *txt)		// DbgSend("Text example") - takie wywolania z wielu watkow nie zatraci bufora bo sa one przechowywane we flashu i wskaznik do nich zawsze istnieje.
{
    if (xLogQueue != NULL) {
        xQueueSend(xLogQueue, &txt, portMAX_DELAY);
    }
}

// 2. Dedykowany wątek obsługi logów
void vLogTask(void *pvParameters)
{
    const char *txt_ptr;
    xLogQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(char *));
    xLogTaskHandle = xTaskGetCurrentTaskHandle();

    for (;;) {
        if (xQueueReceive(xLogQueue, &txt_ptr, portMAX_DELAY) == pdTRUE) {

            // 1. ZAPAMIĘTUJEMY STAN: Czy przed dodaniem TEKSTU bufor był pusty?
            // Jeśli dbg_head == dbg_tail, to znaczy, że DMA na pewno stoi.
            BaseType_t dma_was_idle = (dbg_head == dbg_tail);

            // 2. WPISUJEMY CAŁY STRING DO BUFORA KOŁOWEGO (Wiele bajtów)
            while (*txt_ptr) {
                uint16_t next_head = (dbg_head + 1) % SEND_BUFF_SIZE;

                if (next_head == dbg_tail) {
                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                    continue;
                }

                dbgSendBuffer[dbg_head] = *txt_ptr++;
                dbg_head = next_head;
            }

            // 3. DOPIERO PO WPISANIU CAŁEGO TEKSTU DECYDUJEMY O DMA
            // Jeśli przed pętlą bufor był pusty (dma_was_idle), to teraz, po wpisaniu
            // całego stringa, odpalamy DMA RAZ dla całej zgromadzonej paczki danych.
            if (dma_was_idle && (dbg_head != dbg_tail)) {
                if (dbg_head > dbg_tail) {
                    DEBUG_SendDma((uint8_t*)&dbgSendBuffer[dbg_tail], dbg_head - dbg_tail);
                    dbg_tail = dbg_head;
                } else {
                    DEBUG_SendDma((uint8_t*)&dbgSendBuffer[dbg_tail], SEND_BUFF_SIZE - dbg_tail);
                    dbg_tail = 0;
                }
            }
            // Jeśli dma_was_idle było fałszywe, to znaczy, że DMA już działało w tle
            // i przerwanie samo zgarnie nowo dopisane znaki. Nic tu nie robimy.
        }
    }
}

// 3. Callback z przerwania DMA (ISR)
void DBG_EndSendInterrupt(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Przerwanie sprawdza, czy w buforze kołowym są kolejne dane do wysłania
    if (dbg_head != dbg_tail) {
        if (dbg_head > dbg_tail) {
            DEBUG_SendDma((uint8_t*)&dbgSendBuffer[dbg_tail], dbg_head - dbg_tail);
            dbg_tail = dbg_head;
        } else {
            DEBUG_SendDma((uint8_t*)&dbgSendBuffer[dbg_tail], SEND_BUFF_SIZE - dbg_tail);
            dbg_tail = 0;
        }

        // Ponieważ właśnie przesunęliśmy dbg_tail (zwolniliśmy miejsce),
        // wysyłamy sygnał wybudzenia do wątku logującego na wypadek, gdyby spał z powodu pełnego bufora.
        if (xLogTaskHandle != NULL) {
            vTaskNotifyGiveFromISR(xLogTaskHandle, &xHigherPriorityTaskWoken);
        }
    } else {
        // Bufor jest pusty. Nie robimy nic. DMA odpocznie, dopóki wątek vLogTask
        // nie wpisze nowego znaku i sam go ponownie nie uruchomi.
    }

    // Jeśli wybudzony wątek ma wyższy priorytet, przełącz kontekst od razu
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
*/

