-----
--Script for [Grand Merchant] Kaishan in Moradon.

--Still not working correctly, Maybe it's just me not understanding
--how it all works but comparing it to the way I did it which worked
--I cant see much more of a difference... (Equalsmc2 - 30/03/2013)
-----

local Ret = 0		-- Dont Know what this is about.
local QuestNum

level = nil
nation = nil
class = nil

pUser = nil
pNpc = nil



function Main(event)

	print(pUser:GetName() .. ": Running event " .. event .. " from NPC " .. pNpc:GetName())

if (event == 100) then
	QuestNum = pUser:SearchQuest()
		if (QuestNum == 0) then
			 pUser:SelectMsg(2, -1, 146, 10, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
			 Ret = 1		-- Whats going on?..
		elseif (QuestNum > 1 and QuestNum < 100) then 
          pUser:NpcMsg(147, 18004)
      else
          event = QuestNum
		end
end

if (event == 101) then
    Ret = 1		-- Again, What to do with this?...
end



local savenum = 406

if (event == 400) then
   class = pUser:CheckClass()
   if (class == 1 or class == 2 or class == 3 or class == 4) then
      pUser:SaveEvent(4061)
      nation = pUser:CheckNation()
      if (nation == 1) then
      pUser:SelectMsg(1, savenum, 4062, 4061, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
      else
      pUser:SelectMsg(1, savenum, 4063, 4061, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
      end
   else
   Ret = 1		-- And again....
   end
end

if (event == 403) then   
level = pUser:CheckLevel()		-- Script errors on this "attempt to call method 'CheckLevel' (a nil value).. What??
   if (Level > 9) then   
   class = pUser:CheckClass()
      if (class == 1 or class == 2 or class == 3 or class == 4) then
       item_count = pUser:HowmuchItem(900000000)
         if (item_count < 3000) then 
         pUser:SelectMsg(2, savenum, 4065, 10, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
         else
         pUser:SelectMsg(2, savenum, 4064, 4062, 404, 4063, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
         end
      else
      pUser:SelectMsg(2, savenum, 4068, 10, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
      end
   else
   pUser:SelectMsg(2, savenum, 4068, 10, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
   end
end

if (event) == 404 then
   pUser:SaveEvent(4063)
   pUser:PromoteUserNovice()
   pUser:RobItem(900000000, 3000)
   nation = pUser:CheckNation()
   if (nation == 1) then
   pUser:SelectMsg(1, savenum, 4066, 4064, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
   else
   pUser:SelectMsg(1, savenum, 4067, 4064, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
   end
end



local savenum = 407

if (event == 407) then
   pUser:SaveEvent(4070)
   pUser:SelectMsg(2, savenum, 4070, 4070, 408, 10, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
end

if (event == 408) then
pUser:StatPointDistribute()
end



local savenum = 453

if (event == 500) then
   pUser:SelectMsg(3, savenum, 4704, 4238, 501, 4239, 502, 4240, 503, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
end


if (event == 501) then
   pUser:SelectMsg(9, savenum, 4704, 4070, 408, 10, 101, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
end

if (event == 502) then
   pUser:SelectMsg(9, savenum, 4704, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
end

if (event == 503) then
   pUser:SelectMsg(9, savenum, 4704, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)
	
	end
end