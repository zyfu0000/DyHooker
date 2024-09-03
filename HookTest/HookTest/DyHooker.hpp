//
//  DyHooker.hpp
//  HookFramework
//
//  Created by zhiyangfu on 2024/7/22.
//

#ifndef DyHooker_hpp
#define DyHooker_hpp

#include <stdio.h>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <LuaBridge/LuaBridge.h>

#define DYH_TYPE_VOID       0
#define DYH_TYPE_INT        1
#define DYH_TYPE_FLOAT      2
#define DYH_TYPE_DOUBLE     3
#define DYH_TYPE_UINT8      5
#define DYH_TYPE_SINT8      6
#define DYH_TYPE_UINT16     7
#define DYH_TYPE_SINT16     8
#define DYH_TYPE_UINT32     9
#define DYH_TYPE_SINT32     10
#define DYH_TYPE_UINT64     11
#define DYH_TYPE_SINT64     12
#define DYH_TYPE_STRUCT     13
#define DYH_TYPE_POINTER    14
#define DYH_TYPE_COMPLEX    15

void register_with_lua(lua_State* L);

void hook_func(const char* framework, const char* symbol, int8_t returnType, int8_t* argTypes, int8_t argCount, luabridge::LuaRef *func);

#endif /* DyHooker_hpp */
