-----
-- Script for [Sentinel] Patrick in Moradon
-----

pUser = nil

function Main(event)
	if (event == 165) then
						BaseMenu()
	end
end

function BaseMenu()
						pUser:SelectMsg(3,-1,4,27,0)
end
