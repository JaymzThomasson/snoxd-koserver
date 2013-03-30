pUser = nil
pNpc = nil

-- Map the events to functions
local eventMap = {
	-- Default event for this NPC
	[100]						= HandleNPCSelection,

	[400]						= Event400,
	[403]						= Event403,
	[404]						= Event404,
	[405]						= Event404,
	[407]						= Event407,
	[408]						= Event408,
	[500]						= Event500,
	[501]						= Event501,
	[502]						= Event502,
	[503]						= Event503,
}

function Main(event)
	print("Event: " .. event)
	local func = eventMap[event]
	if (func and type(func) == "function") then
		func()
	end
end

function HandleNPCSelection()
	local sQuest = pUser:SearchQuest()
	if (sQuest == 0) then 
		pUser:SelectMsg(2, -1, 146, 10, 101)
	elseif (sQuest > 1) and (sQuest < 100) then 
		pUser:NpcMsg(147)
	else
		Main(sQuest)
	end
end

function Event400()
	local sClass = pUser:GetClass() % 100
	if (sClass == 1 or sClass == 2 or sClass == 3 or sClass == 4) then
		pUser:SaveEvent(4061)
		if (pUser:GetNation() == 1) then
			pUser:SelectMsg(1, 406, 4062, 4061, 101)
		else
			pUser:SelectMsg(1, 406, 4063, 4061, 101)
		end
	end
end

function Event403()
	if (pUser:GetLevel() < 10) then
		pUser:SelectMsg(2, 406, 4068, 10, 101)
		return
	end
	
	local sClass = pUser:GetClass() % 100
	if (sClass ~= 1 and sClass ~= 2 and sClass ~= 3 and sClass ~= 4) then
		pUser:SelectMsg(2, 406, 4068, 10, 101)
		return
	end

	if (not pUser:hasCoins(3000)) then
		pUser:SelectMsg(2, 406, 4065, 10, 101)
	else
		pUser:SelectMsg(2, 406, 4064, 4062, 404, 4063, 101)
	end
end

function Event404()
	pUser:SaveEvent(4063)
	pUser:PromoteUserNovice()
	pUser:GoldLose(3000)

	if (pUser:GetNation() == 1) then
		pUser:SelectMsg(1, 406, 4066, 4064, 101)
	else
		pUser:SelectMsg(1, 406, 4067, 4064, 101)
	end
end

function Event407()
   pUser:SaveEvent(4070)
   pUser:SelectMsg(2, 407, 4070, 4070, 408, 10, 101)
end

function Event408()
	pUser:StatPointDistribute()
end

function Event500()
	pUser:SelectMsg(3, 453, 4704, 4238, 501, 4239, 502, 4240, 503)
end

function Event501()
	pUser:SelectMsg(9, 453, 4704, 4070, 408, 10, 101)
end

function Event502()
	pUser:SelectMsg(9, 453, 4704)
end

function Event503()
	pUser:SelectMsg(9, 453, 4704)
end