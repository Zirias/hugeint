#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "hugeint.h"

#define HUGEINT_ELEMENT_BITS (CHAR_BIT * sizeof(unsigned int))
//#define HUGEINT_INITIAL_ELEMENTS 1
#define HUGEINT_INITIAL_ELEMENTS (256 / HUGEINT_ELEMENT_BITS)

struct hugeint
{
    size_t s;
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
    if (newSize > self->s)
    {
        self = xrealloc(self,
                sizeof(hugeint) + newSize * sizeof(unsigned int));
        self->s = newSize;
    }
    if (newSize > self->n)
    {
        memset(&(self->e[self->n]), 0,
                (newSize - self->n) * sizeof(unsigned int));
    }
    else if (newSize < self->n)
    {
        memset(&(self->e[newSize]), 0,
                (self->n - newSize) * sizeof(unsigned int));
    }
    self->n = newSize;
    return self;
}

static void hugeint_autoscale(hugeint **self)
{
    (*self)->n = (*self)->s;
    while ((*self)->n > 1 && !(*self)->e[(*self)->n-1]) --(*self)->n;
}

static hugeint *hugeint_createSized(size_t size)
{
    size_t s = size;
    if (s < HUGEINT_INITIAL_ELEMENTS) s = HUGEINT_INITIAL_ELEMENTS;
    hugeint *self = xmalloc(sizeof(hugeint) + s * sizeof(unsigned int));
    memset(self, 0, sizeof(hugeint) + s * sizeof(unsigned int));
    self->s = s;
    self->n = size;
    return self;
}

hugeint *hugeint_create(void)
{
    return hugeint_createSized(1);
}

