#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/node.h"

namespace sodium {

namespace impl {

SodiumCtx::SodiumCtx() {
    SodiumCtxData* data = new SodiumCtxData();
    data->transaction_depth = 0;
    data->allow_collect_cycles_counter = 0;
    this->data = std::unique_ptr<SodiumCtxData>(data);
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

void SodiumCtx::inc_node_count() {
    unsigned int& node_count = *(this->node_count);
    ++node_count;
}

void SodiumCtx::dec_node_count() {
    unsigned int& node_count = *(this->node_count);
    --node_count;
}

void SodiumCtx::inc_node_ref_count() {
    unsigned int& node_ref_count = *(this->node_ref_count);
    ++node_ref_count;
}

void SodiumCtx::dec_node_ref_count() {
    unsigned int& node_ref_count = *(this->node_ref_count);
    --node_ref_count;
}

void SodiumCtx::add_dependents_to_changed_nodes(IsNode& node) {
    std::vector<std::unique_ptr<IsWeakNode>>& dependents = node.node().data->dependents;
    for (auto dependent = dependents.begin(); dependent != dependents.end(); ++dependent) {
        boost::optional<std::unique_ptr<IsNode>> dependent2_op = (*dependent)->upgrade();
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
    if (node.data->visited) {
        return;
    }
    node.data->visited = true;
    std::vector<std::unique_ptr<IsNode>> dependencies = box_clone_vec_is_node(node.data->dependencies);
    {
        Node node2 = node;
        this->pre_post([node2]() {
            node2.data->visited = false;
        });
    }
    bool any_changed = false;
    for (auto dependency = dependencies.begin(); dependency != dependencies.end(); ++dependency) {
        Node dependency2 = (*dependency)->node();
        if (!dependency2.data->visited) {
            this->update_node(dependency2);
            any_changed |= dependency2.data->changed;
        }
    }
    if (any_changed) {
        (node.data->update)();
    }
    if (node.data->changed) {
        std::vector<std::unique_ptr<IsWeakNode>> dependents = box_clone_vec_is_weak_node(node.data->dependents);
        for (auto dependent = dependents.begin(); dependent != dependents.end(); ++dependent) {
            boost::optional<std::unique_ptr<IsNode>> dependent2_op = (*dependent)->upgrade();
            if (dependent2_op) {
                std::unique_ptr<IsNode> dependent2 = std::move(*dependent2_op);
                this->update_node(dependent2->node());
            }
        }
    }
}

void SodiumCtx::collect_cycles() {
    this->gc_ctx().collect_cycles();
}

void SodiumCtx::add_listener_to_keep_alive(Listener& l) {
    this->data->keep_alive.push_back(l);
}

void SodiumCtx::remove_listener_from_keep_alive(Listener& l) {
    std::vector<Listener>& keep_alive = this->data->keep_alive;
    for (auto l2 = keep_alive.begin(); l2 != keep_alive.end(); ++l2) {
        if (l2->data == l.data) {
            keep_alive.erase(l2);
            break;
        }
    }
}

}

}
