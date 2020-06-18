#ifndef __SODIUM_SODIUM_H__
#define __SODIUM_SODIUM_H__

#include "sodium/config.h"
#include "sodium/transaction.h"
#include "sodium/impl/cell.h"
#include "sodium/impl/cell_impl.h"
#include "sodium/impl/cell_loop.h"
#include "sodium/impl/cell_sink.h"
#include "sodium/impl/dep.h"
#include "sodium/impl/lambda.h"
#include "sodium/impl/lazy.h"
#include "sodium/impl/stream.h"
#include "sodium/impl/stream_impl.h"
#include "sodium/impl/stream_loop.h"
#include "sodium/impl/stream_loop_impl.h"
#include "sodium/impl/stream_sink.h"
#include <boost/optional.hpp>
#include <list>

#ifdef _WIN32
// maybe replace this with c++14 attributes..
#define __attribute__(x)
#endif

namespace sodium {

    namespace impl {
        extern SodiumCtx sodium_ctx;
    }

    void collect_cycles();

    template <typename A> class stream;
    template <typename A> class cell;
    template <typename A> class cell_sink;
    template <typename A> class cell_loop;
    template <typename A> class stream_loop;
    template <typename A> class stream_sink;
    template <typename A, typename Selector> class router;
    template <typename A, typename B>
    cell<B> apply(const cell<std::function<B(const A&)>>& bf,
                  const cell<A>& ba);
    template <typename A>
    stream<A> filter_optional(const stream<boost::optional<A>>& input);
    template <typename A> stream<A> split(const stream<std::list<A>>& e);
    template <typename A> stream<A> switch_s(const cell<stream<A>>& bea);
    template <typename T> cell<typename T::time> clock(const T& t);

    template <typename A> class lazy {
    private:
        std::function<A()> f;

    public:
        lazy(const A& a) : f([a]() -> A { return a; }) {}
        lazy(const std::function<A()>& f_) : f(f_) {}
        A operator()() const { return f(); }

        template <typename Fn>
        lazy<typename std::result_of<Fn(A)>::type> map(const Fn& fn) const {
            typedef typename std::result_of<Fn(A)>::type B;
            const auto& a(*this);
            return lazy<B>([fn, a]() { return fn(a()); });
        }

        template <typename B, typename Fn>
        lazy<typename std::result_of<Fn(A, B)>::type> lift(const lazy<B>& b,
                                                           const Fn& fn) const {
            typedef typename std::result_of<Fn(A, B)>::type C;
            const auto& a(*this);
            return lazy<C>([fn, a, b]() { return fn(a(), b()); });
        }

        template <typename B, typename C, typename Fn>
        lazy<typename std::result_of<Fn(A, B, C)>::type> lift(
            const lazy<B>& b, const lazy<C>& c, const Fn& fn) const {
            typedef typename std::result_of<Fn(A, B, C)>::type D;
            const auto& a(*this);
            return lazy<D>([fn, a, b, c]() { return fn(a(), b(), c()); });
        }

        template <typename B, typename C, typename D, typename Fn>
        lazy<typename std::result_of<Fn(A, B, C, D)>::type> lift(
            const lazy<B>& b, const lazy<C>& c, const lazy<D>& d,
            const Fn& fn) const {
            typedef typename std::result_of<Fn(A, B, C, D)>::type E;
            const auto& a(*this);
            return lazy<E>(
                [fn, a, b, c, d]() { return fn(a(), b(), c(), d()); });
        }
    };

    template <typename A> class stream;

