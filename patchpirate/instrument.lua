function request_update()
   instrument:send_sysex({0xf0, 0x7e, 0xf7})
end

gui:add_toolbutton("Request update", request_update)

--
-- Called by the Patch Pirate core
--

function parameter_changed(param, value)
   io.write(string.format("LUA: GUI changed param: %s (%s) = %s\n",
                          param.label, param.id, value))
   instrument:send_sysex({0xf0, 0x7f,
                          high8(param.number), low8(param.number),
                          high7(value), low7(value),
			  0xf7})
end

function handle_sysex_data(data)
   io.write("LUA got sysex: ")
   for i, d in ipairs(data) do
      io.write(string.format("%02X", d))
      if ((i%4)==0) then io.write(" ") end
   end
   io.write("\n")

   if (data:matches({ 0xf0, 0x7f })) then
      local param = bit32.bor(bit32.lshift(data[3], 8), data[4])
      local value = sexwordfrom7s(data[5], data[6])

      io.write(string.format("Got param: %04x %04x\n", param, value))
      instrument:update_parameter(param, value)
   end
end
