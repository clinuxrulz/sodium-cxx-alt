#ifndef __SODIUM_CXX_IMPL_STREAM_H__
#define __SODIUM_CXX_IMPL_STREAM_H__

#include <functional>

#include "impl/node.h"

namespace sodium {

namespace impl {

template <typename A>
class StreamData {
public:
    nonstd::optional<A> firing_op;
    SodiumCtx sodium_ctx;
    nonstd::optional<std::function<A(A&,A&)>> coalescer_op;
};

template <typename A>
class WeakStream;

template <typename A>
class StreamWeakForwardRef;

template <typename A>
class Stream: public IsNode {
public:
    std::shared_ptr<StreamData<A>> data;
    Node _node;

    Stream(const Stream<A>& s): data(s.data), _node(s._node) {}

    Stream(std::shared_ptr<StreamData<A>> data, Node node): data(data), _node(node) {}

    Stream(SodiumCtx& sodium_ctx)
    : Stream(
        mkStream(
            sodium_ctx,
            [this](StreamWeakForwardRef<A> s) {
                return this->_node.sodium_ctx.null_node();
            }
        )
    )
    {
    }

    template <typename MK_NODE>
    static Stream<A> mkStream(SodiumCtx& sodium_ctx, MK_NODE mk_node);

    virtual Node node() {
        return this->_node;
    }

    virtual std::unique_ptr<IsNode> box_clone() {
        return std::unique_ptr<IsNode>(new Stream<A>(*this));
    }

    virtual std::unique_ptr<IsWeakNode> downgrade() {
        return std::unique_ptr<IsWeakNode>(new WeakStream<A>(this->data, this->_node.downgrade2()));
    }

    WeakStream<A> downgrade2() {
        return WeakStream<A>(this->data, this->_node.downgrade2());
    }
};

template <typename A>
class WeakStream: public IsWeakNode {
public:
    std::weak_ptr<StreamData<A>> data;
    WeakNode _node;

    WeakStream(std::weak_ptr<StreamData<A>> data, WeakNode node): data(data), _node(node) {}

    virtual WeakNode node() {
        return this->_node;
    }

    virtual std::unique_ptr<IsWeakNode> box_clone() {
        return std::unique_ptr<IsWeakNode>(new WeakStream<A>(*this));
    }

    virtual nonstd::optional<std::unique_ptr<IsNode>> upgrade() {
        std::shared_ptr<StreamData<A>> data = this->data.lock();
        nonstd::optional<Node> node_op = this->_node.upgrade2();
        if (data && node_op) {
            Node node = std::move(*node_op);
            return nonstd::optional<std::unique_ptr<IsNode>>(
                std::unique_ptr<IsNode>(
                    (IsNode*)new Stream<A>(
                        data,
                        node
                    )
                )
            );
        } else {
            return nonstd::nullopt;
        }
    }

    nonstd::optional<Stream<A>> upgrade2() {
        std::shared_ptr<StreamData<A>> data = this->data.lock();
        if (data) {
            return nonstd::optional<Stream<A>>(new Stream<A>(data, node));
        } else {
            return nonstd::nullopt;
        }
    }
};

template <typename A>
class StreamWeakForwardRef {
public:
    std::shared_ptr<std::vector<WeakStream<A>>> data;

    StreamWeakForwardRef() {
        this->data = std::unique_ptr<std::vector<WeakStream<A>>>(
            new std::vector<WeakStream<A>>()
        );
    }

    StreamWeakForwardRef& operator=(Stream<A>& rhs) {
        this->data->clear();
        this->data->push_back(rhs.downgrade2());
        return *this;
    }

    Stream<A>& operator*() const {
        return (*(this->data))[0];
    }
};


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

}

}

#endif // __SODIUM_CXX_IMPL_STREAM_H__
