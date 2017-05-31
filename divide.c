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
    free(divisor);
    free(dividend);
    if (!result)
    {
        fputs("Error: division unsuccessful.\n", stderr);
        return 1;
    }
    char *resultstr = hugeint_toString(result);
    free(result);
    char *remainstr = hugeint_toString(remain);
    free(remain);
    puts(resultstr);
    free(resultstr);
    fputs("remainder: ", stdout);
    puts(remainstr);
    free(remainstr);
    return 0;
}
