#ifndef __SODIUM_IMPL_STREAM_LOOP_IMPL_H__
#define __SODIUM_IMPL_STREAM_LOOP_IMPL_H__

#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/stream.h"
#include "sodium/impl/stream_impl.h"
#include "sodium/impl/stream_loop.h"

namespace sodium {

namespace impl {

template <typename A>
class StreamLoopData {
public:
    Stream<A> stream;
    bool looped;

    StreamLoopData(const Stream<A>& stream): stream(stream), looped(false) {}
};

template <typename A>
StreamLoop<A>::StreamLoop(std::shared_ptr<StreamLoopData<A>> data, GcNode gc_node)
: data(data), gc_node(gc_node) {}

/*
    static StreamLoop<A> mkStreamLoop(SodiumCtx& sodium_ctx);
*/

template <typename A>
StreamLoop<A> StreamLoop<A>::mkStreamLoop(SodiumCtx& sodium_ctx) {
    std::shared_ptr<StreamLoopData<A>> stream_loop_data;
    {
        StreamLoopData<A> *_stream_loop_data = new StreamLoopData<A>(Stream<A>(sodium_ctx));
        stream_loop_data = std::unique_ptr<StreamLoopData<A>>(_stream_loop_data);
    }
    std::weak_ptr<StreamLoopData<A>> weak_stream_loop_data = stream_loop_data;
    auto gc_node_deconstructor = [weak_stream_loop_data, sodium_ctx]() mutable {
        std::shared_ptr<StreamLoopData<A>> stream_loop_data = weak_stream_loop_data.lock();
        if (!stream_loop_data) {
            return;
        }
        stream_loop_data->stream = Stream<A>(sodium_ctx);
    };
    auto gc_node_trace = [weak_stream_loop_data, sodium_ctx](std::function<Tracer> tracer) {
        std::shared_ptr<StreamLoopData<A>> stream_loop_data = weak_stream_loop_data.lock();
        if (!stream_loop_data) {
            return;
        }
        tracer(stream_loop_data->stream.gc_node());
    };
    return StreamLoop<A>(
        stream_loop_data,
        GcNode(
            sodium_ctx.gc_ctx(),
            "StreamLoop::new",
            gc_node_deconstructor,
            gc_node_trace
        )
    );
}

template <typename A>
StreamLoop<A>::StreamLoop(const StreamLoop<A>& stream_loop): data(stream_loop.data), gc_node(stream_loop.gc_node) {
    this->gc_node.inc_ref();
}

template <typename A>
StreamLoop<A>::~StreamLoop() {
    this->gc_node.dec_ref();
}

template <typename A>
StreamLoop<A>& StreamLoop<A>::operator=(const StreamLoop<A>& rhs) {
    this->gc_node->dec_ref();
    this->data = rhs.data;
    this->gc_node = rhs.gc_node;
    this->gc_node->inc_ref();
}

template <typename A>
Stream<A> StreamLoop<A>::stream() const {
    return this->data->stream;
}

template <typename A>
void StreamLoop<A>::loop(const Stream<A>& s) {
    if (this->data->looped) {
        // TODO: panic!("StreamLoop already looped.");
    }
    this->data->looped = true;
    this->data->stream.node().add_dependency(s);
    this->data->stream.node().add_update_dependency(s.to_dep());
    WeakStream<A> s_out = this->data->stream.downgrade2();
    this->data->stream.node().data->update = [s, s_out]() mutable {
        if (s.data->firing_op) {
            A& firing = *s.data->firing_op;
            (*s_out.upgrade2())._send(firing);
        }
    };
}

}

}

#endif // __SODIUM_IMPL_STREAM_LOOP_IMPL_H__
