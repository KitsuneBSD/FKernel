#include <LibFK/Utilities/Archive/tar.h>

namespace fk {
namespace archive {

size_t TarParser::octal_to_int(const char *str, size_t max_len) {
  size_t result = 0;
  for (size_t i = 0; i < max_len && str[i]; ++i) {
    if (str[i] < '0' || str[i] > '7')
      break;
    result = result * 8 + (str[i] - '0');
  }
  return result;
}

} // namespace archive
} // namespace fk
