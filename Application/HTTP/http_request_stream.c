#include "http_request_stream.h"

#include <string.h>

enum
{
	HTTP_STREAM_SEEK,
	HTTP_STREAM_CHANNEL,
	HTTP_STREAM_LENGTH,
	HTTP_STREAM_PAYLOAD
};

static void http_stream_at_line ( HTTP_RequestStream *stream )
{
	uint32_t channel = 0u;
	size_t index = 0u;
	size_t suffix;

	if ( stream->at_line_overflow || stream->at_line_length == 0u ||
		 stream->at_line[stream->at_line_length - 1u] != '\r' )
	{
		return;
	}
	while ( index < stream->at_line_length &&
			stream->at_line[index] >= '0' && stream->at_line[index] <= '9' )
	{
		uint32_t digit = (uint32_t) ( stream->at_line[index] - '0' );
		if ( channel > ( UINT32_MAX - digit ) / 10u )
		{
			return;
		}
		channel = channel * 10u + digit;
		++index;
	}
	if ( index == 0u || index >= stream->at_line_length ||
		 stream->at_line[index] != ',' )
	{
		return;
	}
	++index;
	suffix = stream->at_line_length - index;
	if ( ( suffix == 8u && memcmp ( stream->at_line + index, "CONNECT\r", 8u ) == 0 ) ||
		 ( suffix == 7u && memcmp ( stream->at_line + index, "CLOSED\r", 7u ) == 0 ) )
	{
		if ( channel < HTTP_REQUEST_STREAM_CHANNELS )
		{
			HTTP_RequestStreamResetChannel ( stream, (unsigned) channel );
		}
	}
}

static void http_stream_at_byte ( HTTP_RequestStream *stream, uint8_t byte )
{
	if ( byte == (uint8_t) '\n' )
	{
		http_stream_at_line ( stream );
		stream->at_line_length = 0u;
		stream->at_line_overflow = 0u;
	}
	else if ( stream->at_line_length < sizeof ( stream->at_line ) )
	{
		stream->at_line[stream->at_line_length++] = (char) byte;
	}
	else
	{
		stream->at_line_overflow = 1u;
	}
}

static void http_stream_seek ( HTTP_RequestStream *stream, uint8_t byte )
{
	static const char prefix[] = "+IPD,";

	if ( byte == (uint8_t) prefix[stream->prefix_length] )
	{
		++stream->prefix_length;
		if ( stream->prefix_length == sizeof ( prefix ) - 1u )
		{
			stream->state = HTTP_STREAM_CHANNEL;
			stream->prefix_length = 0u;
			stream->number = 0u;
			stream->digits = 0u;
			stream->frame_channel = UINT32_MAX;
			stream->at_line_length = 0u;
			stream->at_line_overflow = 0u;
		}
	}
	else
	{
		/* '+' is the only possible overlapping prefix. */
		stream->prefix_length = ( byte == (uint8_t) '+' ) ? 1u : 0u;
	}
}

static void http_stream_bad_request ( HTTP_RequestStream *stream )
{
	if ( stream->frame_channel < HTTP_REQUEST_STREAM_CHANNELS )
	{
		HTTP_RequestStreamChannel *channel =
			&stream->channels[stream->frame_channel];
		if ( !channel->finished )
		{
			channel->finished = 1u;
			if ( stream->callback != NULL )
			{
				stream->callback ( (unsigned) stream->frame_channel,
								   NULL, 0u, stream->context );
			}
		}
	}
}

static void http_stream_bad_header ( HTTP_RequestStream *stream, uint8_t byte )
{
	http_stream_bad_request ( stream );
	stream->state = HTTP_STREAM_SEEK;
	stream->prefix_length = 0u;
	http_stream_seek ( stream, byte );
}

