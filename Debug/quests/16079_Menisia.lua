-----
-- Script for [Grand Merchant's Daughter] Menissiah in Moradon
-----

-- Globals
pUser = nil
pNpc = nil

local WormSilkSpool			= 110
local WormSilkSpoolRequest	= 111
local WormSilkSpoolAccepted	= 112

local SilkBundleMenu		= 280	
local SilkBundleExchange	= 281

function Main(event)
	-- Map the events to functions
	local t = {
		-- Default event for this NPC
		[190]					= HandleNPCSelection,

		-- Unknown, possibly not even used.
		[105]					= Quest105,
		[120]					= Quest120,
		
		-- Worm silk spool quest
		[WormSilkSpool]			= function() pUser:SelectMsg(2, 100, 150, 29, WormSilkSpoolRequest) end,
		-- intermediary dialog before you accept the quest. 
		[WormSilkSpoolRequest]	= function() pUser:SelectMsg(4, 100, 156, 22, WormSilkSpoolAccepted, 23, 193) end, 
		-- fancy new quest dialog to accept quests...
		[WormSilkSpoolAccepted]	= function() pUser:SaveEvent(7) end,

		-- Main menu for silk bundle exchanges
		[SilkBundleMenu]		= function()
			if (pUser:CheckExistItem(379048000, 5)) then
				pUser:SelectMsg(2, 100, 158, 4006, SilkBundleExchange, 27, 193)
			else
				pUser:SelectMsg(2, 100, 157, 10, 193)
			end
		end,
		
		-- The silk bundle exchange itself
		[SilkBundleExchange]	= function()
			if (pUser:CheckExchange(1)) then
				pUser:SaveEvent(8)
				pUser:RunExchange(1)
			end
		end,
	}
	
	local func = t[event]
	if (func and type(func) == "function") then
		func()
	end
end

function HandleNPCSelection()
	-- Map specific quest events to functions
	local sQuest = pUser:SearchQuest()
	
	-- Nothing to do, user's completed everything available for them.
	if (sQuest == 0) then
		pUser:SelectMsg(2, -1, 191, 10, 193)
	-- Show main quest list
	elseif (sQuest > 1 and sQuest < 100) then
		pUser:NpcMsg(193)
	-- Handle individual quests
	else
		Main(sQuest)
	end
end

-- Possibly not used
function Quest105()
	pUser:SaveEvent(6)
	if (pUser:GetNation() == 1) then
		pUser:SelectMsg(1, 100, 105, 28, 107)
	else
		pUser:SelectMsg(1, 100, 111, 28, 107)
	end
end

-- Possibly not used
function Quest120()
	pUser:SaveEvent(9)
	if (pUser:GetNation() == 1) then
		pUser:SelectMsg(1, 100, 131, 14, 193)
	else
		pUser:SelectMsg(1, 100, 132, 14, 193)
	end
end