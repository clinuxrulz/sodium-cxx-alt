#include "./gc_node.h"

#include <algorithm>

using namespace sodium::impl;

GcCtx::GcCtx() {
    GcCtxData* data = new GcCtxData();
    data->next_id = 0;
    this->data = std::unique_ptr<GcCtxData>(data);
}

unsigned int GcCtx::make_id() {
    unsigned int id = this->data->next_id;
    this->data->next_id = this->data->next_id + 1;
    return id;
}

void GcCtx::add_possible_root(GcNode& node) {
    this->data->roots.push_back(node);
}

void GcCtx::collect_cycles() {
    do {
        this->mark_roots();
        this->scan_roots();
        this->collect_roots();
    } while (this->data->roots.size() != 0);
}

void GcCtx::mark_roots() {
    std::vector<GcNode> old_roots;
    old_roots.swap(this->data->roots);
    std::vector<GcNode> new_roots;
    for (auto root = old_roots.begin(); root != old_roots.end(); ++root) {
        if (root->data->color == Color::Purple) {
            this->mark_gray(*root);
            new_roots.push_back(*root);
        } else {
            root->data->buffered = false;
            if (root->data->color == Color::Black && root->data->ref_count == 0 && !root->data->freed) {
                this->data->to_be_freed.push_back(*root);
            }
        }
    }
    this->data->roots.swap(new_roots);
}

void GcCtx::mark_gray(GcNode& s) {
    if (s.data->color == Color::Gray) {
        return;
    }
    s.data->color = Color::Gray;

    s.trace([this](GcNode& t) {
        t.data->ref_count_adj = t.data->ref_count_adj + 1;
        this->mark_gray(t);
    });
}

void GcCtx::scan_roots() {
    std::vector<GcNode>& roots = this->data->roots;
    for (auto root = roots.begin(); root != roots.end(); ++root) {
        this->scan(*root);
    }
    for (auto root = roots.begin(); root != roots.end(); ++root) {
        this->reset_ref_count_adj(*root);
    }
}

void GcCtx::scan(GcNode& s) {
    if (s.data->color != Color::Gray) {
        return;
    }
    if (s.data->ref_count_adj == s.data->ref_count) {
        s.data->color = Color::White;
        s.trace([this](GcNode& t) {
            this->scan(t);
        });
    } else {
        this->scan_black(s);
    }
}

void GcCtx::scan_black(GcNode& s) {
    s.data->color = Color::Black;
    s.trace([this](GcNode& t) {
        if (t.data->color != Color::Black) {
            this->scan_black(t);
        }
    });
}

void GcCtx::reset_ref_count_adj(GcNode& s) {
    if (s.data->visited) {
        return;
    }
    s.data->visited = true;
    s.data->ref_count_adj = 0;
    s.trace([this](GcNode& t) {
        this->reset_ref_count_adj(t);
    });
    s.data->visited = false;
}

void GcCtx::collect_roots() {
    std::vector<GcNode> white;
    std::vector<GcNode> roots;
    roots.swap(this->data->roots);
    for (auto root = roots.begin(); root != roots.end(); ++root) {
        root->data->buffered = false;
        this->collect_white(*root, white);
    }
    for (auto i = white.begin(); i != white.end(); ++i) {
        if (!i->data->freed) {
            i->free();
        }
    }
    std::vector<GcNode> to_be_freed;
    to_be_freed.swap(this->data->to_be_freed);
    for (auto i = to_be_freed.begin(); i != to_be_freed.end(); ++i) {
        if (!i->data->freed) {
            i->free();
        }
    }
}

void GcCtx::collect_white(GcNode& s, std::vector<GcNode>& white) {
    if (s.data->color == Color::White) {
        s.data->color = Color::Black;
        s.trace([this, &white](GcNode& t) {
            this->collect_white(t, white);
        });
    }
}

GcCtxData::GcCtxData(): next_id(0) {
}

unsigned int GcNode::ref_count() const {
    return this->data->ref_count;
}

void GcNode::inc_ref() {
    if (this->data->freed) {
        // TODO: panic!("gc_node {} inc_ref on freed node ({})", self.id, self.name);
    }
    this->data->ref_count = this->data->ref_count + 1;
    this->data->color = Color::Black;
}

void GcNode::dec_ref() {
    if (this->data->ref_count == 0) {
        return;
    }
    this->data->ref_count = this->data->ref_count - 1;
    if (this->data->ref_count == 0) {
        this->release();
    } else {
        this->possible_root();
    }
}

void GcNode::release() {
    this->data->color = Color::Black;
    if (!this->data->buffered) {
        this->free();
    }
}

void GcNode::possible_root() {
    if (this->data->color != Color::Purple) {
        this->data->color = Color::Purple;
        if (!this->data->buffered) {
            this->data->buffered = true;
            this->gc_ctx.add_possible_root(*this);
        }
    }
}

void GcNode::free() {
    this->data->freed = true;
    std::function<Deconstructor> deconstructor = []() {};
    std::swap(deconstructor, this->data->deconstructor);
    deconstructor();
    this->data->trace = [](std::function<Tracer>& tracer) {};
}
