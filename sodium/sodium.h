#ifndef __SODIUM_SODIUM_H__
#define __SODIUM_SODIUM_H__

#include "sodium/impl/cell.h"
#include "sodium/impl/cell_impl.h"
#include "sodium/impl/dep.h"
#include "sodium/impl/lambda.h"
#include "sodium/impl/stream.h"
#include "sodium/impl/stream_impl.h"
#include "sodium/impl/stream_loop.h"
#include "sodium/impl/stream_loop_impl.h"

namespace sodium {

    // TODO: Create user API via wrappers of all the impl folder stuff.
    //
    // TODO: Create global SodiumCtx to be used via the wrappers, so
    //       end user does not need to create and pass around a SodiumCtx.

}

#endif // __SODIUM_SODIUM_H__
