#ifndef __SODIUM_CXX_IMPL_LISTENER_H__
#define __SODIUM_CXX_IMPL_LISTENER_H__

#include <memory>
#include "optional.h"
#include "impl/gc_node.h"

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

    void unlisten();
} Listener;

}

}

#include "impl/sodium_ctx.h"
#include "impl/node.h"

namespace sodium {

namespace impl {

struct ListenerData {
    SodiumCtx sodium_ctx;
    bool is_weak;
    nonstd::optional<Node> node_op;
};

Listener Listener::mkListener(SodiumCtx& sodium_ctx, bool is_weak, Node node) {
    std::shared_ptr<ListenerData> listener_data;
    {
        ListenerData* _listener_data = new ListenerData();
        _listener_data->sodium_ctx = sodium_ctx;
        _listener_data->is_weak = is_weak;
        _listener_data->node_op = nonstd::optional<Node>(node);
        listener_data = std::unique_ptr<ListenerData>(_listener_data);
    }
    auto gc_node_deconstructor = [listener_data]() mutable {
        listener_data->node_op = nonstd::nullopt;
    };
    auto gc_node_trace = [listener_data](std::function<Tracer>& tracer) {
        if (listener_data->node_op) {
            Node& node = *listener_data->node_op;
            tracer(node.gc_node);
        }
    };
    Listener listener(
        listener_data,
        GcNode(
            node.sodium_ctx.gc_ctx(),
            std::string("Listener::mkListener"),
            gc_node_deconstructor,
            gc_node_trace
        )
    );
    if (!is_weak) {
        sodium_ctx.add_listener_to_keep_alive(listener);
    }
    return listener;
}

void Listener::unlisten() {
    this->data->node_op = nonstd::nullopt;
    if (!this->data->is_weak) {
        SodiumCtx& sodium_ctx = this->data->sodium_ctx;
        sodium_ctx.remove_listener_from_keep_alive(*this);
    }
}

}

}

#endif // __SODIUM_CXX_IMPL_LISTENER_H__
