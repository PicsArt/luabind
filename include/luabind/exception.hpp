#ifndef LUABIND_EXCEPTION_HPP
#define LUABIND_EXCEPTION_HPP

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

} // namespace luabind

#endif // LUABIND_EXCEPTION_HPP
