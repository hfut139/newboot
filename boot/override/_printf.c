#include "_printf_.h"

//c��̫�����ˣ�Ҳ�����̰߳�ȫ�͵ĺ������Լ���д�Ļ�������д���̰߳�ȫ��
//��Դռ�ã�
//��д�� printf�����Ƕ��ʽϵͳ�������Ż�������ϸ��������������˱�׼���п��ܴ��ڵĶ�̬�ڴ���䣨�� malloc�����������Դ���޵�Ƕ��ʽϵͳ���� STM32���ǳ���Ҫ��
//��׼�� printf��ͨ����ͨ��ʵ�֣�����ȫ�棬�����ܻ�ʹ�ö�̬�ڴ���䣬���¸��ߵ��ڴ�ռ�ú�����ʱ������

//�ɶ����ԣ�
//��д�� printf������ͨ���궨�壨�� PRINTF_SUPPORT_FLOAT��PRINTF_SUPPORT_LONG_LONG �ȣ����û�����ض����ܣ��Ӷ���һ�����ٴ����������Դռ�á�
//��׼�� printf�����̶ܹ����޷����ɲü�����Ҫ�Ĺ��ܡ�
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
