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

using namespace std;

struct custom_data {
    ffi_type *returnType;
    ffi_type** argTypes;
    int8_t argCount;
    void *origin_func_pointer;
    luabridge::LuaRef *luaFunc;
};

struct HookFuncInfo {
    string frameworkName;
    string funcName;
    int8_t returnType;
    int8_t* argTypes;
    int8_t argCount;
    void *origin_func_pointer;
    void *replace_func_pointer;
};

unordered_map<string, HookFuncInfo *> gSymbolMap;
vector<string> gCallFuncs;

luabridge::LuaRef callLuaFunction(luabridge::LuaRef& func, const vector<luabridge::LuaRef>& args) {
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
        throw runtime_error("LuaRef is not a function");
    }
    
    lua_State* L = luaFunc->state();
    
    vector<luabridge::LuaRef> luaArgs;
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
        cout << "replace Result: " << result << endl;
    } catch (const luabridge::LuaException& e) {
        cerr << "Error calling Lua function: " << e.what() << endl;
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
        return;
    }
    
    HookFuncInfo *info = gSymbolMap[func_name];
    info->origin_func_pointer = origin_func_pointer;
    info->replace_func_pointer = replacement_func_pointer;
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

void *getFuncSiganature(const char* framework, const char* symbol) {
    void* handle = nullptr;
    if (strlen(framework) > 0) {
        char realFramework[128];
        sprintf(realFramework, "%s.framework/%s", framework, framework);
        handle = dlopen(realFramework, RTLD_NOLOAD | RTLD_NOW | RTLD_GLOBAL);
    }
    if (!handle) {
        printf("Failed to open lib: ");
        return nullptr;
    }
    
    // 清除任何现有的错误
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        printf("Failed to load symbol: %s", dlsym_error);
        dlclose(handle);
        return nullptr;
    }
    
    void* origin_func_pointer = dlsym(handle, symbol);
    const char* dlsym_error2 = dlerror();
    if (dlsym_error2) {
        printf("Failed to load symbol: %s", dlsym_error2);
        dlclose(handle);
        return nullptr;
    }
    
    return origin_func_pointer;
}

void hook_func(const char* framework, const char* symbol, int8_t returnType, int8_t* argTypes, int8_t argCount, luabridge::LuaRef *func) {
    void* origin_func_pointer = getFuncSiganature(framework, symbol);
    
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
    HookFuncInfo *info = gSymbolMap[symbol];
    void *func_pointer = nullptr;
    if (info) {
        func_pointer = info->replace_func_pointer;
    }
    else {
        func_pointer = getFuncSiganature(framework, symbol);
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
        printf("Failed to prepare CIF in call_func");
    }
}

void callCppFunction(lua_State* L) {
    // 第一个参数：string
    const char* funcName = luaL_checkstring(L, 1);
    HookFuncInfo *info = gSymbolMap[funcName];
    
    void *args[info->argCount];
    for (int i=0;i<info->argCount;++i) {
        int8_t argType = info->argTypes[i];
        if (argType == DYH_TYPE_INT) {
            int *arg = new int;
            *arg = luaL_checkinteger(L, i+2);
            args[i] = arg;
        }
    }
    if (info->returnType == DYH_TYPE_INT) {
        int returnValue = 0;
        call_func(info->frameworkName.c_str(), funcName, info->returnType, info->argTypes, info->argCount, (void *)&returnValue, args);
        
        lua_pushinteger(L, returnValue);         // 将结果推回Lua栈
        
        cout << "cpp Result: " << returnValue << endl;
    }
}

void hookCppFunction(const char* symbol, luabridge::LuaRef func) {
    HookFuncInfo *info = gSymbolMap[symbol];
    void* origin_func_pointer = getFuncSiganature(info->frameworkName.c_str(), symbol);
    
    int8_t argTypes[info->argCount];
    for (int i=0;i<info->argCount;++i) {
        argTypes[i] = info->argTypes[i];
    }
    luabridge::LuaRef *luaFunc = new luabridge::LuaRef(func);
    
    hook_func(info->frameworkName.c_str(), symbol, info->returnType, argTypes, info->argCount, luaFunc);
}

int8_t ffiTypeFromString(string str) {
    if (str == "int") {
        return 1;
    }
    
    return 0;
}

vector<string> stringsplit(const char *str, const char *delim) {
    vector <string> strlist;
    char *saveptr = NULL;
    char *p = const_cast<char*>(str);
    char *input = strdup(p);
    while (NULL != (input = strtok_r(input, delim, &saveptr))) {
        strlist.push_back(input);
        input = NULL;
    }
    free(input);
    return strlist;
}

void useFunction(const char* frameworkName, const char* funcName, const char* types) {
    HookFuncInfo *info = new HookFuncInfo();
    info->frameworkName = frameworkName;
    info->funcName = funcName;
    
    if (strlen(types) > 0) {
        vector<string> typeStrs = stringsplit(types, ",");
        info->argCount = typeStrs.size() - 1;
        info->returnType = ffiTypeFromString(typeStrs[0]);
        if (info->argCount > 0) {
            info->argTypes = new int8_t[info->argCount];
            for (int i = 1;i < typeStrs.size();++i) {
                info->argTypes[i-1] = ffiTypeFromString(typeStrs[i]);
            }
        }
    }
    else {
        info->returnType = 0;
        info->argCount = 0;
        info->argTypes = nullptr;
    }
    gSymbolMap[funcName] = info;
}

// 注册函数
void register_with_lua(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginNamespace("DyHookCore")
        .addFunction("useFunction", useFunction)
        .addFunction("callCppFunction", callCppFunction)
        .addFunction("hookCppFunction", hookCppFunction)
        .endNamespace();
    
}
