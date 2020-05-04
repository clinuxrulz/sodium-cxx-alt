#include <iostream>

#include "impl/sodium_ctx.h"
#include "impl/stream.h"
#include "impl/gc_node.h"

int main() {
    sodium::impl::SodiumCtx sodium_ctx;
    sodium::impl::Stream<int> s(sodium_ctx);
    sodium_ctx.transaction_void([]() {});
    sodium::impl::GcCtx gc_ctx;
    sodium::impl::GcNode node(gc_ctx, "test_node", []() {}, [](std::function<sodium::impl::Tracer> tracer) {});
    std::cout << "test" << std::endl;
    return 0;
}
