#ifndef __SODIUM_CXX_IMPL_STREAM_H__
#define __SODIUM_CXX_IMPL_STREAM_H__

#include <functional>
#include <list>

#include "sodium/impl/lazy.h"
#include "sodium/impl/node.h"

namespace sodium {

namespace impl {

template <typename A>
class StreamData {
public:
    boost::optional<A> firing_op;
    SodiumCtx sodium_ctx;
    boost::optional<std::function<A(const A&,const A&)>> coalescer_op;
};

template <typename A>
class WeakStream;

template <typename A>
class StreamWeakForwardRef;

template <typename A>
class Cell;

typedef struct Listener Listener;

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
        this->data->coalescer_op = boost::optional<std::function<A(A&,A&)>>(coalescer);
    }

    template <typename MK_NODE>
    static Stream<A> mkStream(SodiumCtx& sodium_ctx, MK_NODE mk_node);

    virtual Node node() const {
        return this->_node;
    }

    virtual std::unique_ptr<IsNode> box_clone() const {
        return std::unique_ptr<IsNode>(new Stream<A>(*this));
    }

    virtual std::unique_ptr<IsWeakNode> downgrade() {
        return std::unique_ptr<IsWeakNode>(new WeakStream<A>(this->data, this->_node.downgrade2()));
    }

    Dep to_dep() const {
        return Dep(this->_node.gc_node);
    }

    SodiumCtx& sodium_ctx() const {
        return this->_node.data->sodium_ctx;
    }

    WeakStream<A> downgrade2() {
        return WeakStream<A>(this->data, this->_node.downgrade2());
    }

    template <typename B, typename FN>
    Stream<typename std::result_of<FN(const A&, const B&)>::type> snapshot(const Cell<B>& cb, FN fn) const;

    template <typename FN>
    Stream<typename std::result_of<FN(const A&)>::type> map(FN f) const;

    template <typename PRED>
    Stream<A> filter(PRED pred) const;

    Stream<A> or_else(const Stream<A>& s2) const;

    template <typename FN>
    Stream<A> merge(const Stream<A>& s2, FN fn) const;

    Cell<A> hold(A a) const {
        return this->hold_lazy(Lazy<A>::of_value(a));
    }

    Cell<A> hold_lazy(Lazy<A> a) const;

    template <typename B, typename S, typename FN>
    Stream<B> collect_lazy(Lazy<S> init_state, FN fn) const;

    template <typename S, typename FN>
    Cell<S> accum_lazy(Lazy<S> init_state, FN fn) const;

    Stream<A> defer() const;

    static Stream<A> split(const Stream<std::list<A>>& sxa);

    Stream<A> once() const;

    template <typename K>
    Listener _listen(K k, bool is_weak) const;

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
                    this->data->firing_op = boost::optional<A>(coalescer(firing, a));
                } else {
                    this->data->firing_op = boost::optional<A>(a);
                }
            } else {
                this->data->firing_op = boost::optional<A>(a);
            }
            this->node().data->changed = true;
            if (is_first) {
                sodium_ctx.pre_post([this]() {
                    this->data->firing_op = boost::none;
                    this->node().data->changed = false;
                });
            }
        });
    }

    template <typename CLEANUP>
    Stream<A> add_cleanup(CLEANUP cleanup) const {
        // TODO: Implement this
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

    virtual boost::optional<std::unique_ptr<IsNode>> upgrade() {
        std::shared_ptr<StreamData<A>> data = this->data.lock();
        boost::optional<Node> node_op = this->_node.upgrade2();
        if (data && node_op) {
            Node node = std::move(*node_op);
            return boost::optional<std::unique_ptr<IsNode>>(
                std::unique_ptr<IsNode>(
                    (IsNode*)new Stream<A>(
                        data,
                        node
                    )
                )
            );
        } else {
            return boost::none;
        }
    }

    boost::optional<Stream<A>> upgrade2() {
        std::shared_ptr<StreamData<A>> data = this->data.lock();
        boost::optional<Node> node_op = this->_node.upgrade2();
        if (data && node_op) {
            Node node = *node_op;
            return boost::optional<Stream<A>>(Stream<A>(data, node));
        } else {
            return boost::none;
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

    StreamWeakForwardRef<A>& operator=(Stream<A>& rhs) {
        this->data->clear();
        this->data->push_back(rhs.downgrade2());
        return *this;
    }

    Stream<A> unwrap() const {
        WeakStream<A>& s = (*(this->data))[0];
        boost::optional<Stream<A>> s2 = s.upgrade2();
        return *s2;
    }
};

}

}

#endif // __SODIUM_CXX_IMPL_STREAM_H__
