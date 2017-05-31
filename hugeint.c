#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "hugeint.h"

#define HUGEINT_ELEMENT_BITS (CHAR_BIT * sizeof(unsigned int))

// start each new "hugeint" with 256 bits:
#define HUGEINT_INITIAL_ELEMENTS (256 / HUGEINT_ELEMENT_BITS)

struct hugeint
{
    size_t n;
    unsigned int e[];
};

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

static size_t copyNum(char **out, const char *str)
{
    const char *p = str;
    const char *start;
    size_t length = 0;
    while (*p && (*p == ' ' || *p == '\t' || *p == '0')) ++p;
    if (*p < '0' || *p > '9') return 0;

    start = p;
    while (*p >= '0' && *p <= '9')
    {
        ++p;
        ++length;
    }

    *out = xmalloc(length + 1);
    (*out)[length] = 0;
    memcpy(*out, start, length);
    return length;
}

static hugeint *hugeint_expand(hugeint *self)
{
    hugeint *expanded = xrealloc(self,
            sizeof(hugeint) + 2 * self->n * sizeof(unsigned int));
    memset(&(expanded->e[expanded->n]), 0, expanded->n * sizeof(unsigned int));
    expanded->n *= 2;
    return expanded;
}

hugeint *hugeint_create(void)
{
    hugeint *self = xmalloc(sizeof(hugeint)
            + HUGEINT_INITIAL_ELEMENTS * sizeof(unsigned int));
    memset(self, 0, sizeof(hugeint)
            + HUGEINT_INITIAL_ELEMENTS * sizeof(unsigned int));
    self->n = HUGEINT_INITIAL_ELEMENTS;
    return self;
}

hugeint *hugeint_clone(const hugeint *self)
{
    hugeint *clone = xmalloc(sizeof(hugeint) + self->n * sizeof(unsigned int));
    memcpy(clone, self, sizeof(hugeint) + self->n * sizeof(unsigned int));
    return clone;
}

hugeint *hugeint_fromUint(unsigned int val)
{
    hugeint *self = hugeint_create();
    self->e[0] = val;
    return self;
}

hugeint *hugeint_parse(const char *str)
{
    char *buf;
    hugeint *result = hugeint_create();
    size_t bcdsize = copyNum(&buf, str);
    if (!bcdsize) return result;

    size_t scanstart = 0;
    size_t n = 0;
    size_t i;
    unsigned int mask = 1;

    for (i = 0; i < bcdsize; ++i) buf[i] -= '0';

    while (scanstart < bcdsize)
    {
        if (buf[bcdsize - 1] & 1) result->e[n] |= mask;
        mask <<= 1;
        if (!mask)
        {
            mask = 1;
            if (++n == result->n) result = hugeint_expand(result);
        }
        for (i = bcdsize - 1; i > scanstart; --i)
        {
            buf[i] >>= 1;
            if (buf[i-1] & 1) buf[i] |= 8;
        }
        buf[scanstart] >>= 1;
        while (scanstart < bcdsize && !buf[scanstart]) ++scanstart;
        for (i = scanstart; i < bcdsize; ++i)
        {
            if (buf[i] > 7) buf[i] -= 3;
        }
    }

    free(buf);
    return result;
}

