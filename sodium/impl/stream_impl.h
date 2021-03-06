#ifndef __SODIUM_IMPL_STREAM_IMPL_H__
#define __SODIUM_IMPL_STREAM_IMPL_H__

#include "sodium/impl/cell.h"
#include "sodium/impl/lambda.h"
#include "sodium/impl/listener.h"
#include "sodium/impl/stream.h"

namespace sodium {

namespace impl {

template <typename A>
template <typename MK_NODE>
Stream<A> Stream<A>::mkStream(const SodiumCtx& sodium_ctx, MK_NODE mk_node) {
    StreamWeakForwardRef<A> stream_weak_forward_ref;
    Node node = mk_node(stream_weak_forward_ref);
    std::shared_ptr<StreamData<A>> stream_data;
    {
        StreamData<A>* _stream_data = new StreamData<A>();
        _stream_data->firing_op = boost::none;
        _stream_data->sodium_ctx = sodium_ctx;
        _stream_data->coalescer_op = boost::none;
        stream_data = std::unique_ptr<StreamData<A>>(_stream_data);
    }
    Stream<A> s(stream_data, node);
    stream_weak_forward_ref = s;
    sodium_ctx.pre_eot([sodium_ctx, s, node]() {
        (node.data->update)();
        if (s.data->firing_op) {
            node.data->changed = true;
            sodium_ctx.pre_post([s]() {
                s.data->firing_op = boost::none;
                s._node.data->changed = false;
            });
        }
    });
    return s;
}

template <typename A>
template <typename B, typename FN>
Stream<typename std::result_of<FN(const A&, const B&)>::type> Stream<A>::snapshot(const Cell<B>& cb, FN fn) const {
    std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
    return this->map(lambda1([fn,cb](const A& a) { return fn(a, cb.sample()); }) << fn_deps << cb);
}

template <typename A>
template <typename FN>
Stream<typename std::result_of<FN(const A&)>::type> Stream<A>::map(FN fn) const {
    typedef typename std::result_of<FN(const A&)>::type R;
    Stream<A> this_ = *this;
    return Stream<R>::mkStream(
        this->sodium_ctx(),
        [this_, fn](StreamWeakForwardRef<R> s) {
            std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
            std::vector<std::unique_ptr<IsNode>> dependencies;
            dependencies.push_back(this_.box_clone());
            Node node = Node::mk_node(
                this_.sodium_ctx(),
                "Stream::map",
                [this_, s, fn]() {
                    boost::optional<std::shared_ptr<A>>& firing_op = this_.data->firing_op;
                    if (firing_op) {
                        s.unwrap()._send(fn(**firing_op));
                    }
                },
                std::move(dependencies)
            );
            node.add_update_dependency(this_.to_dep());
            node.add_update_dependencies(fn_deps);
            return node;
        }
    );
}

template <typename A>
template <typename PRED>
Stream<A> Stream<A>::filter(PRED pred) const {
    Stream<A> this_ = *this;
    return Stream::mkStream(
        this->sodium_ctx(),
        [this_, pred](StreamWeakForwardRef<A> s) {
            std::vector<Dep> pred_deps = GetDeps<PRED>::call(pred);
            std::vector<std::unique_ptr<IsNode>> dependencies;
            dependencies.push_back(this_.box_clone());
            Node node = Node::mk_node(
                this_.sodium_ctx(),
                "Stream::filter",
                [this_, s, pred]() {
                    if (this_.data->firing_op) {
                        A& firing = **this_.data->firing_op;
                        if (pred(firing)) {
                            s.unwrap()._send(firing);
                        }
                    }
                },
                std::move(dependencies)
            );
            node.add_update_dependency(this_.to_dep());
            node.add_update_dependencies(pred_deps);
            return node;
        }
    );
}

template <typename A>
Stream<A> Stream<A>::or_else(const Stream<A>& s2) const {
    return this->merge(s2, [](const A& lhs, const A& rhs) { return lhs; });
}

template <typename A>
template <typename FN>
Stream<A> Stream<A>::merge(const Stream<A>& s2, FN fn) const {
    Stream<A> this_ = *this;
    return Stream::mkStream(
        this->sodium_ctx(),
        [this_, s2, fn](StreamWeakForwardRef<A> s) {
            std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
            std::vector<std::unique_ptr<IsNode>> dependencies;
            dependencies.push_back(this_.box_clone());
            dependencies.push_back(s2.box_clone());
            Node node = Node::mk_node(
                this_.sodium_ctx(),
                "Stream::merge",
                [this_, s2, s, fn]() {
                    boost::optional<std::shared_ptr<A>>& firing1_op = this_.data->firing_op;
                    boost::optional<std::shared_ptr<A>>& firing2_op = s2.data->firing_op;
                    if (firing1_op) {
                        A& firing1 = **firing1_op;
                        if (firing2_op) {
                            A& firing2 = **firing2_op;
                            s.unwrap()._send(fn(firing1, firing2));
                        } else {
                            s.unwrap()._send(firing1);
                        }
                    } else {
                        if (firing2_op) {
                            A& firing2 = **firing2_op;
                            s.unwrap()._send(firing2);
                        }
                    }
                },
                std::move(dependencies)
            );
            node.add_update_dependencies(fn_deps);
            node.add_update_dependency(this_.to_dep());
            node.add_update_dependency(s2.to_dep());
            return node;
        }
    );
}

template <typename A>
Cell<A> Stream<A>::hold_lazy(Lazy<A> a) const {
    SodiumCtx sodium_ctx = this->sodium_ctx();
    return sodium_ctx.transaction([sodium_ctx, this, a]() mutable {
        return Cell<A>(sodium_ctx, *this, a);
    });
}

template <typename A>
template <typename S, typename FN>
Stream<typename std::tuple_element<
        0, typename std::result_of<FN(A, S)>::type>::type>
Stream<A>::collect_lazy(Lazy<S> init_state, FN fn) const {
    typedef typename std::tuple_element<
        0, typename std::result_of<FN(A, S)>::type>::type B;
    SodiumCtx sodium_ctx = this->sodium_ctx();
    Stream<A> sa = *this;
    return this->sodium_ctx().transaction([sodium_ctx, sa, init_state, fn]() {
        StreamLoop<S> ssl(sodium_ctx);
        Cell<S> cs = ssl.stream().hold_lazy(init_state);
        Stream<std::tuple<B,S>> sbs = sa.snapshot(cs, fn);
        Stream<B> sb = sbs.map([](const std::tuple<B,S> x) { return std::get<0>(x); });
        Stream<S> ss = sbs.map([](const std::tuple<B,S> x) { return std::get<1>(x); });
        ssl.loop(ss);
        return sb;
    });
}

template <typename A>
template <typename S, typename FN>
Cell<S> Stream<A>::accum_lazy(Lazy<S> init_state, FN fn) const {
    SodiumCtx sodium_ctx = this->sodium_ctx();
    Stream<A> sa = *this;
    return this->sodium_ctx().transaction([sodium_ctx, sa, init_state, fn]() {
        StreamLoop<S> ssl(sodium_ctx);
        Cell<S> cs = ssl.stream().hold_lazy(init_state);
        Stream<S> ss = sa.snapshot(cs, fn);
        ssl.loop(ss);
        return cs;
    });
}

template <typename A>
Stream<A> Stream<A>::defer() const {
    SodiumCtx sodium_ctx = this->sodium_ctx();
    Stream<A> this_ = *this;
    return this->sodium_ctx().transaction([sodium_ctx, this_]() {
        StreamSink<A> ss(sodium_ctx);
        Stream<A> s = ss.stream();
        WeakStreamSink<A> weak_ss = ss.downgrade();
        Listener listener = this_.listen_weak([sodium_ctx, weak_ss](const A& a) {
            boost::optional<StreamSink<A>> ss_op = weak_ss.upgrade();
            StreamSink<A> ss = *ss_op;
            sodium_ctx.post([ss, a] { ss.send(a); });
        });
        s.node().add_keep_alive(listener.gc_node);
        return s;
    });
}

template <typename A>
Stream<A> Stream<A>::split(const Stream<std::list<A>>& sxa) {
    SodiumCtx sodium_ctx = sxa.sodium_ctx();
    return sodium_ctx.transaction([sodium_ctx, sxa]() {
        StreamSink<A> ss(sodium_ctx);
        Stream<A> s = ss.stream();
        WeakStreamSink<A> weak_ss = ss.downgrade();
        Listener listener = sxa.listen_weak([sodium_ctx, weak_ss](const std::list<A>& xs) {
            boost::optional<StreamSink<A>> ss_op = weak_ss.upgrade();
            StreamSink<A> ss = *ss_op;
            for (auto x = xs.cbegin(); x != xs.cend(); ++x) {
                const A& a = *x;
                sodium_ctx.post([ss, a] { ss.send(a); });
            }
        });
        std::shared_ptr<boost::optional<Listener>> keep_alive = std::make_shared<boost::optional<Listener>>(boost::optional<Listener>(listener));
        GcNode gc_node(
            sodium_ctx.gc_ctx(),
            "Stream::split",
            [keep_alive]() {
                *keep_alive = boost::none;
            },
            [keep_alive](std::function<Tracer> tracer) {
                if (*keep_alive) {
                    tracer((*keep_alive)->gc_node);
                }
            }
        );
        s.node().add_keep_alive(gc_node);
        return s;
    });
}

template <typename A>
Stream<A> Stream<A>::once() const {
    Stream<A> this_ = *this;
    SodiumCtx sodium_ctx = this->sodium_ctx();
    return Stream::mkStream(
        this->sodium_ctx(),
        [sodium_ctx, this_](StreamWeakForwardRef<A> s) {
            std::vector<std::unique_ptr<IsNode>> dependencies;
            dependencies.push_back(this_.box_clone());
            Node node = Node::mk_node(
                sodium_ctx,
                "Stream::once",
                [sodium_ctx, this_, s]() {
                    if (this_.data->firing_op) {
                        A& firing = **this_.data->firing_op;
                        Stream<A> s2 = s.unwrap();
                        s2._send(firing);
                        sodium_ctx.post([s2]() mutable {
                            std::vector<std::unique_ptr<IsNode>> deps = box_clone_vec_is_node(s2.node().data->dependencies);
                            for (auto dep = deps.begin(); dep != deps.end(); ++dep) {
                                s2.remove_dependency(**dep);
                            }
                        });
                    }
                },
                std::move(dependencies)
            );
            node.add_update_dependency(this_.to_dep());
            return node;
        }
    );
}

template <typename A>
template <typename K>
Listener Stream<A>::_listen(K k, bool is_weak) const {
    Stream<A> this_ = *this;
    std::vector<Dep> k_deps = GetDeps<K>::call(k);
    std::vector<std::unique_ptr<IsNode>> dependencies;
    dependencies.push_back(this->box_clone());
    Node node = Node::mk_node(
        this->sodium_ctx(),
        "Stream::listen",
        [this_, k]() {
            if (this_.data->firing_op) {
                A& firing = **this_.data->firing_op;
                InCallback in_callback = this_.sodium_ctx().in_callback();
                k(firing);
            }
        },
        std::move(dependencies)
    );
    node.add_update_dependencies(k_deps);
    node.add_update_dependency(this_.to_dep());
    return Listener::mkListener(this->sodium_ctx(), is_weak, node);
}

template <typename A>
template <typename K>
Listener Stream<A>::listen_weak(K k) const {
    return this->_listen(k, true);
}

template <typename A>
template <typename K>
Listener Stream<A>::listen(K k) const {
    return this->_listen(k, false);
}

}

}

#endif // __SODIUM_IMPL_STREAM_IMPL_H__
