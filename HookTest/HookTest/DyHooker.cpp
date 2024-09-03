//
//  DyHooker.cpp
//  HookFramework
//
//  Created by zhiyangfu on 2024/7/22.
//

#include "DyHooker.hpp"
#include "libffi/include/ffi/ffi.h"
#include <dlfcn.h>
#include "fishhook/fishhook.h"
#include <map>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <LuaBridge/LuaBridge.h>

struct custom_data {
    ffi_type *returnType;
    ffi_type** argTypes;
    int8_t argCount;
    void *origin_func_pointer;
    luabridge::LuaRef *luaFunc;
};

const std::unordered_map<std::string, std::string> symbolMap = {{"HookFramwork", ""}};

luabridge::LuaRef callLuaFunction(luabridge::LuaRef& func, const std::vector<luabridge::LuaRef>& args) {
    lua_State* L = func.state();
    func.push(L);
    for (const auto& arg : args) {
        arg.push();
    }
    if (lua_pcall(L, args.size(), 1, 0) != LUA_OK) {
        throw luabridge::LuaException(L, makeErrorCode(luabridge::ErrorCode::LuaFunctionCallFailed));
    }
    return luabridge::LuaRef::fromStack(L, -1);
}

void replacement_function(ffi_cif* cif, void* ret, void** args, void* userdata) {
    // 调用原实现
    custom_data *customData = (custom_data *)userdata;
    
    ffi_type *returnType = customData->returnType;
    ffi_type** argTypes = customData->argTypes;
    int8_t argCount = customData->argCount;
    
//    ffi_cif cif2;
//    if (ffi_prep_cif(&cif2, FFI_DEFAULT_ABI, argCount, returnType, argTypes) == FFI_OK) {
//        ffi_call(&cif2, FFI_FN(customData->origin_func_pointer), ret, args);
//        printf("result: ");
//    } else {
//        printf("Failed to prepare CIF in replacement_function");
//    }
    
    // call lua function
    luabridge::LuaRef *luaFunc = customData->luaFunc;
    if (!luaFunc->isFunction()) {
        throw std::runtime_error("LuaRef is not a function");
    }
    
    lua_State* L = luaFunc->state();
    
    std::vector<luabridge::LuaRef> luaArgs;
    for (int i = 0; i < argCount; ++i) {
        ffi_type* argType = argTypes[i];
        if (argType == &ffi_type_sint) {
            luaArgs.push_back(luabridge::LuaRef(L, *static_cast<int*>(args[i])));
        } else if (argType == &ffi_type_float) {
            luaArgs.push_back(luabridge::LuaRef(L, *static_cast<float*>(args[i])));
        } else if (argType == &ffi_type_double) {
            luaArgs.push_back(luabridge::LuaRef(L, *static_cast<double*>(args[i])));
        } else if (argType == &ffi_type_uint8) {
            
        }
    }
//    lua_pushcclosure(L, <#lua_CFunction fn#>, <#int n#>)
    
    try {
        luabridge::LuaRef result = callLuaFunction(*luaFunc, luaArgs);
        if (returnType == &ffi_type_sint) {
            *(int*)ret = result.unsafe_cast<int>();
        } else if (returnType == &ffi_type_float) {
            *(float*)ret = result.unsafe_cast<float>();
        } else if (returnType == &ffi_type_double) {
            *(double*)ret = result.unsafe_cast<double>();
        } else if (returnType == &ffi_type_uint8) {
            
        }
        std::cout << "Result: " << result << std::endl;
    } catch (const luabridge::LuaException& e) {
        std::cerr << "Error calling Lua function: " << e.what() << std::endl;
    }
}

