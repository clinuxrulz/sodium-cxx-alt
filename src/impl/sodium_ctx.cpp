#include "./sodium_ctx.h"
#include "./node.h"

namespace sodium {

namespace impl {

SodiumCtx::SodiumCtx() {
    this->node_count = 0;
    this->node_ref_count = 0;
}

GcCtx SodiumCtx::gc_ctx() {
    return this->_gc_ctx;
}

Node SodiumCtx::null_node() {
    return Node::mk_node(
        *this,
        std::string("null_node"),
        []() {},
        std::vector<std::unique_ptr<IsNode>>()
    );
}

void SodiumCtx::add_dependents_to_changed_nodes(IsNode& node) {
    // TODO
}

void SodiumCtx::end_of_transaction() {
    // TODO
}

void SodiumCtx::update_node(Node& node) {
    // TODO
}

void SodiumCtx::collect_cycles() {
    this->gc_ctx().collect_cycles();
}

}

}
