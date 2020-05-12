#ifndef __SODIUM_IMPL_CELL_IMPL_H__
#define __SODIUM_IMPL_CELL_IMPL_H__

#include "sodium/optional.h"
#include "sodium/impl/cell.h"
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
    Lazy<A> value;
    nonstd::optional<A> next_value_op;

    CellData(Stream<A> stream, Lazy<A> value, nonstd::optional<A> next_value_op)
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
Cell<A> Cell<A>::mkConstCell(SodiumCtx& sodium_ctx, A value) {
    std::shared_ptr<CellData<A>> cell_data;
    {
        CellData<A>* _cell_data = new CellData<A>(
            Stream<A>(sodium_ctx),
            Lazy<A>::of_value(value),
            nonstd::nullopt
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
    Lazy<A> init_value =
        stream.data->firing_op ?
            Lazy<A>::of_value(*stream.data->firing_op) :
            value;
    std::shared_ptr<CellData<A>> cell_data =
        std::unique_ptr<CellData<A>>(new CellData<A>(
            stream,
            init_value,
            nonstd::nullopt
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
                A& firing = *stream.data->firing_op;
                bool is_first = !c.data->next_value_op;
                c.data->next_value_op = nonstd::optional<A>(firing);
                if (is_first) {
                    sodium_ctx.post([c]() {
                        if (c.data->next_value_op) {
                            c.data->value = Lazy<A>::of_value(*c.data->next_value_op);
                            c.data->next_value_op = nonstd::nullopt;
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
A Cell<A>::sample() const {
    return *this->data->value;
}

template <typename A>
Lazy<A> Cell<A>::sample_lazy() const {
    return this->data->value;
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
        sodium_ctx.post([sodium_ctx, this_, spark]() mutable {
            const A& fire = *this_.data->value;
            sodium_ctx.transaction_void([sodium_ctx, spark, fire]() mutable {
                Node node = spark.node();
                node.data->changed = true;
                sodium_ctx.data->changed_nodes.push_back(spark.box_clone());
                spark._send(fire);
            });
        });
        return s1.or_else(spark);
    });
}

template <typename A>
template <typename FN>
Cell<typename std::result_of<FN(const A&)>::type> Cell<A>::map(FN fn) const {
    Cell<A> this_= *this;
    Lazy<A> init = Lazy<A>([this_, fn]() { return fn(this_.sample()); });
    return this->updates().map(fn).hold_lazy(init);
}

}

}

#endif // __SODIUM_IMPL_CELL_IMPL_H__
