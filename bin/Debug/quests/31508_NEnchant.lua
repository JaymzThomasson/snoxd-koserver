-----
-- Script for [National Enchanter] in Moradon
-----

pUser = nil
pNpc = nil

function Main(event)
	if (event == 100) then
				DefaultMenu()
		elseif (event == 7091) then
					ShoutForCharge()
		elseif (event == 7092) then
					FirmWill()
		elseif (event == 7093) then
					EndlessPatience()
	end
end

function DefaultMenu()
	pUser:SelectMsg(3, -1, 9138, 7091, 7091, 7092, 7092, 7093, 7093, 27, 0)
end

function ShoutForCharge()
	if (pUser:hasCoins(20000)) then
		if (pUser:GetLevel() <20) then
				if (pUser:isMage()) then
						pNpc:CastSkill(pUser, 302326)
						pUser:NpcMsg(9137)
				else
						pNpc:CastSkill(pUser, 302327)
						pUser:NpcMsg(9137)
				end
		elseif (pUser:GetLevel() <60) then
				if (pUser:isMage()) then
						pNpc:CastSkill(pUser, 302326)
						pUser:GoldLose(10000)
						pUser:NpcMsg(9137)
				else
						pNpc:CastSkill(pUser, 302327)
						pUser:GoldLose(10000)
						pUser:NpcMsg(9137)
				end
		else
				if (pUser:isMage()) then
                        pNpc:CastSkill(pUser, 302326)
                        pUser:GoldLose(20000)
						pUser:NpcMsg(9137)
                else
						pNpc:CastSkill(pUser, 302327)
						pUser:GoldLose(20000)
						pUser:NpcMsg(9137)
				end
		end
	else
                        pUser:NpcMsg(9118)
	end
end

function FirmWill()
	if (pUser:hasCoins(20000)) then
		if (pUser:GetLevel() < 20) then
						pNpc:CastSkill(pUser, 302331)
						pUser:NpcMsg(9137)
        elseif (pUser:GetLevel() <60) then
                        pNpc:CastSkill(pUser, 302332)
                        pUser:GoldLose(10000)
						pUser:NpcMsg(9137)
                else
                        pNpc:CastSkill(pUser, 302333)
                        pUser:GoldLose(20000)
						pUser:NpcMsg(9137)
        end
	else
                        pUser:NpcMsg(9118)
    end
end

function EndlessPatience()
	if (pUser:hasCoins(20000)) then
		if (pUser:GetLevel() <20) then
						pNpc:CastSkill(pUser, 302328)
						pUser:NpcMsg(9137)
		elseif (pUser:GetLevel() <60) then
                        pNpc:CastSkill(pUser, 302329)
                        pUser:GoldLose(10000)
						pUser:NpcMsg(9137)
                else
                        pNpc:CastSkill(pUser, 302330)
                        pUser:GoldLose(20000)
						pUser:NpcMsg(9137)
        end
	else
                        pUser:NpcMsg(9118)
    end
end