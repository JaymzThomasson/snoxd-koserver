-----
-- Script for [Grand Merchant] Kaishan in Moradon.

-- This is a basic script - Needs work!
-----

pUser = nil
pNpc = nil

function Main(event)
	if (event == 100) then
		MainMenu()
	elseif (event == 10) then
		JobChange()
	elseif (event == 11) then
		JobChangeAccept()
	end
end

function	MainMenu()
		pUser:SelectMsg(2, -1, 102, 4062, 10, 47, 0)
		
end

function	JobChange()
		pUser:SelectMsg(2, -1, 4064, 4524, 11, 47, 0)
end

function	JobChangeAccept()
		pUser:PromoteUserNovice() -- Not Implemented in the Server yet.
		pUser:SaveEvent(110) -- Not sure of this Event Number yet.
end