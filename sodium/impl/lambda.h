#ifndef __SODIUM_IMPL_LAMBDA_H__
#define __SODIUM_IMPL_LAMBDA_H__

#include <vector>
#include "sodium/impl/dep.h"

namespace sodium {

namespace impl {

template<typename FN>
class Lambda {
public:
    FN fn;
    std::vector<Dep> deps;

    Lambda(FN fn): fn(fn), deps(deps) {}
};

template <typename FN>
class Lambda1: public Lambda<FN> {
    Lambda1(FN fn): Lambda(fn) {}

    template <typename A>
    std::result_of<FN(const A&)> operator()(const A& a) const {
        return fn(a);
    }

    Lambda1& operator<<(Dep dep) {
        this->deps.push(dep);
        return *this;
    }
};


template <typename FN>
class Lambda2: public Lambda<FN> {
    Lambda2(FN fn): Lambda(fn) {}

    template <typename A, typename B>
    std::result_of<FN(const A&, const B&)> operator()(const A& a, const B& b) const {
        return fn(a, b);
    }

    Lambda1& operator<<(Dep dep) {
        this->deps.push(dep);
        return *this;
    }
};

template <typename FN>
class Lambda3: public Lambda<FN> {
    Lambda3(FN fn): Lambda(fn) {}

    template <typename A, typename B, typename C>
    std::result_of<FN(const A&, const B&, const C&)> operator()(const A& a, const B& b, const C& c) const {
        return fn(a, b, c);
    }

    Lambda1& operator<<(Dep dep) {
        this->deps.push(dep);
        return *this;
    }
};

template <typename FN>
class Lambda4: public Lambda<FN> {
    Lambda4(FN fn): Lambda(fn) {}

    template <typename A, typename B, typename C, typename D>
    std::result_of<FN(const A&, const B&, const C&, const D&)> operator()(const A& a, const B& b, const C& c, const D& d) const {
        return fn(a, b, c, d);
    }

    Lambda1& operator<<(Dep dep) {
        this->deps.push(dep);
        return *this;
    }
};

template <typename FN>
class Lambda5: public Lambda<FN> {
    Lambda5(FN fn): Lambda(fn) {}

    template <typename A, typename B, typename C, typename D, typename E>
    std::result_of<FN(const A&, const B&, const C&, const D&, const E&)> operator()(const A& a, const B& b, const C& c, const D& d, const E& e) const {
        return fn(a, b, c, d, e);
    }

    Lambda1& operator<<(Dep dep) {
        this->deps.push(dep);
        return *this;
    }
};


template <typename FN>
class Lambda6: public Lambda<FN> {
public:
    Lambda6(FN fn): Lambda(fn) {}

    template <typename A, typename B, typename C, typename D, typename E, typename F>
    std::result_of<FN(const A&, const B&, const C&, const D&, const E&, const F&)> operator()(const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) const {
        return fn(a, b, c, d, e, f);
    }

    Lambda1& operator<<(Dep dep) {
        this->deps.push(dep);
        return *this;
    }
};

template<typename FN>
struct ToStdFunction {
    std::function<FN> to_std_function(FN fn) const {
        return std::function<FN>(fn);
    }
};

template<typename FN>
struct ToStdFunction<Lambda1<FN>> {
    std::function<FN> to_std_function(Lambda1<FN> fn) const {
        return std::function<FN>(fn.fn);
    }
};

template<typename FN>
struct ToStdFunction<Lambda2<FN>> {
    std::function<FN> to_std_function(Lambda2<FN> fn) const {
        return std::function<FN>(fn.fn);
    }
};

template<typename FN>
struct ToStdFunction<Lambda3<FN>> {
    std::function<FN> to_std_function(Lambda3<FN> fn) const {
        return std::function<FN>(fn.fn);
    }
};

template<typename FN>
struct ToStdFunction<Lambda4<FN>> {
    std::function<FN> to_std_function(Lambda4<FN> fn) const {
        return std::function<FN>(fn.fn);
    }
};

template<typename FN>
struct ToStdFunction<Lambda5<FN>> {
    std::function<FN> to_std_function(Lambda5<FN> fn) const {
        return std::function<FN>(fn.fn);
    }
};

template<typename FN>
struct ToStdFunction<Lambda6<FN>> {
    std::function<FN> to_std_function(Lambda6<FN> fn) const {
        return std::function<FN>(fn.fn);
    }
};

template<typename FN>
Lambda1<FN> lambda1(FN fn) {
    return Lambda1<FN>(fn);
}

template<typename FN>
Lambda2<FN> lambda2(FN fn) {
    return Lambda2<FN>(fn);
}

template<typename FN>
Lambda3<FN> lambda3(FN fn) {
    return Lambda3<FN>(fn);
}

template<typename FN>
Lambda4<FN> lambda4(FN fn) {
    return Lambda4<FN>(fn);
}

template<typename FN>
Lambda5<FN> lambda5(FN fn) {
    return Lambda5<FN>(fn);
}

template<typename FN>
Lambda6<FN> lambda6(FN fn) {
    return Lambda6<FN>(fn);
}

template<typename FN>
struct GetDeps {
    std::vector<Dep> operator()() const {
        return std::vector<Dep>();
    }
};

template<typename FN>
struct GetDeps<Lambda<FN>> {
    std::vector<Dep> operator()() const {
        return this->deps;
    }
};

}

}

#endif // __SODIUM_IMPL_LAMBDA_H__
