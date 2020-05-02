#ifndef __SODIUM_CXX_IMPL_NODE_H__
#define __SODIUM_CXX_IMPL_NODE_H__

#include <memory>
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
    std::shared_ptr<NodeData> data;
    GcNode gc_node;
    SodiumCtx sodium_ctx;

    Node(std::shared_ptr<NodeData> data, GcNode gc_node, SodiumCtx sodium_ctx)
    : data(data), gc_node(gc_node), sodium_ctx(sodium_ctx) {}

    virtual Node node() {
        return *this;
    }

    virtual std::unique_ptr<IsNode> box_clone() {
        Node* node = new Node(*this);
        return std::unique_ptr<IsNode>(node);
    }

    virtual std::unique_ptr<IsWeakNode> downgrade() {
        WeakNode* node = new WeakNode(this->data, this->gc_node, this->sodium_ctx);
        return std::unique_ptr<IsWeakNode>(node);
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
            return nonstd::optional<std::unique_ptr<IsNode>>(new Node(*node_op));
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

    virtual std::unique_ptr<IsWeakNode> box_clone() = 0;

    virtual nonstd::optional<std::unique_ptr<IsNode>> upgrade() = 0;
};

}

}

#endif // __SODIUM_CXX_IMPL_NODE_H__
