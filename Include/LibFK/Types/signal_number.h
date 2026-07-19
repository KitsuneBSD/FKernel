#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class SignalNumber {
public:
  static constexpr int INVALID = -1;
  static constexpr int SIGKILL = 9;
  static constexpr int SIGTERM = 15;
  static constexpr int SIGINT  = 2;
  static constexpr int SIGSEGV = 11;
  static constexpr int SIGCHLD = 17;
  static constexpr int SIGUSR1 = 10;
  static constexpr int SIGUSR2 = 12;

  constexpr SignalNumber() : m_sig(INVALID) {}
  constexpr explicit SignalNumber(int sig) : m_sig(sig) {}

  int value() const { return m_sig; }
  bool is_valid() const { return m_sig > 0 && m_sig <= 64; }
  bool is_real_time() const { return m_sig >= 32; }

  bool operator==(SignalNumber o) const { return m_sig == o.m_sig; }
  bool operator!=(SignalNumber o) const { return m_sig != o.m_sig; }
  bool operator<(SignalNumber o) const  { return m_sig < o.m_sig; }

private:
  int m_sig;
};

} // namespace fk
