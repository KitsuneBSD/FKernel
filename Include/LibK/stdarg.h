#pragma once

#if defined(__x86_64__)

// Definição correta de `va_list` para ABI System V x86_64
typedef struct {
  unsigned int gp_offset;  // Offset para o próximo argumento geral
  unsigned int fp_offset;  // Offset para o próximo argumento de ponto flutuante
  void *overflow_arg_area; // Ponteiro para argumentos na pilha
  void *reg_save_area;     // Base onde registradores foram salvos
} __va_list_struct;

typedef __va_list_struct va_list[1];
typedef va_list __gnuc_va_list; // Garante compatibilidade com stdio.h

#define _VA_GP_MAX 48  // 6 registradores inteiros (6 * 8 bytes)
#define _VA_FP_MAX 176 // 8 registradores de ponto flutuante (8 * 16 bytes)

#define va_start(ap, last)                                                     \
  do {                                                                         \
    (ap)->gp_offset = (last < _VA_GP_MAX) ? (last + 8) : _VA_GP_MAX;           \
    (ap)->fp_offset = _VA_FP_MAX;                                              \
    (ap)->overflow_arg_area = __builtin_frame_address(0) + 16;                 \
    (ap)->reg_save_area = __builtin_frame_address(0);                          \
  } while (0)

#define va_arg(ap, type)                                                       \
  ((ap)->gp_offset < _VA_GP_MAX                                                \
       ? ((ap)->gp_offset += sizeof(type),                                     \
          (*(type *)((ap)->reg_save_area + (ap)->gp_offset - sizeof(type))))   \
       : ((ap)->overflow_arg_area += sizeof(type),                             \
          (*(type *)(ap)->overflow_arg_area - sizeof(type))))

#define va_end(ap) (void)(ap)

#else // Fallback para outras arquiteturas
typedef char *va_list;
#define va_start(ap, last) (ap = (va_list) & last + _INTSIZEOF(last))
#define va_arg(ap, type)                                                       \
  (*(type *)((ap += _INTSIZEOF(type)) - _INTSIZEOF(type)))
#define va_end(ap) (ap = (va_list)0)
#endif