    /*!
     * A like an stream, but it tracks the input stream's current value and
     * causes it always to be output once at the beginning for each listener.
     */
    template <typename A> class cell {
        template <typename AA> friend class stream;
        template <typename AA> friend class cell;
        template <typename AA> friend class cell_loop;
        template <typename AA, typename BB>
        friend cell<BB> apply(const cell<std::function<BB(const AA&)>>& bf,
                              const cell<AA>& ba);
        template <typename AA>
        friend cell<AA> switch_c(const cell<cell<AA>>& bba);
        template <typename AA>
        friend stream<AA> switch_s(const cell<stream<AA>>& bea);
        template <typename TT>
        friend cell<typename TT::time> clock(const TT& t);

            private:
        impl::Cell<A> impl_;
        
            protected:
        cell(impl::Cell<A> impl_)
            : impl_(impl_) {}

            public:
        /*!
         * Constant value.
         */
        cell(const A& a) : impl_(impl::sodium_ctx, a) {}

        cell(A&& a) : impl_(impl::sodium_ctx, std::move(a)) {}

        /*!
         * Sample the value of this cell.
         */
        A sample() const {
            return this->impl_.sample();
        }

        lazy<A> sample_lazy() const {
            impl::Lazy<A> value = this->impl_.sample_lazy();
            return lazy<A>([value]() -> A { return *value; });
        }

        /*!
         * Returns a new cell with the specified cleanup added to it, such that
         * it will be executed when no copies of the new cell are referenced.
         */
        cell<A> add_cleanup(const std::function<void()>& cleanup) const {
            transaction trans;
            cell<A> ca = updates().add_cleanup(cleanup).hold(sample());
            trans.close();
            return ca;
        }

        /*!
         * Map a function over this cell to modify the output value.
         */
        template <typename Fn>
        cell<typename std::result_of<Fn(A)>::type> map(const Fn& f) const {
            typedef typename std::result_of<Fn(A)>::type B;
            return cell<B>(this->impl_.map(f));
        }

        /*!
         * Lift a binary function into cells.
         */
        template <typename B, typename Fn>
        cell<typename std::result_of<Fn(A, B)>::type> lift(const cell<B>& bb,
                                                           const Fn& f) const {
            typedef typename std::result_of<Fn(A, B)>::type C;
            std::function<std::function<C(const B&)>(const A&)> fa(
                [f](const A& a) -> std::function<C(const B&)> {
                    return [f, a](const B& b) -> C { return f(a, b); };
                });
            transaction trans;
            auto ca = apply<B, C>(map(fa), bb);
            trans.close();
            return ca;
        }

        /*!
         * Lift a ternary function into cells.
         */
        template <typename B, typename C, typename Fn>
        cell<typename std::result_of<Fn(A, B, C)>::type> lift(
            const cell<B>& bb, const cell<C>& bc, const Fn& f) const {
            typedef typename std::result_of<Fn(A, B, C)>::type D;
            std::function<std::function<std::function<D(const C&)>(const B&)>(
                const A&)>
                fa([f](const A& a)
                       -> std::function<std::function<D(const C&)>(const B&)> {
                    return [f, a](const B& b) -> std::function<D(const C&)> {
                        return
                            [f, a, b](const C& c) -> D { return f(a, b, c); };
                    };
                });
            return apply(apply(map(fa), bb), bc);
        }

        /*!
         * Lift a quaternary function into cells.
         */
        template <typename B, typename C, typename D, typename Fn>
        cell<typename std::result_of<Fn(A, B, C, D)>::type> lift(
            const cell<B>& bb, const cell<C>& bc, const cell<D>& bd,
            const Fn& f) const {
            typedef typename std::result_of<Fn(A, B, C, D)>::type E;
            std::function<std::function<std::function<
                std::function<E(const D&)>(const C&)>(const B&)>(const A&)>
                fa([f](const A& a)
                       -> std::function<std::function<
                           std::function<E(const D&)>(const C&)>(const B&)> {
                    return [f, a](const B& b)
                               -> std::function<std::function<E(const D&)>(
                                   const C&)> {
                        return [f, a,
                                b](const C& c) -> std::function<E(const D&)> {
                            return [f, a, b, c](const D& d) -> E {
                                return f(a, b, c, d);
                            };
                        };
                    };
                });
            return apply(apply(apply(map(fa), bb), bc), bd);
        }

        /*!
         * Lift a 5-argument function into cells.
         */
        template <typename B, typename C, typename D, typename E, typename Fn>
        cell<typename std::result_of<Fn(A, B, C, D, E)>::type> lift(
            const cell<B>& bb, const cell<C>& bc, const cell<D>& bd,
            const cell<E>& be, const Fn& f) const {
            typedef typename std::result_of<Fn(A, B, C, D, E)>::type F;
            std::function<std::function<std::function<std::function<
                std::function<F(const E&)>(const D&)>(const C&)>(const B&)>(
                const A&)>
                fa([f](const A& a)
                       -> std::function<std::function<std::function<
                           std::function<F(const E&)>(const D&)>(const C&)>(
                           const B&)> {
                    return [f, a](const B& b)
                               -> std::function<std::function<std::function<F(
                                   const E&)>(const D&)>(const C&)> {
                        return [f, a, b](const C& c)
                                   -> std::function<std::function<F(const E&)>(
                                       const D&)> {
                            return
                                [f, a, b,
                                 c](const D& d) -> std::function<F(const E&)> {
                                    return [f, a, b, c, d](const E& e) -> F {
                                        return f(a, b, c, d, e);
                                    };
                                };
                        };
                    };
                });
            return apply(apply(apply(apply(map(fa), bb), bc), bd), be);
        }

        /*!
         * Lift a 6-argument function into cells.
         */
        template <typename B, typename C, typename D, typename E, typename F,
                  typename Fn>
        cell<typename std::result_of<Fn(A, B, C, D, E, F)>::type> lift(
            const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
            const cell<D>& bd, const cell<E>& be, const cell<F>& bf,
            const Fn& fn) const {
            typedef typename std::result_of<Fn(A, B, C, D, E, F)>::type G;
            std::function<std::function<std::function<std::function<
                std::function<std::function<G(const F&)>(const E&)>(const D&)>(
                const C&)>(const B&)>(const A&)>
                fa([fn](const A& a)
                       -> std::function<std::function<std::function<
                           std::function<std::function<G(const F&)>(const E&)>(
                               const D&)>(const C&)>(const B&)> {
                    return [fn, a](const B& b)
                               -> std::function<std::function<std::function<
                                   std::function<G(const F&)>(const E&)>(
                                   const D&)>(const C&)> {
                        return [fn, a, b](const C& c)
                                   -> std::function<std::function<std::function<
                                       G(const F&)>(const E&)>(const D&)> {
                            return
                                [fn, a, b, c](const D& d)
                                    -> std::function<std::function<G(const F&)>(
                                        const E&)> {
                                    return [fn, a, b, c, d](const E& e)
                                               -> std::function<G(const F&)> {
                                        return [fn, a, b, c, d,
                                                e](const F& f) -> G {
                                            return fn(a, b, c, d, e, f);
                                        };
                                    };
                                };
                        };
                    };
                });
            return apply(apply(apply(apply(apply(map(fa), bb), bc), bd), be),
                         bf);
        }

        /*!
         * Lift a 7-argument function into cells.
         */
        template <typename B, typename C, typename D, typename E, typename F,
                  typename G, typename Fn>
        cell<typename std::result_of<Fn(A, B, C, D, E, F, G)>::type> lift(
            const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
            const cell<D>& bd, const cell<E>& be, const cell<F>& bf,
            const cell<G>& bg, const Fn& fn) const {
            typedef typename std::result_of<Fn(A, B, C, D, E, F, G)>::type H;
            std::function<std::function<std::function<std::function<
                std::function<std::function<std::function<H(const G&)>(
                    const F&)>(const E&)>(const D&)>(const C&)>(const B&)>(
                const A&)>
                fa([fn](const A& a)
                       -> std::function<std::function<
                           std::function<std::function<std::function<
                               std::function<H(const G&)>(const F&)>(const E&)>(
                               const D&)>(const C&)>(const B&)> {
                    return [fn, a](const B& b)
                               -> std::function<std::function<std::function<
                                   std::function<std::function<H(const G&)>(
                                       const F&)>(const E&)>(const D&)>(
                                   const C&)> {
                        return [fn, a, b](const C& c)
                                   -> std::function<std::function<std::function<
                                       std::function<H(const G&)>(const F&)>(
                                       const E&)>(const D&)> {
                            return
                                [fn, a, b, c](const D& d)
                                    -> std::function<std::function<
                                        std::function<H(const G&)>(const F&)>(
                                        const E&)> {
                                    return [fn, a, b, c, d](const E& e)
                                               -> std::function<std::function<H(
                                                   const G&)>(const F&)> {
                                        return
                                            [fn, a, b, c, d, e](const F& f)
                                                -> std::function<H(const G&)> {
                                                return [fn, a, b, c, d, e,
                                                        f](const G& g) {
                                                    return fn(a, b, c, d, e, f,
                                                              g);
                                                };
                                            };
                                    };
                                };
                        };
                    };
                });
            return apply(
                apply(apply(apply(apply(apply(map(fa), bb), bc), bd), be), bf),
                bg);
        }

        /*!
         * Returns an stream giving the updates to a cell. If this cell was
         * created by a hold, then this gives you back an stream equivalent to
         * the one that was held.
         */
        stream<A> updates() const { return stream<A>(this->impl_.updates()); }

        /*!
         * Returns an stream describing the value of a cell, where there's an
         * initial stream giving the current value.
         */
        stream<A> value() const {
            return stream<A>(this->impl_.value());
        }

        /*!
         * Listen for updates to the value of this cell. This is the observer
         * pattern. The returned {@link Listener} has a {@link
         * Listener#unlisten()} method to cause the listener to be removed. This
         * is an OPERATIONAL mechanism is for interfacing between the world of
         * I/O and for FRP.
         * @param action The handler to execute when there's a new value.
         *   You should make no assumptions about what thread you are called on,
         * and the handler should not block. You are not allowed to use {@link
         * CellSink#send(Object)} or {@link StreamSink#send(Object)} in the
         * handler. An exception will be thrown, because you are not meant to
         * use this to create your own primitives.
         */
        std::function<void()> listen(std::function<void(const A&)> handle) const {
            transaction trans;
            auto kill = stream<A>(value_(trans.impl()))
                            .coalesce([](const A&, const A& b) { return b; })
                            .listen(handle);
            trans.close();
            return kill;
        }

        /**
         * Transform a cell with a generalized state loop (a mealy machine). The
         * function is passed the input and the old state and returns the new
         * state and output value.
         *
         * The supplied function should have the signature std::tuple<B, S>(A,
         * S), where B is the return cell's type, and S is the state type.
         */
        template <typename S, typename Fn>
        cell<typename std::tuple_element<
            0, typename std::result_of<Fn(A, S)>::type>::type>
        collect_lazy(const lazy<S>& initS, const Fn& f) const {
            typedef typename std::tuple_element<
                0, typename std::result_of<Fn(A, S)>::type>::type B;
            lazy<A> za_lazy = sample_lazy();
            impl::Lazy<SODIUM_TUPLE<B, S>> zbs([za_lazy, initS, f]() -> SODIUM_TUPLE<B, S> {
                return f(za_lazy(), initS());
            });
            return cell<B>(
                this->impl_
                    .updates()
                    .collect_lazy(
                        impl::Lazy<S>([zbs]() { return std::get<1>(*zbs); }),
                        f
                    )
                    .hold_lazy(impl::Lazy<B>([zbs]() -> B {
                        return std::get<0>(*zbs);
                    }))
            );
            // TODO: Implement this
            //SODIUM_THROW("not implemented yet!");
            /*
            transaction trans1;
            auto ea = updates().coalesce(
                [](const A&, const A& snd) -> A { return snd; });
            lazy<A> za_lazy = sample_lazy();
            std::function<SODIUM_TUPLE<B, S>()> zbs =
                [za_lazy, initS, f]() -> SODIUM_TUPLE<B, S> {
                return f(za_lazy(), initS());
            };
            SODIUM_SHARED_PTR<lazy<S>> pState(new lazy<S>(
                [zbs]() -> S { return SODIUM_TUPLE_GET<1>(zbs()); }));
            SODIUM_TUPLE<impl::stream_, SODIUM_SHARED_PTR<impl::node>> p =
                impl::unsafe_new_stream();
            auto kill = updates().listen_raw(trans1.impl(), SODIUM_TUPLE_GET<1>(p),
                new impl::collect_lazy_handler<A, S, Fn>(pState, f), false);
            auto ca = stream<B>(SODIUM_TUPLE_GET<0>(p).unsafe_add_cleanup(kill))
                          .hold_lazy(lazy<B>([zbs]() -> B {
                              return SODIUM_TUPLE_GET<0>(zbs());
                          }));
            trans1.close();
            return ca;
            */
        }

        /**
         * Transform a cell with a generalized state loop (a mealy machine). The
         * function is passed the input and the old state and returns the new
         * state and output value.
         *
         * The supplied function should have the signature std::tuple<B, S>(A,
         * S), where B is the return cell's type, and S is the state type.
         */
        template <typename S, typename Fn>
        cell<typename std::tuple_element<
            0, typename std::result_of<Fn(A, S)>::type>::type>
        collect(const S& initS, const Fn& f) const {
            return collect_lazy<S, Fn>(lazy<S>(initS), f);
        }

    };  // end class cell

