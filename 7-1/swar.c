#include <stdio.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Nonzero if X is not aligned on a "long" boundary */
#define UNALIGNED(X) ((long) X & (sizeof(long) - 1))

/* How many bytes are loaded each iteration of the word copy loop */
#define LBLOCKSIZE (sizeof(long))

/* Threshhold for punting to the bytewise iterator */
#define TOO_SMALL(LEN) ((LEN) < LBLOCKSIZE)

#if LONG_MAX == 2147483647L
#define DETECT_NULL(X) (((X) - (0x01010101)) & ~(X) & (0x80808080))
#else
#if LONG_MAX == 9223372036854775807L
/* Nonzero if X (a long int) contains a NULL byte. */
#define DETECT_NULL(X) \
    (((X) - (0x0101010101010101)) & ~(X) & (0x8080808080808080))
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

/* @return nonzero if (long)X contains the byte used to fill MASK. */
#define DETECT_CHAR(X, mask) DETECT_NULL(X ^ mask)

void *memchr_opt(const void *str, int c, size_t len)
{
    const unsigned char *src = (const unsigned char *) str;
    unsigned char d = c;

    while (UNALIGNED(src)) {
        if (!len--)
            return NULL;
        if (*src == d)
            return (void *) src;
        src++;
    }

    if (!TOO_SMALL(len)) {
        /* If we get this far, len is large and src is word-aligned. */

        /* The fast code reads the source one word at a time and only performs
         * a bytewise search on word-sized segments if they contain the search
         * character. This is detected by XORing the word-sized segment with a
         * word-sized block of the search character, and then checking for the
         * presence of NULL in the result.
         */
        unsigned long *asrc = (unsigned long *) src;
        unsigned long mask = d << 8 | d;
        mask |= mask << 16;
        for (unsigned int i = 32; i < LBLOCKSIZE * 8; i <<= 1)
            mask |= mask << i;

        while (len >= LBLOCKSIZE) {
            if (DETECT_CHAR(*asrc, mask))
                break;
            asrc++;
            len -= LBLOCKSIZE;
        }

        /* If there are fewer than LBLOCKSIZE characters left, then we resort to
         * the bytewise loop.
         */
        src = (unsigned char *) asrc;
    }

    while (len--) {
        if (*src == d)
            return (void *) src;
        src++;
    }

    return NULL;
}


/**
 * memchr - Find a character in an area of memory.
 * @s: The memory area
 * @c: The byte to search for
 * @n: The size of the area.
 *
 * returns the address of the first occurrence of @c, or %NULL
 * if @c is not found
 */
void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = s;
    while (n-- != 0) {
        if ((unsigned char)c == *p++) {
            return (void *)(p - 1);
        }
    }
    return NULL;
}

int main()
{   
    const char str[] = "https://wiki.csie.ncku.edu.tw";
    const char ch = '.';

    char *ret = memchr(str, ch, strlen(str));
    printf("String after |%c| is - |%s| [memchr]\n", ch, ret);
    char *ret_opt = memchr_opt(str, ch, strlen(str));
    printf("String after |%c| is - |%s| [memchr_opt]\n", ch, ret_opt);
    return 0;
}