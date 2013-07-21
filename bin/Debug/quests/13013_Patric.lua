-----
-- Script for [National Enchanter] in Moradon
-----

pUser = nil
pNpc = nil
bSelectedReward = -1


function Main(event)
	if (event == 165) then
				DefaultMenu()
	end
end

function DefaultMenu()
		pUser:SelectMsg(3, -1, 167, 27, 0)
end