#pragma once

#include <LibFK/Types/Process/process_id.h>
#include <LibFK/Text/fixed_string.h>
#include <LibFK/Types/types.h>

struct TaskIdentity {
  fk::ProcessId id;
  fk::ProcessId tgid;
  fk::ProcessId ppid;
  fk::ProcessId pgid;
  fk::ProcessId sid;
  fk::text::fixed_string<64> name;
  uint32_t umask{0022};
  bool is_session_leader{false};
  uint32_t uid{0};
  uint32_t gid{0};
  uint32_t euid{0};
  uint32_t egid{0};
  uint32_t suid{0};
  uint32_t sgid{0};
  uint32_t supplementary_gids[16]{};
  uint32_t ngroups{0};
};
