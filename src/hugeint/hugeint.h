#ifndef HUGEINT_H
#define HUGEINT_H

#include <stdint.h>

typedef uintmax_t hugeint_Uint;
typedef struct hugeint hugeint;

hugeint *hugeint_create(void);
hugeint *hugeint_clone(const hugeint *self);
hugeint *hugeint_fromUint(hugeint_Uint val);
hugeint *hugeint_parse(const char *str);
hugeint *hugeint_parseHex(const char *str);

hugeint *hugeint_add(const hugeint *a, const hugeint *b);
hugeint *hugeint_sub(const hugeint *minuend, const hugeint *subtrahend);
hugeint *hugeint_mult(const hugeint *a, const hugeint *b);
hugeint *hugeint_div(const hugeint *dividend, const hugeint *divisor,
        hugeint **remainder);

int hugeint_isZero(const hugeint *self);
int hugeint_compare(const hugeint *self, const hugeint *other);
int hugeint_compareUint(const hugeint *self, hugeint_Uint other);

void hugeint_increment(hugeint **self);
void hugeint_decrement(hugeint **self);
void hugeint_addToSelf(hugeint **self, const hugeint *other);
void hugeint_addUintToSelf(hugeint **self, hugeint_Uint other);
void hugeint_subFromSelf(hugeint **self, const hugeint *other);
void hugeint_subUintFromSelf(hugeint **self, hugeint_Uint other);
void hugeint_shiftLeft(hugeint **self, size_t positions);
void hugeint_shiftRight(hugeint **self, size_t positions);

char *hugeint_toString(const hugeint *self);
char *hugeint_toHexString(const hugeint *self);

#endif
