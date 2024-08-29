hookCppFunction("HookFramework", "_Z11hook_c_funcii", function(a, b, origin) {
    local result = origin();
    return a + b + result;
})

callCppFunction("_Z11hook_c_funcii", {1,1,1}, 1, 2)
