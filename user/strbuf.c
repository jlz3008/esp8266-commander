
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"

#include "console.h"
#include "user_interface.h"

#include "strbuf.h"

#define STRBUF_DEFAULT_SIZE	512

void strbuf_init (strbuf* sb)
{
	strbuf empty = STRBUF_INIT;
	*sb = empty;
}

void strbuf_clear (strbuf* sb)
{
	// initialize (including user data)
    // but keep allocation
	strbuf clear = STRBUF_INIT;
	clear.buf = sb->buf;
	clear.size = sb->size;
    *sb = clear;
}

int strbuf_incsize (strbuf* sb, size_t grow)
{
    char* newbuf = (char*)mem_realloc(sb->buf, sb->size + grow);
	if (newbuf == NULL)
	{
		LOG(LOG_ERR, "cannot realloc from %d by %d bytes\n", sb->size, grow);
		return -1;
	}
	sb->buf = newbuf;
	sb->size += grow;
	return 0;
}

int strbuf_memcpy (strbuf* sb, const void* src, size_t srclen)
{
	if (   (sb->size == 0 || sb->len + srclen > sb->size)
	    && strbuf_incsize(sb, MAX(STRBUF_INC_SIZE, sb->size - sb->len + srclen)) == -1)
		return -1;
	memcpy(sb->buf + sb->len, src, srclen);
	sb->len += srclen;
	return 0;
}

void strbuf_release (strbuf* sb)
{
	if (sb->buf)
		mem_free(sb->buf);
	sb->size = sb->len = 0;
}
