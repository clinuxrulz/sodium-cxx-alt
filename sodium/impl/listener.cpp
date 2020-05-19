#include "sodium/impl/listener.h"
#include "sodium/impl/node.h"
#include "sodium/impl/sodium_ctx.h"

namespace sodium {

namespace impl {


struct ListenerData {
    SodiumCtx sodium_ctx;
    bool is_weak;
    boost::optional<Node> node_op;
};

Listener Listener::mkListener(SodiumCtx& sodium_ctx, bool is_weak, Node node) {
    std::shared_ptr<ListenerData> listener_data;
    {
        ListenerData* _listener_data = new ListenerData();
        _listener_data->sodium_ctx = sodium_ctx;
        _listener_data->is_weak = is_weak;
        _listener_data->node_op = boost::optional<Node>(node);
        listener_data = std::unique_ptr<ListenerData>(_listener_data);
    }
    auto gc_node_deconstructor = [listener_data]() mutable {
        listener_data->node_op = boost::none;
    };
    auto gc_node_trace = [listener_data](std::function<Tracer>& tracer) {
        if (listener_data->node_op) {
            IsNode& node = *listener_data->node_op;
            tracer(node.gc_node());
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

void Listener::unlisten() const {
    this->data->node_op = boost::none;
    if (!this->data->is_weak) {
        SodiumCtx& sodium_ctx = this->data->sodium_ctx;
        sodium_ctx.remove_listener_from_keep_alive(*this);
    }
}

}

}
