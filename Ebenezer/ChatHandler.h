#pragma once

typedef std::list<std::string> CommandArgs;

template <class T>
class Command
{
public:
	const char * Name;
	bool (T::*Handler)(CommandArgs & vargs, const char *args, const char *description);
	const char * Help;
};

__forceinline void* allocate_and_copy(uint32 len, void * pointer)
{
	void * data = (void*)malloc(len);
	if (data == NULL)
		return data;

	memcpy(data, pointer, len);
	return data;
}

#define init_command_table(t, command_table, command_map) \
	for (int i = 0; i < sizeof(command_table) / sizeof(*command_table); i++) \
		command_map.insert(std::make_pair(command_table[i].Name, (Command<t> *)(allocate_and_copy(sizeof(*command_table), (void *)&command_table[i]))));

#define free_command_table(command_map) \
	for (auto itr = command_map.begin(); itr != command_map.end(); ++itr) \
		delete itr->second; \
	command_map.clear();

static std::list<std::string> StrSplit(const std::string &src, const std::string &sep)
{
	std::list<std::string> r;
	std::string s;
	for (std::string::const_iterator i = src.begin(); i != src.end(); ++i)
	{
		if (sep.find(*i) != std::string::npos)
		{
			if (!s.empty())
				r.push_back(s);
			s = "";
		}
		else
		{
			s += *i;
		}
	}
	if (!s.empty()) 
		r.push_back(s);
	return r;
}

typedef std::map<std::string, Command<CUser> *> ChatCommandTable;
typedef std::map<std::string, Command<CEbenezerDlg> *> ServerCommandTable;

#define CHAT_COMMAND_PREFIX		'+'
#define SERVER_COMMAND_PREFIX	'/'