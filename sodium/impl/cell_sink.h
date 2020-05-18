#ifndef __SODIUM_IMPL_CELL_SINK_H__
#define __SODIUM_IMPL_CELL_SINK_H__

#include "sodium/impl/cell.h"
#include "sodium/impl/stream_sink.h"
#include "sodium/impl/sodium_ctx.h"

namespace sodium {

namespace impl {

template <typename A>
class CellSink {
public:
    Cell<A> _cell;
    StreamSink<A> _stream_sink;

    CellSink(const CellSink<A>& cs): _cell(cs._cell), _stream_sink(cs._stream_sink) {}

    CellSink(Cell<A> cell, StreamSink<A> stream_sink): _cell(cell), _stream_sink(stream_sink) {}

    CellSink(SodiumCtx& sodium_ctx, A a): CellSink(mkCellSink(sodium_ctx, a)) {}

    static CellSink<A> mkCellSink(SodiumCtx& sodium_ctx, A a) {
        StreamSink<A> stream_sink(sodium_ctx);
        return CellSink<A>(stream_sink.stream().hold(a), stream_sink);
    }

    Cell<A> cell() {
        return this->_cell;
    }

    void send(A a) const {
        this->_stream_sink.send(a);
    }
};

}

}

#endif // __SODIUM_IMPL_CELL_SINK_H__
