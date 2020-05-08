#ifndef __SODIUM_IMPL_STREAM_IMPL_H__
#define __SODIUM_IMPL_STREAM_IMPL_H__

#include "sodium/impl/lambda.h"
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
template <typename B, typename FN>
Stream<typename std::result_of<FN(const A&, const B&)>> Stream<A>::snapshot(const Cell<B>& cb, FN fn) const {
    std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
    fn_deps.push_back(cb.to_dep());
    return this->map(lambda1([fn,cb](const A& a) { return fn(a, cb.sample()); }) << fn_deps << cb);
}

template <typename A>
template <typename FN>
Stream<typename std::result_of<FN(const A&)>::type> Stream<A>::map(FN fn) const {
    typedef typename std::result_of<FN(const A&)>::type R;
    Stream<A> this_ = *this;
    return Stream::mkStream(
        this->sodium_ctx(),
        [this_, fn](StreamWeakForwardRef<R> s) {
            std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
            std::vector<std::unique_ptr<IsNode>> dependencies;
            dependencies.push_back(this_.box_clone());
            Node node = Node::mk_node(
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
                        A& firing = *this_.data->firing_op;
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
                    nonstd::optional<A>& firing1_op = this_.data->firing_op;
                    nonstd::optional<A>& firing2_op = s2.data->firing_op;
                    if (firing1_op) {
                        A& firing1 = *firing1_op;
                        if (firing2_op) {
                            A& firing2 = *firing2_op;
                            s.unwrap()._send(fn(firing1, firing2));
                        } else {
                            s.unwrap()._send(firing1);
                        }
                    } else {
                        if (firing2_op) {
                            A& firing2 = *firing2_op;
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

}

}

#endif // __SODIUM_IMPL_STREAM_IMPL_H__