    template <typename A> class stream {
        template <typename AA> friend class stream;
        template <typename AA> friend class sodium::stream_sink;
        template <typename AA> friend class cell;
        template <typename AA> friend class cell_sink;
        template <typename AA> friend class cell_loop;
        template <typename AA>
        friend stream<AA> filter_optional(
            const stream<boost::optional<AA>>& input);
        template <typename AA>
        friend stream<AA> switch_s(const cell<stream<AA>>& bea);
        template <typename AA>
        friend stream<AA> split(const stream<std::list<AA>>& e);
        template <typename AA> friend class sodium::stream_loop;
        template <typename AA, typename Selector> friend class sodium::router;

    public:
        /*!
         * The 'never' stream (that never fires).
         */
        stream(): impl_(impl::sodium_ctx) {}

    protected:
        impl::Stream<A> impl_;

        stream(const impl::Stream<A>& impl_) : impl_(impl_) {}

    public:
        /*!
         * High-level interface to obtain an stream's value.
         */
        template <typename F>
        std::function<void()> listen(F handle) const {
            impl::Listener l = this->impl_.listen(handle);
            return std::function<void()>([l]() { l.unlisten(); });
        };

        /*!
         * High-level interface to obtain an stream's value.
         */
        template <typename F>
        std::function<void()> listen_weak(F handle) const {
            impl::Listener l = this->impl_.listen_weak(handle);
            return std::function<void()>([l]() { l.unlisten(); });
        };

        /*!
         * A variant of listen that handles the first stream and then
         * automatically deregisters itself. This is useful for implementing
         * things that work like promises.
         */
        std::function<void()> listen_once(
            const std::function<void(const A&)>& handle) const {
            std::shared_ptr<std::function<void()>> lRef(
                new std::function<void()>);
            *lRef = listen([handle, lRef](const A& a) {
                handle(a);
                (*lRef)();
            });
            return *lRef;
        }

        /*!
         * Map a function over this stream to modify the output value. The
         * function must be pure (referentially transparent), that is, it must
         * not have effects.
         */
        template <typename Fn>
        stream<typename std::result_of<Fn(A)>::type> map(const Fn& f) const {
            return this->impl_.map(f);
        }

        /*!
         * map() to an optional and only let present values through. Equivalent
         * to map() followed by filter_optional().
         */
        template <typename Fn>
        stream<typename std::remove_reference<decltype(
            std::declval<typename std::result_of<Fn(A)>::type>().get())>::type>
        map_optional(const Fn& f) const {
            using B = typename std::remove_reference<decltype(
                std::declval<typename std::result_of<Fn(A)>::type>()
                    .get())>::type;
            transaction trans;
            auto sa = filter_optional<B>(map(f));
            trans.close();
            return sa;
        }

        /*!
         * Map a function over this stream that always outputs a constant value.
         */
        template <typename B> stream<B> map_to(const B& value) const {
            return map([value](const A&) -> B { return value; });
        }

        /*!
         * Variant of merge that merges two streams and will drop an stream
         * in the simultaneous case.
         * <p>
         * In the case where two streams are simultaneous (i.e. both
         * within the same transaction), the stream from <em>this</em> will take
         * precedence, and the stream from <em>s</em> will be dropped. If you
         * want to specify your own combining function, use {@link
         * Stream#merge(Stream, Lambda2)}. s1.orElse(s2) is equivalent to
         * s1.merge(s2, (l, r) -&gt; l). <p> The name orElse() is used instead
         * of merge() to make it really clear that care should be taken, because
         * streams can be dropped.
         */
        stream<A> or_else(const stream<A>& s) const {
            return merge(s, [](const A& l, const A& r) { return l; });
        }

        /*!
         * Merge a stream of unary functions with a signature like Arg(Arg), in
         * the simultaneous case, composing them, with the function on the left
         * going first.
         */
        template <typename Arg>
        stream<A> merge_functions(const stream<A>& s) const {
            return merge(s, [](const A& f, const A& g) -> A {
                return [f, g](Arg a) -> Arg { return g(f(a)); };
            });
        }

            private:
        /*!
         * If there's more than one firing in a single transaction, combine them
         * into one using the specified combining function.
         *
         * If the stream firings are ordered, then the first will appear at the
         * left input of the combining function. In most common cases it's best
         * not to make any assumptions about the ordering, and the combining
         * function would ideally be commutative.
         */
        template <typename F>
        stream<A> coalesce(F combine) const {
            transaction trans;
            stream<A> sa(coalesce_<A, F>(trans.impl(), std::move(combine)));
            trans.close();
            return sa;
        }

    public:
        /*!
         * A variant of {@link merge(Stream)} that uses the specified function
         * to combine simultaneous streams. <p> If the streams are simultaneous
         * (that is, one stream from this and one from <em>s</em> occurring in
         * the same transaction), combine them into one using the specified
         * combining function so that the returned stream is guaranteed only
         * ever to have one stream per transaction. The stream from
         * <em>this</em> will appear at the left input of the combining
         * function, and the stream from <em>s</em> will appear at the right.
         * @param f Function to combine the values. It may construct FRP logic
         * or use
         *    {@link Cell#sample()}. Apart from this the function must be
         * <em>referentially transparent</em>.
         */
        stream<A> merge(const stream<A>& s,
                        const std::function<A(const A&, const A&)>& f) const {
            return stream<A>(this->impl_.merge(s.impl_, f));
        }

        /*!
         * Filter this stream based on the specified predicate, passing through
         * values where the predicate returns true.
         */
        stream<A> filter(const std::function<bool(const A&)>& pred) const {
            return stream<A>(this->impl_.filter(pred));
        }

        /*!
         * Create a cell that holds at any given time the most recent value
         * that has arrived from this stream. Since cells must always have a
         * current value, you must supply an initial value that it has until the
         * first stream occurrence updates it.
         */
        cell<A> hold(const A& initA) const {
            return cell<A>(this->impl_.hold(initA));
        }

        cell<A> hold(A&& initA) const {
            return cell<A>(this->impl_.hold(std::move(initA)));
        }

        cell<A> hold_lazy(const lazy<A>& initA) const {
            return cell<A>(this->impl_.hold_lazy(Lazy<A>([initA]() { return initA(); })));
        }

        /*!
         * Sample the cell's value as at the transaction before the
         * current one, i.e. no changes from the current transaction are
         * taken.
         */
        template <typename B, typename Fn>
        stream<typename std::result_of<Fn(A, B)>::type> snapshot(
            const cell<B>& beh, const Fn& combine) const {
            typedef typename std::result_of<Fn(A, B)>::type C;
            return stream<C>(this->impl_.snapshot(beh.impl_, combine));
        }

        template <typename B, typename C, typename Fn>
        stream<typename std::result_of<Fn(A, B, C)>::type> snapshot(
            const cell<B>& bc, const cell<C>& cc, const Fn& f) const {
            typedef typename std::result_of<Fn(A, B, C)>::type D;
            return snapshot(bc, [cc, f](const A& a, const B& b) {
                return f(a, b, cc.sample());
            });
        }

        template <typename B, typename C, typename D, typename Fn>
        stream<typename std::result_of<Fn(A, B, C, D)>::type> snapshot(
            const cell<B>& bc, const cell<C>& cc, const cell<D>& cd,
            const Fn& f) const {
            typedef typename std::result_of<Fn(A, B, C, D)>::type E;
            return snapshot(bc, [cc, cd, f](const A& a, const B& b) {
                return f(a, b, cc.sample(), cd.sample());
            });
        }

        template <typename B, typename C, typename D, typename E, typename Fn>
        stream<typename std::result_of<Fn(A, B, C, D, E)>::type> snapshot(
            const cell<B>& bc, const cell<C>& cc, const cell<D>& cd,
            const cell<E>& ce, const Fn& f) const {
            typedef typename std::result_of<Fn(A, B, C, D, E)>::type F;
            return snapshot(bc, [cc, cd, ce, f](const A& a, const B& b) {
                return f(a, b, cc.sample(), cd.sample(), ce.sample());
            });
        }

        template <typename B, typename C, typename D, typename E, typename F,
                  typename Fn>
        stream<typename std::result_of<Fn(A, B, C, D, E, F)>::type> snapshot(
            const cell<B>& bc, const cell<C>& cc, const cell<D>& cd,
            const cell<E>& ce, const cell<F>& cf, const Fn& f) const {
            typedef typename std::result_of<Fn(A, B, C, D, E, F)>::type G;
            return snapshot(bc, [cc, cd, ce, cf, f](const A& a, const B& b) {
                return f(a, b, cc.sample(), cd.sample(), ce.sample(),
                         cf.sample());
            });
        }

        /*!
         * Sample the cell's value as at the transaction before the
         * current one, i.e. no changes from the current transaction are
         * taken.
         */
        template <typename B> stream<B> snapshot(const cell<B>& beh) const {
            return snapshot(beh, [](const A&, const B& b) { return b; });
        }

        /*!
         * Allow streams through only when the cell's value is true.
         */
        stream<A> gate(const cell<bool>& g) const {
            transaction trans;
            stream<A> sa = filter_optional<A>(
                snapshot(g, [](const A& a, const bool& gated) {
                    return gated ? boost::optional<A>(a) : boost::optional<A>();
                }));
            trans.close();
            return sa;
        }

        /*!
         * Adapt an stream to a new stream statefully.  Always outputs one
         * output for each input.
         *
         * The supplied function should have the signature std::tuple<B, S>(A,
         * S), where B is the return cell's type, and S is the state type.
         */
        template <typename S, typename Fn>
        stream<typename std::tuple_element<
            0, typename std::result_of<Fn(A, S)>::type>::type>
        collect_lazy(const lazy<S>& initS, Fn f) const {
            typedef typename std::tuple_element<
                0, typename std::result_of<Fn(A, S)>::type>::type B;
            return stream<B>(this->impl_.collect_lazy(impl::Lazy<A>([initS]() { return initS(); }), f));
        }

        /*!
         * Adapt an stream to a new stream statefully.  Always outputs one
         * output for each input.
         *
         * The supplied function should have the signature std::tuple<B, S>(A,
         * S), where B is the return cell's type, and S is the state type.
         */
        template <typename S, typename Fn>
        stream<typename std::tuple_element<
            0, typename std::result_of<Fn(A, S)>::type>::type>
        collect(const S& initS, const Fn& f) const {
            return collect_lazy<S, Fn>(lazy<S>(initS), f);
        }

        template <typename B>
        stream<B> accum_s_lazy(
            const lazy<B>& initB,
            std::function<B(const A&, const B&)> f) const {
            // TODO: Implement this
            SODIUM_THROW("Not implemented yet!");
            /*
            transaction trans1;
            SODIUM_SHARED_PTR<lazy<B>> pState(new lazy<B>(initB));
            SODIUM_TUPLE<impl::stream_, SODIUM_SHARED_PTR<impl::node>> p =
                impl::unsafe_new_stream();
            auto kill = listen_raw(
                trans1.impl(), SODIUM_TUPLE_GET<1>(p),
                new impl::accum_s_handler<A, B>(pState, std::move(f)),
                false);
            stream<B> sb(SODIUM_TUPLE_GET<0>(p).unsafe_add_cleanup(kill));
            trans1.close();
            return sb;
            */
        }

        template <typename B>
        stream<B> accum_s(const B& initB,
                          const std::function<B(const A&, const B&)>& f) const {
            return accum_s_lazy<B>(lazy<B>(initB), f);
        }

        /*!
         * Renamed to accum_s.
         */
        template <typename B>
        stream<B> accum_e(const B& initB,
                          const std::function<B(const A&, const B&)>& f) const
            __attribute__((deprecated));

        template <typename B>
        cell<B> accum(const B& initB,
                      const std::function<B(const A&, const B&)>& f) const {
            return cell<B>(this->impl_.accum_lazy(impl::Lazy<B>::of_value(initB), f));
        }

        /*!
         * Variant of accum_s that accumulates a stream of functions.
         */
        template <class B> stream<B> accum_s(const B& initA) const {
            return this->accum_s<B>(
                initA, [](const A& f, const B& a) -> B { return f(a); });
        }

        /*!
         * Variant of accum_s that accumulates a stream of functions - lazy
         * variant.
         */
        template <class B> stream<B> accum_s_lazy(const lazy<B>& initA) const {
            return this->accum_s_lazy<B>(
                initA, [](const A& f, const B& a) -> B { return f(a); });
        }

        /*!
         * Variant of accum_s that accumulates a stream of functions.
         */
        template <class B> cell<B> accum(const B& initA) const {
            transaction trans;
            cell<B> cb = this->accum_s<B>(initA).hold(initA);
            trans.close();
            return cb;
        }

        /*!
         * Variant of accum_s that accumulates a stream of functions.
         */
        template <class B> cell<B> accum_lazy(const lazy<B>& initA) const {
            transaction trans;
            cell<B> cb = this->accum_s_lazy<B>(initA).hold_lazy(initA);
            trans.close();
            return cb;
        }

        template <class B = int> cell<B> count(const B& initial = 0) const {
            return accum<B>(initial, [](const A&, const B& total) -> B {
                return total + 1;
            });
        }

        stream<A> once() const {
            return stream<A>(this->impl_.once());
        }

        /*!
         * Delays each stream occurrence by putting it into a new transaction,
         * using the same method as split.
         */
        stream<A> defer() const {
            return split<A>(
                map([](const A& a) -> std::list<A> { return {a}; }));
        }

        /*!
         * Delays each stream occurrence by putting it into a new transaction,
         * using the same method as split. Renamed to defer();
         */
        stream<A> delay() const __attribute__((deprecated)) {
            return this->defer();
        }

        /*!
         * Add a clean-up operation to be performed when this stream is no
         * longer referenced.
         */
        stream<A> add_cleanup(const std::function<void()>& cleanup) const {
            return stream<A>(this->impl_.add_cleanup(cleanup));
        }
    };  // end class stream

