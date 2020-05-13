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
    sodium::impl::StreamLoop<int> sl(sodium_ctx);
    sodium::impl::Stream<int> s(sodium_ctx);
    sodium::impl::StreamSink<int> ss(sodium_ctx);
    sodium::impl::CellSink<int> cs(sodium_ctx, 1);
    sodium_ctx.transaction_void([sodium_ctx]() mutable {
        sodium::impl::CellLoop<int> cl(sodium_ctx);
        sodium::impl::Cell<int> ca(sodium_ctx, 1);
        cl.loop(ca);
        sodium::impl::Stream<int> s = ca.value();
        sodium::impl::Cell<int> cb = ca.map([](const int& x) { return 2 * x; });
        sodium::impl::Cell<int> cc = ca.lift2(cb, [](const int& x, const int& y) { return x + y; });
        sodium::impl::Cell<int> cd =
            ca.lift3(cb, cc, [](const int& a, const int& b, const int& c) { return a*b*c; });
        sodium::impl::Cell<int> ce =
            ca.lift4(
                cb,
                cc,
                cd,
                [](const int& a, const int& b, const int& c, const int& d) {
                    return a - b - c - d;
                }
            );
        sodium::impl::Cell<int> cf =
            ca.lift5(
                cb,
                cc,
                cd,
                ce,
                [](const int& a, const int& b, const int& c, const int& d, const int& e) {
                    return a + b + c + d + e;
                }
            );
    });
    sl.loop(s);
    ss.send(2);
    auto s2 = s
        .map(sodium::impl::lambda1([](const int& x) { return x + 1; }) << s.to_dep())
        .filter([](const int& x) { return x > 0; })
        .or_else(s);
    sodium_ctx.transaction_void([]() {});
    sodium::impl::GcCtx gc_ctx;
    sodium::impl::GcNode node(gc_ctx, "test_node", []() {}, [](std::function<sodium::impl::Tracer> tracer) {});
    std::cout << "test" << std::endl;
    return 0;
}
