//
//  DyHooker.cpp
//  HookFramework
//
//  Created by zhiyangfu on 2024/7/22.
//

#include "DyHooker.hpp"
#import "libffi/include/ffi/ffi.h"
#include <dlfcn.h>
#include "fishhook/fishhook.h"

struct custom_data {
    ffi_type *returnType;
    ffi_type** argTypes;
    int8_t argCount;
    void *origin_func_pointer;
};

void replacement_function(ffi_cif* cif, void* ret, void** args, void* userdata) {
    // 调用原实现
    custom_data *customData = (custom_data *)userdata;
    ffi_cif cif2;
    if (ffi_prep_cif(&cif2, FFI_DEFAULT_ABI, customData->argCount, customData->returnType, customData->argTypes) == FFI_OK) {
        ffi_call(&cif2, FFI_FN(customData->origin_func_pointer), &ret, args);
        printf("result: ");
    } else {
        printf("Failed to prepare CIF in replacement_function");
    }
    
    // call lua
}

// 使用 libffi 构造一个函数指针
void* create_function_pointer(ffi_type *returnType, ffi_type** argTypes, int8_t argCount, void *origin_func_pointer) {
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

void hook_func(const char* framework, const char* symbol, int8_t returnType, int8_t* argTypes, int8_t argCount) {
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
    
    void* origin_func_pointer = dlsym(handle, symbol);
    const char* dlsym_error2 = dlerror();
    if (dlsym_error2) {
        printf("Failed to load symbol: %s", dlsym_error2);
        dlclose(handle);
        return;
    }
    
    ffi_type* ffi_return_type = getFFIType(returnType);
    
    ffi_type** ffi_arg_types = new ffi_type*[4];
    for (int8_t i = 0; i < argCount; i++) {
        ffi_arg_types[i] = getFFIType(argTypes[i]);
    }
    void* replacement_func_pointer = create_function_pointer(ffi_return_type, ffi_arg_types, argCount, origin_func_pointer);
    if (replacement_func_pointer == nullptr) {
        printf("Failed to create function pointer");
        return;
    }
    
    // 进行符号替换
    rebind_function(symbol, origin_func_pointer, replacement_func_pointer);
}
