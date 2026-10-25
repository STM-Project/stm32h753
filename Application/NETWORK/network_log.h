#ifndef NETWORK_LOG_H
#define NETWORK_LOG_H
/* Same queued logger as the rest of H7, without importing LCD common macros
 * into lwIP's namespace. */
void DbgDma(int on, const char *text);
void DbgVarDma(int on, unsigned int capacity, const char *format, ...);
#endif
