#include "memory_debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"
#include "_debug.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MEMORY_DEBUG_TASKS 64U
static struct { void *handle; size_t stack_bytes; } stacks[MEMORY_DEBUG_TASKS];
static TaskStatus_t statuses[MEMORY_DEBUG_TASKS];
static struct
{
	char name[configMAX_TASK_NAME_LEN];
	uintptr_t base;
	size_t total, minimum;
} rows[MEMORY_DEBUG_TASKS];
static char output[960];
static size_t output_size;

void MemoryDebugTaskCreate ( void *handle, size_t stack_bytes )
{
	for ( unsigned i = 0; i < MEMORY_DEBUG_TASKS; ++i )
		if ( !stacks[i].handle || stacks[i].handle == handle )
		{
			stacks[i].handle = handle;
			stacks[i].stack_bytes = stack_bytes;
			return;
		}
}
void MemoryDebugTaskDelete ( void *handle )
{
	for ( unsigned i = 0; i < MEMORY_DEBUG_TASKS; ++i )
		if ( stacks[i].handle == handle )
		{
			stacks[i].handle = NULL;
			stacks[i].stack_bytes = 0;
			return;
		}
}
static const char *Region ( uintptr_t base )
{
	if ( base >= 0x60000000U && base < 0x61000000U ) return "SDRAM";
	if ( base >= 0x24000000U && base < 0x24080000U ) return "RAM_D1";
	if ( base >= 0x30000000U && base < 0x30048000U ) return "RAM_D2";
	if ( base >= 0x20000000U && base < 0x20020000U ) return "DTCM";
	return "inna";
}
static void Flush ( void )
{
	if ( output_size )
		DbgDmaQue ( 1, output, (int) output_size );
	output_size = 0;
	output[0] = 0;
}
static void Line ( const char *text )
{
	size_t length = strlen ( text );
	if ( output_size + length >= sizeof output ) Flush ();
	if ( length >= sizeof output ) return;
	memcpy ( output + output_size, text, length + 1 );
	output_size += length;
}
void MemoryDebugPrint ( void )
{
	/* WiFi's global debug dispatcher is the only caller. No allocations or
	 * UART output while tasks are suspended; copy names before tasks can die. */
	vTaskSuspendAll ();
	UBaseType_t count = uxTaskGetSystemState ( statuses, MEMORY_DEBUG_TASKS, NULL );
	for ( UBaseType_t i = 0; i < count; ++i )
	{
		memcpy ( rows[i].name, statuses[i].pcTaskName, sizeof rows[i].name );
		rows[i].name[sizeof rows[i].name - 1] = 0;
		rows[i].base = (uintptr_t) statuses[i].pxStackBase;
		rows[i].minimum = (size_t) statuses[i].usStackHighWaterMark * sizeof ( StackType_t );
		rows[i].total = 0;
		for ( unsigned j = 0; j < MEMORY_DEBUG_TASKS; ++j )
			if ( stacks[j].handle == statuses[i].xHandle ) rows[i].total = stacks[j].stack_bytes;
	}
	(void) xTaskResumeAll ();
	HeapStats_t heap;
	vPortGetHeapStats ( &heap );
	char line[200];
	output_size = 0;
	Line ( "\r\n=== PAMIEC FreeRTOS: wszystkie wartosci w bajtach ===\r\n" );
	snprintf ( line, sizeof line, "Sterta SDRAM: rezerwa=%lu, wolne=%lu, zajete/rezerwa=%lu\r\n",
		(unsigned long) configTOTAL_HEAP_SIZE, (unsigned long) heap.xAvailableHeapSpaceInBytes,
		(unsigned long) ( configTOTAL_HEAP_SIZE - heap.xAvailableHeapSpaceInBytes ) );
	Line ( line );
	snprintf ( line, sizeof line, "Sterta: min. wolne=%lu, najwiekszy wolny blok=%lu, wolnych blokow=%lu\r\n",
		(unsigned long) heap.xMinimumEverFreeBytesRemaining, (unsigned long) heap.xSizeOfLargestFreeBlockInBytes,
		(unsigned long) heap.xNumberOfFreeBlocks );
	Line ( line );
	Line ( "+-----------------+----------+------------+------------+------+--------+\r\n" );
	Line ( "| Watek           | Limit B  | Max zaj. B | Min wol. B | Max% | Pamiec |\r\n" );
	Line ( "+-----------------+----------+------------+------------+------+--------+\r\n" );
	for ( UBaseType_t i = 0; i < count; ++i )
	{
		if ( rows[i].total && rows[i].minimum <= rows[i].total )
		{
			size_t used = rows[i].total - rows[i].minimum;
			snprintf ( line, sizeof line, "| %-15s | %8lu | %10lu | %10lu | %3lu%% | %-6s |\r\n",
				rows[i].name, (unsigned long) rows[i].total, (unsigned long) used, (unsigned long) rows[i].minimum,
				(unsigned long) ( used * 100U / rows[i].total ), Region ( rows[i].base ) );
		}
		else
			snprintf ( line, sizeof line, "| %-15s |      n/d |        n/d | %10lu |  n/d | %-6s |\r\n",
				rows[i].name, (unsigned long) rows[i].minimum, Region ( rows[i].base ) );
		Line ( line );
	}
	Line ( "+-----------------+----------+------------+------------+------+--------+\r\n" );
	if ( !count ) Line ( "Nie pobrano listy watkow (limit 64).\r\n" );
	Line ( "Max zaj./Min wol.: historia od utworzenia watku, nie chwilowy wskaznik SP.\r\n" );
	Line ( "Limit B: granica stosu zapisana przez kernel; koncowka wyrownania moze byc pominieta.\r\n" );
	Line ( "Sterta obejmuje tez kolejki, semafory, TCB i narzut alokatora.\r\n" );
	Line ( "Stosy statyczne sa w tabeli, ale nie zuzywaja sterty FreeRTOS.\r\n" );
	Flush ();
}
