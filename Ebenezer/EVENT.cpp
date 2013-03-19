#include "stdafx.h"
#include "EVENT.h"
#include "EVENT_DATA.h"
#include "EXEC.h"
#include "LOGIC_ELSE.h"

EVENT::EVENT()
{
}

EVENT::~EVENT()
{
	DeleteAll();
}

BOOL EVENT::LoadEvent(int zone)
{
	DWORD		length, count;
	string		filename;
	char		byte;
	char		buf[4096];
	char		first[1024];
	char		temp[1024];
	int			index = 0;
	int			t_index = 0;
	int			event_num = -1;

	EVENT_DATA	*newData = NULL;
	EVENT_DATA	*eventData = NULL;

	filename = ".\\MAP\\";
	filename += zone;
	filename += ".evt";

	m_Zone = zone;

	std::ifstream is(filename);
	if (!is)
		return FALSE;

	is.seekg(0, is.end);
    length = (DWORD)is.tellg();
    is.seekg (0, is.beg);

	count = 0;

	while(count < length)
	{
		is.read(&byte, 1);
		count ++;

		if( byte != '\r' && (char)byte != '\n' ) buf[index++] = byte;

		if((byte == '\n' || count == length ) && index > 1 )
		{
			buf[index] = (BYTE) 0;

			t_index = 0;

			if( buf[t_index] == ';' || buf[t_index] == '/' )		// 주석에 대한 처리
			{
				index = 0;
				continue;
			}

			t_index += ParseSpace( first, buf + t_index );

//			if( !strcmp( first, "QUEST" ) )
			if( !strcmp( first, "EVENT" ) )
			{
				t_index += ParseSpace( temp, buf + t_index );	event_num = atoi( temp );

				if( newData )
				{
					delete newData;
					goto cancel_event_load;
				}

				if( m_arEvent.GetData(event_num) )
				{
					TRACE("Quest Double !!\n" );
					goto cancel_event_load;
				}

				eventData = new EVENT_DATA;
				eventData->m_EventNum = event_num;
				if( !m_arEvent.PutData( eventData->m_EventNum, eventData) ) {
					delete eventData;
					eventData = NULL;
				}
				newData = m_arEvent.GetData(event_num);
			}
			else if( !strcmp( first, "E" ) )
			{
				if( !newData )
				{
					goto cancel_event_load;
				}

				EXEC* newExec = new EXEC;

				newExec->Parse( buf + t_index );

				newData->m_arExec.push_back( newExec );				
			}
			else if( !strcmp( first, "A" ) )
			{
				if( !newData )
				{
					goto cancel_event_load;
				}

				LOGIC_ELSE* newLogicElse = new LOGIC_ELSE;

				newLogicElse->Parse_and( buf + t_index );

				newData->m_arLogicElse.push_back( newLogicElse );
			}
			else if( !strcmp( first, "END" ) )
			{
				if( !newData )
				{
					goto cancel_event_load;
				}

				newData = NULL;
			}

			index = 0;
		}
	}

	is.close();
	return TRUE;

cancel_event_load:
	printf("Unable to load EVT (%d.evt), failed in or near event number %d.\n", 
		zone, event_num);
	is.close();
	DeleteAll();
	return FALSE;
}

void EVENT::Init()
{
	DeleteAll();
}

void EVENT::Parsing(char *pBuf)
{
}

void EVENT::DeleteAll()
{
	m_arEvent.DeleteAllData();
}
