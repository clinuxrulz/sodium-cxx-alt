#include "sodium/impl/node.h"

namespace sodium {

namespace impl {

std::unordered_set<NodeData*> living_nodes;

GcNode IsNode::gc_node() {
    return this->node().gc_node;
}

void IsNode::add_dependency(const IsNode& dependency) const {
    this->node().data->dependencies.push_back(dependency.box_clone());
    dependency.node().data->dependents.push_back(this->downgrade());
}

void IsNode::remove_dependency(const IsNode& dependency) const {
    std::vector<std::unique_ptr<IsNode>>& dependencies = this->node().data->dependencies;
    for (auto dependency2 = dependencies.begin(); dependency2 != dependencies.end(); ++dependency2) {
        if ((*dependency2)->node().data == dependency.node().data) {
            dependencies.erase(dependency2);
            break;
        }
    }
    std::vector<std::unique_ptr<IsWeakNode>>& dependents = dependency.node().data->dependents;
    for (auto dependent = dependents.begin(); dependent != dependents.end(); ++dependent) {
        boost::optional<std::unique_ptr<IsNode>> dependent2_op = (*dependent)->node().upgrade();
        if (dependent2_op) {
            std::unique_ptr<IsNode>& dependent2 = *dependent2_op;
            if (dependent2->node().data == this->node().data) {
                dependents.erase(dependent);
                break;
            }
        }
    }
}

void IsNode::add_keep_alive(const GcNode& gc_node) const {
    this->node().data->keep_alive.push_back(gc_node);
}

void IsNode::debug(std::ostream& os) const {
    std::unordered_set<const IsNode*> visited;
    std::vector<const IsNode*> stack;
    stack.push_back(this);
    while (stack.size() != 0) {
        const IsNode* at = stack[stack.size()-1];
        stack.pop_back();
        if (visited.find(at) != visited.end()) {
            continue;
        }
        visited.insert(at);
        os << "(Node N" << at->node().gc_node.id << " (dependencies [";
        auto& dependencies = at->node().data->dependencies;
        {
            bool first = true;
            for (auto dependency = dependencies.begin(); dependency != dependencies.end(); ++dependency) {
                const IsNode* dependency2 = &**dependency;
                if (!first) {
                    os << ", ";
                } else {
                    first = false;
                }
                os << dependency2->node().gc_node.id;
                stack.push_back(dependency2);
            }
        }
        os << "])" << std::endl;
    }
}

void Node::add_update_dependency(Dep update_dependency) const {
    this->data->update_dependencies.push_back(update_dependency);
}

void Node::add_update_dependencies(std::vector<Dep> update_dependencies) const {
    for (auto update_dependency = update_dependencies.begin(); update_dependency != update_dependencies.end(); ++update_dependency) {
        this->data->update_dependencies.push_back(*update_dependency);
    }
}

std::unique_ptr<IsWeakNode> Node::downgrade() const {
    WeakNode* node = new WeakNode(this->data, this->gc_node, this->sodium_ctx);
    return std::unique_ptr<IsWeakNode>((IsWeakNode*)node);
}

WeakNode Node::downgrade2() const {
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
