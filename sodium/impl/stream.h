#ifndef __SODIUM_CXX_IMPL_STREAM_H__
#define __SODIUM_CXX_IMPL_STREAM_H__

#include <functional>
#include <list>
#include <memory>

#include "sodium/impl/lazy.h"
#include "sodium/impl/node.h"

namespace sodium {

namespace impl {

template <typename A>
class StreamData {
public:
    boost::optional<std::shared_ptr<A>> firing_op;
    SodiumCtx sodium_ctx;
    boost::optional<std::function<A(const A&,const A&)>> coalescer_op;
    std::vector<std::function<void()>> cleanups;

    ~StreamData() {
        for (auto cleanup = cleanups.begin(); cleanup != cleanups.end(); ++cleanup) {
            (*cleanup)();
        }
    }
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

    Stream(const SodiumCtx& sodium_ctx)
    : Stream(
        mkStream(
            sodium_ctx,
            [sodium_ctx](StreamWeakForwardRef<A> s) {
                return sodium_ctx.null_node();
            }
        )
    )
    {
    }

    template <typename COALESCER>
    Stream(SodiumCtx& sodium_ctx, COALESCER coalescer)
    : Stream(sodium_ctx)
    {
        this->data->coalescer_op = boost::optional<std::function<A(const A&,const A&)>>(coalescer);
    }

    template <typename MK_NODE>
    static Stream<A> mkStream(const SodiumCtx& sodium_ctx, MK_NODE mk_node);

    virtual const Node& node() const {
        return this->_node;
    }

    virtual std::unique_ptr<IsNode> box_clone() const {
        return std::unique_ptr<IsNode>(new Stream<A>(*this));
    }

    virtual std::unique_ptr<IsWeakNode> downgrade() const {
        return std::unique_ptr<IsWeakNode>(new WeakStream<A>(this->data, this->_node.downgrade2()));
    }

    Dep to_dep() const {
        return Dep(this->_node.gc_node);
    }

    SodiumCtx& sodium_ctx() const {
        return this->_node.data->sodium_ctx;
    }

    WeakStream<A> downgrade2() const {
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

    Cell<A> hold(const A& a) const {
        return this->hold_lazy(Lazy<A>::of_value(a));
    }

    Cell<A> hold(A&& a) const {
        return this->hold_lazy(Lazy<A>::of_value(std::move(a)));
    }

    Cell<A> hold_lazy(Lazy<A> a) const;

    template <typename S, typename FN>
    Stream<typename std::tuple_element<
            0, typename std::result_of<FN(A, S)>::type>::type>
    collect_lazy(Lazy<S> init_state, FN fn) const;

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
                    A& firing = **this->data->firing_op;
                    A firing2 = coalescer(firing, std::move(a));
                    boost::optional<std::shared_ptr<A>> firing_op = boost::optional<std::shared_ptr<A>>(std::unique_ptr<A>(new A(firing2)));
                    this->data->firing_op = firing_op;
                } else {
                    boost::optional<std::shared_ptr<A>> firing_op = boost::optional<std::shared_ptr<A>>(std::unique_ptr<A>(new A(std::move(a))));
                    this->data->firing_op = firing_op;
                }
            } else {
                boost::optional<std::shared_ptr<A>> firing_op = boost::optional<std::shared_ptr<A>>(std::unique_ptr<A>(new A(std::move(a))));
                this->data->firing_op = firing_op;
            }
            this->node().data->changed = true;
            if (is_first) {
                Stream<A> this_ = *this;
                sodium_ctx.pre_post([this_]() {
                    this_.data->firing_op = boost::none;
                    this_.node().data->changed = false;
                });
            }
        });
    }

    template <typename CLEANUP>
    Stream<A> add_cleanup(CLEANUP cleanup) const {
        Stream<A> this_ = *this;
        Stream<A> r = Stream<A>::mkStream(
            this->sodium_ctx(),
            [this_](StreamWeakForwardRef<A> s) {
                std::vector<std::unique_ptr<IsNode>> dependencies;
                dependencies.push_back(this_.box_clone());
                Node node = Node::mk_node(
                    this_.sodium_ctx(),
                    "Stream::map",
                    [this_, s]() {
                        boost::optional<std::shared_ptr<A>>& firing_op = this_.data->firing_op;
                        if (firing_op) {
                            s.unwrap()._send(**firing_op);
                        }
                    },
                    std::move(dependencies)
                );
                node.add_update_dependency(this_.to_dep());
                return node;
            }
        );
        r.data->cleanups.push_back(std::function<void()>(cleanup));
        return r;
    }
};

template <typename A>
class WeakStream: public IsWeakNode {
public:
    std::weak_ptr<StreamData<A>> data;
    WeakNode _node;

    WeakStream(std::weak_ptr<StreamData<A>> data, WeakNode node): data(data), _node(node) {}

    virtual const WeakNode& node() const {
        return this->_node;
    }

    virtual std::unique_ptr<IsWeakNode> box_clone() const {
        return std::unique_ptr<IsWeakNode>(new WeakStream<A>(*this));
    }

    virtual boost::optional<std::unique_ptr<IsNode>> upgrade() const {
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

    boost::optional<Stream<A>> upgrade2() const {
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