static void http_stream_complete_line ( HTTP_RequestStream *stream,
										HTTP_RequestStreamChannel *channel )
{
	size_t suffix;
	size_t target_length;
	size_t index;
	size_t start = 4u;
	int post = channel->length >= 5u && memcmp ( channel->line, "POST ", 5u ) == 0;
	if ( post )
		start = 5u;

	/* "GET / HTTP/1.1" is the shortest supported request line. */
	if ( channel->length < 14u || ( memcmp ( channel->line, "GET ", 4u ) != 0 && !post ) )
	{
		http_stream_bad_request ( stream );
		return;
	}

	suffix = channel->length - 9u;
	if ( memcmp ( channel->line + suffix, " HTTP/1.0", 9u ) != 0 &&
		 memcmp ( channel->line + suffix, " HTTP/1.1", 9u ) != 0 )
	{
		http_stream_bad_request ( stream );
		return;
	}

	target_length = suffix - start;
	if ( target_length == 0u || target_length > HTTP_REQUEST_STREAM_MAX_TARGET ||
		 channel->line[start] != '/' )
	{
		http_stream_bad_request ( stream );
		return;
	}

	for ( index = start; index < suffix; ++index )
	{
		unsigned char byte = (unsigned char) channel->line[index];
		/* Origin-form has no whitespace or fragment; encode non-ASCII data
         * as percent escapes before sending it in an HTTP request target.
         */
		if ( byte <= 0x20u || byte >= 0x7fu || byte == (unsigned char) '#' )
		{
			http_stream_bad_request ( stream );
			return;
		}
	}

	if ( post )
	{
		if ( target_length != 13u || memcmp ( channel->line + start, "/lpc.cgi/A117", 13u ) != 0 )
		{
			http_stream_bad_request ( stream );
			return;
		}
		memmove ( channel->line, channel->line + start, target_length );
		channel->line[target_length] = '?';
		channel->length = target_length + 1u;
		channel->phase = 1u;
		channel->saw_cr = 0u;
		return;
	}
	channel->line[suffix] = '\0';
	channel->finished = 1u;
	if ( stream->callback != NULL )
	{
		stream->callback ( (unsigned) stream->frame_channel,
						   channel->line + 4u, target_length, stream->context );
	}
}

static int http_header_name ( const char *text, size_t length, const char *name )
{
	size_t i;
	if ( strlen ( name ) != length )
		return 0;
	for ( i = 0; i < length; ++i )
	{
		unsigned char c = (unsigned char) text[i];
		if ( c >= 'A' && c <= 'Z' )
			c = (unsigned char) ( c + 'a' - 'A' );
		if ( c != (unsigned char) name[i] )
			return 0;
	}
	return 1;
}

static void http_post_header ( HTTP_RequestStream *stream, HTTP_RequestStreamChannel *c )
{
	char *colon, *value, *end;
	size_t n = 0u;
	c->header[c->header_length] = '\0';
	if ( !c->header_length )
	{
		if ( !c->has_length || !c->has_type || !c->content_length ||
			 c->length > HTTP_REQUEST_STREAM_MAX_TARGET ||
			 c->content_length > HTTP_REQUEST_STREAM_MAX_TARGET - c->length )
		{
			http_stream_bad_request ( stream );
			return;
		}
		c->phase = 2u;
		return;
	}
	colon = strchr ( c->header, ':' );
	if ( !colon || colon == c->header || c->header[0] == ' ' || c->header[0] == '\t' )
	{
		http_stream_bad_request ( stream );
		return;
	}
	value = colon + 1;
	while ( *value == ' ' || *value == '\t' )
		++value;
	end = value + strlen ( value );
	while ( end > value && ( end[-1] == ' ' || end[-1] == '\t' ) )
		--end;
	*end = '\0';
	if ( http_header_name ( c->header, (size_t) ( colon - c->header ), "content-length" ) )
	{
		if ( c->has_length || value == end )
		{
			http_stream_bad_request ( stream );
			return;
		}
		for ( ; value < end; ++value )
		{
			if ( *value < '0' || *value > '9' || n > ( HTTP_REQUEST_STREAM_MAX_TARGET - (unsigned) ( *value - '0' ) ) / 10u )
			{
				http_stream_bad_request ( stream );
				return;
			}
			n = n * 10u + (unsigned) ( *value - '0' );
		}
		c->has_length = 1u;
		c->content_length = n;
	}
	else if ( http_header_name ( c->header, (size_t) ( colon - c->header ), "content-type" ) )
	{
		char *semi = strchr ( value, ';' );
		if ( semi )
		{
			end = semi;
			while ( end > value && end[-1] == ' ' )
				--end;
		}
		if ( c->has_type || !http_header_name ( value, (size_t) ( end - value ), "application/x-www-form-urlencoded" ) )
		{
			http_stream_bad_request ( stream );
			return;
		}
		c->has_type = 1u;
	}
	else if ( http_header_name ( c->header, (size_t) ( colon - c->header ), "transfer-encoding" ) )
	{
		http_stream_bad_request ( stream );
	}
	c->header_length = 0u;
}

