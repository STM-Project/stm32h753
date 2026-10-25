#ifndef GOOGLE_CLOUD_PROTOCOL_H
#define GOOGLE_CLOUD_PROTOCOL_H
#include <stddef.h>

typedef struct
{
	int status;
	char location[2048];
	char body[128];
} CloudHttpResponse;

/* 0 = complete, 1 = need more bytes, -1 = malformed/too large. */
int CloudHttp_Parse ( const char *data, size_t size, int eof, CloudHttpResponse *out );
int CloudReply_Parse ( const char *body, const char *nonce, int *relay );
int CloudRedirect_Path ( const char *url, char *path, size_t capacity );
#endif
