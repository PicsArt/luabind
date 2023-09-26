#ifndef LUABIND_EXCEPTION_HPP
#define LUABIND_EXCEPTION_HPP

#include <cstdarg>
#include <exception>
#include <string>
#include <string_view>

namespace luabind {

class error : public std::exception {
public:
    template <typename StringLike>
    error(StringLike&& str)
        : _message(std::forward<StringLike>(str)) {}

    const char* what() const noexcept override {
        return _message.c_str();
    }

private:
    std::string _message;
};

// TODO replace with std::format when supported by compilers
[[gnu::format(printf, 1, 2)]] inline void reportError(const char* fmt, ...) {
    std::va_list args;
    va_start(args, fmt);
    constexpr size_t bufferSize = 256;
    char buffer[bufferSize];
    std::vsnprintf(buffer, bufferSize, fmt, args);
    va_end(args);
    throw error {buffer};
}

} // namespace luabind

#endif // LUABIND_EXCEPTION_HPP