static void http_stream_payload ( HTTP_RequestStream *stream, uint8_t byte )
{
	HTTP_RequestStreamChannel *channel;

	if ( stream->discard_payload ||
		 stream->frame_channel >= HTTP_REQUEST_STREAM_CHANNELS )
	{
		return;
	}

	channel = &stream->channels[stream->frame_channel];
	if ( channel->finished )
	{
		return;
	}

	if ( channel->phase == 2u )
	{
		if ( byte <= 0x20u || byte >= 0x7fu || byte == '#' )
		{
			http_stream_bad_request ( stream );
			return;
		}
		channel->line[channel->length++] = (char) byte;
		if ( ++channel->body_received == channel->content_length )
		{
			channel->line[channel->length] = '\0';
			channel->finished = 1u;
			if ( stream->callback )
				stream->callback ( (unsigned) stream->frame_channel,
								   channel->line, channel->length, stream->context );
		}
		return;
	}
	if ( channel->phase == 1u )
	{
		if ( ++channel->header_bytes > 8192u )
		{
			http_stream_bad_request ( stream );
			return;
		}
		if ( channel->saw_cr )
		{
			channel->saw_cr = 0u;
			if ( byte != '\n' )
			{
				http_stream_bad_request ( stream );
				return;
			}
			http_post_header ( stream, channel );
		}
		else if ( byte == '\r' )
			channel->saw_cr = 1u;
		else if ( ( byte < 0x20u && byte != '\t' ) || byte >= 0x7fu ||
				  channel->header_length >= sizeof ( channel->header ) - 1u )
			http_stream_bad_request ( stream );
		else
			channel->header[channel->header_length++] = (char) byte;
		return;
	}

	if ( channel->saw_cr )
	{
		if ( byte == (uint8_t) '\n' )
		{
			http_stream_complete_line ( stream, channel );
		}
		else
		{
			http_stream_bad_request ( stream );
		}
		return;
	}

	if ( byte == (uint8_t) '\r' )
	{
		channel->saw_cr = 1u;
	}
	else if ( byte < 0x20u || byte >= 0x7fu ||
			  channel->length >= sizeof ( channel->line ) - 1u )
	{
		http_stream_bad_request ( stream );
	}
	else
	{
		channel->line[channel->length++] = (char) byte;
	}
}

void HTTP_RequestStreamInit ( HTTP_RequestStream *stream,
							  HTTP_RequestStreamCallback callback,
							  void *context )
{
	memset ( stream, 0, sizeof ( *stream ) );
	stream->callback = callback;
	stream->context = context;
	stream->frame_channel = UINT32_MAX;
}

void HTTP_RequestStreamResetChannel ( HTTP_RequestStream *stream,
									  unsigned channel )
{
	if ( channel < HTTP_REQUEST_STREAM_CHANNELS )
	{
		memset ( &stream->channels[channel], 0,
				 sizeof ( stream->channels[channel] ) );
	}
}

void HTTP_RequestStreamFeed ( HTTP_RequestStream *stream,
							  const uint8_t *data,
							  size_t length )
{
	size_t index;

	for ( index = 0u; index < length; ++index )
	{
		uint8_t byte = data[index];

		switch ( stream->state )
		{
		case HTTP_STREAM_PAYLOAD:
			http_stream_payload ( stream, byte );
			--stream->remaining;
			if ( stream->remaining == 0u )
			{
				stream->state = HTTP_STREAM_SEEK;
				stream->prefix_length = 0u;
			}
			break;

		case HTTP_STREAM_CHANNEL:
		case HTTP_STREAM_LENGTH:
			if ( byte >= (uint8_t) '0' && byte <= (uint8_t) '9' )
			{
				uint32_t digit = (uint32_t) ( byte - (uint8_t) '0' );
				if ( stream->digits >= 10u ||
					 stream->number > ( UINT32_MAX - digit ) / 10u )
				{
					http_stream_bad_header ( stream, byte );
				}
				else
				{
					stream->number = stream->number * 10u + digit;
					++stream->digits;
				}
			}
			else if ( stream->state == HTTP_STREAM_CHANNEL &&
					  byte == (uint8_t) ',' && stream->digits != 0u )
			{
				stream->frame_channel = stream->number;
				stream->state = HTTP_STREAM_LENGTH;
				stream->number = 0u;
				stream->digits = 0u;
			}
			else if ( stream->state == HTTP_STREAM_LENGTH &&
					  byte == (uint8_t) ':' && stream->digits != 0u )
			{
				stream->remaining = stream->number;
				stream->discard_payload =
					( stream->remaining > HTTP_REQUEST_STREAM_MAX_FRAME );
				if ( stream->discard_payload )
				{
					http_stream_bad_request ( stream );
				}
				stream->state = ( stream->remaining != 0u ) ? HTTP_STREAM_PAYLOAD : HTTP_STREAM_SEEK;
				stream->prefix_length = 0u;
			}
			else
			{
				http_stream_bad_header ( stream, byte );
			}
			break;

		default:
			http_stream_at_byte ( stream, byte );
			http_stream_seek ( stream, byte );
			break;
		}
	}
}
