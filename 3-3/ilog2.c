#include <stdint.h>
#include <stdio.h>

static int ilog2_v1(int i)
{
    int log = -1;
    while (i) {
        i >>= 1;
        log++;
    }
    return log;
}

static size_t ilog2_v2(size_t i)
{
    size_t result = 0;
    while (i >= 65536) {
        result += 16;
        i >>= 16;
    }
    while (i >= 256) {
        result += 8;
        i >>= 8;
    }
    while (i >= 16) {
        result += 4;
        i >>= 4;
    }
    while (i >= 2) {
        result += 1;
        i >>= 1;
    }
    return result;
}

static int ilog32(uint32_t v)
{
    return (31 - __builtin_clz(v | 1));
}

int main()
{
    int n = 8;
    printf("ilog2_v1(%d) = %d\n", n, ilog2_v1(n));
    printf("ilog2_v2(%d) = %ld\n", n, ilog2_v2(n));
    printf("ilog32(%d) = %d\n", n, ilog32(n));
}