// 使用 libffi 构造一个函数指针
void* create_function_pointer(ffi_type *returnType, ffi_type** argTypes, int8_t argCount, void *origin_func_pointer, luabridge::LuaRef *func) {
    ffi_cif *cif = new ffi_cif;
    
    // 准备调用接口
    if (ffi_prep_cif(cif, FFI_DEFAULT_ABI, argCount, returnType, argTypes) != FFI_OK) {
        printf("Failed to prepare CIF in create_function_pointer");
        return nullptr;
    }
    
    // 创建一个函数指针
    void *function_pointer;
    ffi_closure* closure = (ffi_closure*)ffi_closure_alloc(sizeof(ffi_closure), (void **)&function_pointer);
    if (closure == nullptr) {
        printf("Failed to allocate closure");
        delete cif;
        return nullptr;
    }
    
    custom_data *customData = new custom_data();
    customData->argCount = argCount;
    customData->returnType = returnType;
    customData->argTypes = argTypes;
    customData->origin_func_pointer = origin_func_pointer;
    customData->luaFunc = func;
    
    // 设置函数指针的实现
    if (ffi_prep_closure_loc(closure, cif, replacement_function, customData, function_pointer) != FFI_OK) {
        printf("Failed to prepare closure");
        ffi_closure_free(closure);
        delete cif;
        return nullptr;
    }
    
    return function_pointer;
}


// fishhook 重绑定函数
void rebind_function(const char *func_name, void* origin_func_pointer, void* replacement_func_pointer) {
    struct rebinding target_rebinding;
    target_rebinding.name = func_name;
    target_rebinding.replacement = replacement_func_pointer;
    target_rebinding.replaced = (void **)&origin_func_pointer;
    
    struct rebinding rebindings[] = { target_rebinding };
    int result = rebind_symbols(rebindings, 1);
    if (result != 0) {
        printf("rebind_symbols Failed: %d", result);
    }
}

ffi_type* getFFIType(int8_t type) {
    if (type == DYH_TYPE_VOID) {
        return &ffi_type_void;
    }
    else if (type == DYH_TYPE_INT) {
        return &ffi_type_sint;
    }
    else if (type == DYH_TYPE_FLOAT) {
        return &ffi_type_float;
    }
    else if (type == DYH_TYPE_DOUBLE) {
        return &ffi_type_double;
    }
    else if (type == DYH_TYPE_UINT8) {
        return &ffi_type_uint8;
    }
    else if (type == DYH_TYPE_SINT8) {
        return &ffi_type_sint8;
    }
    else if (type == DYH_TYPE_UINT16) {
        return &ffi_type_uint16;
    }
    else if (type == DYH_TYPE_SINT16) {
        return &ffi_type_sint16;
    }
    else if (type == DYH_TYPE_UINT32) {
        return &ffi_type_uint32;
    }
    else if (type == DYH_TYPE_SINT32) {
        return &ffi_type_sint32;
    }
    else if (type == DYH_TYPE_UINT64) {
        return &ffi_type_uint64;
    }
    else if (type == DYH_TYPE_SINT64) {
        return &ffi_type_sint64;
    }
    else if (type == DYH_TYPE_STRUCT) {
        
    }
    else if (type == DYH_TYPE_POINTER) {
        return &ffi_type_pointer;
    }
    else if (type == DYH_TYPE_COMPLEX) {
        
    }

    return nullptr;
}

void hook_func(const char* framework, const char* symbol, int8_t returnType, int8_t* argTypes, int8_t argCount, luabridge::LuaRef *func) {
    char realFramework[128];
    sprintf(realFramework, "%s.framework/%s", framework, framework);
    void* handle = dlopen(realFramework, RTLD_NOLOAD | RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        printf("Failed to open lib: ");
        return;
    }
    
    // 清除任何现有的错误
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        printf("Failed to load symbol: %s", dlsym_error);
        dlclose(handle);
        return;
    }
    
    void* origin_func_pointer = dlsym(handle, symbol);
    const char* dlsym_error2 = dlerror();
    if (dlsym_error2) {
        printf("Failed to load symbol: %s", dlsym_error2);
        dlclose(handle);
        return;
    }
    
    ffi_type* ffi_return_type = getFFIType(returnType);
    
    ffi_type** ffi_arg_types = new ffi_type*[argCount];
    for (int8_t i = 0; i < argCount; i++) {
        ffi_arg_types[i] = getFFIType(argTypes[i]);
    }
    void* replacement_func_pointer = create_function_pointer(ffi_return_type, ffi_arg_types, argCount, origin_func_pointer, func);
    if (replacement_func_pointer == nullptr) {
        printf("Failed to create function pointer");
        return;
    }
    
    // 进行符号替换
    rebind_function(symbol, origin_func_pointer, replacement_func_pointer);
}

