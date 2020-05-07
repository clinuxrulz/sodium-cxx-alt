#ifndef __SODIUM_IMPL_CELL_H__
#define __SODIUM_IMPL_CELL_H__

#include "sodium/optional.h"

#include <memory>
#include <vector>

namespace sodium {

namespace impl {

typedef struct Node Node;

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

struct Listener;
typedef struct Listener Listener;

template <typename A>
class Cell {
public:
    std::shared_ptr<CellData<A>> data;
    Node _node;

    Cell(SodiumCtx& sodium_ctx, A value);

    Cell(Stream<A> stream, A value);

    SodiumCtx& sodium_ctx() const;

    Dep to_dep() const;

    WeakCell<A> downgrade() const;

    A sample() const;

    Lazy<A> sample_lazy() const;

    Stream<A> updates() const;

    Stream<A> value() const;

    template <typename FN>
    Cell<typename std::result_of<FN(const A&)>::type> map(FN f) const;

    template <typename B, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&)>> lift2(const Cell<B>& cb, FN fn) const;

    template <typename B, typename C, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&, const C&)>> lift3(const Cell<B>& cb, const Cell<C>& cc, FN fn) const;

    template <typename B, typename C, typename D, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&, const C&, const D&)>> lift4(const Cell<B>& cb, const Cell<C>& cc, const Cell<D>& cd, FN fn) const;

    template <typename B, typename C, typename D, typename E, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&, const C&, const D&, const E&)>> lift5(const Cell<B>& cb, const Cell<C>& cc, const Cell<D>& cd, const Cell<E>& ce, FN fn) const;

    template <typename B, typename C, typename D, typename E, typename F, typename FN>
    Cell<typename std::result_of<FN(const A&, const B&, const C&, const D&, const E&, const F&)>> lift5(const Cell<B>& cb, const Cell<C>& cc, const Cell<D>& cd, const Cell<E>& ce, const Cell<F>& cf, FN fn) const;

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