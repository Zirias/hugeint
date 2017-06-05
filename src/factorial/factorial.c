#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "../hugeint/hugeint.h"

static unsigned int floorLog2(unsigned int n)
{
    unsigned int bitIndex = sizeof(unsigned int) * CHAR_BIT - 1;
    while (!(n & 1U<<bitIndex)) --bitIndex;
    return bitIndex;
}

static hugeint *recursiveProduct(int n, hugeint **cn)
{
    int m = n/2;
    if (!m)
    {
	hugeint_addUintToSelf(cn, 2);
	return hugeint_clone(*cn);
    }
    if (n == 2)
    {
	hugeint_addUintToSelf(cn, 2);
	hugeint *f1 = hugeint_clone(*cn);
	hugeint_addUintToSelf(cn, 2);
	hugeint *result = hugeint_mult(*cn, f1);
	free(f1);
	return result;
    }
    hugeint *factor1 = recursiveProduct(n - m, cn);
    hugeint *factor2 = recursiveProduct(m, cn);
    hugeint *result = hugeint_mult(factor1, factor2);
    free(factor1);
    free(factor2);
    return result;
}

hugeint *factorial(unsigned int n)
{
    if (n < 2) return hugeint_fromUint(1);
    hugeint *p = hugeint_fromUint(1);
    hugeint *r = hugeint_fromUint(1);
    hugeint *cn = hugeint_fromUint(1);
    unsigned int h = 0;
    unsigned int shift = 0;
    unsigned int high = 1;
    unsigned int log2n = floorLog2(n);

    while (h != n)
    {
	shift += h;
	h = n >> log2n--;
	unsigned int len = high;
	high = (h - 1) | 1;
	len = (high - len) / 2;

	if (len > 0)
	{
	    hugeint *prod = recursiveProduct(len, &cn);
	    hugeint *tmp = hugeint_mult(p, prod);
	    free(prod);
	    free(p);
	    p = tmp;
	    tmp = hugeint_mult(r, p);
	    free(r);
	    r = tmp;
	}
    }

    free(cn);
    free(p);
    hugeint_shiftLeft(&r, shift);
    return r;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [number]\n", argv[0]);
        return 1;
    }
    unsigned int number = atoi(argv[1]);
    hugeint *result = factorial(number);
    char *factstr = hugeint_toString(result);
    free(result);
    puts(factstr);
    free(factstr);
    return 0;
}
