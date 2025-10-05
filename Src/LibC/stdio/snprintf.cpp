#include <LibC/stdarg.h>
#include <LibC/stdio.h>
#include <LibC/string.h>

extern "C" int snprintf(char *str, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    size_t i = 0;
    size_t pos = 0;

    while (fmt[i] && pos < size - 1)
    {
        if (fmt[i] == '%')
        {
            i++;
            if (fmt[i] == 'd')
            {
                int val = va_arg(args, int);
                char num_buf[32];
                int len = itoa(val, num_buf, 10);
                for (int j = 0; j < len && pos < size - 1; j++)
                {
                    str[pos++] = num_buf[j];
                }
            }
            else if (fmt[i] == 'x')
            {
                int val = va_arg(args, int);
                char num_buf[32];
                int len = itoa(val, num_buf, 16);
                for (int j = 0; j < len && pos < size - 1; j++)
                {
                    str[pos++] = num_buf[j];
                }
            }
            else if (fmt[i] == 'u')
            {
                unsigned int val = va_arg(args, unsigned int);
                char num_buf[32];
                itoa_unsigned(val, num_buf, 10, false);
                int len = strlen(num_buf);
                for (int j = 0; j < len && pos < size - 1; j++)
                {
                    str[pos++] = num_buf[j];
                }
            }
            else if (fmt[i] == 's')
            {
                char *s = va_arg(args, char *);
                while (*s && pos < size - 1)
                {
                    str[pos++] = *s++;
                }
            }
            else
            {
                str[pos++] = '%';
                if (pos < size - 1)
                    str[pos++] = fmt[i];
            }
        }
        else
        {
            str[pos++] = fmt[i];
        }
        i++;
    }

    str[pos] = '\0';
    va_end(args);
    return pos;
}