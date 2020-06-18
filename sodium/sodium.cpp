#include "sodium/sodium.h"

namespace sodium {

namespace impl {
    SodiumCtx sodium_ctx;
}

void reset_num_nodes() {
    *impl::sodium_ctx.node_count = 0;
    *impl::sodium_ctx.node_ref_count = 0;
}

void collect_cycles() {
    impl::sodium_ctx.collect_cycles();
}

int num_nodes() {
    return *impl::sodium_ctx.node_count;
}

}
