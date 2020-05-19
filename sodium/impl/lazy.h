#ifndef __SODIUM_IMPL_LAZY_H__
#define __SODIUM_IMPL_LAZY_H__

#include <functional>
#include <memory>
#include <boost/optional.hpp>

namespace sodium {

namespace impl {

template <typename A>
struct LazyData {
    boost::optional<std::function<A()>> thunk_op;
    boost::optional<A> value_op;
};

template <typename A>
class Lazy {
private:
    Lazy(std::shared_ptr<LazyData<A>> data, int unused): data(data) {}

public:
    std::shared_ptr<LazyData<A>> data;

    Lazy(const Lazy<A>& a): data(a.data) {}

    template <typename K>
    Lazy(K k) {
        LazyData<A>* data = new LazyData<A>();
        data->thunk_op = boost::optional<std::function<A()>>(std::function<A()>(k));
        this->data = std::unique_ptr<LazyData<A>>(data);
    }

    static Lazy<A> of_value(const A& a) {
        return Lazy([a]() { return a; });
    }

    static Lazy<A> of_value(A&& a) {
        LazyData<A>* data = new LazyData<A>();
        data->value_op = boost::optional<A>(std::move(a));
        return Lazy<A>(std::unique_ptr<LazyData<A>>(data), 0);
    }

    Lazy<A>& operator=(const Lazy<A>& a) {
        this->data = a.data;
        return *this;
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
            this->data->value_op = boost::optional<A>(thunk());
            this->data->thunk_op = boost::none;
        }
        A& a = *this->data->value_op;
        return a;
    }
};

}

}

#endif // __SODIUM_IMPL_LAZY_H__
