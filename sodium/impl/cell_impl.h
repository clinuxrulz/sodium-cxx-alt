#ifndef __SODIUM_IMPL_CELL_IMPL_H__
#define __SODIUM_IMPL_CELL_IMPL_H__

#include <tuple>
#include <utility>

#include <boost/optional.hpp>
#include "sodium/unit.h"
#include "sodium/impl/cell.h"
#include "sodium/impl/lambda.h"
#include "sodium/impl/lazy.h"
#include "sodium/impl/sodium_ctx.h"
#include "sodium/impl/stream.h"
#include "sodium/impl/stream_impl.h"

namespace sodium {

namespace impl {

template <typename A>
class CellData {
public:
    Stream<A> stream;
    Lazy<std::shared_ptr<A>> value;
    boost::optional<std::shared_ptr<A>> next_value_op;

    CellData(Stream<A> stream, Lazy<std::shared_ptr<A>> value, boost::optional<std::shared_ptr<A>> next_value_op)
    : stream(stream), value(value), next_value_op(next_value_op)
    {}
};

template <typename A>
CellWeakForwardRef<A>::CellWeakForwardRef() {
    this->data = std::unique_ptr<std::vector<WeakCell<A>>>(new std::vector<WeakCell<A>>());
}

template <typename A>
CellWeakForwardRef<A>& CellWeakForwardRef<A>::operator=(const Cell<A>& rhs) {
    this->data->clear();
    this->data->push_back(rhs.downgrade());
    return *this;
}

template <typename A>
Cell<A> CellWeakForwardRef<A>::unwrap() const {
    WeakCell<A> c1 = (*this->data)[0];
    return *c1.upgrade();
}

template <typename A>
Cell<A>::Cell(const Cell<A>& ca): data(ca.data), _node(ca._node) {}

template <typename A>
Cell<A> Cell<A>::mkConstCell(SodiumCtx& sodium_ctx, A&& value) {
    std::shared_ptr<CellData<A>> cell_data;
    {
        std::shared_ptr<A> value2 = std::unique_ptr<A>(new A(std::move(value)));
        CellData<A>* _cell_data = new CellData<A>(
            Stream<A>(sodium_ctx),
            Lazy<std::shared_ptr<A>>::of_value(value2),
            boost::none
        );
        cell_data = std::unique_ptr<CellData<A>>(_cell_data);
    }
    return Cell<A>(
        cell_data,
        Node::mk_node(
            sodium_ctx,
            "Cell::const",
            []() {},
            std::vector<std::unique_ptr<IsNode>>()
        )
    );
}

template <typename A>
Cell<A> Cell<A>::mkCell(SodiumCtx& sodium_ctx, Stream<A> stream, Lazy<A> value) {
    Lazy<std::shared_ptr<A>> init_value =
        stream.data->firing_op ?
            Lazy<std::shared_ptr<A>>::of_value(*stream.data->firing_op) :
            Lazy<std::shared_ptr<A>>([value]() mutable { return std::shared_ptr<A>(std::unique_ptr<A>(new A(value.move()))); });
    std::shared_ptr<CellData<A>> cell_data =
        std::unique_ptr<CellData<A>>(new CellData<A>(
            stream,
            init_value,
            boost::none
        ));
    CellWeakForwardRef<A> c_forward_ref;
    std::vector<std::unique_ptr<IsNode>> dependencies;
    dependencies.push_back(stream.box_clone());
    Node node(
        sodium_ctx,
        "Cell::hold",
        [sodium_ctx, stream, c_forward_ref]() mutable {
            Cell<A> c = c_forward_ref.unwrap();
            if (stream.data->firing_op) {
                std::shared_ptr<A>& firing = *stream.data->firing_op;
                bool is_first = !c.data->next_value_op;
                c.data->next_value_op = boost::optional<std::shared_ptr<A>>(firing);
                if (is_first) {
                    sodium_ctx.post([c]() {
                        if (c.data->next_value_op) {
                            c.data->value = Lazy<std::shared_ptr<A>>::of_value(*c.data->next_value_op);
                            c.data->next_value_op = boost::none;
                        }
                    });
                }
            }
        },
        std::move(dependencies)
    );
    // Hack: Add stream gc node twice, because one is kepted in the cell_data for Cell::update() to return.
    node.add_update_dependency(stream.to_dep());
    node.add_update_dependency(stream.to_dep());
    //
    Cell c(cell_data, node);
    c_forward_ref = c;
    return c;
}

template <typename A>
SodiumCtx& Cell<A>::sodium_ctx() const {
    return this->data->stream.sodium_ctx();
}

template <typename A>
Dep Cell<A>::to_dep() const {
    return Dep(this->_node.gc_node);
}

template <typename A>
WeakCell<A> Cell<A>::downgrade() const {
    return WeakCell<A>(this->data, this->_node.downgrade2());
}

template <typename A>
A& Cell<A>::sample() const {
    return **this->data->value;
}

template <typename A>
Lazy<A> Cell<A>::sample_lazy() const {
    Cell<A> this_ = *this;
    Lazy<A> l([this_]() { return **this_.data->value; });
    return l;
}

template <typename A>
Stream<A> Cell<A>::updates() const {
    return this->data->stream;
}

template <typename A>
Stream<A> Cell<A>::value() const {
    Cell<A> this_ = *this;
    SodiumCtx sodium_ctx = this->sodium_ctx();
    return sodium_ctx.transaction([sodium_ctx, this_]() mutable {
        Stream<A> s1 = this_.updates();
        Stream<A> spark(sodium_ctx);
        const A& fire = **this_.data->value;
        Node node = spark.node();
        node.data->changed = true;
        sodium_ctx.data->changed_nodes.push_back(spark.box_clone());
        spark._send(fire);
        return s1.or_else(spark);
    });
}

template <typename A>
template <typename FN>
Cell<typename std::result_of<FN(const A&)>::type> Cell<A>::map(FN fn) const {
    typedef typename std::result_of<FN(const A&)>::type B;
    Cell<A> this_= *this;
    Lazy<B> init = Lazy<B>([this_, fn]() { return fn(this_.sample()); });
    return this->updates().map(fn).hold_lazy(init);
}

template <typename A>
template <typename B, typename FN>
Cell<typename std::result_of<FN(const A&, const B&)>::type> Cell<A>::lift2(const Cell<B>& cb, FN fn) const {
    typedef typename std::result_of<FN(const A&, const B&)>::type C;
    std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
    Cell<A> ca = *this;
    Lazy<A> lhs = ca.sample_lazy();
    Lazy<B> rhs = cb.sample_lazy();
    Lazy<C> init([lhs,rhs,fn]() {
        return fn(*lhs,*rhs);
    });
    std::shared_ptr<std::pair<Lazy<A>,Lazy<B>>> state =
        std::unique_ptr<std::pair<Lazy<A>,Lazy<B>>>(
            new std::pair<Lazy<A>,Lazy<B>>(
                lhs,
                rhs
            )
        );
    Stream<unit> s1 =
        this->updates().map([state](const A& a) {
            state->first = Lazy<A>::of_value(a);
            return unit();
        });
    Stream<unit> s2 =
        cb.updates().map([state](const B& b) {
            state->second = Lazy<B>::of_value(b);
            return unit();
        });
    Stream<C> s = s1.or_else(s2).map(lambda1([state, fn](const unit& _) {
        return fn(*state->first, *state->second);
    }).append_vec_deps(fn_deps));
    return s.hold_lazy(init);
}

template <typename A>
template <typename B, typename C, typename FN>
Cell<typename std::result_of<FN(const A&, const B&, const C&)>::type> Cell<A>::lift3(const Cell<B>& cb, const Cell<C>& cc, FN fn) const {
    std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
    return this
        ->lift2(
            cb,
            [](const A& a, const B& b) {
                return std::pair<A,B>(a,b);
            }
        )
        .lift2(
            cc,
            lambda2([fn](const std::pair<A,B>& ab, const C& c) {
                return fn(ab.first, ab.second, c);
            }).append_vec_deps(fn_deps)
        );
}

template <typename A>
template <typename B, typename C, typename D, typename FN>
Cell<typename std::result_of<FN(const A&, const B&, const C&, const D&)>::type> Cell<A>::lift4(const Cell<B>& cb, const Cell<C>& cc, const Cell<D>& cd, FN fn) const {
    std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
    return this
        ->lift3(
            cb,
            cc,
            [](const A& a, const B& b, const C& c) {
                return std::tuple<A,B,C>(a, b, c);
            }
        )
        .lift2(
            cd,
            lambda2([fn](const std::tuple<A,B,C>& abc, const D& d) {
                return fn(std::get<0>(abc), std::get<1>(abc), std::get<2>(abc), d);
            }).append_vec_deps(fn_deps)
        );
}

template <typename A>
template <typename B, typename C, typename D, typename E, typename FN>
Cell<typename std::result_of<FN(const A&, const B&, const C&, const D&, const E&)>::type> Cell<A>::lift5(const Cell<B>& cb, const Cell<C>& cc, const Cell<D>& cd, const Cell<E>& ce, FN fn) const {
    std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
    return this
        ->lift3(
            cb,
            cc,
            [](const A& a, const B& b, const C& c) {
                return std::tuple<A,B,C>(a, b, c);
            }
        )
        .lift3(
            cd,
            ce,
            lambda3([fn](const std::tuple<A,B,C>& abc, const D& d, const E& e) {
                return fn(std::get<0>(abc), std::get<1>(abc), std::get<2>(abc), d, e);
            }).append_vec_deps(fn_deps)
        );
}

template <typename A>
template <typename B, typename C, typename D, typename E, typename F, typename FN>
Cell<typename std::result_of<FN(const A&, const B&, const C&, const D&, const E&, const F&)>::type> Cell<A>::lift6(const Cell<B>& cb, const Cell<C>& cc, const Cell<D>& cd, const Cell<E>& ce, const Cell<F>& cf, FN fn) const {
    std::vector<Dep> fn_deps = GetDeps<FN>::call(fn);
    return this
        ->lift4(
            cb,
            cc,
            cd,
            [](const A& a, const B& b, const C& c, const D& d) {
                return std::tuple<A,B,C,D>(a, b, c, d);
            }
        )
        .lift3(
            ce,
            cf,
            lambda3([fn](const std::tuple<A,B,C,D>& abcd, const E& e, const F& f) {
                return fn(std::get<0>(abcd), std::get<1>(abcd), std::get<2>(abcd), std::get<3>(abcd), e, f);
            }).append_vec_deps(fn_deps)
        );
}

template <typename A>
Stream<A> Cell<A>::switch_s(const Cell<Stream<A>>& csa) {
    SodiumCtx sodium_ctx = csa.sodium_ctx();
    return Stream<A>::mkStream(
        sodium_ctx,
        [sodium_ctx, csa](StreamWeakForwardRef<A> sa) {
            std::shared_ptr<std::tuple<WeakStream<A>>> inner_s =
                std::shared_ptr<std::tuple<WeakStream<A>>>(
                    new std::tuple<WeakStream<A>>(
                        csa.sample().downgrade2()
                    )
                );
            std::vector<std::unique_ptr<IsNode>> node1_deps;
            node1_deps.push_back(csa.sample().box_clone());
            Node node1(
                sodium_ctx,
                "switch_s inner node",
                [inner_s, sa]() {
                    Stream<A> inner_s2 = *std::get<0>(*inner_s).upgrade2();
                    if (inner_s2.data->firing_op) {
                        A& firing = **inner_s2.data->firing_op;
                        sa.unwrap()._send(firing);
                    }
                },
                std::move(node1_deps)
            );
            Stream<Stream<A>> csa_updates = csa.updates();
            std::vector<std::unique_ptr<IsNode>> node2_deps;
            node2_deps.push_back(csa_updates.box_clone());
            Node node2(
                sodium_ctx,
                "switch_s outer node",
                [sodium_ctx, inner_s, node1, csa_updates]() mutable {
                    if (csa_updates.data->firing_op) {
                        Stream<A>& firing = **csa_updates.data->firing_op;
                        sodium_ctx.pre_post([node1, inner_s, firing]() mutable {
                            Stream<A> inner_s2 = *std::get<0>(*inner_s).upgrade2();
                            node1.remove_dependency(inner_s2);
                            node1.add_dependency(firing);
                            *inner_s = std::tuple<WeakStream<A>>(firing.downgrade2());
                        });
                    }
                },
                std::move(node2_deps)
            );
            node1.add_update_dependency(csa_updates.to_dep());
            node1.add_update_dependency(Dep(node1.gc_node));
            node1.add_dependency(node2);
            return node1;
        }
    );
}

template <typename A>
Cell<A> Cell<A>::switch_c(const Cell<Cell<A>>& cca) {
    SodiumCtx sodium_ctx = cca.sodium_ctx();
    return Stream<A>::mkStream(
        sodium_ctx,
        [sodium_ctx, cca](StreamWeakForwardRef<A> sa) {
            std::vector<std::unique_ptr<IsNode>> node1_deps;
            node1_deps.push_back(cca.updates().box_clone());
            Node node1(
                sodium_ctx,
                "switch_c outer node",
                []() {},
                std::move(node1_deps)
            );
            Stream<A> init_inner_s = cca.sample().updates();
            std::shared_ptr<std::tuple<WeakStream<A>>> last_inner_s =
                std::shared_ptr<std::tuple<WeakStream<A>>>(
                    new std::tuple<WeakStream<A>>(
                        init_inner_s.downgrade2()
                    )
                );
            std::vector<std::unique_ptr<IsNode>> node2_deps;
            node2_deps.push_back(node1.box_clone());
            node2_deps.push_back(init_inner_s.box_clone());
            Node node2(
                sodium_ctx,
                "switch_c inner node",
                []() {},
                std::move(node2_deps)
            );
            std::function<void()> node1_update = [sodium_ctx, cca, last_inner_s, node1, node2, sa]() mutable {
                boost::optional<std::shared_ptr<Cell<A>>>& firing_op = cca.updates().data->firing_op;
                if (firing_op) {
                    Cell<A>& firing = **firing_op;
                    // will be overwriten by node2 firing if there is one
                    sodium_ctx.update_node(firing.updates().node());
                    Stream<A> sa2 = sa.unwrap();
                    sa2._send(firing.sample());
                    //
                    node1.data->changed = true;
                    node2.data->changed = true;
                    Stream<A> new_inner_s = firing.updates();
                    if (new_inner_s.data->firing_op) {
                        A& firing2 = **new_inner_s.data->firing_op;
                        sa2._send(firing2);
                    }
                    WeakStream<A>& last_inner_s2 = std::get<0>(*last_inner_s);
                    node2.remove_dependency(*last_inner_s2.upgrade2());
                    node2.add_dependency(new_inner_s);
                    node2.data->changed = true;
                    last_inner_s2 = new_inner_s.downgrade2();
                }
            };
            node1.add_update_dependency(Dep(node1.gc_node));
            node1.add_update_dependency(Dep(node2.gc_node));
            node1.data->update = node1_update;
            std::function<void()> node2_update = [last_inner_s, sa]() {
                WeakStream<A>& last_inner_s2 = std::get<0>(*last_inner_s);
                Stream<A> last_inner_s3 = *last_inner_s2.upgrade2();
                if (last_inner_s3.data->firing_op) {
                    A& firing = **last_inner_s3.data->firing_op;
                    sa.unwrap()._send(firing);
                }
            };
            node2.data->update = node2_update;
            return node2;
        }
    ).hold_lazy(Lazy<A>([cca]() { return cca.sample().sample(); }));
}

template <typename A>
template <typename K>
Listener Cell<A>::listen_weak(K k) const {
    return this->sodium_ctx().transaction([this, k]() {
        return this->value().listen_weak(k);
    });
}

template <typename A>
template <typename K>
Listener Cell<A>::listen(K k) const {
    return this->sodium_ctx().transaction([this, k]() {
        return this->value().listen(k);
    });
}

template <typename A>
boost::optional<Cell<A>> WeakCell<A>::upgrade() const {
    std::shared_ptr<CellData<A>> data = this->data.lock();
    boost::optional<Node> node_op = this->_node.upgrade2();
    if (data && node_op) {
        Node& node = *node_op;
        return boost::optional<Cell<A>>(Cell<A>(data, node));
    } else {
        return boost::none;
    }
}

}

}

#endif // __SODIUM_IMPL_CELL_IMPL_H__
