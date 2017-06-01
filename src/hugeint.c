#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "hugeint.h"

#define HUGEINT_ELEMENT_BITS (CHAR_BIT * sizeof(unsigned int))
#define HUGEINT_INITIAL_ELEMENTS 1

static hugeint *hugeint_createSized(size_t size);
static hugeint *hugeint_scale(hugeint *self, size_t newSize);
static hugeint *hugeint_lsum_cutoverflow(size_t n,
        const hugeint *const *summands, hugeint **residue);
static hugeint *hugeint_2comp(const hugeint *self, size_t size);

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

static hugeint *hugeint_scale(hugeint *self, size_t newSize)
{
    if (newSize == self->n) return self;
    hugeint *scaled = xrealloc(self,
            sizeof(hugeint) + newSize * sizeof(unsigned int));
    if (newSize > scaled->n)
    {
        memset(&(scaled->e[scaled->n]), 0,
                (newSize - scaled->n) * sizeof(unsigned int));
    }
    scaled->n = newSize;
    return scaled;
}

static hugeint *hugeint_createSized(size_t size)
{
    hugeint *self = xmalloc(sizeof(hugeint) + size * sizeof(unsigned int));
    memset(self, 0, sizeof(hugeint) + size * sizeof(unsigned int));
    self->n = size;
    return self;
}

