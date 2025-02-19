#pragma once

// === Macros for integer constant definitions with proper suffixes

#define INT8_C(val) val
#define UINT8_C(val) val##U

#define INT16_C(val) val
#define UINT16_C(val) val##U

#define INT32_C(val) val
#define UINT32_C(val) val##U

#define INT64_C(val) val##LL
#define UINT64_C(val) val##ULL

// === Type Definition

// Fixed-width integer types

typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long long int int64_t;
typedef unsigned long long int uint64_t;

// Minimum-width integer types (guaranteeing at least N bits)

typedef signed char int_least8_t;
typedef unsigned char uint_least8_t;

typedef short int_least16_t;
typedef unsigned short uint_least16_t;

typedef int int_least32_t;
typedef unsigned int uint_least32_t;

typedef long long int int_least64_t;
typedef unsigned long long int uint_least64_t;

// Fastest minimum-width integer types (the ones your CPU will love)

typedef signed char int_fast8_t;
typedef unsigned char uint_fast8_t;

typedef int int_fast16_t;
typedef unsigned int uint_fast16_t;

typedef int int_fast32_t;
typedef unsigned int uint_fast32_t;

typedef long long int int_fast64_t;
typedef unsigned long long int uint_fast64_t;
