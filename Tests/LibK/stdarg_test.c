#include "../../Include/LibK/stdarg.h"
#include "Include/tests.h"

void test_va_start() {
  va_list args;
  int last_arg = 0;

  va_start(args, last_arg); // Inicia a lista

  ASSERT_EQ(args[0].gp_offset, 8); // Verifica o offset do argumento geral
  ASSERT_EQ(args[0].fp_offset,
            _VA_FP_MAX); // Verifica o offset do argumento de ponto flutuante
  ASSERT_TRUE(args[0].overflow_arg_area !=
              NULL); // Verifica a área de argumentos

  va_end(args); // Finaliza a lista
}

void test_va_arg() {
  va_list args;
  int last_arg = 0;

  va_start(args, last_arg);

  int test_value = 100;
  args[0].gp_offset = 0;
  args[0].reg_save_area = (void *)&test_value;

  int result = va_arg(args, int);

  ASSERT_EQ(result, test_value);

  va_end(args);
}

void test_va_end() {
  va_list args;
  int last_arg = 0;

  va_start(args, last_arg); // Inicia a lista
  va_end(args);             // Finaliza a lista

  ASSERT_TRUE(args[0].gp_offset >= 0); // Verifica se o offset é não negativo
}

int main(void) {
  RUN_TEST(test_va_start);
  RUN_TEST(test_va_arg);
  RUN_TEST(test_va_end);
  print_test_summary();

  return tests_failed > 0 ? 1 : 0;
}
