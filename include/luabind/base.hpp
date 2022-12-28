#ifndef LUABIND_BASE_HPP
#define LUABIND_BASE_HPP

namespace luabind {

class Object {
public:
    virtual ~Object() = 0;
};

inline Object::~Object() = default;

} // namespace luabind

#endif // LUABIND_BASE_HPP
