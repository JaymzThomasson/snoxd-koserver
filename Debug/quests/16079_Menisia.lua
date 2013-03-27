-----
-- Script for [Grand Merchant's Daughter] Menissiah in Moradon
-----

-- Globals
pUser = nil
pNpc = nil

local CancelButton				= 0 -- could be any event that doesn't exist

local WormSilkSpool				= 110
local WormSilkSpoolRequest		= 111
local WormSilkSpoolAccepted		= 112

local BandicootTeeth			= 200
local BandicootTeethRequest		= 201
local BandicootTeethAccepted	= 202

local SilkBundleMenu			= 280	
local SilkBundleExchange		= 281

function Main(event)
	-- Map the events to functions
	local t = {
		-- Default event for this NPC
		[190]						= HandleNPCSelection,
		
		[BandicootTeeth]			= function() pUser:SelectMsg(2, 102, 200, 29, BandicootTeethRequest) end,
		[BandicootTeethRequest]		= function() pUser:SelectMsg(4, 102, 201, 22, BandicootTeethAccepted, 23, CancelButton) end,
		[BandicootTeethAccepted]	= function() pUser:SaveEvent(57) end,
		
		-- Unknown, possibly not even used.
		[105]						= Quest105,
		[120]						= Quest120,
		[205]						= Quest205,

		-- Also unknown, the only one that works (because there's no text references) is 196
		-- but that shows a map telling you where Menissiah is; the NPC we're talking to. 
		-- Probably not useful in this case.
		[195]						= function() pUser:SelectMsg(1, 102, 195, 28, 196) end,
		[196]						= 
			function()
				pUser:ShowMap(5)
				pUser:SaveEvent(56)
			end,
		[197]						= function() pUser:SelectMsg(1, 102, 197, 29, 196) end,
		
		-- Worm silk spool quest menu.
		[WormSilkSpool]				= function() pUser:SelectMsg(2, 100, 150, 29, WormSilkSpoolRequest) end,
		-- Do you wish to accept this quest? Fancy new dialog. 
		[WormSilkSpoolRequest]		= function() pUser:SelectMsg(4, 100, 156, 22, WormSilkSpoolAccepted, 23, CancelButton) end, 
		-- Accepted the worm silk spool quest..
		[WormSilkSpoolAccepted]		= function() pUser:SaveEvent(7) end,

		-- Main menu for silk bundle exchanges
		[SilkBundleMenu]			= function()
			if (pUser:CheckExistItem(379048000, 5)) then
				pUser:SelectMsg(2, 100, 158, 4006, SilkBundleExchange, 27, CancelButton)
			else
				pUser:SelectMsg(2, 100, 157, 10, CancelButton)
			end
		end,
		
		-- The silk bundle exchange itself
		[SilkBundleExchange]		= function()
			if (pUser:CheckExchange(1)) then
				pUser:SaveEvent(8)
				pUser:RunExchange(1)
			end
		end,
	}

	print("Event: " .. event)
	local func = t[event]
	if (func and type(func) == "function") then
		func()
	end
end

function HandleNPCSelection()
	-- Lookup available quest (if none, it'll return 0)
	local sQuest = pUser:SearchQuest()
	
	-- Nothing to do, user's completed everything available for them.
	if (sQuest == 0) then
		pUser:SelectMsg(2, -1, 191, 10, CancelButton)
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
		pUser:SelectMsg(1, 100, 131, 14, CancelButton)
	else
		pUser:SelectMsg(1, 100, 132, 14, CancelButton)
	end
end

-- Possibly not used
function Quest205()
	pUser:SaveEvent(59)
	if (pUser:GetNation() == 1) then
		pUser:SelectMsg(1, 102, 206, 32, 189)
	else
		pUser:SelectMsg(1, 102, 207, 4080, 189)
	end
end