#ifndef __SODIUM_IMPL_STREAM_IMPL_H__
#define __SODIUM_IMPL_STREAM_IMPL_H__

#include "sodium/impl/stream.h"

namespace sodium {

namespace impl {

template <typename A>
template <typename MK_NODE>
Stream<A> Stream<A>::mkStream(SodiumCtx& sodium_ctx, MK_NODE mk_node) {
    StreamWeakForwardRef<A> stream_weak_forward_ref;
    Node node = mk_node(stream_weak_forward_ref);
    std::shared_ptr<StreamData<A>> stream_data;
    {
        StreamData<A>* _stream_data = new StreamData<A>();
        _stream_data->firing_op = nonstd::nullopt;
        _stream_data->sodium_ctx = sodium_ctx;
        _stream_data->coalescer_op = nonstd::nullopt;
        stream_data = std::unique_ptr<StreamData<A>>(_stream_data);
    }
    Stream<A> s(stream_data, node);
    stream_weak_forward_ref = s;
    (node.data->update)();
    if (s.data->firing_op) {
        node.data->changed = true;
    }
    sodium_ctx.pre_post([s]() {
        s.data->firing_op = nonstd::nullopt;
        s._node.data->changed = false;
    });
    return s;
}

template <typename A>
template <typename FN>
Stream<typename std::result_of<FN(const A&)>::type> Stream<A>::map(FN fn) const {
    Stream<A> this_ = *this;
    return Stream::mkStream(
        this->sodium_ctx(),
        [this_, fn](StreamWeakForwardRef<A> s) {
            std::vector<std::unique_ptr<IsNode>> dependencies;
            dependencies.push_back(this_.box_clone());
            return Node::mk_node(
                this_.sodium_ctx(),
                "Stream::map",
                [this_, s, fn]() {
                    nonstd::optional<A>& firing_op = this_.data->firing_op;
                    if (firing_op) {
                        s.unwrap()._send(fn(*firing_op));
                    }
                },
                std::move(dependencies)
            );
        }
    );
}

}

}

#endif // __SODIUM_IMPL_STREAM_IMPL_H__