
CREATE TABLE [dbo].[QUEST_HELPER](
	[nIndex] [int] NOT NULL,
	[bMessageType] [tinyint] NOT NULL,
	[bLevel] [tinyint] NOT NULL,
	[nExp] [int] NOT NULL,
	[bClass] [tinyint] NOT NULL,
	[bNation] [tinyint] NOT NULL,
	[bQuestType] [tinyint] NOT NULL,
	[bZone] [tinyint] NOT NULL,
	[sNpcId] [smallint] NOT NULL,
	[sEventDataIndex] [smallint] NOT NULL,
	[bEventStatus] [tinyint] NOT NULL,
	[nEventTriggerIndex] [int] NOT NULL,
	[nEventCompleteIndex] [int] NOT NULL,
	[nExchangeIndex] [int] NOT NULL,
	[nEventTalkIndex] [int] NOT NULL,
	[strLuaFilename] [char](40) NOT NULL
) ON [PRIMARY]

GO


CREATE TABLE [dbo].[QUEST_HELPER](
	[nIndex] [int] NOT NULL,
	[bMessageType] [tinyint] NOT NULL,
	[bLevel] [tinyint] NOT NULL,
	[nExp] [int] NOT NULL,
	[bClass] [tinyint] NOT NULL,
	[bNation] [tinyint] NOT NULL,
	[bQuestType] [tinyint] NOT NULL,
	[bZone] [tinyint] NOT NULL,
	[sNpcId] [smallint] NOT NULL,
	[sEventDataIndex] [smallint] NOT NULL,
	[bEventStatus] [tinyint] NOT NULL,
	[nEventTriggerIndex] [int] NOT NULL,
	[nEventCompleteIndex] [int] NOT NULL,
	[nExchangeIndex] [int] NOT NULL,
	[nEventTalkIndex] [int] NOT NULL,
	[strLuaFilename] [char](40) NOT NULL
) ON [PRIMARY]

GO