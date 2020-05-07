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

    // Lambda(FN fn, std::vector<Dep> deps): fn(fn), deps(deps) {}
};

template<typename LAMBDA>
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
