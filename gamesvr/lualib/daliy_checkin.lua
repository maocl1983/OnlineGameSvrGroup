local log = require "log"
local utils = require "utils"

local daliy_check_stat = 10001
local month_check_stat = 20001

function daliy_checkin(p)
	local daliy_flag = p:get_res_value(daliy_check_stat)
	if daliy_flag ~= 0 then
		return 10011
	end

	local cur_time = utils.get_cur_time()
	local cur_day = utils.get_day(cur_time)
	local month_stat = p:get_res_value(month_check_stat)
	if utils.test_bit_on(month_stat, cur_day) ~= false then
		log.T_ERROR_LOG(string.format("daliy chekin error2[%d %d]", month_stat, cur_day))
		return 10012
	end

	p:set_res_value(daliy_check_stat,1)
	month_stat = utils.set_bit_on(month_stat, cur_day)
	p:set_res_value(month_check_stat, month_stat)

	p:add_item(110000, 10)

	log.T_DEBUG_LOG(string.format("daliy chekin [%d]", 110000))

	return 0
end
