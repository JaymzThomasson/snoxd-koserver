-----
-- Script for [Knight Clerk] Charel in Elmorad Castle
-----

pUser = nil
pNpc = nil

function Main(event)
	if (event == 240) then
						BaseMenu()
	elseif (event == 241) then
						Close()
	elseif (event == 100) then
						KnightPrivileges()
	elseif (event == 110) then
						AcreditedTest()
	elseif (event == 111) then
						AcreditedTestStory()
	elseif (event == 112) then
						AcreditedTestStory1()
	elseif (event == 113) then
						AcreditedTestStory2()
	elseif (event == 114) then
						AcreditedGiveRequiredItems()
	elseif (event == 115) then
						AcreditedTakeTest()
	elseif (event == 116) then
						AcreditedAcceptRequiredItems()
	end
end

function BaseMenu()
						pUser:SelectMsg(3,-1,4150,4150,100,4151,360,4155,370,4156,380,4154,241)
end

function Close()
	return 1
end

local Knight
local ClanGrade
local Chief

function KnightPrivileges()
						pUser:SelectMsg(3,-1,4150,4333,280,4334,110,4335,120,4154,241)
end

function AcreditedTest()
	if (pUser:GetClanRank() == 2) then
		if (pUser:GetClanGrade() == 1) then
			if (pUser:isClanLeader() == 1) then
						pUser:SelectMsg(3,-1,6393,4157,111,4158,114,4159,115)
			else
						pUser:SelectMsg(2,-1,6384,10,241)
			end
		else
						pUser:SelectMsg(2,-1,6385,10,241)
		end
	else
						pUser:SelectMsg(2,-1,6386,10,241)
	end
end

function AcreditedTestStory()
						pUser:SelectMsg(2,-1,6387,4160,112,27,241)
end

function AcreditedTestStory1()
						pUser:SelectMsg(2,-1,6388,4160,113,27,241)
end

function AcreditedTestStory2()
						pUser:SelectMsg(2,-1,6389,27,241)
end

function AcreditedGiveRequiredItems()
						pUser:SelectMsg(4,953,6390,4161,116,4162,241)
end

function AcreditedTakeTest()
						pUser:ZoneClanChange(93,444,350)
end

function AcreditedAcceptRequiredItems()
	if (pUser:hasCoins(10000000)) then
		if (pUser:CheckExistItem(389221000)) then
						pUser:RunExchange(467)
						pUser:PromoteKnight(3)
		else
						pUser:SelectMsg(2,-1,6392,10,241)
		end
	else
						pUser:SelectMsg(2,-1,6392,10,241)
	end
end
