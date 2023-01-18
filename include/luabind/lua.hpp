#ifndef LUABIND_LUA_HPP
#define LUABIND_LUA_HPP

#ifdef INCLUDE_LUA_LIB_WITH_EXTERN_C
extern "C" {
#endif // INCLUDE_LUA_LIB_WITH_EXTERN_C

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef INCLUDE_LUA_LIB_WITH_EXTERN_C
}
#endif // INCLUDE_LUA_LIB_WITH_EXTERN_C

#endif // LUABIND_LUA_HPP
