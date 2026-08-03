#pragma once

#include <LibFK/Container/Sequence/intrusive_list.h>
#include <Kernel/Fs/Vfs/Events/knote_hook.h>

namespace fkernel {

using KNoteList = fk::containers::IntrusiveList<KNoteHook, &KNoteHook::hook>;

} // namespace fkernel
