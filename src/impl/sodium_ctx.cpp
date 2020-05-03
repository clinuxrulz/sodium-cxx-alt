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
    std::vector<std::unique_ptr<IsWeakNode>>& dependents = node.node().data->dependents;
    for (auto dependent = dependents.begin(); dependent != dependents.end(); ++dependent) {
        nonstd::optional<std::unique_ptr<IsNode>> dependent2_op = (*dependent)->upgrade();
        if (dependent2_op) {
            std::unique_ptr<IsNode> dependent2 = std::move(*dependent2_op);
            this->data->changed_nodes.push_back(std::move(dependent2));
        }
    }
}

void SodiumCtx::end_of_transaction() {
    this->data->transaction_depth = this->data->transaction_depth + 1;
    this->data->allow_collect_cycles_counter = this->data->allow_collect_cycles_counter + 1;
    while (this->data->changed_nodes.size() != 0) {
        std::vector<std::unique_ptr<IsNode>> changed_nodes;
        changed_nodes.swap(this->data->changed_nodes);
        for (auto node = changed_nodes.begin(); node != changed_nodes.end(); ++node) {
            this->update_node((*node)->node());
        }
    }
    this->data->transaction_depth = this->data->transaction_depth - 1;
    this->data->allow_collect_cycles_counter = this->data->allow_collect_cycles_counter - 1;
    {
        std::vector<std::function<void()>> pre_post;
        pre_post.swap(this->data->pre_post);
        for (auto k = pre_post.begin(); k != pre_post.end(); ++k) {
            (*k)();
        }
    }
    {
        std::vector<std::function<void()>> post;
        post.swap(this->data->post);
        for (auto k = post.begin(); k != post.end(); ++k) {
            (*k)();
        }
    }
    if (this->data->allow_collect_cycles_counter == 0) {
        this->collect_cycles();
    }
}

void SodiumCtx::update_node(Node& node) {
    // TODO
}

void SodiumCtx::collect_cycles() {
    this->gc_ctx().collect_cycles();
}

}

}
