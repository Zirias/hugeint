#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UINTMAX_T_BITS (CHAR_BIT * sizeof(uintmax_t))
#define HUGEINT_INITIAL_ELEMENTS 4

typedef struct hugeint
{
    size_t n;
    uintmax_t e[];
} hugeint;

static void *xmalloc(size_t size)
{
    void *m = malloc(size);
    if (!m) exit(1);
    return m;
}

static void *xrealloc(void *m, size_t size)
{
    void *m2 = realloc(m, size);
    if (!m2) exit(1);
    return m2;
}

hugeint *hugeint_create(void)
{
    hugeint *self = xmalloc(sizeof(hugeint)
            + HUGEINT_INITIAL_ELEMENTS * sizeof(uintmax_t));
    memset(self, 0, sizeof(hugeint)
            + HUGEINT_INITIAL_ELEMENTS * sizeof(uintmax_t));
    self->n = HUGEINT_INITIAL_ELEMENTS;
    return self;
}

hugeint *hugeint_clone(hugeint *self)
{
    hugeint *clone = xmalloc(sizeof(hugeint) + self->n * sizeof(uintmax_t));
    memcpy(clone, self, sizeof(hugeint) + self->n * sizeof(uintmax_t));
    return clone;
}

hugeint *hugeint_fromUint(uintmax_t val)
{
    hugeint *self = hugeint_create();
    self->e[0] = val;
    return self;
}

hugeint *hugeint_expand(hugeint *self)
{
    hugeint *expanded = xrealloc(self,
            sizeof(hugeint) + 2 * self->n * sizeof(uintmax_t));
    memset(&(expanded->e[expanded->n]), 0, expanded->n * sizeof(uintmax_t));
    expanded->n *= 2;
    return expanded;
}

hugeint *hugeint_ladd_cutoverflow(size_t n, hugeint **summands,
        uintmax_t *residue)
{
    hugeint *result = hugeint_create();

    size_t i;

    for (i = 0; i < n; ++i)
    {
        while (summands[i]->n > result->n)
        {
            result = hugeint_expand(result);
        }
    }

    uintmax_t res = 0;

    for (i = 0; i < result->n; ++i)
    {
        for (uintmax_t bit = 1; bit; bit <<= 1)
        {
            for (size_t j = 0; j < n; ++j)
            {
                if (i >= summands[j]->n) continue;
                if (summands[j]->e[i] & bit) ++res;
            }
            if (res & 1) result->e[i] |= bit;
            res >>= 1;
        }
    }

    if (residue) *residue = res;
    return result;
}

hugeint *hugeint_ladd(size_t n, hugeint **summands)
{
    uintmax_t res;
    hugeint *result = hugeint_ladd_cutoverflow(n, summands, &res);

    if (res)
    {
        size_t i = result->n;
        result = hugeint_expand(result);
        result->e[i] = res;
    }

    return result;
}

hugeint *hugeint_vadd(size_t n, va_list ap)
{
    hugeint **summands = xmalloc(n * sizeof(hugeint *));

    for (size_t i = 0; i < n; ++i)
    {
        summands[i] = va_arg(ap, hugeint *);
    }

    hugeint *result = hugeint_ladd(n, summands);

    free(summands);

    return result;
}

hugeint *hugeint_add(size_t n, ...)
{
    va_list ap;
    va_start(ap, n);
    hugeint *result = hugeint_vadd(n, ap);
    va_end(ap);
    return result;
}

int hugeint_isZero(hugeint *self)
{
    for (size_t i = 0; i < self->n; ++i)
    {
        if (self->e[i]) return 0;
    }
    return 1;
}

void hugeint_increment(hugeint **self)
{
    int carry = 0;
    for (size_t i = 0; i < (*self)->n; ++i)
    {
        carry = !++(*self)->e[i];
        if (!carry) break;
    }
    if (carry)
    {
        size_t n = (*self)->n;
        *self = hugeint_expand(*self);
        if (*self) (*self)->e[n] = 1;
    }
}

void hugeint_decrement(hugeint **self)
{
    if (hugeint_isZero(*self)) return;
    for (size_t i = 0; i < (*self)->n; ++i)
    {
        if ((*self)->e[i]--) break;
    }
}

