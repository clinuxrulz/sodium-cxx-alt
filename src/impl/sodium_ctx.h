#ifndef __SODIUM_CXX_IMPL_SODIUM_CTX_H__
#define __SODIUM_CXX_IMPL_SODIUM_CTX_H__

#include <memory>
#include <type_traits>
#include <vector>

#include "impl/gc_node.h"
#include "impl/listener.h"

namespace sodium {

namespace impl {

struct Node;
typedef struct Node Node;

struct SodiumCtxData;
typedef struct SodiumCtxData SodiumCtxData;

class IsNode;

typedef struct SodiumCtx {
    std::shared_ptr<SodiumCtxData> data;
    std::shared_ptr<unsigned int> node_count;
    std::shared_ptr<unsigned int> node_ref_count;

    SodiumCtx();

    GcCtx gc_ctx();

    Node null_node();

    template<typename K>
    typename std::result_of<K()>::type transaction(K k) {
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
    void transaction_void(K k) {
        this->transaction([k]() { k(); return 0; });
    }

    void add_dependents_to_changed_nodes(IsNode& node);

    void end_of_transaction();

    void update_node(Node& node);

    void collect_cycles();

} SodiumCtx;

}

}

#include "impl/node.h"

namespace sodium {

namespace impl {

struct SodiumCtxData {
    std::vector<std::unique_ptr<IsNode>> changed_nodes;
    std::vector<std::unique_ptr<IsNode>> visited_nodes;
    unsigned int transaction_depth;
    std::vector<std::function<void()>> pre_post;
    std::vector<std::function<void()>> post;
    std::vector<Listener> keep_alive;
    unsigned int allow_collect_cycles_counter;
};

}

}

#endif // __SODIUM_CXX_IMPL_SODIUM_CTX_H__
