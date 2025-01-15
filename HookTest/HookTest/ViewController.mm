//
//  ViewController.m
//  HookTest
//
//  Created by ZhiyangFu on 2024/7/20.
//

#import "ViewController.h"

#include <HookFramework/HookFramework.h>
#include <pthread.h>
#include "DyHooker.hpp"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

@interface ViewController ()

@end

@implementation ViewController

+ (void)load {

}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    register_with_lua(L);
    
    NSString *filePath = [[NSBundle mainBundle] pathForResource:@"script" ofType:@"lua"];
    // 运行Lua脚本
    if (luaL_dofile(L, [filePath UTF8String]) != LUA_OK) {
        const char *error = lua_tostring(L, -1);
        printf("Error: %s\n", error);
        lua_pop(L, 1);  // 从栈中移除错误信息
    }

//    lua_close(L);  // 关闭Lua环境
    
//    int result =  hook_c_func(1, 2);
//    
//    printf("Error: %d\n", result);
}


@end
