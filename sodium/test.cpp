#include <iostream>

#include "sodium/sodium.h"
#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/gc_node.h"
#include "sodium/impl/lazy.h"

struct TestStruct {
    int a;
    int b;
};

int main() {
    sodium::impl::Lazy<int> a([]() { return 1; });
    const int& b = *a;
    sodium::impl::Lazy<TestStruct> c([]() { return TestStruct(); });
    int d = c->b;
    sodium::impl::SodiumCtx sodium_ctx;
    sodium::impl::Stream<int> s(sodium_ctx);
    auto s2 = s.map([](const int& x) { return x + 1; });
    sodium_ctx.transaction_void([]() {});
    sodium::impl::GcCtx gc_ctx;
    sodium::impl::GcNode node(gc_ctx, "test_node", []() {}, [](std::function<sodium::impl::Tracer> tracer) {});
    std::cout << "test" << std::endl;
    return 0;
}