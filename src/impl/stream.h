#ifndef __SODIUM_CXX_IMPL_STREAM_H__
#define __SODIUM_CXX_IMPL_STREAM_H__

#include <functional>

#include "impl/lazy.h"
#include "impl/node.h"

namespace sodium {

namespace impl {

template <typename A>
class StreamData {
public:
    nonstd::optional<A> firing_op;
    SodiumCtx sodium_ctx;
    nonstd::optional<std::function<A(const A&,const A&)>> coalescer_op;
};

template <typename A>
class WeakStream;

template <typename A>
class StreamWeakForwardRef;

template <typename A>
class Cell;

class Listener;

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

    template <typename COALESCER>
    Stream(SodiumCtx& sodium_ctx, COALESCER coalescer)
    : Stream(sodium_ctx)
    {
        this->data->coalescer_op = nonstd::optional<std::function<A(A&,A&)>>(coalescer);
    }

    template <typename MK_NODE>
    static Stream<A> mkStream(SodiumCtx& sodium_ctx, MK_NODE mk_node);

    virtual Node node() {
        return this->_node;
    }

    virtual std::unique_ptr<IsNode> box_clone() const {
        return std::unique_ptr<IsNode>(new Stream<A>(*this));
    }

    virtual std::unique_ptr<IsWeakNode> downgrade() {
        return std::unique_ptr<IsWeakNode>(new WeakStream<A>(this->data, this->_node.downgrade2()));
    }

    SodiumCtx& sodium_ctx() const {
        return this->_node.data->sodium_ctx;
    }

    WeakStream<A> downgrade2() {
        return WeakStream<A>(this->data, this->_node.downgrade2());
    }

    template <typename B, typename FN>
    Stream<typename std::result_of<FN(const A&, const B&)>> snapshot(Cell<B>& cb, FN fn) const;

    template <typename FN>
    Stream<typename std::result_of<FN(const A&)>::type> map(FN f) const;

    template <typename PRED>
    Stream<A> filter(PRED pred) const;

    Stream<A> or_else(Stream<A>& s2) const;

    template <typename FN>
    Stream<A> merge(Stream<A>& s2, FN fn) const;

    Cell<A> hold_lazy(Lazy<A> a) const;

    template <typename B, typename S, typename FN>
    Stream<B> collect_lazy(Lazy<S> init_state, FN fn) const;

    template <typename S, typename FN>
    Cell<S> accum_lazy(Lazy<S> init_state, FN fn) const;

    Stream<A> defer() const;

    Stream<A> once() const;

    template <typename K>
    Listener _listen(K k, bool weak) const;

    template <typename K>
    Listener listen_weak(K k) const;

    template <typename K>
    Listener listen(K k) const;

    void _send(A a) {
        SodiumCtx sodium_ctx = this->sodium_ctx();
        sodium_ctx.transaction_void([this, sodium_ctx, a]() mutable {
            bool is_first = !(bool)this->data->firing_op;
            if (this->data->coalescer_op) {
                std::function<A(const A&, const A&)>& coalescer = *this->data->coalescer_op;
                if (this->data->firing_op) {
                    A& firing = *this->data->firing_op;
                    this->data->firing_op = nonstd::optional<A>(coalescer(firing, a));
                } else {
                    this->data->firing_op = nonstd::optional<A>(a);
                }
            } else {
                this->data->firing_op = nonstd::optional<A>(a);
            }
            this->node().data->changed = true;
            if (is_first) {
                sodium_ctx.pre_post([this]() {
                    this->data->firing_op = nonstd::nullopt;
                    this->node().data->changed = false;
                });
            }
        });
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
        nonstd::optional<Node> node_op = this->_node.upgrade2();
        if (data && node_op) {
            Node node = *node_op;
            return nonstd::optional<Stream<A>>(Stream<A>(data, node));
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

    Stream<A> unwrap() const {
        WeakStream<A>& s = (*(this->data))[0];
        nonstd::optional<Stream<A>> s2 = s.upgrade2();
        return *s2;
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

#endif // __SODIUM_CXX_IMPL_STREAM_H__
