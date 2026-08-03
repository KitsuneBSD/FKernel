#pragma once

#include <LibFK/Container/Sequence/intrusive_list.h>
#include <LibFK/Types/types.h>

namespace fkernel {
    class KQueueNode;
}

namespace fkernel {

struct KNoteHook {
    fk::containers::IntrusiveListNode<KNoteHook> hook;
    KQueueNode* kq{nullptr};
    uint64_t ident{0};
    int16_t filter{0};
    uint32_t pending_fflags{0};
};

} // namespace fkernel
