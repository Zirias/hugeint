#ifndef HUGEINT_H
#define HUGEINT_H

#include <stdarg.h>
#include <stdint.h>

typedef struct hugeint hugeint;

hugeint *hugeint_create(void);
hugeint *hugeint_clone(const hugeint *self);
hugeint *hugeint_fromUint(uintmax_t val);
hugeint *hugeint_parse(const char *str);

hugeint *hugeint_ladd_cutoverflow(size_t n, const hugeint *const *summands,
        uintmax_t *residue);
hugeint *hugeint_ladd(size_t n, const hugeint *const *summands);
hugeint *hugeint_vadd(size_t n, va_list ap);
hugeint *hugeint_add(size_t n, ...);
hugeint *hugeint_2comp(const hugeint *self);
hugeint *hugeint_sub(const hugeint *self, const hugeint *diff);
hugeint *hugeint_shiftleft(const hugeint *hi, const hugeint *positions);
hugeint *hugeint_shiftright(const hugeint *hi, const hugeint *positions);
hugeint *hugeint_mult(const hugeint *hi, const hugeint *factor);
hugeint *hugeint_div(const hugeint *hi, const hugeint *divisor,
        hugeint **mod);

int hugeint_isZero(const hugeint *self);
int hugeint_compare(const hugeint *self, const hugeint *other);
int hugeint_compareUint(const hugeint *self, uintmax_t other);

void hugeint_increment(hugeint **self);
void hugeint_decrement(hugeint **self);

char *hugeint_toString(const hugeint *self);

#endif
