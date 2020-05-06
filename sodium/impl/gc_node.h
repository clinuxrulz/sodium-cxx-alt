#ifndef __SODIUM_CXX_IMPL_GC_NODE_H__
#define __SODIUM_CXX_IMPL_GC_NODE_H__

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace sodium {

namespace impl {

struct GcNode;
typedef struct GcNode GcNode;

typedef void Tracer(GcNode&);

typedef void Trace(std::function<Tracer>&);

typedef void Deconstructor();

typedef enum Color {
    Black,
    Gray,
    Purple,
    White
} Color;

struct GcCtxData;
typedef struct GcCtxData GcCtxData;

typedef struct GcCtx {
    std::shared_ptr<GcCtxData> data;

    GcCtx();

    unsigned int make_id();

    void add_possible_root(GcNode& node);

    void collect_cycles();

private:

    void mark_roots();

    void mark_gray(GcNode& s);

    void scan_roots();

    void scan(GcNode& s);

    void scan_black(GcNode& s);

    void reset_ref_count_adj(GcNode& s);

    void collect_roots();

    void collect_white(GcNode& s, std::vector<GcNode>& white);
} GcCtx;

struct GcNodeData;
typedef struct GcNodeData GcNodeData;

struct GcNode {
    unsigned int id;
    std::string name;
    GcCtx gc_ctx;
    std::shared_ptr<GcNodeData> data;

    template <typename DECONSTRUCTOR, typename TRACE>
    GcNode(GcCtx gc_ctx, std::string name, DECONSTRUCTOR deconstructor, TRACE trace);

    unsigned int ref_count() const;

    void inc_ref();
    
    void dec_ref();

    void release();

    void possible_root();

    void free();

    template <typename TRACER>
    void trace(TRACER tracer);
};

struct GcNodeData {
    bool freed;
    unsigned int ref_count;
    unsigned int ref_count_adj;
    bool visited;
    Color color;
    bool buffered;
    std::function<Deconstructor> deconstructor;
    std::function<Trace> trace;
};

struct GcCtxData {
    unsigned int next_id;
    std::vector<GcNode> roots;
    std::vector<GcNode> to_be_freed;

    GcCtxData();
};


template <typename DECONSTRUCTOR, typename TRACE>
GcNode::GcNode(GcCtx gc_ctx, std::string name, DECONSTRUCTOR deconstructor, TRACE trace)
: gc_ctx(gc_ctx), name(name) {
    this->id = gc_ctx.make_id();
    GcNodeData* data = new GcNodeData();
    data->freed = false;
    data->ref_count = 0;
    data->ref_count_adj = 0;
    data->visited = false;
    data->color = Color::Black;
    data->buffered = false;
    data->deconstructor = deconstructor;
    data->trace = trace;
    this->data = std::shared_ptr<GcNodeData>(data);
}

template <typename TRACER>
void GcNode::trace(TRACER tracer) {
    (this->data->trace)(std::function<Tracer>(tracer));
}

}

}

#endif // __SODIUM_CXX_IMPL_GC_NODE_H__
