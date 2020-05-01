#include <iostream>

#include "impl/gc_node.h"

int main() {
    sodium::impl::GcCtx gc_ctx;
    sodium::impl::GcNode node(gc_ctx, "test_node", []() {}, [](std::function<sodium::impl::Tracer> tracer) {});
    std::cout << "test" << std::endl;
    return 0;
}
