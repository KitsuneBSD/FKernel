#pragma once

#include <LibFK/Core/result.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

struct DateTime {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint16_t year;

  fk::text::String to_string() const;
  void print();
};
