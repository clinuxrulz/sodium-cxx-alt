#ifndef __SODIUM_IMPL_CELL_IMPL_H__
#define __SODIUM_IMPL_CELL_IMPL_H__

#include "sodium/optional.h"
#include "sodium/impl/cell.h"
#include "sodium/impl/lazy.h"
#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/stream.h"
#include "sodium/impl/stream_impl.h"

namespace sodium {

namespace impl {

template <typename A>
class CellData {
public:
    Stream<A> stream;
    Lazy<A> value;
    nonstd::optional<A> next_value_op;

    CellData(Stream<A> stream, Lazy<A> value, nonstd::optional<A> next_value_op)
    : stream(stream), value(value), next_value_op(next_value_op)
    {}
};

template <typename A>
CellWeakForwardRef<A>::CellWeakForwardRef() {
    this->data = std::unique_ptr<std::vector<WeakCell<A>>>(new std::Vector<WeakCell<A>>());
}

template <typename A>
CellWeakForwardRef<A>& CellWeakForwardRef<A>::operator=(const Cell<A>& rhs) {
    this->data->clean();
    this->data->push_back(rhs.downgrade());
}

template <typename A>
Cell<A> CellWeakForwardRef<A>::unwrap() const {
    WeakCell<A> c1 = (*this->data)[0];
    return *c1.upgrade();
}

template <typename A>
Cell<A>::Cell(const Cell<A>& ca): data(ca.data), _node(ca._node) {}

template <typename A>
Cell<A> Cell<A>::mkConstCell(SodiumCtx& sodium_ctx, A value) {
    std::shared_ptr<CellData<A>> cell_data;
    {
        CellData<A>* _cell_data = new CellData<A>(
            Stream<A>(sodium_ctx),
            Lazy<A>::of_value(value),
            nonstd::nullopt
        );
        cell_data = std::unique_ptr<CellData<A>>(_cell_data);
    }
    return Cell<A>(
        cell_data,
        Node::mk_node(
            sodium_ctx,
            "Cell::const",
            []() {},
            std::vector<std::unique_ptr<IsNode>>()
        )
    );
}

}

}

#endif // __SODIUM_IMPL_CELL_IMPL_H__