    template <typename A>
    template <typename B>
    stream<B> stream<A>::accum_e(
        const B& initB, const std::function<B(const A&, const B&)>& f) const {
        return accum_s<B>(initB, f);
    }

    namespace impl {
        template <typename A, typename L>
        stream<A> merge(const L& sas, size_t start, size_t end,
                        const std::function<A(const A&, const A&)>& f) {
            size_t len = end - start;
            if (len == 0)
                return stream<A>();
            else if (len == 1)
                return sas[start];
            else if (len == 2)
                return sas[start].merge(sas[start + 1], f);
            else {
                int mid = (start + end) / 2;
                return merge<A, L>(sas, start, mid, f)
                    .merge(merge<A, L>(sas, mid, end, f), f);
            }
        }
    }  // namespace impl

    /*!
     * Variant of merge that merges a collection of streams.
     */
    template <typename A, typename L> stream<A> or_else(const L& sas) {
        return impl::merge<A, L>(sas, 0, sas.size(),
                                 [](const A& l, const A& r) { return l; });
    }

    /*!
     * Variant of merge that merges a collection of streams.
     */
    template <typename A, typename L>
    stream<A> merge(const L& sas,
                    const std::function<A(const A&, const A&)>& f) {
        return impl::merge<A, L>(sas, 0, sas.size(), f);
    }

