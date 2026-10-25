#include "lpc_pages.h"
#include "timer.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>
static int settings_ready, recovered_backup;
static FIL file __attribute__ ( ( aligned ( 32 ) ) );
static int read_settings_file ( const char *path, void *out, size_t length )
{
	FRESULT r = f_open ( &file, path, FA_READ );
	UINT got = 0;
	if ( r == FR_NO_FILE || r == FR_NO_PATH )
		return 0;
	if ( r != FR_OK )
		return -1;
	int ok = f_size ( &file ) == length;
	if ( ok )
		ok = f_read ( &file, out, (UINT) length, &got ) == FR_OK && got == length;
	if ( f_close ( &file ) != FR_OK )
		ok = 0;
	return ok && LPC_ValidateSettings ( out, length ) ? 1 : -1;
}
static int load ( void *out, size_t length, void *context )
{
	(void) context;
	if ( !TakeMutex ( Semphr_cardSD, 1000 ) )
		return -1;
	int a = read_settings_file ( "0:/LPCSET.BIN", out, length ), b = 0;
	if ( a != 1 )
		b = read_settings_file ( "0:/LPCSET.BAK", out, length );
	recovered_backup = a != 1 && b == 1;
	GiveMutex ( Semphr_cardSD );
	return a == 1 || b == 1 ? 1 : a == 0 && b == 0 ? 0
												   : -1;
}
static int save ( const void *data, size_t length, void *context )
{
	(void) context;
	UINT wrote = 0;
	FRESULT r;
	if ( !settings_ready )
		return -1;
	if ( !TakeMutex ( Semphr_cardSD, 1000 ) )
		return -1;
	int ok = f_open ( &file, "0:/LPCSET.TMP", FA_CREATE_ALWAYS | FA_WRITE ) == FR_OK;
	if ( ok )
	{
		ok = f_write ( &file, data, (UINT) length, &wrote ) == FR_OK && wrote == length;
		if ( ok )
			ok = f_sync ( &file ) == FR_OK;
		if ( f_close ( &file ) != FR_OK )
			ok = 0;
	}
	if ( ok )
	{
		if ( recovered_backup )
		{
			r = f_unlink ( "0:/LPCSET.BIN" );
			ok = r == FR_OK || r == FR_NO_FILE;
		}
		else
		{
			r = f_unlink ( "0:/LPCSET.BAK" );
			ok = r == FR_OK || r == FR_NO_FILE;
			if ( ok )
			{
				r = f_rename ( "0:/LPCSET.BIN", "0:/LPCSET.BAK" );
				ok = r == FR_OK || r == FR_NO_FILE;
				if ( r == FR_OK )
					recovered_backup = 1;
			}
		}
		if ( ok )
			ok = f_rename ( "0:/LPCSET.TMP", "0:/LPCSET.BIN" ) == FR_OK;
	}
	if ( ok )
		recovered_backup = 0;
	GiveMutex ( Semphr_cardSD );
	return ok ? 1 : -1;
}
int LPC_StorageInit ( void )
{
	int result = LPC_SettingsInit ( load, save, NULL );
	settings_ready = result >= 0;
	return result;
}
/* DMA reads use internal, cache-line-aligned staging memory. Copy completed
 * chunks to the queued SDRAM response; never issue a whole-page DMA transfer. */
