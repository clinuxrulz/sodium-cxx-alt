#include "sodium/impl/node.h"

namespace sodium {

namespace impl {

GcNode IsNode::gc_node() {
    return this->node().gc_node;
}

void IsNode::add_dependency(const IsNode& dependency) {
    this->node().data->dependencies.push_back(dependency.box_clone());
    dependency.node().data->dependents.push_back(this->downgrade());
}


void Node::add_update_dependency(Dep update_dependency) {
    this->data->update_dependencies.push_back(update_dependency);
}

void Node::add_update_dependencies(std::vector<Dep> update_dependencies) {
    for (auto update_dependency = update_dependencies.begin(); update_dependency != update_dependencies.end(); ++update_dependency) {
        this->data->update_dependencies.push_back(*update_dependency);
    }
}

std::unique_ptr<IsWeakNode> Node::downgrade() {
    WeakNode* node = new WeakNode(this->data, this->gc_node, this->sodium_ctx);
    return std::unique_ptr<IsWeakNode>((IsWeakNode*)node);
}

WeakNode Node::downgrade2() {
    return WeakNode(this->data, this->gc_node, this->sodium_ctx);
}

std::vector<std::unique_ptr<IsNode>> box_clone_vec_is_node(std::vector<std::unique_ptr<IsNode>>& xs) {
    std::vector<std::unique_ptr<IsNode>> result;
    for (auto x = xs.begin(); x != xs.end(); ++x) {
        result.push_back((*x)->box_clone());
    }
    return result;
}

std::vector<std::unique_ptr<IsWeakNode>> box_clone_vec_is_weak_node(std::vector<std::unique_ptr<IsWeakNode>>& xs) {
    std::vector<std::unique_ptr<IsWeakNode>> result;
    for (auto x = xs.begin(); x != xs.end(); ++x) {
        result.push_back((*x)->box_clone());
    }
    return result;
}

}

}
