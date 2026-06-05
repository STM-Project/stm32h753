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
#include "queue.h"

#define LOG_QUEUE_SIZE 	20
#define HEAP_MAX_ALLOC_BYTE	 2048

#define RECV_BUFF_SIZE	128
#define SEND_BUFF_SIZE	1024				/* musi byc wiekszy lub rowny dlugosci wyslanej za jednym razem */

#define RECV_BUFF_SIZE_QUE	128
#define SEND_BUFF_SIZE_QUE	1024			/* musi byc wiekszy lub rowny dlugosci wyslanej za jednym razem */

RAM_D2_ALIGN32 static char dbgRecvBuffer[RECV_BUFF_SIZE];
RAM_D2_ALIGN32 static char dbgSendBuffer[SEND_BUFF_SIZE];

/* RAM_D2_ALIGN32 static char dbgRecvBuffQue[RECV_BUFF_SIZE_QUE]; */
RAM_D2_ALIGN32 static char dbgSendBuffQue[SEND_BUFF_SIZE_QUE];

static volatile uint16_t dbg_head = 0;			/* Uzywaj volatile jesli sa modyfikowane w przerwaniach i korzystane w petli w watku */
static volatile uint16_t dbg_tail = 0;
static volatile uint8_t dbg_dma_busy = 0;

static volatile uint16_t dbg_head_que = 0;
static volatile uint16_t dbg_tail_que = 0;
static volatile uint8_t dbg_dma_busy_que = 0;

QueueHandle_t xLogQueue = NULL;
TaskHandle_t xLogTaskHandle = NULL;

void DEBUG_Init(void)
{
	memset(dbgRecvBuffer,0,RECV_BUFF_SIZE);
	DEBUG_ReceiveStart((uint8_t*)dbgRecvBuffer, RECV_BUFF_SIZE);
}

static void BuffCirc_SendData(void){			/*  Kompilator (GCC) optymalizuje kod. Jeśli widzi pętlę while(dbg_dma_busy == 1), a wewnątrz pętli wątek nie modyfikuje tej zmiennej, kompilator dla szybkości zapisze sobie wartość tej flagi w wewnętrznym rejestrze procesora i nigdy nie odczyta jej ponownie z pamięci RAM. Słowo volatile nakazuje procesorowi: "Zawsze odczytuj tę zmienną bezpośrednio z fizycznej pamięci RAM, bo ktoś inny (np. przerwanie) może ją zmienić w tle". */
	if ((dbg_head != dbg_tail)){
		dbg_dma_busy = 1;
	    if (dbg_head > dbg_tail){	DEBUG_SendDma( (uint8_t*)&dbgSendBuffer[dbg_tail], dbg_head - dbg_tail 		 );	 	dbg_tail = dbg_head;	}		/* to nic NIE szkodzi ze w przerwaniu 'dbg_tail' modyfikuje a w watku odczyuje i na odwrot z inna zmienna */
	    else					{	DEBUG_SendDma( (uint8_t*)&dbgSendBuffer[dbg_tail], SEND_BUFF_SIZE - dbg_tail );	 	dbg_tail = 0;			} 		/* Reszta danych pójdzie w callbacku */
	}
	else dbg_dma_busy = 0;
}

void DBG_EndSendInterrupt(void)
{
	if (dbg_head != dbg_tail) BuffCirc_SendData();
	else{					  dbg_dma_busy = 0;
		/* Give SemaphorFromISR (in future) */
	}
}
												/* WAZNE: Wersja ta z buforem kołowym bedzie używana tylko do szczególowego debagowania dla wybranego watku, NIGDY wlaczona na stałe aby mogly wiecej watkow naraz korzystac ze szczegolowego debagowania. Do logów z roznych wątków w przyszlosci bedzie uruchomiony nowy dedykowany temu wątek obsługujacy kolejki RTOS */