void call_func(const char* framework, const char* symbol, int8_t returnType, int8_t* argTypes, int8_t argCount, void* ret, void** args) {
    void* handle = dlopen(framework, RTLD_NOLOAD | RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        printf("Failed to open lib: ");
        return;
    }
    
    // 清除任何现有的错误
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        printf("Failed to load symbol: %s", dlsym_error);
        dlclose(handle);
        return;
    }
    
    void* func_pointer = dlsym(handle, symbol);
    const char* dlsym_error2 = dlerror();
    if (dlsym_error2) {
        printf("Failed to load symbol: %s", dlsym_error2);
        dlclose(handle);
        return;
    }
    
    ffi_type* ffi_return_type = getFFIType(returnType);
    
    ffi_type** ffi_arg_types = new ffi_type*[argCount];
    for (int8_t i = 0; i < argCount; i++) {
        ffi_arg_types[i] = getFFIType(argTypes[i]);
    }
    ffi_cif cif;
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argCount, ffi_return_type, ffi_arg_types) == FFI_OK) {
        ffi_call(&cif, FFI_FN(func_pointer), ret, args);
    } else {
        printf("Failed to prepare CIF in replacement_function");
    }
}

// Lua包装器
int l_cpp_function(lua_State* L) {
    // 第一个参数：string
    const char* funcName = luaL_checkstring(L, 1);

    // 第二个参数：array (table)
    luaL_checktype(L, 2, LUA_TTABLE);
    int argCount = luaL_len(L, 2);
    int8_t returnType;
    int8_t argTypes[argCount-1];
    printf("Array length: %d\n", argCount);
    for (int i = 1; i <= argCount; i++) {
        lua_rawgeti(L, 2, i);
        int8_t value = lua_tointeger(L, -1);
        printf("Array[%d] = %d\n", i, value);
        lua_pop(L, 1);
        
        if (i == 1) {
            returnType = value;
        } else {
            argTypes[i - 2] = value;
        }
    }

    // 第三个参数：int
    int arg1 = luaL_checkinteger(L, 3);

    // 第四个参数：int
    int arg2 = luaL_checkinteger(L, 4);
    
    int *args[argCount];
    args[0] = &arg1;
    args[1] = &arg2;
    
    int returnValue = 0;
    call_func("HookFramework.framework/HookFramework", funcName, returnType, argTypes, argCount-1, (void *)&returnValue, (void **)args);
    
    lua_pushinteger(L, returnValue);         // 将结果推回Lua栈
    return 1;                           // 返回值的数量
}

int l_hook_cpp_function(const char* framework, const char* funcName, luabridge::LuaRef func) {
    int argCount = 2;
    
    int8_t argTypes[argCount];
    argTypes[0] = DYH_TYPE_INT;
    argTypes[1] = DYH_TYPE_INT;
    
    luabridge::LuaRef *luaFunc = new luabridge::LuaRef(func);
    
    hook_func(framework, funcName, DYH_TYPE_INT, argTypes, argCount, luaFunc);
    
    // 第三个参数：int
//    int arg1 = luaL_checkinteger(L, 3);
//    
//    // 第四个参数：int
//    int arg2 = luaL_checkinteger(L, 4);
//    
//    int *args[argCount];
//    args[0] = &arg1;
//    args[1] = &arg2;
//    
//    int returnValue = 0;
//    call_func("HookFramework.framework/HookFramework", funcName, returnType, argTypes, argCount-1, (void *)&returnValue, (void **)args);
//    
//    lua_pushinteger(L, returnValue);         // 将结果推回Lua栈
    return 1;                           // 返回值的数量
}

// 注册函数
void register_with_lua(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .addFunction("callCppFunction", l_cpp_function);
    luabridge::getGlobalNamespace(L)
        .addFunction("hookCppFunction", l_hook_cpp_function);
}
