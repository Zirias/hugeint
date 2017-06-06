#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "hugeint.h"

#define HUGEINT_ELEMENT_BITS (CHAR_BIT * sizeof(hugeint_Uint))
#define HUGEINT_INITIAL_ELEMENTS (256 / HUGEINT_ELEMENT_BITS)

struct hugeint
{
    size_t s;
    size_t n;
    hugeint_Uint e[];
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
        size_t s = self->s;
        while (newSize > s) s *= 2;
        self = xrealloc(self,
                sizeof(hugeint) + s * sizeof(hugeint_Uint));
        self->s = s;
    }
    if (newSize > self->n)
    {
        memset(&(self->e[self->n]), 0,
                (self->s - self->n) * sizeof(hugeint_Uint));
    }
    else if (newSize < self->n)
    {
        memset(&(self->e[newSize]), 0,
                (self->n - newSize) * sizeof(hugeint_Uint));
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
    hugeint *self = xmalloc(sizeof(hugeint) + s * sizeof(hugeint_Uint));
    memset(self, 0, sizeof(hugeint) + s * sizeof(hugeint_Uint));
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
    hugeint *clone = xmalloc(sizeof(hugeint) + self->s * sizeof(hugeint_Uint));
    memcpy(clone, self, sizeof(hugeint) + self->s * sizeof(hugeint_Uint));
    return clone;
}

hugeint *hugeint_fromUint(hugeint_Uint val)
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
    hugeint_Uint mask = 1;

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