    /*!
     * An stream with a send() method to allow values to be pushed into it
     * from the imperative world.
     */
    template <typename A> class stream_sink : public stream<A> {
    private:
        impl::StreamSink<A> impl_;
        stream_sink(const impl::StreamSink<A>& impl_) : impl_(impl_), stream(impl_.stream()) {}

    public:
        stream_sink(): stream_sink(impl::StreamSink<A>(impl::sodium_ctx)) {
        }

        stream_sink(const std::function<A(const A&, const A&)>& f): stream_sink(impl::StreamSink<A>(impl::sodium_ctx, f)) {
        }

        void send(const A& a) const {
            transaction trans;
            if (trans.is_in_callback())
                SODIUM_THROW(
                    "You are not allowed to use send() inside a Sodium "
                    "callback");
            this->impl_.send(a);
            trans.close();
        }

        void send(A&& a) const {
            transaction trans;
            if (trans.is_in_callback())
                SODIUM_THROW(
                    "You are not allowed to use send() inside a Sodium "
                    "callback");
            this->impl_.send(std::move(a));
            trans.close();
        }
    };

    /*!
     * Filter an stream of optionals, keeping only the defined values.
     */
    template <typename A>
    stream<A> filter_optional(const stream<boost::optional<A>>& input) {
        return stream<A>(
            input
                .filter([](const boost::optional<A>& x) { return (bool)x; })
                .map([](const boost::optional<A>& x) { return *x; })
        );
    }

