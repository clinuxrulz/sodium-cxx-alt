#ifndef __SODIUM_IMPL_LAZY_H__
#define __SODIUM_IMPL_LAZY_H__

#include <functional>
#include <memory>
#include "optional.h"

namespace sodium {

namespace impl {

template <typename A>
struct LazyData {
    nonstd::optional<std::function<A()>> thunk_op;
    nonstd::optional<A> value_op;
};

template <typename A>
class Lazy {
public:
    std::shared_ptr<LazyData<A>> data;

    template <typename K>
    Lazy(K k) {
        LazyData<A>* data = new LazyData<A>();
        data->thunk_op = nonstd::optional<std::function<A()>>(std::function<A()>(k));
        this->data = std::unique_ptr<LazyData<A>>(data);
    }

    static Lazy<A> of_value(A a) {
        return Lazy([a]() { return a; });
    }

    const A& operator*() const {
        return this->get();
    }

    const A* operator->() const {
        return &this->get();
    }

private:
    const A& get() const {
        if (this->data->thunk_op) {
            std::function<A()>& thunk = *this->data->thunk_op;
            this->data->value_op = nonstd::optional<A>(thunk());
            this->data->thunk_op = nonstd::nullopt;
        }
        A& a = *this->data->value_op;
        return a;
    }
};

}

}

#endif // __SODIUM_IMPL_LAZY_H__
