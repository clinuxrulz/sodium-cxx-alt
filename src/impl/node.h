#ifndef __SODIUM_CXX_IMPL_NODE_H__
#define __SODIUM_CXX_IMPL_NODE_H__

#include <memory>
#include <string>
#include <vector>

#include "optional.h"
#include "impl/dep.h"
#include "impl/gc_node.h"
#include "impl/sodium_ctx.h"

namespace sodium {

namespace impl {

struct NodeData;
typedef struct NodeData NodeData;

struct SodiumCtx;
typedef struct SodiumCtx SodiumCtx;

typedef struct Node: public IsNode {
public:
    std::shared_ptr<NodeData> data;
    GcNode gc_node;
    SodiumCtx sodium_ctx;

    template <typename UPDATE>
    Node(SodiumCtx sodium_ctx, std::string name, UPDATE update, std::vector<std::unique_ptr<IsNode>> dependencies) {
        std::shared_ptr<std::vector<std::weak_ptr<NodeData>>> forward_ref = std::shared_ptr<std::vector<std::weak_ptr<NodeData>>>(new std::vector<NodeData>());
        auto deconstructor;
        {
            auto forward_ref2 = forward_ref;
            deconstructor = [forward_ref2]() {
                std::shared_ptr<NodeData> node_data = (*forward_ref2)[0].lock();
                std::vector<std::unique_ptr<IsNode>> dependencies;
                dependencies.swap(node_data->dependencies);
                std::vector<std::unique_ptr<IsWeakNode>> dependents;
                dependents.swap(node_data->dependents);
                node_data->update_dependencies.clear();
                std::vector<GcNode> keep_alive;
                keep_alive.swap(node_data->keep_alive);
                node_data->update = std::function<void()>([] {});
                for (auto dependency = dependencies.begin(); dependency != dependencies.end(); ++dependency) {
                    std::vector<std::unique_ptr<IsWeakNode>>& dependents = (*dependency)->node().data->dependents;
                    for (auto dependent = dependents.begin(); dependent != dependents.end(); ++dependent) {
                        std::shared_ptr<IsNode> dependent2 = (*dependent)->node().data.lock();
                        if (dependent2) {
                            if (dependent2->node().data == node_data) {
                                dependents.erase(dependent);
                                break;
                            }
                        }
                    }
                }
                for (auto dependent = dependents.begin(); dependent != dependents.end(); ++dependent) {
                    nonstd::optional<std::unique_ptr<IsNode>> dependent2_op = (*dependent)->upgrade();
                    if (dependent2_op) {
                        std::unique_ptr<IsNode> dependent2 = *dependent2_op;
                        std::vector<std::unique_ptr<IsNode>>& dependencies = dependent2->node().data->dependencies;
                        for (auto dependency = dependencies.begin(); dependency != dependency.end(); ++dependency) {
                            if ((*dependency)->node().data == node_data) {
                                dependencies.erase(dependency);
                                break;
                            }
                        }
                    }
                }
                for (auto node = keep_alive.begin(); node != keep_alive.end(); ++node) {
                    node->dec_ref();
                }
                forward_ref2->clear();
            };
        }
        //forward_ref->push_back();
        this->sodium_ctx = sodium_ctx;
    }

    Node(Node& node): data(node.data), gc_node(node.gc_node), sodium_ctx(node.sodium_ctx) {
        node.gc_node.inc_ref();
    }

    Node(std::shared_ptr<NodeData> data, GcNode gc_node, SodiumCtx sodium_ctx)
    : data(data), gc_node(gc_node), sodium_ctx(sodium_ctx) {}

    virtual ~Node() {
        this->gc_node.dec_ref();
    }

    virtual Node node() {
        return *this;
    }

    virtual std::unique_ptr<IsNode> box_clone() {
        Node* node = new Node(*this);
        return std::unique_ptr<IsNode>(node);
    }

    virtual std::unique_ptr<IsWeakNode> downgrade() {
        WeakNode* node = new WeakNode(this->data, this->gc_node, this->sodium_ctx);
        return std::unique_ptr<IsWeakNode>((IsWeakNode*)node);
    }
} Node;

typedef struct NodeData {
    bool visited;
    bool changed;
    std::function<void()> update;
    std::vector<Dep> update_dependencies;
    std::vector<std::unique_ptr<IsNode>> dependencies;
    std::vector<std::unique_ptr<IsWeakNode>> dependents;
    std::vector<GcNode> keep_alive;
    SodiumCtx sodium_ctx;
} NodeData;

typedef struct WeakNode: public IsWeakNode {
    std::weak_ptr<NodeData> data;
    GcNode gc_node;
    SodiumCtx sodium_ctx;

    WeakNode(std::weak_ptr<NodeData> data, GcNode gc_node, SodiumCtx sodium_ctx)
    : data(data), gc_node(gc_node), sodium_ctx(sodium_ctx) {}

    virtual WeakNode node() {
        return *this;
    }

    virtual std::unique_ptr<IsWeakNode> box_clone() {
        WeakNode* node = new WeakNode(*this);
        return std::unique_ptr<IsWeakNode>(node);
    }

    virtual nonstd::optional<std::unique_ptr<IsNode>> upgrade() {
        auto node_op = this->upgrade2();
        if (node_op) {
            return nonstd::optional<std::unique_ptr<IsNode>>(std::unique_ptr<IsNode>((IsNode*)new Node(*node_op)));
        } else {
            return nonstd::nullopt;
        }
    }

    virtual nonstd::optional<Node> upgrade2() {
        std::shared_ptr<NodeData> data = this->data.lock();
        if (data) {
            return nonstd::optional<Node>(Node(data, this->gc_node, this->sodium_ctx));
        } else {
            return nonstd::nullopt;
        }
    }
} WeakNode;

class IsWeakNode;

class IsNode {
public:
    virtual ~IsNode() {}

    virtual Node node() = 0;

    virtual std::unique_ptr<IsNode> box_clone() = 0;

    virtual std::unique_ptr<IsWeakNode> downgrade() = 0;

    GcNode gc_node() {
        return this->node().gc_node;
    }
};

class IsWeakNode {
public:
    virtual ~IsWeakNode() {}

    virtual WeakNode node() = 0;

    virtual std::unique_ptr<IsWeakNode> box_clone() = 0;

    virtual nonstd::optional<std::unique_ptr<IsNode>> upgrade() = 0;
};

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

#endif // __SODIUM_CXX_IMPL_NODE_H__
