#ifndef coroutines_hpp
#define coroutines_hpp

#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>
#include <exception>

namespace NetworkLayer {

// Completion token for co_await: an operation's error is stored in `error`
// instead of being thrown, so the loops branch on it as completion handlers
// do. (Not as_tuple with a structured binding: GCC 15 never destroyed the
// binding's tuple in Node::connectLoop, leaking the resolver results.)
inline auto nothrowAwaitable(boost::system::error_code& error) {
    return boost::asio::redirect_error(boost::asio::use_awaitable, error);
}

// Completion handler for co_spawn. An exception that escapes a coroutine
// propagates out of io_context::run(), as one thrown from a completion
// handler does, instead of being dropped as boost::asio::detached would.
inline void rethrowException(std::exception_ptr exception) {
    if (exception) {
        std::rethrow_exception(exception);
    }
}

}  // namespace NetworkLayer

#endif /* coroutines_hpp */
