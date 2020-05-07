#ifndef __SODIUM_IMPL_CELL_IMPL_H__
#define __SODIUM_IMPL_CELL_IMPL_H__

#include "sodium/optional.h"
#include "sodium/impl/cell.h"
#include "sodium/impl/lazy.h"
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

}

}

#endif // __SODIUM_IMPL_CELL_IMPL_H__
