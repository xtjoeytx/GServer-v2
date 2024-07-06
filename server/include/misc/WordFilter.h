#ifndef CWORDFILTER_H
#define CWORDFILTER_H

#include <memory>
#include <vector>

#include <CString.h>

enum
{
	FILTER_CHECK_CHAT = 0x1,
	FILTER_CHECK_PM = 0x2,
	FILTER_CHECK_NICK = 0x4,
	FILTER_CHECK_TOALL = 0x8,
};

enum
{
	FILTER_POSITION_FULL = 1,
	FILTER_POSITION_START = 2,
	FILTER_POSITION_PART = 3,
};

enum
{
	FILTER_ACTION_LOG = 0x1,
	FILTER_ACTION_TELLRC = 0x2,
	FILTER_ACTION_REPLACE = 0x4,
	FILTER_ACTION_WARN = 0x8,
	FILTER_ACTION_JAIL = 0x10,
	FILTER_ACTION_BAN = 0x20,
};

struct WordFilterRule
{
	int check = 0;
	int wordPosition = 0;
	int action = 0;
	int precision = 100;
	bool precisionPercentage = true;
	CString match;
	CString warnMessage;
};
using WordFilterRulePtr = std::unique_ptr<WordFilterRule>;

class Server;
class Player;
class WordFilter
{
public:
	WordFilter(Server* pServer) : m_server(pServer) {}
	~WordFilter();

	void load(const CString& file);
	int apply(const Player* player, CString& chat, int check);

private:
	Server* m_server;

	bool m_showWordsToRC = false;
	CString m_defaultWarnMessage;
	std::vector<WordFilterRulePtr> m_rules;
};

#endif