static void DbgSendDma___(char *txt)			/* funkcja ta wywolywana z roznych watkow, trzeba zastosowac mutex i semafor, ktory jest zwalniany w przerwaniu przy wyjsciu a najlepiej zastosowac kolejke (dla logow), ktora jest obslugiwana w osobnym watku */
{												/* Jeśli zablokujesz Mutex, a potem uśpisz wątek semaforem oczekującym na koniec DMA, zablokujesz możliwość logowania innym wątkom na bardzo długi czas (czas transmisji UART/DMA). Logowanie stanie się operacją blokującą, co przeczy idei używania bufora kołowego i DMA */
	/* Take Mutex (in future) */
    while (*txt){								/* zapis do bufora kolowego */
        uint16_t next_head = (dbg_head + 1) % SEND_BUFF_SIZE;
        if (next_head == dbg_tail){				/* bufor pelny, niedapisujemy, mozemy tez poczekac az zwolni sie bufor w DBG_EndSendInterrupt() ale my jednak obserwujemy zawieszenie sie przerwania TX dlatego musimy je odblokowac wysylajac tu dane */
        	vTaskDelay(10);						/* przy 2000000 Mb/s, zakladamy 8 bajtów + 2 bajty kontrolne, mamy 1000 bajtów UART7 wysyla w ciągu 5ms wiec dajmy pewnosc opróżnienia całego bufora nadawczego wciągu: (SEND_BUFF_SIZE*5[ms])/1000[B]. Na wszelki wypadek dajmy np 10ms opóznienia i jeśli w tym czasie bufor nie zostanie opróżniony stwierdzamy uszkodzenie przerwania HAL_UART_TxCpltCallback() i inicjalizujemy HAL_UART_Transmit_DMA() żeby odblokowac przerwanie i opróżnic bufor. */
        	if(next_head == dbg_tail)
        		BuffCirc_SendData();			/* wchodzac w ten warunek stwierdzamy że dbg_dma_busy=1 */
        }
        dbgSendBuffer[dbg_head] = *txt++;
        dbg_head = next_head;
    }
    if (dbg_dma_busy==0) BuffCirc_SendData();		/* Ta czesc jes wykonywana gdy na 100% nie przyjdzie przerwania DBG_EndSendInterrupt() wiec mozemy modygikowac 'dbg_tail' */

    /* Wait for Semaphor (in future) */		/* Tu zasyiam i oddaje czas innym watkom */
    /* Give Mutex 		 (in future) */
}

void DbgDmaQue(int on, char *txt, int lenTxt);

static void DbgSendDma(char *txt)				/* funkcja ta wywolywana z roznych watkow, trzeba zastosowac mutex i semafor, ktory jest zwalniany w przerwaniu przy wyjsciu a najlepiej zastosowac kolejke (dla logow), ktora jest obslugiwana w osobnym watku */
{
	DbgDmaQue(1,txt,0);
}

void DbgDma(int on, char *txt)
{
	if(on)
		DbgSendDma(txt);
}

void DbgDma2(int on, char *txt, int len)
{
	DbgDmaQue(on,txt,len);
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
		char temp[buffLen];
		va_list va;
		va_start(va, fmt);
		mini_vsnprintf(temp, buffLen, fmt, va);
		va_end(va);
		DEBUG_Send(temp);
	}
}

void DbgVarDma(int on, unsigned int buffLen, const char *fmt, ...)
{
	if(on)
	{
		char temp[buffLen];
		va_list va;
		va_start(va, fmt);
		mini_vsnprintf(temp, buffLen, fmt, va);
		va_end(va);
		DbgSendDma(temp);
	}
}

void DbgVar2(int on, unsigned int buffLen, const char *fmt, ...)
{
	if(on)
	{
		char temp[buffLen];
		va_list va;
		va_start(va, fmt);
		vsnprintf(temp,buffLen, fmt, va);			/* W niektórych implementacjach standardowej biblioteki C (szczególnie pełnej newlib), vsnprintf może pod spodem niejawnie wywołać malloc(). W systemie wielowątkowym klasyczny malloc bez odpowiednich wrapperów nitkowych (np. malloc_r lub mutexów) nie jest bezpieczny i prowadzi do uszkodzenia pamięci heap */
		va_end(va);
		DEBUG_Send(temp);
	}
}

