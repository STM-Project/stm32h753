#ifndef MEMORY_DEBUG_H
#define MEMORY_DEBUG_H
#include <stddef.h>
/* FreeRTOS trace hooks run inside the kernel's task-list critical section. */
void MemoryDebugTaskCreate ( void *handle, size_t stack_bytes );
void MemoryDebugTaskDelete ( void *handle );
/* Task context only; outputs through the existing queued debug logger. */
void MemoryDebugPrint ( void );
#endif
