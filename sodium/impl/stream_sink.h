#ifndef __SODIUM_IMPL_STREAM_SINK_H__
#define __SODIUM_IMPL_STREAM_SINK_H__

#include <boost/optional.hpp>
#include "sodium/impl/stream.h"

namespace sodium {

namespace impl {

template <typename A>
class WeakStreamSink;

template <typename A>
class StreamSink {
public:
    Stream<A> _stream;
    SodiumCtx _sodium_ctx;

    StreamSink(Stream<A> stream, SodiumCtx sodium_ctx): _stream(sodium_ctx), _sodium_ctx(_sodium_ctx) {}

    StreamSink(SodiumCtx sodium_ctx): _stream(sodium_ctx), _sodium_ctx(sodium_ctx) {}

    template <typename COALESCER>
    StreamSink(SodiumCtx sodium_ctx, COALESCER coalescer): _stream(sodium_ctx, coalescer), _sodium_ctx(sodium_ctx) {}

    Stream<A> stream() const {
        return _stream;
    }

    void send(A a) const {
        StreamSink<A> this_ = *this;
        this->_sodium_ctx.transaction_void([this_, a]() mutable {
            this_._stream.node().data->changed = true;
            this_._sodium_ctx.data->changed_nodes.push_back(this_._stream.box_clone());
            this_._stream._send(a);
        });
    }

    WeakStreamSink<A> downgrade();
};

template <typename A>
class WeakStreamSink {
public:
    WeakStream<A> _stream;
    SodiumCtx _sodium_ctx;

    WeakStreamSink(WeakStream<A> stream, SodiumCtx sodium_ctx)
    : _stream(stream), sodium_ctx(sodium_ctx) {}

    boost::optional<StreamSink<A>> upgrade() {
        boost::optional<Stream<A>> stream_op = this->_stream.upgrade2();
        if (stream_op) {
            Stream<A>& stream = *stream_op;
            return boost::optional<StreamSink<A>>(StreamSink<A>(stream));
        } else {
            return boost::none;
        }
    }
};

template <typename A>
WeakStreamSink<A> StreamSink<A>::downgrade() {
    WeakStream<A> stream = this->_stream.downgrade2();
    SodiumCtx sodium_ctx = this->_sodium_ctx;
    return WeakStreamSink<A>(stream, sodium_ctx);
}

}

}

#endif // __SODIUM_IMPL_STREAM_SINK_H__