static unsigned char pageReadBuffer[4096] __attribute__ ( ( aligned ( 32 ) ) );
static char pageError[128];
const char *LPC_StoragePageError ( void )
{
	return pageError;
}
size_t LPC_StorageReadPage ( unsigned page, char *out, size_t capacity )
{
	pageError[0] = 0;
	if ( !page || page > LPC_PAGE_COUNT || !out || capacity < 2 )
		return 0;
	if ( !settings_ready )
		LPC_StorageInit ();
	out[0] = 0;
	char path[32];
	snprintf ( path, sizeof path, "0:/LPC/P%05u.HTM", page );
	if ( !TakeMutex ( Semphr_cardSD, 1000 ) )
	{
		snprintf ( pageError, sizeof pageError, "SD zajeta: nie uzyskano mutexu." );
		return 0;
	}
	size_t size = 0, offset = 0;
	FRESULT result = f_open ( &file, path, FA_READ );
	if ( result != FR_OK )
	{
		snprintf ( pageError, sizeof pageError, "Nie otwarto %s (FatFs=%u).", path, (unsigned) result );
	}
	else
	{
		size = (size_t) f_size ( &file );
		if ( !size || size >= capacity )
		{
			snprintf ( pageError, sizeof pageError, "Plik %s ma %u bajtow; limit %u.", path, (unsigned) size, (unsigned) ( capacity - 1 ) );
			size = 0;
		}
		while ( offset < size )
		{
			UINT chunk = (UINT) ( size - offset ), got = 0;
			if ( chunk > sizeof pageReadBuffer )
				chunk = sizeof pageReadBuffer;
			result = f_read ( &file, pageReadBuffer, chunk, &got );
			if ( result != FR_OK || got != chunk )
			{
				snprintf ( pageError, sizeof pageError, "Blad odczytu %s po %u bajtach (FatFs=%u).", path, (unsigned) offset, (unsigned) result );
				size = 0;
				break;
			}
			memcpy ( out + offset, pageReadBuffer, chunk );
			offset += chunk;
		}
		result = f_close ( &file );
		if ( result != FR_OK )
		{
			snprintf ( pageError, sizeof pageError, "Blad zamkniecia %s (FatFs=%u).", path, (unsigned) result );
			size = 0;
		}
	}
	GiveMutex ( Semphr_cardSD );
	out[size] = 0;
	return size;
}

/* Open only while inspecting/reading SD; no FIL or SD mutex survives a packet.
 * This also makes cancellation, timeout and channel reuse resource-free. */
size_t LPC_StoragePageSize ( unsigned page )
{
	pageError[0] = 0;
	if ( !page || page > LPC_PAGE_COUNT )
		return 0;
	if ( !settings_ready )
		LPC_StorageInit ();
	char path[32];
	snprintf ( path, sizeof path, "0:/LPC/P%05u.HTM", page );
	if ( !TakeMutex ( Semphr_cardSD, 1000 ) )
	{
		snprintf ( pageError, sizeof pageError, "SD zajeta." );
		return 0;
	}
	size_t size = 0;
	FRESULT result = f_open ( &file, path, FA_READ );
	if ( result == FR_OK )
	{
		size = f_size ( &file );
		result = f_close ( &file );
		if ( result != FR_OK )
			size = 0;
	}
	if ( !size )
		snprintf ( pageError, sizeof pageError, "Nie odczytano rozmiaru %s (FatFs=%u).", path, (unsigned) result );
	GiveMutex ( Semphr_cardSD );
	return size;
}

size_t LPC_StorageReadPageChunk ( unsigned page, size_t expected, size_t offset, char *out, size_t length )
{
	if ( !page || page > LPC_PAGE_COUNT || !out || !length || length > sizeof pageReadBuffer || offset > expected || length > expected - offset )
		return 0;
	char path[32];
	snprintf ( path, sizeof path, "0:/LPC/P%05u.HTM", page );
	if ( !TakeMutex ( Semphr_cardSD, 1000 ) )
		return 0;
	UINT got = 0;
	FRESULT result = f_open ( &file, path, FA_READ );
	if ( result == FR_OK )
	{
		if ( f_size ( &file ) != expected )
			result = FR_INVALID_OBJECT;
		if ( result == FR_OK )
			result = f_lseek ( &file, offset );
		if ( result == FR_OK )
			result = f_read ( &file, pageReadBuffer, (UINT) length, &got );
		FRESULT closed = f_close ( &file );
		if ( result == FR_OK )
			result = closed;
	}
	if ( result == FR_OK && got == length )
		memcpy ( out, pageReadBuffer, length );
	else
	{
		got = 0;
		snprintf ( pageError, sizeof pageError, "Blad strumienia %s po %u bajtach (FatFs=%u).", path, (unsigned) offset, (unsigned) result );
	}
	GiveMutex ( Semphr_cardSD );
	return got;
}
