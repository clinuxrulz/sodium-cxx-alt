#ifndef __SODIUM_IMPL_STREAM_LOOP_H__
#define __SODIUM_IMPL_STREAM_LOOP_H__

#include <memory>

#include "sodium/impl/gc_node.h"

namespace sodium {

namespace impl {

typedef struct SodiumCtx SodiumCtx;

template <typename A>
class Stream;

template <typename A>
class StreamLoopData;

template <typename A>
class StreamLoop {
public:
    std::shared_ptr<StreamLoopData<A>> data;
    GcNode gc_node;

    StreamLoop(std::shared_ptr<StreamLoopData<A>> data, GcNode gc_node);

    StreamLoop(SodiumCtx& sodium_ctx): StreamLoop(StreamLoop<A>::mkStreamLoop(sodium_ctx)) {}

    static StreamLoop<A> mkStreamLoop(SodiumCtx& sodium_ctx);

    StreamLoop(const StreamLoop<A>& stream_loop);

    ~StreamLoop();

    StreamLoop<A>& operator=(const StreamLoop<A>& rhs);

    Stream<A> stream() const;

    void loop(const Stream<A>& s);
};

}

}

#endif // __SODIUM_IMPL_STREAM_LOOP_H__
