#include "sodium/impl/sodium_ctx.h"

namespace sodium {

    namespace impl {
        extern SodiumCtx sodium_ctx;
    }

    class transaction
    {
        private:
            sodium::impl::Transaction impl_;
            // Disallow copying
            transaction(const transaction&): impl_(impl::sodium_ctx) {}
            // Disallow copying
            transaction& operator = (const transaction&) { return *this; };
        public:
            transaction(): impl_(impl::sodium_ctx) {}
            /*!
             * The destructor will close the transaction, so normally close() isn't needed.
             * But, in some cases you might want to close it earlier, and close() will do this for you.
             */
            inline void close() { impl_.close(); }

            bool is_in_callback() const {
                return impl_.is_in_callback();
            }

            /* TODO: ???
            void prioritized(impl::prioritized_entry* e)
            {
                impl()->prioritized(e);
            }*/

            void post(std::function<void()> f) {
                impl::sodium_ctx.post(f);
            }

            // TODO: Implement this
            /*
            static void on_start(std::function<void()> f) {
                transaction trans;
                trans.impl()->part->on_start(std::move(f));
            }*/
    };    
}
