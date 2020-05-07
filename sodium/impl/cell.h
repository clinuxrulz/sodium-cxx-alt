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

template <typename A>
class Cell {
public:
    std::shared_ptr<CellData<A>> data;
    Node _node;

    WeakCell<A> downgrade() const;
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
