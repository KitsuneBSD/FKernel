#pragma once

#include <LibC/stdint.h>
#include <LibC/stdio.h>

/**
 * @brief Template de traits para tipos genéricos.
 *
 * Permite definir funções auxiliares como hash e dump
 * para diferentes tipos de dados.
 *
 * @tparam T Tipo de dado
 */
template <typename T> struct Traits {};

/**
 * @brief Traits para int
 */
template <> struct Traits<int> {
  /**
   * @brief Calcula o hash de um inteiro.
   *
   * Atualmente retorna o valor diretamente.
   * TODO: implementar função de hash real, ex: DJB2 ou CRC32.
   *
   * @param i Valor a ser hasheado
   * @return Valor hash
   */
  static unsigned hash(int i) { return i; }

  /**
   * @brief Imprime o valor do inteiro.
   * @param i Valor a ser impresso
   */
  static void dump(int i) { kprintf("%d", i); }
};

/**
 * @brief Traits para unsigned int
 */
template <> struct Traits<unsigned> {
  /**
   * @brief Calcula o hash de um unsigned int.
   *
   * Atualmente retorna o valor diretamente.
   * TODO: implementar função de hash real, ex: DJB2 ou CRC32.
   *
   * @param i Valor a ser hasheado
   * @return Valor hash
   */
  static unsigned hash(unsigned i) { return i; }

  /**
   * @brief Imprime o valor do unsigned int.
   * @param i Valor a ser impresso
   */
  static void dump(unsigned i) { kprintf("%u", i); }
};
