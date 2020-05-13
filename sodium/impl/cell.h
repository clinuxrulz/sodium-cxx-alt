#ifndef __SODIUM_IMPL_CELL_H__
#define __SODIUM_IMPL_CELL_H__

#include "sodium/optional.h"
#include "sodium/impl/lazy.h"
#include "sodium/impl/node.h"

#include <memory>
#include <vector>

namespace sodium {

namespace impl {

typedef struct WeakNode WeakNode;

template <typename A>
class CellData;

template <typename A>
class WeakCell;

class SodiumCtx;

class Dep;

template <typename A>
class Stream;

template <typename A>
class Lazy;

typedef struct Listener Listener;

template <typename A>
class Cell {
public:
    std::shared_ptr<CellData<A>> data;
    Node _node;

    Cell(const Cell<A>& ca);

    Cell(std::shared_ptr<CellData<A>> data, Node node): data(data), _node(node) {}

    Cell(SodiumCtx& sodium_ctx, A value): Cell(mkConstCell(sodium_ctx, value)) {}

    Cell(SodiumCtx& sodium_ctx, Stream<A> stream, Lazy<A> value): Cell(mkCell(sodium_ctx, stream, value)) {}

    static Cell<A> mkConstCell(SodiumCtx& sodium_ctx, A value);

    static Cell<A> mkCell(SodiumCtx& sodium_ctx, Stream<A> stream, Lazy<A> value);

    SodiumCtx& sodium_ctx() const;

    Dep to_dep() const;

    WeakCell<A> downgrade() const;

    A sample() const;

    Lazy<A> sample_lazy() const;

    Stream<A> updates() const;

    Stream<A> value() const;

    template <typename FN>
    Cell<typename std::result_of<FN(const A&)>::type> map(FN fn) const;

    template <typename B, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&)>::type> lift2(const Cell<B>& cb, FN fn) const;

    template <typename B, typename C, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&, const C&)>::type> lift3(const Cell<B>& cb, const Cell<C>& cc, FN fn) const;

    template <typename B, typename C, typename D, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&, const C&, const D&)>::type> lift4(const Cell<B>& cb, const Cell<C>& cc, const Cell<D>& cd, FN fn) const;

    template <typename B, typename C, typename D, typename E, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&, const C&, const D&, const E&)>::type> lift5(const Cell<B>& cb, const Cell<C>& cc, const Cell<D>& cd, const Cell<E>& ce, FN fn) const;

    template <typename B, typename C, typename D, typename E, typename F, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&, const C&, const D&, const E&, const F&)>::type> lift6(const Cell<B>& cb, const Cell<C>& cc, const Cell<D>& cd, const Cell<E>& ce, const Cell<F>& cf, FN fn) const;

    static Cell<A> switch_s(const Cell<Stream<A>>& csa);

    static Cell<A> switch_c(const Cell<Cell<A>>& cca);

    template <typename K>
    Listener listen_weak(K k) const;

    template <typename K>
    Listener listen(K k) const;
};

template <typename A>
class WeakCell {
public:
    std::weak_ptr<CellData<A>> data;
    WeakNode _node;

    WeakCell(std::weak_ptr<CellData<A>> data, WeakNode node): data(data), _node(node) {}

    nonstd::optional<Cell<A>> upgrade() const;
};

template <typename A>
class CellWeakForwardRef {
public:
    std::shared_ptr<std::vector<WeakCell<A>>> data;

    CellWeakForwardRef();

    CellWeakForwardRef<A>& operator=(const Cell<A>& rhs);

    Cell<A> unwrap() const;
};

}

}

#endif // __SODIUM_IMPL_CELL_H__
