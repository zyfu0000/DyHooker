DyHookCore.useFunction("HookFramework", "_Z11hook_c_funcii", "int,int,int")


DyHookCore.hookCppFunction("_Z11hook_c_funcii", function(a, b)
    -- local result = origin();
    -- return a + b + result;
    
    return a + b + b;
end)

DyHookCore.callCppFunction("_Z11hook_c_funcii", 3, 4)

