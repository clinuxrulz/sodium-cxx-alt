#ifndef __SODIUM_CXX_IMPL_SODIUM_CTX_H__
#define __SODIUM_CXX_IMPL_SODIUM_CTX_H__

#include <memory>

namespace sodium {

namespace impl {

struct SodiumCtxData;
typedef struct SodiumCtxData SodiumCtxData;

typedef struct SodiumCtx {
    std::shared_ptr<SodiumCtxData> data;
} SodiumCtx;

}

}

#endif // __SODIUM_CXX_IMPL_SODIUM_CTX_H__
