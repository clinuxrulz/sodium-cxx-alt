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
public:
    Lambda1(FN fn): Lambda(fn) {}

    template <typename A>
    typename std::result_of<FN(const A&)>::type operator()(const A& a) const {
        return fn(a);
    }

    template <typename TO_DEP>
    Lambda1<FN>& operator<<(const TO_DEP& to_dep) {
        this->deps.push_back(to_dep.to_dep());
        return *this;
    }

    Lambda1<FN>& operator<<(const Dep& dep) {
        this->deps.push_back(dep);
        return *this;
    }

    Lambda1<FN>& operator<<(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }

    Lambda1<FN>& append_vec_deps(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }
};


template <typename FN>
class Lambda2: public Lambda<FN> {
public:
    Lambda2(FN fn): Lambda(fn) {}

    template <typename A, typename B>
    typename std::result_of<FN(const A&, const B&)>::type operator()(const A& a, const B& b) const {
        return fn(a, b);
    }

    template <typename TO_DEP>
    Lambda2<FN>& operator<<(const TO_DEP& to_dep) {
        this->deps.push_back(to_dep.to_dep());
        return *this;
    }

    Lambda2<FN>& operator<<(const Dep& dep) {
        this->deps.push_back(dep);
        return *this;
    }

    Lambda2<FN>& operator<<(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }

    Lambda2<FN>& append_vec_deps(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }
};

template <typename FN>
class Lambda3: public Lambda<FN> {
public:
    Lambda3(FN fn): Lambda(fn) {}

    template <typename A, typename B, typename C>
    typename std::result_of<FN(const A&, const B&, const C&)>::type operator()(const A& a, const B& b, const C& c) const {
        return fn(a, b, c);
    }

    template <typename TO_DEP>
    Lambda3<FN>& operator<<(const TO_DEP& to_dep) {
        this->deps.push_back(to_dep.to_dep());
        return *this;
    }

    Lambda3<FN>& operator<<(const Dep& dep) {
        this->deps.push_back(dep);
        return *this;
    }

    Lambda3<FN>& operator<<(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }

    Lambda3<FN>& append_vec_deps(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }
};

template <typename FN>
class Lambda4: public Lambda<FN> {
public:
    Lambda4(FN fn): Lambda(fn) {}

    template <typename A, typename B, typename C, typename D>
    typename std::result_of<FN(const A&, const B&, const C&, const D&)>::type operator()(const A& a, const B& b, const C& c, const D& d) const {
        return fn(a, b, c, d);
    }

    template <typename TO_DEP>
    Lambda4<FN>& operator<<(const TO_DEP& to_dep) {
        this->deps.push_back(to_dep.to_dep());
        return *this;
    }

    Lambda4<FN>& operator<<(const Dep& dep) {
        this->deps.push_back(dep);
        return *this;
    }

    Lambda4<FN>& operator<<(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }

    Lambda4<FN>& append_vec_deps(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }
};

template <typename FN>
class Lambda5: public Lambda<FN> {
public:
    Lambda5(FN fn): Lambda(fn) {}

    template <typename A, typename B, typename C, typename D, typename E>
    typename std::result_of<FN(const A&, const B&, const C&, const D&, const E&)>::type operator()(const A& a, const B& b, const C& c, const D& d, const E& e) const {
        return fn(a, b, c, d, e);
    }

    template <typename TO_DEP>
    Lambda5<FN>& operator<<(const TO_DEP& to_dep) {
        this->deps.push_back(to_dep.to_dep());
        return *this;
    }

    Lambda5<FN>& operator<<(const Dep& dep) {
        this->deps.push_back(dep);
        return *this;
    }

    Lambda5<FN>& operator<<(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }

    Lambda5<FN>& append_vec_deps(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }
};


template <typename FN>
class Lambda6: public Lambda<FN> {
public:
    Lambda6(FN fn): Lambda(fn) {}

    template <typename A, typename B, typename C, typename D, typename E, typename F>
    typename std::result_of<FN(const A&, const B&, const C&, const D&, const E&, const F&)>::type operator()(const A& a, const B& b, const C& c, const D& d, const E& e, const F& f) const {
        return fn(a, b, c, d, e, f);
    }

    template <typename TO_DEP>
    Lambda6<FN>& operator<<(const TO_DEP& to_dep) {
        this->deps.push_back(to_dep.to_dep());
        return *this;
    }

    Lambda6<FN>& operator<<(const Dep& dep) {
        this->deps.push_back(dep);
        return *this;
    }

    Lambda6<FN>& operator<<(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
        return *this;
    }

    Lambda6<FN>& append_vec_deps(const std::vector<Dep>& deps) {
        for (auto dep = deps.cbegin(); dep != deps.cend(); ++dep) {
            this->deps.push_back(*dep);
        }
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
    static std::vector<Dep> call(FN fn) {
        return std::vector<Dep>();
    }
};

template<typename FN>
struct GetDeps<Lambda<FN>> {
    static std::vector<Dep> call(FN fn) {
        return fn.deps;
    }
};

}

}

#endif // __SODIUM_IMPL_LAMBDA_H__
