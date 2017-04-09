local function filesize (fd)
   local current = fd:seek()
   local size = fd:seek("end")
   fd:seek("set", current)
   return size
end

function make(infilename,outfilename,data_name)
	f = io.open(infilename)
	o = io.open(outfilename,"w")
	str = string.format("static const unsigned char %s[]={\n",data_name) --filesize??
	o:write(str)
	print(filesize(f))
	len=0
	counter=0
	for i=1,filesize(f)-1 do
		counter = counter + 1
		c=f:read(1) or '\0'
		str = string.format("0x%x,",c:byte()) --"0x%02x" read(1) ??
		print(str)
		o:write(str)

		len = len + #str
		if len>80 then
			o:write("\n")
			len = 0
		end
	end

	counter = counter + 1
	c=f:read(1) or '\0'
	str = string.format("0x%x",c:byte())
	print(str)
	o:write(str .. "};")

	print(counter)
	
	f:close()
	o:close()
end

--make("flare.rgba","flare_rgba.h","flare_rgba")
make("build/Fonts/Future2.pkm","Future2_pkm.h","Future2_pkm")