hugeint *hugeint_create(void)
{
    return hugeint_createSized(HUGEINT_INITIAL_ELEMENTS);
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
            if (++n == result->n) result = hugeint_scale(result, result->n + 1);
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

static hugeint *hugeint_lsum_cutoverflow(size_t n,
        const hugeint *const *summands, hugeint **residue)
{
    size_t i;
    size_t resultSize = 1;
    for (i = 0; i < n; ++i)
    {
        while (summands[i]->n > resultSize)
        {
            resultSize = summands[i]->n;
        }
    }

    hugeint *result = hugeint_createSized(resultSize);
    hugeint *res = hugeint_create();

    for (i = 0; i < result->n; ++i)
    {
        for (unsigned int bit = 1; bit; bit <<= 1)
        {
            for (size_t j = 0; j < n; ++j)
            {
                if (i >= summands[j]->n) continue;
                if (summands[j]->e[i] & bit) hugeint_increment(&res);
            }
            if (res->e[0] & 1) result->e[i] |= bit;
            hugeint_shiftright(&res, 1);
        }
    }

    if (hugeint_isZero(res))
    {
        free(res);
        res = 0;
    }

    if (residue) *residue = res;
    else free(res);

    return result;
}

hugeint *hugeint_lsum(size_t n, const hugeint *const *summands)
{
    hugeint *res;
    hugeint *result = hugeint_lsum_cutoverflow(n, summands, &res);

    if (res)
    {
        size_t i = result->n;
        result = hugeint_scale(result, i + res->n);
        memcpy(&(result->e[i]), &(res->e[0]), res->n * sizeof(unsigned int));
        free(res);
    }

    return result;
}

hugeint *hugeint_vsum(size_t n, va_list ap)
{
    const hugeint **summands = xmalloc(n * sizeof(hugeint *));

    for (size_t i = 0; i < n; ++i)
    {
        summands[i] = va_arg(ap, hugeint *);
    }

    hugeint *result = hugeint_lsum(n, summands);

    free(summands);

    return result;
}

hugeint *hugeint_sum(size_t n, ...)
{
    va_list ap;
    va_start(ap, n);
    hugeint *result = hugeint_vsum(n, ap);
    va_end(ap);
    return result;
}

static hugeint *hugeint_2comp(const hugeint *self, size_t size)
{
    hugeint *result = hugeint_createSized(size);
    if (hugeint_isZero(self)) return result;
    for (size_t i = 0; i < size; ++i)
    {
        if (i >= self->n) result->e[i] = ~0U;
        else result->e[i] = ~(self->e[i]);
    }
    hugeint_increment(&result);
    return result;
}

hugeint *hugeint_sub(const hugeint *minuend, const hugeint *subtrahend)
{
    if (subtrahend->n > minuend->n) return 0;
    hugeint *tmp = hugeint_2comp(subtrahend, minuend->n);

    hugeint *res;
    hugeint *result = hugeint_lsum_cutoverflow(2,
            (const hugeint *const []){minuend, tmp}, &res);
    free(tmp);
    if (!res)
    {
        free(result);
        return 0;
    }
    if (hugeint_compareUint(res, 1) != 0)
    {
        free(res);
        free(result);
        return 0;
    }
    free(res);
    return result;
}

hugeint *hugeint_mult(const hugeint *a, const hugeint *b)
{
    if (hugeint_compare(a, b) < 0)
    {
        const hugeint *tmp = b;
        b = a;
        a = tmp;
    }

    hugeint **summands=xmalloc(b->n * HUGEINT_ELEMENT_BITS * sizeof(hugeint *));
    size_t n = 0;
    size_t bitnum = 0;

    for (size_t i = 0; i < b->n; ++i)
    {
        for (unsigned int bit = 1; bit; bit <<= 1)
        {
            if (b->e[i] & bit)
            {
                hugeint *summand = hugeint_clone(a);
                hugeint_shiftleft(&summand, bitnum);
                summands[n++] = summand;
            }
            ++bitnum;
        }
    }

    hugeint *result = hugeint_lsum(n, (const hugeint **)summands);
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
        hugeint_shiftleft(&scaled_divisor, 1);
        hugeint_shiftleft(&multiple, 1);
    }

    do
    {
        if (hugeint_compare(remain, scaled_divisor) >= 0)
        {
            hugeint *tmp = hugeint_sub(remain, scaled_divisor);
            free(remain);
            remain = tmp;
            tmp = hugeint_add(result, multiple);
            free(result);
            result = tmp;
        }
        hugeint_shiftright(&scaled_divisor, 1);
        hugeint_shiftright(&multiple, 1);
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
        *self = hugeint_scale(*self, (*self)->n + 1);
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
    if ((*self)->n > 1 && !((*self)->e[(*self)->n - 1]))
    {
        *self = hugeint_scale(*self, (*self)->n - 1);
    }
}

void hugeint_shiftleft(hugeint **self, size_t positions)
{
    if (!positions) return;
    size_t shiftElements = positions / HUGEINT_ELEMENT_BITS;
    unsigned int shiftBits = positions % HUGEINT_ELEMENT_BITS;
    size_t oldSize = (*self)->n;
    unsigned int topBits = (*self)->e[oldSize - 1]
            >> (HUGEINT_ELEMENT_BITS - shiftBits);
    size_t newSize = oldSize + shiftElements + !!topBits;

    if (newSize > oldSize) *self = hugeint_scale(*self, newSize);

    if (shiftElements)
    {
        memmove(&((*self)->e[shiftElements]), &((*self)->e[0]),
                oldSize * sizeof(unsigned int));
        memset(&((*self)->e[0]), 0, shiftElements * sizeof(unsigned int));
    }

    if (shiftBits)
    {
        unsigned int overflowBits = 0;
        for (size_t i = shiftElements; i < newSize; ++i)
        {
            unsigned int nextOverflow = (*self)->e[i]
                    >> (HUGEINT_ELEMENT_BITS - shiftBits);
            (*self)->e[i] <<= shiftBits;
            (*self)->e[i] |= overflowBits;
            overflowBits = nextOverflow;
        }
    }
}

void hugeint_shiftright(hugeint **self, size_t positions)
{
    if (!positions) return;
    size_t shiftElements = positions / HUGEINT_ELEMENT_BITS;
    if (shiftElements >= (*self)->n)
    {
        *self = hugeint_scale(*self, 1);
        (*self)->e[0] = 0;
        return;
    }

    unsigned int shiftBits = positions % HUGEINT_ELEMENT_BITS;
    unsigned int topBits = (*self)->e[(*self)->n - 1]
            & ~((1U << shiftBits) - 1);

    if (shiftElements)
    {
        memmove(&((*self)->e[0]), &((*self)->e[shiftElements]),
                ((*self)->n - shiftElements) * sizeof(unsigned int));
    }

    if (shiftBits)
    {
        unsigned int overflowBits = 0;
        size_t i = (*self)->n - shiftElements;
        while (i)
        {
            --i;
            unsigned int nextOverflow = (*self)->e[i]
                    << (HUGEINT_ELEMENT_BITS - shiftBits);
            (*self)->e[i] >>= shiftBits;
            (*self)->e[i] |= overflowBits;
            overflowBits = nextOverflow;
        }
    }

    size_t newSize = (*self)->n - shiftElements - !!topBits;
    if (!newSize) newSize = 1;
    if (newSize != (*self)->n) *self = hugeint_scale(*self, newSize);
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
