#ifndef __SODIUM_IMPL_CELL_LOOP_H__
#define __SODIUM_IMPL_CELL_LOOP_H__

#include <boost/optional.hpp>
#include "sodium/impl/cell.h"
#include "sodium/impl/lazy.h"
#include "sodium/impl/stream_loop.h"

#include <memory>
#include <stdexcept>
#include <vector>

namespace sodium {

namespace impl {

template <typename A>
class CellLoop {
public:
    std::shared_ptr<std::vector<Lazy<A>>> _init_value_op;
    StreamLoop<A> _stream_loop;
    Cell<A> _cell;

    CellLoop(const CellLoop<A>& cl)
    : _init_value_op(cl._init_value_op), _stream_loop(cl._stream_loop), _cell(cl._cell) {}

    CellLoop(std::shared_ptr<std::vector<Lazy<A>>> init_value_op, StreamLoop<A> stream_loop, Cell<A> cell)
    : _init_value_op(init_value_op), _stream_loop(stream_loop), _cell(cell) {}

    CellLoop(SodiumCtx& sodium_ctx): CellLoop(mkCellLoop(sodium_ctx)) {}

    static CellLoop<A> mkCellLoop(SodiumCtx& sodium_ctx) {
        std::shared_ptr<std::vector<Lazy<A>>> init_value_op = std::unique_ptr<std::vector<Lazy<A>>>(new std::vector<Lazy<A>>());
        Lazy<A> init_value = Lazy<A>([init_value_op]() {
            std::vector<Lazy<A>> init_value = *init_value_op;
            if (init_value.size() == 0) {
                throw std::runtime_error("CellLoop sampled before looped");
            }
            return *init_value[0];
        });
        StreamLoop<A> stream_loop(sodium_ctx);
        Stream<A> stream = stream_loop.stream();
        return CellLoop(init_value_op, stream_loop, stream.hold_lazy(init_value));
    }

    Cell<A> cell() {
        return this->_cell;
    }

    void loop(const Cell<A>& ca) {
        this->_stream_loop.loop(ca.updates());
        this->_init_value_op->clear();
        this->_init_value_op->push_back(ca.sample_lazy());
    }

};

}

}

#endif // __SODIUM_IMPL_CELL_LOOP_H__