hugeint *hugeint_ladd_cutoverflow(size_t n, const hugeint *const *summands,
        unsigned int *residue)
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

    unsigned int res = 0;

    for (i = 0; i < result->n; ++i)
    {
        for (unsigned int bit = 1; bit; bit <<= 1)
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

hugeint *hugeint_ladd(size_t n, const hugeint *const *summands)
{
    unsigned int res;
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
    const hugeint **summands = xmalloc(n * sizeof(hugeint *));

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

hugeint *hugeint_2comp(const hugeint *self)
{
    hugeint *tmp = hugeint_clone(self);
    if (hugeint_isZero(tmp)) return tmp;
    hugeint *one = hugeint_fromUint(1);
    for (size_t i = 0; i < tmp->n; ++i)
    {
        tmp->e[i] = ~(tmp->e[i]);
    }
    hugeint *result = hugeint_add(2, tmp, one);
    free(tmp);
    free(one);
    return result;
}

hugeint *hugeint_sub(const hugeint *self, const hugeint *diff)
{
    if (diff->n > self->n) return 0;
    int freediff = 0;
    if (diff->n < self->n)
    {
        freediff = 1;
        diff = hugeint_clone(diff);
        while (diff->n < self->n) diff = hugeint_expand((hugeint *)diff);
    }
    hugeint *tmp = hugeint_2comp(diff);
    if (freediff) free((hugeint *)diff);

    unsigned int res;
    hugeint *result = hugeint_ladd_cutoverflow(2,
            (const hugeint *const []){self, tmp}, &res);
    free(tmp);
    if (res > 1)
    {
        free(result);
        return 0;
    }
    return result;
}

hugeint *hugeint_shiftleft(const hugeint *hi, const hugeint *positions)
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
    unsigned int highbit = 1U << (HUGEINT_ELEMENT_BITS - 1);
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

hugeint *hugeint_shiftright(const hugeint *hi, const hugeint *positions)
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
    unsigned int highbit = 1U << (HUGEINT_ELEMENT_BITS - 1);
    do
    {
        int incelement = 0;
        size_t i = result->n;
        while (i > 0)
        {
            --i;
            int overflow = result->e[i] & 1;
            result->e[i] >>= 1;
            if (incelement) result->e[i] += highbit;
            incelement = overflow;
        }
        if (count) hugeint_decrement(&count);
    } while (count && !hugeint_isZero(count));
    free(count);
    return result;
}

hugeint *hugeint_mult(const hugeint *hi, const hugeint *factor)
{
    hugeint **summands=xmalloc(factor->n * HUGEINT_ELEMENT_BITS * sizeof(hugeint *));
    size_t n = 0;
    hugeint *bitnum = hugeint_create();

    for (size_t i = 0; i < factor->n; ++i)
    {
        for (unsigned int bit = 1; bit; bit <<= 1)
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
    hugeint *result = hugeint_ladd(n, (const hugeint **)summands);
    for (size_t j = 0; j < n; ++j) free(summands[j]);
    free(summands);
    return result;
}

hugeint *hugeint_div(const hugeint *hi, const hugeint *divisor,
        hugeint **mod)
{
    if (hugeint_isZero(divisor)) return 0;

    hugeint *scaled_divisor = hugeint_clone(divisor);
    hugeint *remain = hugeint_clone(hi);
    hugeint *result = hugeint_create();
    hugeint *multiple = hugeint_fromUint(1);

    while (hugeint_compare(scaled_divisor, hi) < 0)
    {
        hugeint *tmp = hugeint_shiftleft(scaled_divisor, 0);
        free(scaled_divisor);
        scaled_divisor = tmp;
        tmp = hugeint_shiftleft(multiple, 0);
        free(multiple);
        multiple = tmp;
    }

    do
    {
        if (hugeint_compare(remain, scaled_divisor) >= 0)
        {
            hugeint *tmp = hugeint_sub(remain, scaled_divisor);
            free(remain);
            remain = tmp;
            tmp = hugeint_add(2, result, multiple);
            free(result);
            result = tmp;
        }
        hugeint *tmp = hugeint_shiftright(scaled_divisor, 0);
        free(scaled_divisor);
        scaled_divisor = tmp;
        tmp = hugeint_shiftright(multiple, 0);
        free(multiple);
        multiple = tmp;
    } while (!hugeint_isZero(multiple));

    if (mod) *mod = remain;
    else free(remain);
    free(multiple);
    free(scaled_divisor);
    return result;
}

int hugeint_isZero(const hugeint *self)
{
    for (size_t i = 0; i < self->n; ++i)
    {
        if (self->e[i]) return 0;
    }
    return 1;
}

int hugeint_compare(const hugeint *self, const hugeint *other)
{
    size_t n;
    if (self->n > other->n)
    {
        for (size_t i = other->n; i < self->n; ++i)
        {
            if (self->e[i]) return 1;
        }
        n = other->n;
    }
    else if (self->n < other->n)
    {
        for (size_t i = self->n; i < other->n; ++i)
        {
            if (other->e[i]) return -1;
        }
        n = self->n;
    }
    else n = self->n;

    while (n > 0)
    {
        --n;
        if (self->e[n] > other->e[n]) return 1;
        if (self->e[n] < other->e[n]) return -1;
    }

    return 0;
}

int hugeint_compareUint(const hugeint *self, unsigned int other)
{
    for (size_t i = self->n - 1; i > 0; --i)
    {
        if (self->e[i]) return 1;
    }
    if (self->e[0] > other) return 1;
    if (self->e[0] < other) return -1;
    return 0;
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

char *hugeint_toString(const hugeint *self)
{
    if (hugeint_isZero(self))
    {
        char *zero = malloc(2);
        zero[0] = '0';
        zero[1] = 0;
        return zero;
    }

    size_t nbits = HUGEINT_ELEMENT_BITS * self->n;
    size_t bcdsize = nbits/3;
    size_t scanstart = bcdsize - 2;
    char *buf = xmalloc(bcdsize + 1);
    memset(buf, 0, bcdsize + 1);

    size_t i, j;

    i = self->n;
    while(i--)
    {
        unsigned int mask = 1U << (HUGEINT_ELEMENT_BITS - 1);
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
