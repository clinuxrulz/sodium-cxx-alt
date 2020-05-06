#ifndef __SODIUM_CXX_IMPL_DEP_H__
#define __SODIUM_CXX_IMPL_DEP_H__

#include "sodium/impl/gc_node.h"

namespace sodium {

namespace impl {

typedef struct Dep {
    GcNode _gc_node;

    Dep(GcNode gc_node): _gc_node(gc_node) {}

    GcNode gc_node() const {
        return _gc_node;
    }
} Dep;

}

}

#endif // __SODIUM_CXX_IMPL_DEP_H__