hugeint *hugeint_clone(const hugeint *self)
{
    hugeint *clone = xmalloc(sizeof(hugeint) + self->s * sizeof(unsigned int));
    memcpy(clone, self, sizeof(hugeint) + self->s * sizeof(unsigned int));
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

static unsigned char hexToNibble(unsigned char hex)
{
    if (hex >= 'a' && hex <='f')
    {
        hex -= ('a' - 10);
    }
    else if (hex >= 'A' && hex <= 'F')
    {
        hex -= ('A' - 10);
    }
    else if (hex >= '0' && hex <= '9')
    {
        hex -= '0';
    }
    else
    {
        hex = 0;
    }
    return hex;
}

static unsigned char hexByte(const char *str)
{
    unsigned char hi = hexToNibble(*str);
    unsigned char lo = hexToNibble(str[1]);
    return (hi<<4)|lo;
}

hugeint *hugeint_parseHex(const char *str)
{
    size_t len = strlen(str);
    size_t bytes = len / 2;
    size_t n = sizeof(unsigned int) / bytes;
    size_t leading = sizeof(unsigned int) % bytes;
    size_t i = n;
    if (leading) ++n;
    hugeint *result = hugeint_createSized(n);
    if (leading)
    {
        unsigned int shift = leading * CHAR_BIT;
        while (shift)
        {
            shift -= CHAR_BIT;
            unsigned char byte = hexByte(str);
            str += 2;
            result->e[i] |= ((unsigned int)byte << shift);
        }
    }
    while (i)
    {
        --i;
        unsigned int shift = HUGEINT_ELEMENT_BITS;
        while (shift)
        {
            shift -= CHAR_BIT;
            unsigned char byte = hexByte(str);
            str += 2;
            result->e[i] |= ((unsigned int)byte << shift);
        }
    }
    return result;
}

hugeint *hugeint_add(const hugeint *a, const hugeint *b)
{
    if (a->n < b->n)
    {
        const hugeint *tmp = a;
        a = b;
        b = tmp;
    }

    hugeint *result = hugeint_createSized(a->n);
    unsigned int carry = 0;
    size_t i;
    for (i = 0; i < b->n; ++i)
    {
        unsigned int v = a->e[i] + b->e[i];
        unsigned int nextCarry = v < a->e[i];
        result->e[i] = v + carry;
        if (!carry || result->e[i]) carry = nextCarry;
    }
    for (i = b->n; i < a->n; ++i)
    {
        result->e[i] = a->e[i] + carry;
        carry = !result->e[i];
    }
    if (carry)
    {
        result = hugeint_scale(result, a->n + 1);
        result->e[a->n] = 1;
    }

    return result;
}

hugeint *hugeint_sub(const hugeint *minuend, const hugeint *subtrahend)
{
    if (hugeint_isZero(subtrahend)) return hugeint_clone(minuend);
    int comp = hugeint_compare(minuend, subtrahend);
    if (comp < 0) return 0;
    if (comp == 0) return hugeint_create();

    hugeint *result = hugeint_createSized(minuend->n);
    for (size_t i = 0; i < minuend->n; ++i)
    {
        if (i >= subtrahend->n) result->e[i] = ~0U;
        else result->e[i] = ~(subtrahend->e[i]);
    }
    hugeint_increment(&result);
    unsigned int carry = 0;
    for (size_t i = 0; i < minuend->n; ++i)
    {
        unsigned int v = result->e[i] + minuend->e[i];
        unsigned int nextCarry = v < result->e[i];
        result->e[i] = v + carry;
        if (!carry || result->e[i]) carry = nextCarry;
    }
    hugeint_autoscale(&result);
    return result;
}

static hugeint *multUint(unsigned int a, unsigned int b)
{
    if (!a || !b) return hugeint_create();
    unsigned int maxDirectFactor = (1U << (HUGEINT_ELEMENT_BITS / 2)) - 1;
    if (a < maxDirectFactor && b < maxDirectFactor)
    {
        return hugeint_fromUint(a * b);
    }
    unsigned int ah = a >> (HUGEINT_ELEMENT_BITS / 2);
    unsigned int al = a & maxDirectFactor;
    unsigned int bh = b >> (HUGEINT_ELEMENT_BITS / 2);
    unsigned int bl = b & maxDirectFactor;

    hugeint *p1 = multUint(ah, bh);
    hugeint *p2 = multUint(al, bl);
    unsigned int p3f1u = ah + al;
    unsigned int p3f2u = bh + bl;
    hugeint *p3;
    if (p3f1u < ah || p3f2u < bh)
    {
        hugeint *p3f1;
        hugeint *p3f2;
        if (p3f1u < ah)
        {
            p3f1 = hugeint_createSized(2);
            p3f1->e[0] = p3f1u;
            p3f1->e[1] = 1;
        }
        else p3f1 = hugeint_fromUint(p3f1u);
        if (p3f2u < bh)
        {
            p3f2 = hugeint_createSized(2);
            p3f2->e[0] = p3f2u;
            p3f2->e[1] = 1;
        }
        else p3f2 = hugeint_fromUint(p3f2u);
        p3 = hugeint_mult(p3f1, p3f2);
        free(p3f1);
        free(p3f2);
    }
    else
    {
        p3 = multUint(p3f1u, p3f2u);
    }
    hugeint *tmp = hugeint_sub(p3, p2);
    free(p3);
    p3 = tmp;
    tmp = hugeint_sub(p3, p1);
    free(p3);
    p3 = tmp;
    hugeint_shiftleft(&p3, HUGEINT_ELEMENT_BITS - (HUGEINT_ELEMENT_BITS / 2));
    hugeint_shiftleft(&p1, HUGEINT_ELEMENT_BITS);
    if (!hugeint_isZero(p1))
    {
        tmp = hugeint_add(p3, p1);
        free(p3);
        p3 = tmp;
    }
    if (!hugeint_isZero(p2))
    {
        tmp = hugeint_add(p3, p2);
        free(p3);
        p3 = tmp;
    }
    free(p2);
    free(p1);
    return p3;
}

#include <stdio.h>

hugeint *hugeint_mult(const hugeint *a, const hugeint *b)
{
    if (hugeint_isZero(a) || hugeint_isZero(b)) return hugeint_create();
    if (b->n > a->n)
    {
        const hugeint *tmp = a;
        a = b;
        b = tmp;
    }
    if (a->n == 1) return multUint(a->e[0], b->e[0]);

    size_t nh = a->n / 2;
    size_t nl = a->n - nh;
    size_t bnl = nl;
    if (b->n < bnl) bnl = b->n;

    hugeint *ah = hugeint_createSized(nh);
    memcpy(&(ah->e), &(a->e[nl]), nh * sizeof(unsigned int));
    hugeint *al = hugeint_createSized(nl);
    memcpy(&(al->e), &(a->e), nl * sizeof(unsigned int));
    hugeint *bh;
    if (b->n > nl)
    {
        bh = hugeint_createSized(b->n - nl);
        memcpy(&(bh->e), &(b->e[nl]), (b->n - nl) * sizeof(unsigned int));
    }
    else
    {
        bh = hugeint_create();
    }
    hugeint *bl = hugeint_createSized(bnl);
    memcpy(&(bl->e), &(b->e), bnl * sizeof(unsigned int));

    hugeint_autoscale(&ah);
    hugeint_autoscale(&al);
    hugeint_autoscale(&bh);
    hugeint_autoscale(&bl);

    hugeint *p1 = hugeint_mult(ah, bh);
    hugeint *p2 = hugeint_mult(al, bl);
    hugeint *p3f1 = hugeint_add(ah, al);
    hugeint *p3f2 = hugeint_add(bh, bl);
    free(ah);
    free(al);
    free(bh);
    free(bl);
    hugeint *p3 = hugeint_mult(p3f1, p3f2);
    free(p3f1);
    free(p3f2);
    hugeint *tmp = hugeint_sub(p3, p2);
    if (!tmp)
    {
        char *as = hugeint_toString(a);
        char *bs = hugeint_toString(b);
        fprintf(stderr, "[DEBUG] %s * %s\n", as, bs);
        fflush(stderr);
        free(as);
        free(bs);
	__builtin_trap();
    }
    free(p3);
    p3 = tmp;
    tmp = hugeint_sub(p3, p1);
    if (!tmp) __builtin_trap();
    free(p3);
    p3 = tmp;
    hugeint_shiftleft(&p3, nl * HUGEINT_ELEMENT_BITS);
    hugeint_shiftleft(&p1, 2 * nl * HUGEINT_ELEMENT_BITS);
    if (!hugeint_isZero(p1))
    {
        tmp = hugeint_add(p3, p1);
        free(p3);
        p3 = tmp;
    }
    if (!hugeint_isZero(p2))
    {
        tmp = hugeint_add(p3, p2);
        free(p3);
        p3 = tmp;
    }
    free(p2);
    free(p1);
    return p3;
}

hugeint *hugeint_div(const hugeint *dividend, const hugeint *divisor,
        hugeint **remainder)
{
    if (hugeint_isZero(divisor)) return 0;

    hugeint *scaledDivisor = hugeint_clone(divisor);
    hugeint *remain = hugeint_clone(dividend);
    hugeint *result = hugeint_create();
    hugeint *multiple = hugeint_fromUint(1);

    while (hugeint_compare(scaledDivisor, dividend) < 0)
    {
        hugeint_shiftleft(&scaledDivisor, 1);
        hugeint_shiftleft(&multiple, 1);
    }

    do
    {
        if (hugeint_compare(remain, scaledDivisor) >= 0)
        {
            hugeint *tmp = hugeint_sub(remain, scaledDivisor);
            free(remain);
            remain = tmp;
            tmp = hugeint_add(result, multiple);
            free(result);
            result = tmp;
        }
        hugeint_shiftright(&scaledDivisor, 1);
        hugeint_shiftright(&multiple, 1);
    } while (!hugeint_isZero(multiple));

    if (remainder) *remainder = remain;
    else free(remain);
    free(multiple);
    free(scaledDivisor);
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
        *self = hugeint_scale(*self, n + 1);
        (*self)->e[n] = 1;
    }
}

void hugeint_decrement(hugeint **self)
{
    if (hugeint_isZero(*self)) return;
    for (size_t i = 0; i < (*self)->n; ++i)
    {
        if ((*self)->e[i]--) break;
    }
    hugeint_autoscale(self);
}

void hugeint_shiftleft(hugeint **self, size_t positions)
{
    if (!positions) return;
    if (hugeint_isZero(*self)) return;
    size_t shiftElements = positions / HUGEINT_ELEMENT_BITS;
    unsigned int shiftBits = positions % HUGEINT_ELEMENT_BITS;
    size_t oldSize = (*self)->n;
    unsigned int topBits = 0;
    if (shiftBits) topBits = (*self)->e[oldSize - 1]
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
    hugeint_autoscale(self);
}

void hugeint_shiftright(hugeint **self, size_t positions)
{
    if (!positions) return;
    if (hugeint_isZero(*self)) return;
    size_t shiftElements = positions / HUGEINT_ELEMENT_BITS;
    if (shiftElements >= (*self)->n)
    {
        *self = hugeint_scale(*self, 1);
        (*self)->e[0] = 0;
        return;
    }

    unsigned int shiftBits = positions % HUGEINT_ELEMENT_BITS;

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
    hugeint_autoscale(self);
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
