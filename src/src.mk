hugeint_MODULES:= hugeint
hugeint_V_MAJ:= 0
hugeint_V_MIN:= 0
hugeint_V_REV:= 1
$(call librules,hugeint)

divide_MODULES:= divide
divide_STATICDEPS:= hugeint
divide_STATICLIBS:= hugeint
$(call binrules,divide)

factorial_MODULES:= factorial
factorial_STATICDEPS:= hugeint
factorial_STATICLIBS:= hugeint
$(call binrules,factorial)

$(call zinc,test/test.mk)
