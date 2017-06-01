#ifndef HUGEINT_H
#define HUGEINT_H

#include <stdarg.h>
#include <stddef.h>

typedef struct hugeint hugeint;

hugeint *hugeint_create(void);
hugeint *hugeint_clone(const hugeint *self);
hugeint *hugeint_fromUint(unsigned int val);
hugeint *hugeint_parse(const char *str);

hugeint *hugeint_lsum(size_t n, const hugeint *const *summands);
hugeint *hugeint_vsum(size_t n, va_list ap);
hugeint *hugeint_sum(size_t n, ...);
#define hugeint_add(a, b) hugeint_sum(2, (a), (b))

hugeint *hugeint_2comp(const hugeint *self);
hugeint *hugeint_sub(const hugeint *self, const hugeint *diff);
hugeint *hugeint_mult(const hugeint *hi, const hugeint *factor);
hugeint *hugeint_div(const hugeint *hi, const hugeint *divisor,
        hugeint **mod);

int hugeint_isZero(const hugeint *self);
int hugeint_compare(const hugeint *self, const hugeint *other);
int hugeint_compareUint(const hugeint *self, unsigned int other);

void hugeint_increment(hugeint **self);
void hugeint_decrement(hugeint **self);
void hugeint_shiftleft(hugeint **self, size_t positions);
void hugeint_shiftright(hugeint **self, size_t positions);

char *hugeint_toString(const hugeint *self);

#endif
