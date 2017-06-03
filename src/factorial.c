#include <stdio.h>
#include <stdlib.h>
#include "hugeint.h"

hugeint *factorial(hugeint *self)
{
    hugeint *result = hugeint_fromUint(1);
    hugeint *factor = hugeint_clone(self);

    while (hugeint_compareUint(factor, 1) > 0)
    {
        hugeint *tmp = hugeint_mult(result, factor);
        free(result);
        result = tmp;
#if (0)
        char *fs = hugeint_toString(factor);
        char *rs = hugeint_toString(result);
        fprintf(stderr, "[DEBUG] %s: %s\n", fs, rs);
        free(rs);
        free(fs);
#endif
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
    free(number);
    char *factstr = hugeint_toString(result);
    free(result);
    puts(factstr);
    free(factstr);
    return 0;
}