hugeint *hugeint_shiftleft(hugeint *hi, hugeint *positions)
{
    hugeint *result = hugeint_clone(hi);
    hugeint *count;
    if (positions)
    {
        if (hugeint_isZero(positions)) return result;
        count = hugeint_clone(positions);
    }
    else
    {
        count = 0;
    }
    uintmax_t highbit = UINTMAX_C(1) << (UINTMAX_T_BITS - 1);
    do
    {
        if (result->e[result->n - 1] & highbit)
        {
            result = hugeint_expand(result);
        }
        int incelement = 0;
        for (size_t i = 0; i < result->n; ++i)
        {
            int overflow = !!(result->e[i] & highbit);
            result->e[i] <<= 1;
            if (incelement) ++result->e[i];
            incelement = overflow;
        }
        if (count) hugeint_decrement(&count);
    } while (count && !hugeint_isZero(count));
    free(count);
    return result;
}

hugeint *hugeint_mult(hugeint *hi, hugeint *factor)
{
    hugeint **summands=xmalloc(factor->n * UINTMAX_T_BITS * sizeof(hugeint *));
    size_t n = 0;
    hugeint *bitnum = hugeint_create();

    for (size_t i = 0; i < factor->n; ++i)
    {
        for (uintmax_t bit = 1; bit; bit <<= 1)
        {
            if (factor->e[i] & bit)
            {
                hugeint *summand = hugeint_shiftleft(hi, bitnum);
                summands[n++] = summand;
            }
            hugeint_increment(&bitnum);
        }
    }

    free(bitnum);
    hugeint *result = hugeint_ladd(n, summands);
    for (size_t j = 0; j < n; ++j) free(summands[j]);
    free(summands);
    return result;
}

hugeint *hugeint_parse(const char *str)
{
    hugeint *result = hugeint_create();
    hugeint *ten = hugeint_fromUint(10);
    while (*str && *str >= '0' && *str <= '9')
    {
        hugeint *posval = hugeint_fromUint((*str) - '0');
        hugeint *shifted = hugeint_mult(result, ten);
        hugeint *newres = hugeint_add(2, shifted, posval);
        free(posval);
        free(shifted);
        free(result);
        result = newres;
        ++str;
    }
    free(ten);
    return result;
}

char *hugeint_toString(hugeint *self)
{
    size_t nbits = UINTMAX_T_BITS * self->n;
    size_t bcdsize = nbits/3;
    size_t scanstart = bcdsize - 2;
    char *buf = xmalloc(bcdsize + 1);
    memset(buf, 0, bcdsize + 1);

    size_t i, j;

    i = self->n;
    while(i--)
    {
        uintmax_t mask = UINTMAX_C(1) << (UINTMAX_T_BITS - 1);
        while (mask)
        {
            int bit = !!(self->e[i] & mask);
            for (j = scanstart; j < bcdsize; ++j)
            {
                if (buf[j] > 4) buf[j] += 3;
            }
            if (buf[scanstart] > 7) scanstart -= 1;
            for (j = scanstart; j < bcdsize - 1; ++j)
            {
                buf[j] <<= 1;
                buf[j] &= 0xf;
                buf[j] |= (buf[j+1] > 7);
            }
            buf[bcdsize-1] <<= 1;
            buf[bcdsize-1] &= 0xf;
            buf[bcdsize-1] |= bit;
            mask >>= 1;
        }
    }

    for (i = 0; i < bcdsize - 1; ++i)
    {
        if (buf[i]) break;
    }

    bcdsize -= i;
    memmove(buf, buf + i, bcdsize + 1);

    for (i = 0; i < bcdsize; ++i) buf[i] += '0';
    buf = xrealloc(buf, bcdsize + 1);
    return buf;
}

hugeint *hugeint_factorial(hugeint *self)
{
    hugeint *result = hugeint_fromUint(1);
    hugeint *factor = hugeint_clone(self);

    while (!hugeint_isZero(factor))
    {
        hugeint *tmp = hugeint_mult(result, factor);
        free(result);
        result = tmp;
        hugeint_decrement(&factor);
    }
    free(factor);
    return result;
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        hugeint *test = hugeint_parse(argv[1]);
        hugeint *factorial = hugeint_factorial(test);
        char *factstr = hugeint_toString(factorial);
        printf("%s! = %s\n", argv[1], factstr);
        free(factstr);
        free(factorial);
        free(test);
        return 0;
    }
    return 1;
}
