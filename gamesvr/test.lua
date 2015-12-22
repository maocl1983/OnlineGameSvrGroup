local myLog = require "log"

function testSimple(i,j)
	return i+j
end

function test(p)
	local res_value = get_res_value(p,17)
	myLog.T_DEBUG_LOG(string.format("res value=%d", res_value))
	myLog.T_DEBUG_LOG("lua test ok")
	return res_value;
end 
