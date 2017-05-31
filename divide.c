#include <stdio.h>
#include <stdlib.h>
#include "hugeint.h"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s [dividend] [divisor]\n", argv[0]);
        return 1;
    }
    hugeint *dividend = hugeint_parse(argv[1]);
    hugeint *divisor = hugeint_parse(argv[2]);
    hugeint *remain;
    hugeint *result = hugeint_div(dividend, divisor, &remain);
    if (!result)
    {
        free(divisor);
        free(dividend);
        fputs("Error: division unsuccessful.\n", stderr);
        return 1;
    }
    char *resultstr = hugeint_toString(result);
    char *remainstr = hugeint_toString(remain);
    puts(resultstr);
    fputs("remainder: ", stdout);
    puts(remainstr);
    free(remainstr);
    free(resultstr);
    free(result);
    free(divisor);
    free(dividend);
    return 0;
}
