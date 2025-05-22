#include "_printf_.h"

//c库太复杂了，也不是线程安全型的函数，自己重写的话，可以写成线程安全性
//资源占用：
//重写的 printf：针对嵌入式系统进行了优化，设计上更轻量级，避免了标准库中可能存在的动态内存分配（如 malloc）。这对于资源有限的嵌入式系统（如 STM32）非常重要。
//标准库 printf：通常是通用实现，功能全面，但可能会使用动态内存分配，导致更高的内存占用和运行时开销。

//可定制性：
//重写的 printf：可以通过宏定义（如 PRINTF_SUPPORT_FLOAT、PRINTF_SUPPORT_LONG_LONG 等）启用或禁用特定功能，从而进一步减少代码体积和资源占用。
//标准库 printf：功能固定，无法轻松裁剪不需要的功能。
int printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vprintf_(format, args);
    va_end(args);
    return ret;
}

int sprintf(char* buffer, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vsnprintf_(buffer, ~0, format, args);
    va_end(args);
    return ret;
}

int snprintf(char* buffer, size_t count, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vsnprintf_(buffer, count, format, args);
    va_end(args);
    return ret;
}

int vsnprintf(char* buffer, size_t count, const char* format, va_list va)
{
    return vsnprintf_(buffer, count, format, va);
}

int vprintf(const char* format, va_list va)
{
    return vprintf_(format, va);
}
