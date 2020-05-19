#ifndef __SODIUM_CXX_IMPL_LISTENER_H__
#define __SODIUM_CXX_IMPL_LISTENER_H__

#include <memory>
#include <boost/optional.hpp>
#include "sodium/impl/gc_node.h"

namespace sodium {

namespace impl {

struct ListenerData;
typedef struct ListenerData ListenerData;

struct SodiumCtx;
typedef struct SodiumCtx SodiumCtx;

class Node;

typedef struct Listener {
    std::shared_ptr<ListenerData> data;
    GcNode gc_node;

    Listener(std::shared_ptr<ListenerData> data, GcNode gc_node): data(data), gc_node(gc_node) {}

    static Listener mkListener(SodiumCtx& sodium_ctx, bool is_weak, Node node);

    void unlisten() const;
} Listener;

}

}

#endif // __SODIUM_CXX_IMPL_LISTENER_H__
