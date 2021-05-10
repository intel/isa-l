#ifdef __USER_LABEL_PREFIX__
#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b
#define cdecl(x) CONCAT1 (__USER_LABEL_PREFIX__, x)
#else
#define cdecl(x) x
#endif