    /*!
     * A cell with a send() method to allow its value to be changed
     * from the imperative world.
     */
    template <typename A> class cell_sink : public cell<A> {
    private:
        impl::CellSink<A> impl_;

        cell_sink(impl::CellSink<A> impl_) : impl_(impl_), cell<A>(impl_.cell()) {}

    public:
        cell_sink(const A& initA): cell_sink(impl::CellSink<A>(impl::sodium_ctx, initA)) {
        }

        cell_sink(A&& initA): cell_sink(impl::CellSink<A>(impl::sodium_ctx, std::move(initA))) {
        }

        void send(const A& a) const { impl_.send(a); }

        void send(A&& a) const { impl_.send(std::move(a)); }
    };

    /*!
     * Apply a function contained in a cell to a cell value. This is the
     * primitive for all lifting of functions into cells.
     */
    template <typename A, typename B>
    cell<B> apply(const cell<std::function<B(const A&)>>& bf,
                  const cell<A>& ba) {
        return cell<B>(bf.impl_.lift2(ba.impl_, [](const std::function<B(const A&)>& f, const A& a) { return f(a); }));
    }

    /*!
     * Enable the construction of stream loops, like this. This gives the
     * ability to forward reference an stream.
     *
     *   stream_loop<A> ea;
     *   auto ea_out = do_something(ea);
     *   ea.loop(ea_out);  // ea is now the same as ea_out
     *
     * TO DO: Loops do not yet get deallocated properly.
     */
    template <typename A> class stream_loop : public stream<A> {
    private:
        impl::StreamLoop<A> impl_;

    private:
        stream_loop(const impl::StreamLoop<A>& impl_)
            : impl_(impl_), stream(impl_.stream()) {}

    public:
        stream_loop(): stream_loop(impl::StreamLoop<A>(impl::sodium_ctx)) {
        }

        void loop(const stream<A>& e) {
            impl_.loop(e.impl_);
        }
    };

