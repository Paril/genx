#define _USE_MATH_DEFINES

#ifdef _WIN64
typedef __int64 ssize_t;
#define SSIZE_MAX _I64_MAX
#else
typedef __int32 ssize_t;
#define SSIZE_MAX _I32_MAX
#endif