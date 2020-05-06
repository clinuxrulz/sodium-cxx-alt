#ifndef __SODIUM_CXX_IMPL_NODE_H__
#define __SODIUM_CXX_IMPL_NODE_H__

#include <memory>
#include <string>
#include <vector>

#include "sodium/optional.h"
#include "sodium/impl/dep.h"
#include "sodium/impl/gc_node.h"
#include "sodium/impl/sodium_ctx.h"

namespace sodium {

namespace impl {

struct NodeData;
typedef struct NodeData NodeData;

struct Node;
typedef struct Node Node;

struct WeakNode;
typedef struct WeakNode WeakNode;

class IsWeakNode;

class IsNode {
public:
    virtual ~IsNode() {}

    virtual Node node() = 0;

    virtual std::unique_ptr<IsNode> box_clone() const = 0;

    virtual std::unique_ptr<IsWeakNode> downgrade() = 0;

    GcNode gc_node();
};

class IsWeakNode {
public:
    virtual ~IsWeakNode() {}

    virtual WeakNode node() = 0;

    virtual std::unique_ptr<IsWeakNode> box_clone() = 0;

    virtual nonstd::optional<std::unique_ptr<IsNode>> upgrade() = 0;
};

typedef struct Node: public IsNode {
public:
    std::shared_ptr<NodeData> data;
    GcNode gc_node;
    SodiumCtx sodium_ctx;

    template <typename UPDATE>
    static Node mk_node(SodiumCtx sodium_ctx, std::string name, UPDATE update, std::vector<std::unique_ptr<IsNode>> dependencies) {
        std::shared_ptr<std::vector<std::weak_ptr<NodeData>>> forward_ref = std::shared_ptr<std::vector<std::weak_ptr<NodeData>>>(new std::vector<std::weak_ptr<NodeData>>());
        auto deconstructor = [forward_ref]() {
            std::shared_ptr<NodeData> node_data = (*forward_ref)[0].lock();
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
                    std::shared_ptr<NodeData> dependent2 = (*dependent)->node().data.lock();
                    if (dependent2) {
                        if (dependent2 == node_data) {
                            dependents.erase(dependent);
                            break;
                        }
                    }
                }
            }
            for (auto dependent = dependents.begin(); dependent != dependents.end(); ++dependent) {
                nonstd::optional<std::unique_ptr<IsNode>> dependent2_op = (*dependent)->upgrade();
                if (dependent2_op) {
                    std::unique_ptr<IsNode> dependent2 = std::move(*dependent2_op);
                    std::vector<std::unique_ptr<IsNode>>& dependencies = dependent2->node().data->dependencies;
                    for (auto dependency = dependencies.begin(); dependency != dependencies.end(); ++dependency) {
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
            forward_ref->clear();
        };
        auto trace = [forward_ref](std::function<Tracer> tracer) {
            std::shared_ptr<NodeData> node_data = (*forward_ref)[0].lock();
            {
                std::vector<std::unique_ptr<IsNode>>& dependencies = node_data->dependencies;
                for (auto dependency = dependencies.begin(); dependency != dependencies.end(); ++dependency) {
                    tracer((*dependency)->gc_node());
                }
            }
            {
                std::vector<Dep>& update_dependencies = node_data->update_dependencies;
                for (auto update_dependency = update_dependencies.begin(); update_dependency != update_dependencies.end(); ++update_dependency) {
                    tracer(update_dependency->gc_node());
                }
            }
            {
                std::vector<GcNode>& keep_alive = node_data->keep_alive;
                for (auto gc_node = keep_alive.begin(); gc_node != keep_alive.end(); ++gc_node) {
                    tracer(*gc_node);
                }
            }
        };
        std::shared_ptr<NodeData> node_data;
        {
            NodeData* _node_data = new NodeData(sodium_ctx);
            _node_data->visited = false;
            _node_data->changed = false;
            _node_data->update = update;
            _node_data->dependencies = box_clone_vec_is_node(dependencies);
            node_data = std::unique_ptr<NodeData>(_node_data);
        }
        Node node(
            node_data,
            GcNode(sodium_ctx.gc_ctx(), name, deconstructor, trace),
            sodium_ctx
        );
        forward_ref->push_back(std::weak_ptr<NodeData>(node_data));
        return node;
    }

    template <typename UPDATE>
    Node(SodiumCtx sodium_ctx, std::string name, UPDATE update, std::vector<std::unique_ptr<IsNode>> dependencies)
    : Node(Node::mk_node(sodium_ctx, name, update, dependencies))
    {
    }


    Node(const Node& node): data(node.data), gc_node(node.gc_node), sodium_ctx(node.sodium_ctx) {
        this->gc_node.inc_ref();
        this->sodium_ctx.inc_node_ref_count();
    }

    Node(std::shared_ptr<NodeData> data, GcNode gc_node, SodiumCtx sodium_ctx)
    : data(data), gc_node(gc_node), sodium_ctx(sodium_ctx) {
        this->sodium_ctx.inc_node_ref_count();
    }

    virtual ~Node() {
        this->gc_node.dec_ref();
        this->sodium_ctx.dec_node_ref_count();
    }

    virtual Node node() {
        return *this;
    }

    virtual std::unique_ptr<IsNode> box_clone() const {
        Node* node = new Node(*this);
        return std::unique_ptr<IsNode>(node);
    }

    virtual std::unique_ptr<IsWeakNode> downgrade();

    WeakNode downgrade2();
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

    NodeData(SodiumCtx sodium_ctx): sodium_ctx(sodium_ctx) {
        this->sodium_ctx.inc_node_count();
    }

    ~NodeData() {
        this->sodium_ctx.dec_node_count();
    }

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
            this->gc_node.inc_ref();
            return nonstd::optional<Node>(Node(data, this->gc_node, this->sodium_ctx));
        } else {
            return nonstd::nullopt;
        }
    }
} WeakNode;

std::vector<std::unique_ptr<IsNode>> box_clone_vec_is_node(std::vector<std::unique_ptr<IsNode>>& xs);

std::vector<std::unique_ptr<IsWeakNode>> box_clone_vec_is_weak_node(std::vector<std::unique_ptr<IsWeakNode>>& xs);

}

}

#endif // __SODIUM_CXX_IMPL_NODE_H__
