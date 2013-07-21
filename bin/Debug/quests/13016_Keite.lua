-----
-- Script for [Trainer] Kate in Moradon
-----

pUser = nil
pNpc = nil

function Main(event)
	if (event == 500) then
						BaseMenu()
	elseif (event == 104) then
						UseVoucher()
	elseif (event == 3001) then
						Decline()
	elseif (event == 101) then
						FamiliarHatchTrans()
	elseif (event == 102) then
						FamiliarNameChange()
	elseif (event == 103) then
						FamiliarRandom()
	elseif (event == 105) then
						ExchangeMagicBag()
	elseif (event == 106) then
						ExchangeAutoLoot()
	end
end

function BaseMenu()
						pUser:SelectMsg(3, -1, 4834, 4263, 101, 4264, 102, 4265, 103, 4337, 104, 4199, 3001)
end

function UseVoucher()
						pUser:SelectMsg(3, -1, 820, 4344, 105, 4345, 106, 4199, 3001)
end

function Decline()
						return
end

function FamiliarHatchTrans()
						return
end

function FamiliarNameChange()
						return
end

function FamiliarRandom()
						return
end

function ExchangeMagicBag()
						return
end

function ExchangeAutoLoot()
						return
end