#ifndef __SODIUM_CXX_IMPL_LISTENER_H__
#define __SODIUM_CXX_IMPL_LISTENER_H__

#include <memory>

namespace sodium {

namespace impl {

struct ListenerData;
typedef struct ListenerData ListenerData;

typedef struct Listener {
    std::shared_ptr<ListenerData> data;
} Listener;

}

}

#endif // __SODIUM_CXX_IMPL_LISTENER_H__