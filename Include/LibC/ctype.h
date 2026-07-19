#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Previously existing functions (renamed for POSIX compliance) */
int toupper(int c);
int tolower(int c);

/* Classification functions */
static inline int isalpha(int c)  { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline int isdigit(int c)  { return c >= '0' && c <= '9'; }
static inline int isalnum(int c)  { return isalpha(c) || isdigit(c); }
static inline int isspace(int c)  { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
static inline int isupper(int c)  { return c >= 'A' && c <= 'Z'; }
static inline int islower(int c)  { return c >= 'a' && c <= 'z'; }
static inline int isprint(int c)  { return c >= 0x20 && c < 0x7F; }
static inline int isgraph(int c)  { return c > 0x20 && c < 0x7F; }
static inline int ispunct(int c)  { return isgraph(c) && !isalnum(c); }
static inline int iscntrl(int c)  { return (unsigned char)c < 0x20 || c == 0x7F; }
static inline int isblank(int c)  { return c == ' ' || c == '\t'; }
static inline int isxdigit(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static inline int isascii(int c)  { return (unsigned int)c < 128; }
static inline int toascii(int c)  { return c & 0x7F; }

/* Legacy names (keep for compatibility) */
void capitalize(char *str, size_t maxlen);

#ifdef __cplusplus
}
#endif