void DbgVarDma2(int on, unsigned int buffLen, const char *fmt, ...)
{
	if(on)
	{
		char temp[buffLen];
		va_list va;
		va_start(va, fmt);
		vsnprintf(temp,buffLen, fmt, va);
		va_end(va);
		DbgSendDma(temp);
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
	Dbg(1,"\r\n -----  DEBUG_RxFullBuffService -------  ");  //DO KOLEJKI vTASKLOG !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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

void MyTraceMalloc(void *pvAddress, size_t xSize)	//Wpisywac w tablice allocacji
{
/*    void *caller = __builtin_return_address(0);
    printf("M:[%p] Sz:%d od:%p\n", pvAddress, xSize, caller); */
/*
	HeapStats_t xHeapStats;
	vPortGetHeapStats(&xHeapStats);

	// Teraz masz dostęp do parametrów daj w debug na zadanie wyswietlenie:
	size_t wolne_bajty = xHeapStats.xAvailableHeapSpaceInBytes;
	size_t liczba_wolnych_blokow = xHeapStats.xNumberOfFreeBlocks;
	size_t najwiekszy_wolny_blok = xHeapStats.xSizeOfLargestFreeBlockInBytes;
*/
/*
	void vApplicationMallocFailedHook(void) {
	    // Kompilator zatrzyma się tutaj, gdy pvPortMalloc zwróci NULL
	    __disable_irq();
	    while(1);
	}
	*/
/*
#ifndef configUSE_MALLOC_FAILED_HOOK
	#define configUSE_MALLOC_FAILED_HOOK 0
#endif
	*/
}

void MyTraceFree(void *pvAddress, size_t xSize)		//Wypisywac z tablicy allocacji
{
 /*   printf("F:[%p]\n", pvAddress); */
}

void DbgDmaQue(int on, char *txt, int lenTxt)		/* DbgSend("Text") - takie wywolania z wielu watkow nie zatraci bufora bo sa one przechowywane we flashu i wskaznik do nich zawsze istnieje. */
{
	if(on)
	{ 	if (xLogQueue != NULL)			/* Skoro wywołujesz funkcję ze statycznymi ciągami tekstowymi (np. DbgSend("Uruchamianie systemu...")), które na stałe rezydują w pamięci Flash, alokowanie pamięci przez pvPortMalloc i kopiowanie tekstu przez strncpy jest zbędnym marnowaniem czasu procesora i pamięci RAM. */
    	{
			int len = CONDITION(lenTxt,lenTxt+1,mini_strlen(txt)+1);   if(len>=HEAP_MAX_ALLOC_BYTE-1) len=HEAP_MAX_ALLOC_BYTE-1;
			char* msg = pvPortMalloc(CACHE_ALLIGN_LEN(len) * sizeof(char));

			if (NULL != msg)
			{
		    	strncpy(msg,txt,len-1);  *(msg+(len-1))=0;
		    	if(pdFALSE == xQueueSend(xLogQueue, &msg, 0/*200*/))
		    		vPortFree(msg);
			}
		}
	}
}

static void BuffCirc_SendDataQue(void)
{
    if (dbg_head_que != dbg_tail_que){		/* Jeśli dma_was_idle==0, to znaczy, że DMA na pewno stoi */
    	dbg_dma_busy_que = 1;
        if (dbg_head_que > dbg_tail_que){	DEBUG_SendDma((uint8_t*)&dbgSendBuffQue[dbg_tail_que], dbg_head_que - dbg_tail_que);		 dbg_tail_que = dbg_head_que;	}
        else 							{	DEBUG_SendDma((uint8_t*)&dbgSendBuffQue[dbg_tail_que], SEND_BUFF_SIZE_QUE - dbg_tail_que);	 dbg_tail_que = 0;				}		/* reszta danych zostanie wyslana w przerwaniu */
    }
}

void DBG_EndSendInterruptQue(void)
{
    if (dbg_head_que == dbg_tail_que)  dbg_dma_busy_que=0;			/* Bufor jest pusty. DMA odpocznie, dopóki wątek vLogTask nie wpisze nowego znaku i sam go ponownie nie uruchomi. */
    else							   BuffCirc_SendDataQue();
}

void vLogTask(void *pvParameters)		/* UWAGA: Jesli korzystamy z tego wątku dla logów musimy zrezygnowac z używanie DbgSendDma___() */
{
    char *txt_ptr=NULL, *txt_ptr_copy=NULL;
    xLogQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(char *));
 /* xLogTaskHandle = xTaskGetCurrentTaskHandle(); */

    for (;;){
        if (xQueueReceive(xLogQueue, &txt_ptr, portMAX_DELAY) == pdTRUE)
        {
        	txt_ptr_copy = txt_ptr;
            while (*txt_ptr) {
                uint16_t next_head = (dbg_head_que + 1) % SEND_BUFF_SIZE_QUE;

                if (next_head == dbg_tail_que) {
    	        	vTaskDelay(10);						/* przy 2000000 Mb/s, zakladamy 8 bajtów + 2 bajty kontrolne, mamy 1000 bajtów UART7 wysyla w ciągu 5ms wiec dajmy pewnosc opróżnienia całego bufora nadawczego wciągu: (SEND_BUFF_SIZE*5[ms])/1000[B]. Na wszelki wypadek dajmy np 10ms opóznienia i jeśli w tym czasie bufor nie zostanie opróżniony stwierdzamy uszkodzenie przerwania HAL_UART_TxCpltCallback() i inicjalizujemy HAL_UART_Transmit_DMA() żeby odblokowac przerwanie i opróżnic bufor. */
    	        	if(next_head == dbg_tail_que)		/* wchodzac w ten warunek stwierdzamy że dbg_dma_busy==1 bo przerwanie HAL_UART_TxCpltCallback() przestało sie wywoływac. */
    	        		BuffCirc_SendDataQue();
                }
                dbgSendBuffQue[dbg_head_que] = *txt_ptr++;
                dbg_head_que = next_head;
            }
            vPortFree(txt_ptr_copy);

            if (dbg_dma_busy_que == 0) BuffCirc_SendDataQue();		/* Jeśli dma_was_idle==0, to znaczy, że DMA na pewno stoi */

        }
    }
}

void CreateLogTask(void)		/* --- Najbezpiecznaiejsza wersja logów --- */
{
	xTaskCreate(vLogTask, "vLogTask", 1024, NULL, (unsigned portBASE_TYPE ) 4, &xLogTaskHandle);
}