    /*!
     * Enable the construction of cell loops, like this. This gives the ability
     * to forward reference a cell.
     *
     *   cell_loop<A> ba;
     *   auto ba_out = do_something(ea);
     *   ea.loop(ba_out);  // ba is now the same as ba_out
     *
     * TO DO: Loops do not yet get deallocated properly.
     */
    template <typename A> class cell_loop : public cell<A> {
    private:
        impl::CellLoop<A> impl_;

        cell_loop(impl::CellLoop<A> impl_): impl_(impl_), cell<A>(impl_.cell()) {
        }

    public:
        cell_loop(): cell_loop(impl::CellLoop<A>(impl::sodium_ctx)) {
        }

        void loop(const cell<A>& b) {
            this->impl_.loop(b.impl_);
        }
    };

    /*!
     * Flatten a cell that contains an stream to give an stream that reflects
     * the current state of the cell. Note that when an stream is updated,
     * due to cell's delay semantics, stream occurrences for the new
     * stream won't come through until the following transaction.
     */
    template <typename A> stream<A> switch_s(const cell<stream<A>>& bea) {
        return stream<A>(impl::Cell<A>::switch_s(bea.impl_.map([](const stream<A>& sa) { return sa.impl_; })));
    }

    template <typename A>
    stream<A> switch_e(const cell<stream<A>>& bea) __attribute__((deprecated));

    /*!
     * Deprecated old name.
     */
    template <typename A> stream<A> switch_e(const cell<stream<A>>& bea) {
        return switch_s<A>(bea);
    }

    /*!
     * Cell variant of switch.
     */
    template <typename A> cell<A> switch_c(const cell<cell<A>>& bba) {
        return cell<A>(impl::Cell<A>::switch_c(bba.impl_.map([](const cell<A>& ca) { return ca.impl_; })));
    }

    template <typename A>
    cell<A> switch_b(const cell<cell<A>>& bba) __attribute__((deprecated));

    /*!
     * Cell variant of switch - deprecated old name.
     */
    template <typename A> cell<A> switch_b(const cell<cell<A>>& bba) {
        return switch_c<A>(bba);
    }

    template <typename A, typename B, typename C>
    cell<C> lift(const std::function<C(const A&, const B&)>& f,
                 const cell<A>& ba, const cell<B>& bb)
        __attribute__((deprecated));

    /*!
     * Lift a binary function into cells.
     */
    template <typename A, typename B, typename C>
    cell<C> lift(const std::function<C(const A&, const B&)>& f,
                 const cell<A>& ba, const cell<B>& bb) {
        std::function<std::function<C(const B&)>(const A&)> fa(
            [f](const A& a) -> std::function<C(const B&)> {
                return [f, a](const B& b) -> C { return f(a, b); };
            });
        transaction trans;
        cell<C> cc = apply<B, C>(ba.map(fa), bb);
        trans.close();
        return cc;
    }

    template <typename A, typename B, typename C, typename D>
    cell<D> lift(const std::function<D(const A&, const B&, const C&)>& f,
                 const cell<A>& ba, const cell<B>& bb, const cell<C>& bc)
        __attribute__((deprecated));

    /*!
     * Lift a ternary function into cells.
     */
    template <typename A, typename B, typename C, typename D>
    cell<D> lift(const std::function<D(const A&, const B&, const C&)>& f,
                 const cell<A>& ba, const cell<B>& bb, const cell<C>& bc) {
        std::function<std::function<std::function<D(const C&)>(const B&)>(
            const A&)>
            fa([f](const A& a)
                   -> std::function<std::function<D(const C&)>(const B&)> {
                return [f, a](const B& b) -> std::function<D(const C&)> {
                    return [f, a, b](const C& c) -> D { return f(a, b, c); };
                };
            });
        return apply(apply(ba.map(fa), bb), bc);
    }

    template <typename A, typename B, typename C, typename D, typename E>
    cell<E> lift(
        const std::function<E(const A&, const B&, const C&, const D&)>& f,
        const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
        const cell<D>& bd) __attribute__((deprecated));

    /*!
     * Lift a quaternary function into cells.
     */
    template <typename A, typename B, typename C, typename D, typename E>
    cell<E> lift(
        const std::function<E(const A&, const B&, const C&, const D&)>& f,
        const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
        const cell<D>& bd) {
        std::function<std::function<std::function<std::function<E(const D&)>(
            const C&)>(const B&)>(const A&)>
            fa([f](const A& a)
                   -> std::function<std::function<std::function<E(const D&)>(
                       const C&)>(const B&)> {
                return
                    [f, a](const B& b)
                        -> std::function<std::function<E(const D&)>(const C&)> {
                        return [f, a,
                                b](const C& c) -> std::function<E(const D&)> {
                            return [f, a, b, c](const D& d) -> E {
                                return f(a, b, c, d);
                            };
                        };
                    };
            });
        return apply(apply(apply(ba.map(fa), bb), bc), bd);
    }

    template <typename A, typename B, typename C, typename D, typename E,
              typename F>
    cell<F> lift(const std::function<F(const A&, const B&, const C&, const D&,
                                       const E&)>& f,
                 const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
                 const cell<D>& bd, const cell<E>& be)
        __attribute__((deprecated));

    /*!
     * Lift a 5-argument function into cells.
     */
    template <typename A, typename B, typename C, typename D, typename E,
              typename F>
    cell<F> lift(const std::function<F(const A&, const B&, const C&, const D&,
                                       const E&)>& f,
                 const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
                 const cell<D>& bd, const cell<E>& be) {
        std::function<std::function<std::function<std::function<std::function<F(
            const E&)>(const D&)>(const C&)>(const B&)>(const A&)>
            fa([f](const A& a)
                   -> std::function<std::function<std::function<std::function<F(
                       const E&)>(const D&)>(const C&)>(const B&)> {
                return
                    [f, a](const B& b)
                        -> std::function<std::function<
                            std::function<F(const E&)>(const D&)>(const C&)> {
                        return [f, a, b](const C& c)
                                   -> std::function<std::function<F(const E&)>(
                                       const D&)> {
                            return
                                [f, a, b,
                                 c](const D& d) -> std::function<F(const E&)> {
                                    return [f, a, b, c, d](const E& e) -> F {
                                        return f(a, b, c, d, e);
                                    };
                                };
                        };
                    };
            });
        return apply(apply(apply(apply(ba.map(fa), bb), bc), bd), be);
    }