static unsigned char hexNibble(const char *str)
{
    unsigned char hex = *str;
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

hugeint *hugeint_parseHex(const char *str)
{
    size_t len = strlen(str);
    size_t bits = len * 4;
    size_t n = HUGEINT_ELEMENT_BITS / bits;
    size_t leading = HUGEINT_ELEMENT_BITS % bits;
    size_t i = n;
    if (leading) ++n;
    hugeint *result = hugeint_createSized(n);
    if (leading)
    {
        hugeint_Uint shift = leading;
        while (shift)
        {
            shift -= 4;
            unsigned char nibble = hexNibble(str++);
            result->e[i] |= ((hugeint_Uint)nibble << shift);
        }
    }
    while (i)
    {
        --i;
        hugeint_Uint shift = HUGEINT_ELEMENT_BITS;
        while (shift)
        {
            shift -= 4;
            unsigned char nibble = hexNibble(str++);
            result->e[i] |= ((hugeint_Uint)nibble << shift);
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

    hugeint *result = hugeint_clone(a);
    hugeint_addToSelf(&result, b);
    return result;
}

hugeint *hugeint_sub(const hugeint *minuend, const hugeint *subtrahend)
{
    hugeint *result = hugeint_clone(minuend);
    hugeint_subFromSelf(&result, subtrahend);
    return result;
}

static hugeint *multUint(hugeint_Uint a, hugeint_Uint b)
{
    if (!a || !b) return hugeint_create();
    if (a < b)
    {
        hugeint_Uint tmp = a;
        a = b;
        b = tmp;
    }
    hugeint_Uint halfMask = ((hugeint_Uint)1U << (HUGEINT_ELEMENT_BITS / 2)) - 1;
    hugeint_Uint maskA = halfMask;
    hugeint_Uint maskB = halfMask;
    hugeint_Uint direct = 0;
    while (maskB)
    {
        if (a > maskA && b > maskB) break;
        if (a <= maskA && b <= maskB)
        {
            direct = 1;
            break;
        }
        maskA = (maskA << 1) | 1;
        maskB >>= 1;
    }
    if (direct)
    {
        return hugeint_fromUint(a * b);
    }
    hugeint_Uint ah = a >> (HUGEINT_ELEMENT_BITS / 2);
    hugeint_Uint al = a & halfMask;
    hugeint_Uint bh = b >> (HUGEINT_ELEMENT_BITS / 2);
    hugeint_Uint bl = b & halfMask;

    hugeint *p1 = multUint(ah, bh);
    hugeint *p2 = multUint(al, bl);
    hugeint_Uint p3f1u = ah + al;
    hugeint_Uint p3f2u = bh + bl;
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
    hugeint_subFromSelf(&p3, p2);
    hugeint_subFromSelf(&p3, p1);
    hugeint_shiftLeft(&p3, HUGEINT_ELEMENT_BITS - (HUGEINT_ELEMENT_BITS / 2));
    hugeint_shiftLeft(&p1, HUGEINT_ELEMENT_BITS);
    hugeint_addToSelf(&p3, p1);
    hugeint_addToSelf(&p3, p2);
    free(p2);
    free(p1);
    return p3;
}

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
    memcpy(&(ah->e), &(a->e[nl]), nh * sizeof(hugeint_Uint));
    hugeint *al = hugeint_createSized(nl);
    memcpy(&(al->e), &(a->e), nl * sizeof(hugeint_Uint));
    hugeint *bh;
    if (b->n > nl)
    {
        bh = hugeint_createSized(b->n - nl);
        memcpy(&(bh->e), &(b->e[nl]), (b->n - nl) * sizeof(hugeint_Uint));
    }
    else
    {
        bh = hugeint_create();
    }
    hugeint *bl = hugeint_createSized(bnl);
    memcpy(&(bl->e), &(b->e), bnl * sizeof(hugeint_Uint));

    hugeint_autoscale(&ah);
    hugeint_autoscale(&al);
    hugeint_autoscale(&bh);
    hugeint_autoscale(&bl);

    hugeint *p1 = hugeint_mult(ah, bh);
    hugeint *p2 = hugeint_mult(al, bl);
    hugeint_addToSelf(&ah, al);
    hugeint_addToSelf(&bh, bl);
    free(al);
    free(bl);
    hugeint *p3 = hugeint_mult(ah, bh);
    free(ah);
    free(bh);
    hugeint_subFromSelf(&p3, p2);
    hugeint_subFromSelf(&p3, p1);
    hugeint_shiftLeft(&p3, nl * HUGEINT_ELEMENT_BITS);
    hugeint_shiftLeft(&p1, 2 * nl * HUGEINT_ELEMENT_BITS);
    hugeint_addToSelf(&p3, p1);
    hugeint_addToSelf(&p3, p2);
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
        hugeint_shiftLeft(&scaledDivisor, 1);
        hugeint_shiftLeft(&multiple, 1);
    }

    do
    {
        if (hugeint_compare(remain, scaledDivisor) >= 0)
        {
            hugeint_subFromSelf(&remain, scaledDivisor);
            hugeint_addToSelf(&result, multiple);
        }
        hugeint_shiftRight(&scaledDivisor, 1);
        hugeint_shiftRight(&multiple, 1);
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

int hugeint_compareUint(const hugeint *self, hugeint_Uint other)
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

void hugeint_addToSelf(hugeint **self, const hugeint *other)
{
    if ((*self)->n < other->n)
    {
        *self = hugeint_scale(*self, other->n);
    }

    unsigned int carry = 0;
    size_t i;
    for (i = 0; i < other->n; ++i)
    {
        hugeint_Uint v = (*self)->e[i] + other->e[i];
        unsigned int nextCarry = v < other->e[i];
        (*self)->e[i] = v + carry;
        if (!carry || (*self)->e[i]) carry = nextCarry;
    }
    for (; i < (*self)->n; ++i)
    {
        (*self)->e[i] = (*self)->e[i] + carry;
        carry = carry && !(*self)->e[i];
    }
    if (carry)
    {
        *self = hugeint_scale(*self, (*self)->n + 1);
        (*self)->e[(*self)->n - 1] = 1;
    }
}

void hugeint_addUintToSelf(hugeint **self, hugeint_Uint other)
{
    (*self)->e[0] += other;
    if ((*self)->e[0] < other)
    {
        unsigned int carry = 1;
        for (size_t i = 1; i < (*self)->n; ++i)
        {
            (*self)->e[i] = (*self)->e[i] + carry;
            carry = carry && !(*self)->e[i];
        }
        if (carry)
        {
            *self = hugeint_scale(*self, (*self)->n + 1);
            (*self)->e[(*self)->n - 1] = 1;
        }
    }
}

void hugeint_subFromSelf(hugeint **self, const hugeint *other)
{
    if (hugeint_isZero(other)) return;
    int comp = hugeint_compare(*self, other);
    if (comp < 0)
    {
        free(*self);
        *self = 0;
        return;
    }
    if (comp == 0)
    {
        *self = hugeint_scale(*self, 1);
        (*self)->e[0] = 0;
        return;
    }

    unsigned int carry = 1;
    for (size_t i = 0; i < (*self)->n; ++i)
    {
        hugeint_Uint v = (*self)->e[i] + (i < other->n ?
                        ~(other->e[i]) : ~(hugeint_Uint)0U);
        unsigned int nextCarry = v < (*self)->e[i];
        (*self)->e[i] = v + carry;
        if (!carry || (*self)->e[i]) carry = nextCarry;
    }
    hugeint_autoscale(self);
}

void hugeint_subUintFromSelf(hugeint **self, hugeint_Uint other)
{
    if (!other) return;
    if ((*self)->n == 1)
    {
        if ((*self)->e[0] < other)
        {
            free(*self);
            *self = 0;
            return;
        }
        else if ((*self)->e[0] == other)
        {
            (*self)->e[0] = 0;
            return;
        }
    }
    hugeint_Uint v = (*self)->e[0] + ~other;
    unsigned int carry = v < ~other;
    ++(*self)->e[0];
    for (size_t i = 1; i < (*self)->n; ++i)
    {
        (*self)->e[i] += ~0U + carry;
        carry = carry && !(*self)->e[i];
    }
    hugeint_autoscale(self);

}

void hugeint_shiftLeft(hugeint **self, size_t positions)
{
    if (!positions) return;
    if (hugeint_isZero(*self)) return;
    size_t shiftElements = positions / HUGEINT_ELEMENT_BITS;
    unsigned int shiftBits = positions % HUGEINT_ELEMENT_BITS;
    size_t oldSize = (*self)->n;
    hugeint_Uint topBits = 0;
    if (shiftBits) topBits = (*self)->e[oldSize - 1]
            >> (HUGEINT_ELEMENT_BITS - shiftBits);
    size_t newSize = oldSize + shiftElements + !!topBits;

    if (newSize > oldSize) *self = hugeint_scale(*self, newSize);

    if (shiftElements)
    {
        memmove(&((*self)->e[shiftElements]), &((*self)->e[0]),
                oldSize * sizeof(hugeint_Uint));
        memset(&((*self)->e[0]), 0, shiftElements * sizeof(hugeint_Uint));
    }

    if (shiftBits)
    {
        hugeint_Uint overflowBits = 0;
        for (size_t i = shiftElements; i < newSize; ++i)
        {
            hugeint_Uint nextOverflow = (*self)->e[i]
                    >> (HUGEINT_ELEMENT_BITS - shiftBits);
            (*self)->e[i] <<= shiftBits;
            (*self)->e[i] |= overflowBits;
            overflowBits = nextOverflow;
        }
    }
    hugeint_autoscale(self);
}

void hugeint_shiftRight(hugeint **self, size_t positions)
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
                ((*self)->n - shiftElements) * sizeof(hugeint_Uint));
    }

    if (shiftBits)
    {
        hugeint_Uint overflowBits = 0;
        size_t i = (*self)->n - shiftElements;
        while (i)
        {
            --i;
            hugeint_Uint nextOverflow = (*self)->e[i]
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
        hugeint_Uint mask = (hugeint_Uint)1U << (HUGEINT_ELEMENT_BITS - 1);
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

char *hugeint_toHexString(const hugeint *self)
{
    size_t len = self->n * HUGEINT_ELEMENT_BITS / 4;

    hugeint_Uint mask = (hugeint_Uint)0xfU << (HUGEINT_ELEMENT_BITS - 4);
    while (mask && !(self->e[self->n-1] & mask))
    {
        --len;
        mask >>= 4;
    }
    if (!len)
    {
        char *result = malloc(2);
        result[0] = '0';
        result[1] = 0;
        return result;
    }

    char *result = malloc(len + 1);
    result[len] = 0;

    size_t i = 0;
    while (len)
    {
        hugeint_Uint v = self->e[i];
        for (hugeint_Uint j = 0; j < HUGEINT_ELEMENT_BITS / 4; ++j)
        {
            unsigned char nibble = v & 0xf;
            result[--len] = nibble > 9 ? nibble - 10 + 'a' : nibble + '0';
            if (!len) break;
            v >>= 4;
        }
        ++i;
    }
    return result;
}

