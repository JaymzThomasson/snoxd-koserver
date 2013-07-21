-----
-- Script for [National Enchanter] in Moradon
-----

pUser = nil
pNpc = nil

function Main(event)
	if (event == 100) then
				BaseMenu()
		elseif (event == 7098) then
					VictoryMenu()
		elseif (event == 7099) then
					NoahMenu()
		elseif (event == 7091) then
					ShoutForCharge()
		elseif (event == 7092) then
					FirmWill()
		elseif (event == 7093) then
					EndlessPatience()
	end
end

function BaseMenu()
	pUser:SelectMsg(3,-1,9138,7098,7098,7099,7099)
end

function VictoryMenu()
	pUser:SelectMsg(3,-1,3,20,0)
end

function NoahMenu()
	pUser:SelectMsg(3,-1,9138,7091,7091,7092,7092,7093,7093)
end

function ShoutForCharge()
	if (pUser:hasCoins(53000)) then
		if (pUser:GetLevel() <20) then
				if (not pNpc:CastSkill(pUser,302327)) then
						pUser:NpcMsg(9140)
						return
				end
						pNpc:CastSkill(pUser,302327)
						pUser:ShowNpcEffect(31033)
						pUser:NpcMsg(9137)
		elseif (pUser:GetLevel() <60) then
				if (not pNpc:CastSkill(pUser,302327)) then
						pUser:NpcMsg(9140)
						return
				end
						pNpc:CastSkill(pUser,302327)
						pUser:ShowNpcEffect(31033)
						pUser:GoldLose(3000)
						pUser:NpcMsg(9137)
		else
				if (not pNpc:CastSkill(pUser,302327)) then
						pUser:NpcMsg(9140)
						return
				end
						pNpc:CastSkill(pUser,302327)
						pUser:ShowNpcEffect(31033)
						pUser:GoldLose(50000)
						pUser:NpcMsg(9137)
		end
	else
						pUser:NpcMsg(9118)
	end
end

function FirmWill()
	if (pUser:hasCoins(53000)) then
		if (pUser:GetLevel() < 20) then
				if (not pNpc:CastSkill(pUser,302331)) then
						pUser:NpcMsg(9140)
						return
				end
						pNpc:CastSkill(pUser,302331)
						pUser:ShowNpcEffect(31033)
						pUser:NpcMsg(9137)
		elseif (pUser:GetLevel() <60) then
				if (not pNpc:CastSkill(pUser,302332)) then
						pUser:NpcMsg(9140)
						return
				end
						pNpc:CastSkill(pUser,302332)
						pUser:ShowNpcEffect(31033)
						pUser:GoldLose(3000)
						pUser:NpcMsg(9137)
		else
				if (not pNpc:CastSkill(pUser,302333)) then
						pUser:NpcMsg(9140)
						return
				end
						pNpc:CastSkill(pUser,302333)
						pUser:ShowNpcEffect(31033)
						pUser:GoldLose(50000)
						pUser:NpcMsg(9137)
		end
	else
						pUser:NpcMsg(9118)
	end
end

function EndlessPatience()
	if (pUser:hasCoins(53000)) then
		if (pUser:GetLevel() <20) then
				if (not pNpc:CastSkill(pUser,302328)) then
						pUser:NpcMsg(9140)
						return
				end
						pNpc:CastSkill(pUser,302328)
						pUser:ShowNpcEffect(31033)
						pUser:NpcMsg(9137)
		elseif (pUser:GetLevel() <60) then
				if (not pNpc:CastSkill(pUser,302329)) then
						pUser:NpcMsg(9140)
						return
				end
						pNpc:CastSkill(pUser,302329)
						pUser:ShowNpcEffect(31033)
						pUser:GoldLose(3000)
						pUser:NpcMsg(9137)
		else
				if (not pNpc:CastSkill(pUser,302330)) then
						pUser:NpcMsg(9140)
						return
				end
						pNpc:CastSkill(pUser,302330)
						pUser:ShowNpcEffect(31033)
						pUser:GoldLose(50000)
						pUser:NpcMsg(9137)
		end
	else
						pUser:NpcMsg(9118)
	end
end
