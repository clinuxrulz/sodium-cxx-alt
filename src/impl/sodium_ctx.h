#ifndef __SODIUM_CXX_IMPL_SODIUM_CTX_H__
#define __SODIUM_CXX_IMPL_SODIUM_CTX_H__

#include <memory>
#include <vector>

#include "impl/gc_node.h"
#include "impl/listener.h"

namespace sodium {

namespace impl {

struct Node;
typedef struct Node Node;

struct SodiumCtxData;
typedef struct SodiumCtxData SodiumCtxData;

typedef struct SodiumCtx {
    std::shared_ptr<SodiumCtxData> data;
    std::shared_ptr<unsigned int> node_count;
    std::shared_ptr<unsigned int> node_ref_count;

    SodiumCtx();

    GcCtx gc_ctx();

    Node null_node();
    
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
