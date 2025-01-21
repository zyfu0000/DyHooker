local instanceObject = { point = nilObject}
function instanceObject:new(o)
    o = o or {}
    setmetatable(o,self)--设置instanceObject对象为生成对象o的元表
    self.__index = function ( self, key )--设置全局instanceObject对象的__index是一个函数
        --假如键是super，返回一个函数，调用这个函数生成isSuper为true的对象
        if key == "super" then
            return function ( ... )
                return instanceObject:new({point = self.point, isSuper = true})
            end
        end
        return function ( ... )
            local method = key
            --假如是调用super的情况
            local arglist = buildArgList(...)
            printLog("call callI in instanceObject instance:",self.point,"method",method)
            printLog("arglist")
            printLog(tableToStr(arglist))
            local ret = nil
            if self.isSuper == true then
                ret = luapatch_core.callSuperI(self.point,method,table.unpack(arglist))
            else
                ret = luapatch_core.callI(self.point,method,table.unpack(arglist))
            end
            if type(ret) == "userdata" then
                return instanceObject:new({point = ret})
            end
            return ret
        end
    end
    return o
end

local classObject = { className = "UObject" }
function classObject:new(o)
    o = o or {}
    setmetatable(o,self)
    self.__index = function (self, key) --这里后面的self和前面的self是不一样的
        return function(...)
            local method = string.gsub(key,"_",":") -- 用冒号替换_，还原真实的函数名称，这里规定假如有两个__相连的表示真实的_符
            method = string.gsub(method,"::","_")
            --假如参数个数比:号多的话，要在最后添加:号
            if #{...} > charAppearCount(method,":") then
                method = method..":"
            end
            --遍历参数
            local arglist = buildArgList(...)
            printLog("call callC in classObject className:"..self.className.." method:"..method)
            printLog("arglist")
            printLog(tableToStr(arglist))
            local ret = luapatch_core.callC(self.className,method,table.unpack(arglist))
            if type(ret) == "userdata" then
                return instanceObject:new({point = ret})
            end
            return ret
        end
    end
    return o
end
