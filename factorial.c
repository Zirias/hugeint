#include <stdio.h>
#include <stdlib.h>
#include "hugeint.h"

hugeint *factorial(hugeint *self)
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
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [number]\n", argv[0]);
        return 1;
    }
    hugeint *number = hugeint_parse(argv[1]);
    hugeint *result = factorial(number);
    char *factstr = hugeint_toString(result);
    puts(factstr);
    free(factstr);
    free(result);
    free(number);
    return 0;
}
