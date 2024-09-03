hookCppFunction("HookFramework", "_Z11hook_c_funcii", function(a, b)
    -- local result = origin();
    -- return a + b + result;
    
    return a + b + b;
end)

-- callCppFunction("_Z11hook_c_funcii", {1,1,1}, 1, 2)
