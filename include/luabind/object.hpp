#ifndef LUABIND_OBJECT_HPP
#define LUABIND_OBJECT_HPP

namespace luabind {

class Object {
public:
    virtual ~Object() = 0;
};

inline Object::~Object() = default;

} // namespace luabind

#endif // LUABIND_OBJECT_HPP