    template <typename A, typename B, typename C, typename D, typename E,
              typename F, typename G>
    cell<G> lift(const std::function<G(const A&, const B&, const C&, const D&,
                                       const E&, const F&)>& fn,
                 const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
                 const cell<D>& bd, const cell<E>& be, const cell<F>& bf)
        __attribute__((deprecated));

    /*!
     * Lift a 6-argument function into cells.
     */
    template <typename A, typename B, typename C, typename D, typename E,
              typename F, typename G>
    cell<G> lift(const std::function<G(const A&, const B&, const C&, const D&,
                                       const E&, const F&)>& fn,
                 const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
                 const cell<D>& bd, const cell<E>& be, const cell<F>& bf) {
        std::function<std::function<std::function<std::function<std::function<
            std::function<G(const F&)>(const E&)>(const D&)>(const C&)>(
            const B&)>(const A&)>
            fa([fn](const A& a)
                   -> std::function<std::function<std::function<std::function<
                       std::function<G(const F&)>(const E&)>(const D&)>(
                       const C&)>(const B&)> {
                return [fn, a](const B& b)
                           -> std::function<std::function<std::function<
                               std::function<G(const F&)>(const E&)>(const D&)>(
                               const C&)> {
                    return [fn, a, b](const C& c)
                               -> std::function<std::function<std::function<G(
                                   const F&)>(const E&)>(const D&)> {
                        return [fn, a, b, c](const D& d)
                                   -> std::function<std::function<G(const F&)>(
                                       const E&)> {
                            return
                                [fn, a, b, c,
                                 d](const E& e) -> std::function<G(const F&)> {
                                    return
                                        [fn, a, b, c, d, e](const F& f) -> G {
                                            return fn(a, b, c, d, e, f);
                                        };
                                };
                        };
                    };
                };
            });
        return apply(apply(apply(apply(apply(ba.map(fa), bb), bc), bd), be),
                     bf);
    }

    template <typename A, typename B, typename C, typename D, typename E,
              typename F, typename G, typename H>
    cell<H> lift(const std::function<H(const A&, const B&, const C&, const D&,
                                       const E&, const F&, const G&)>& fn,
                 const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
                 const cell<D>& bd, const cell<E>& be, const cell<F>& bf,
                 const cell<G>& bg) __attribute__((deprecated));

    /*!
     * Lift a 7-argument function into cells.
     */
    template <typename A, typename B, typename C, typename D, typename E,
              typename F, typename G, typename H>
    cell<H> lift(const std::function<H(const A&, const B&, const C&, const D&,
                                       const E&, const F&, const G&)>& fn,
                 const cell<A>& ba, const cell<B>& bb, const cell<C>& bc,
                 const cell<D>& bd, const cell<E>& be, const cell<F>& bf,
                 const cell<G>& bg) {
        std::function<std::function<std::function<std::function<std::function<
            std::function<std::function<H(const G&)>(const F&)>(const E&)>(
            const D&)>(const C&)>(const B&)>(const A&)>
            fa([fn](const A& a)
                   -> std::function<std::function<std::function<std::function<
                       std::function<std::function<H(const G&)>(const F&)>(
                           const E&)>(const D&)>(const C&)>(const B&)> {
                return [fn, a](const B& b)
                           -> std::function<std::function<std::function<
                               std::function<std::function<H(const G&)>(
                                   const F&)>(const E&)>(const D&)>(const C&)> {
                    return [fn, a, b](const C& c)
                               -> std::function<std::function<std::function<
                                   std::function<H(const G&)>(const F&)>(
                                   const E&)>(const D&)> {
                        return [fn, a, b, c](const D& d)
                                   -> std::function<std::function<std::function<
                                       H(const G&)>(const F&)>(const E&)> {
                            return
                                [fn, a, b, c, d](const E& e)
                                    -> std::function<std::function<H(const G&)>(
                                        const F&)> {
                                    return [fn, a, b, c, d, e](const F& f)
                                               -> std::function<H(const G&)> {
                                        return
                                            [fn, a, b, c, d, e, f](const G& g) {
                                                return fn(a, b, c, d, e, f, g);
                                            };
                                    };
                                };
                        };
                    };
                };
            });
        return apply(
            apply(apply(apply(apply(apply(ba.map(fa), bb), bc), bd), be), bf),
            bg);
    }

    /*!
     * Take each list item and put it into a new transaction of its own.
     *
     * An example use case of this might be a situation where we are splitting
     * a block of input data into frames. We obviously want each frame to have
     * its own transaction so that state is updated separately for each frame.
     */
    template <typename A> stream<A> split(const stream<std::list<A>>& e) {
        return stream<A>(impl::Stream<A>::split(e.impl_));
    }

    // New type names:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    template <typename A> struct event : stream<A> {
        event() {}
        event(const stream<A>& other) : stream<A>(other) {}
    } __attribute__((deprecated));
    template <typename A> struct event_sink : stream_sink<A> {
        event_sink() {}
        event_sink(const std::function<A(const A&, const A&)>& f)
            : stream_sink<A>(f) {}
        event_sink(const stream_sink<A>& other) : stream_sink<A>(other) {}
    } __attribute__((deprecated));
    template <typename A> struct event_loop : stream_loop<A> {
        event_loop() {}
        event_loop(const stream_loop<A>& other) : event_loop<A>(other) {}
    } __attribute__((deprecated));
    template <typename A> struct behavior : cell<A> {
        behavior(const A& initValue) : cell<A>(initValue) {}
        behavior(const cell<A>& other) : cell<A>(other) {}
    } __attribute__((deprecated));
    template <typename A> struct behavior_sink : cell_sink<A> {
        behavior_sink(const A& initValue) : cell_sink<A>(initValue) {}
    } __attribute__((deprecated));
    template <typename A> struct behavior_loop : cell_loop<A> {
        behavior_loop() {}
    } __attribute__((deprecated));
#pragma GCC diagnostic pop

}

#endif // __SODIUM_SODIUM_H__
