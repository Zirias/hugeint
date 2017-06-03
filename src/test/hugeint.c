#include <stdlib.h>
#include <pocas/test/test.h>
#include "../hugeint.h"

PT_TESTCLASS(hugeint);

PT_TESTMETHOD(additionIsCorrect)
{
    hugeint *a = hugeint_parse("100000002000000000000000");
    hugeint *b = hugeint_parse(        "3000000000000002");
    hugeint *sum = hugeint_add(a, b);
    char *sumStr = hugeint_toString(sum);
    PT_Test_assertStrEqual("100000005000000000000002", sumStr, "wrong result");
    free(sumStr);
    free(sum);
    free(a);
    free(b);
    PT_Test_pass();
}

PT_TESTMETHOD(subtractionIsCorrect)
{
    hugeint *a = hugeint_parse("100000005000000000000002");
    hugeint *b = hugeint_parse(        "3000000000000002");
    hugeint *diff = hugeint_sub(a, b);
    char *diffStr = hugeint_toString(diff);
    PT_Test_assertStrEqual("100000002000000000000000", diffStr, "wrong result");
    free(diffStr);
    free(diff);
    free(a);
    free(b);
    PT_Test_pass();
}

PT_TESTMETHOD(multiplicationIsCorrect)
{
    hugeint *a = hugeint_parse("2432902008176640000");
    hugeint *b = hugeint_fromUint(21);
    hugeint *product = hugeint_mult(a, b);
    char *prodStr = hugeint_toString(product);
    PT_Test_assertStrEqual("51090942171709440000", prodStr, "wrong result");
    free(prodStr);
    free(product);
    free(a);
    free(b);
    a = hugeint_parse("562000363888803840000");
    b = hugeint_fromUint(2);
    product = hugeint_mult(a, b);
    prodStr = hugeint_toString(product);
    PT_Test_assertStrEqual("1124000727777607680000", prodStr, "wrong result");
    free(prodStr);
    free(product);
    free(a);
    free(b);
    a = hugeint_parse("46833363657400320000");
    b = hugeint_fromUint(4);
    product = hugeint_mult(a, b);
    prodStr = hugeint_toString(product);
    PT_Test_assertStrEqual("187333454629601280000", prodStr, "wrong result");
    free(prodStr);
    free(product);
    free(a);
    free(b);
    a = hugeint_parse("83000567673654159270286824042913149749436958055596052501112243788006768842629242159104");
    b = hugeint_fromUint(4);
    product = hugeint_mult(a, b);
    prodStr = hugeint_toString(product);
    PT_Test_assertStrEqual("332002270694616637081147296171652598997747832222384210004448975152027075370516968636416", prodStr, "wrong result");
    free(prodStr);
    free(product);
    free(a);
    free(b);
    PT_Test_pass();
}
