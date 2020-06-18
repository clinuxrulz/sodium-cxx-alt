#include "sodium/sodium.h"

namespace sodium {

namespace impl {
    SodiumCtx sodium_ctx;
}

void collect_cycles() {
    impl::sodium_ctx.collect_cycles();
}

int num_nodes() {
    return *impl::sodium_ctx.node_count;
}

}
