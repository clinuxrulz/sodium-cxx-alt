#ifndef __SODIUM_CXX_IMPL_SODIUM_CTX_H__
#define __SODIUM_CXX_IMPL_SODIUM_CTX_H__

#include <memory>
#include <type_traits>
#include <vector>

#include "sodium/config.h"
#include "sodium/impl/gc_node.h"
#include "sodium/impl/listener.h"

namespace sodium {

namespace impl {

class Node;

struct SodiumCtxData;
typedef struct SodiumCtxData SodiumCtxData;

class IsNode;

struct SodiumCtxData {
    std::vector<std::unique_ptr<IsNode>> changed_nodes;
    std::vector<std::unique_ptr<IsNode>> visited_nodes;
    unsigned int transaction_depth;
    unsigned int callback_depth;
    std::vector<std::function<void()>> pre_eot;
    std::vector<std::function<void()>> pre_post;
    std::vector<std::function<void()>> post;
    std::vector<Listener> keep_alive;
    unsigned int allow_collect_cycles_counter;
};

class InCallback;

typedef struct SodiumCtx {
    std::shared_ptr<SodiumCtxData> data;
    GcCtx _gc_ctx;
    std::shared_ptr<unsigned int> node_count;
    std::shared_ptr<unsigned int> node_ref_count;

    SodiumCtx();

    GcCtx gc_ctx() const;

    Node null_node() const;

    void inc_node_count() const;

    void dec_node_count() const;

    void inc_node_ref_count() const;

    void dec_node_ref_count() const;

    template<typename K>
    typename std::result_of<K()>::type transaction(K k) const {
        typedef typename std::result_of<K()>::type R;
        this->data->transaction_depth = this->data->transaction_depth + 1;
        R result = k();
        this->data->transaction_depth = this->data->transaction_depth - 1;
        if (this->data->transaction_depth == 0) {
            this->end_of_transaction();
        }
        return result;
    }

    template<typename K>
    void transaction_void(K k) const {
        this->transaction([k]() mutable { k(); return 0; });
    }

    void add_dependents_to_changed_nodes(IsNode& node);

    template<typename K>
    void pre_eot(K k) const {
        this->data->pre_eot.push_back(std::function<void()>(k));
    }

    template<typename K>
    void pre_post(K k) const {
        this->data->pre_post.push_back(std::function<void()>(k));
    }

    template<typename K>
    void post(K k) const {
        this->data->post.push_back(std::function<void()>(k));
    }

    void end_of_transaction() const;

    void update_node(const Node& node) const;

    void collect_cycles() const;

    void add_listener_to_keep_alive(const Listener& l) const;

    void remove_listener_from_keep_alive(const Listener& l) const;

    InCallback in_callback() const;

} SodiumCtx;

class InCallback {
public:
    SodiumCtx _sodium_ctx;

    InCallback(const SodiumCtx& sodium_ctx): _sodium_ctx(sodium_ctx) {
        ++this->_sodium_ctx.data->callback_depth;
    }

    ~InCallback() {
        --this->_sodium_ctx.data->callback_depth;
    }
};

typedef struct Transaction {
    SodiumCtx sodium_ctx;
    bool is_closed = false;

    Transaction(const SodiumCtx& sodium_ctx): sodium_ctx(sodium_ctx) {
        sodium_ctx.data->transaction_depth = sodium_ctx.data->transaction_depth + 1;
    }

    ~Transaction() {
        close();
    }

    void close() {
        if (is_closed) {
            return;
        }
        this->sodium_ctx.data->transaction_depth = this->sodium_ctx.data->transaction_depth - 1;
        if (this->sodium_ctx.data->transaction_depth == 0) {
            this->sodium_ctx.end_of_transaction();
        }
        is_closed = true;
    }

    bool is_in_callback() const {
        return this->sodium_ctx.data->callback_depth > 0;
    }

} Transaction;

}

}

#endif // __SODIUM_CXX_IMPL_SODIUM_CTX_H__
