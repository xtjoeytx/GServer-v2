#include "IDebug.h"
#include "CLog.h"
#include "IEnums.h"
#include "CWordFilter.h"
#include "TServer.h"
#include "TPlayer.h"

char bypass[] =
{
	' ', '\r', '\n',
};

static bool isUpper(char c)
{
	return (c >= 65 && c <= 90);
}

static bool isLower(char c)
{
	return (c >= 97 && c <= 122);
}

static char toLower(char c)
{
	if (c >= 65 && c <= 90)
		return c + 32;
	return c;
}

static char toUpper(char c)
{
	if (c >= 97 && c <= 122)
		return c - 32;
	return c;
}

CWordFilter::~CWordFilter()
{
	for (std::vector<SWordFilterRule*>::iterator i = rules.begin(); i != rules.end();)
	{
		SWordFilterRule* rule = *i;
		delete rule;
		i = rules.erase(i);
	}
	rules.clear();
}

void CWordFilter::load(const CString& file)
{
	// If we have rules, delete them.
	if (rules.size() != 0)
	{
		for (std::vector<SWordFilterRule*>::iterator i = rules.begin(); i != rules.end();)
		{
			SWordFilterRule* rule = *i;
			delete rule;
			i = rules.erase(i);
		}
		rules.clear();
	}

	// Load the file.
	std::vector<CString> f = CString::loadToken(file, "\n", true);
	if (f.size() == 0) return;

	// Parse the file.
	for (std::vector<CString>::iterator i = f.begin(); i != f.end(); ++i)
	{
		CString word = *i;
		std::vector<CString> wordParts = word.tokenize();
		if (wordParts.size() == 0) continue;

		if (wordParts[0] == "RULE")
		{
			SWordFilterRule* rule = new SWordFilterRule();
			++i;
			while (i != f.end() && (*i) != "RULEEND")
			{
				CString word2 = *i;
				std::vector<CString> wordParts2 = word2.tokenize();
				if (wordParts2.size() == 0)
				{
					++i;
					continue;
				}

				if (wordParts2[0] == "CHECK")
				{
					for (std::vector<CString>::size_type j = 1; j < wordParts2.size(); ++j)
					{
						if (wordParts2[j] == "chat") rule->check |= FILTER_CHECK_CHAT;
						else if (wordParts2[j] == "pm") rule->check |= FILTER_CHECK_PM;
						else if (wordParts2[j] == "nick") rule->check |= FILTER_CHECK_NICK;
						else if (wordParts2[j] == "toall") rule->check |= FILTER_CHECK_TOALL;
					}
				}
				else if (wordParts2[0] == "MATCH")
				{
					if (wordParts2.size() == 2)
						rule->match = wordParts2[1];
				}
				else if (wordParts2[0] == "PRECISION")
				{
					if (wordParts2.size() == 2)
					{
						if (wordParts2[1].find("%") != -1)
						{
							rule->precisionPercentage = true;
							wordParts2[1].removeAll("%");
						}
						else rule->precisionPercentage = false;
						rule->precision = strtoint(wordParts2[1]);
					}
				}
				else if (wordParts2[0] == "WORDPOSITION")
				{
					for (std::vector<CString>::size_type j = 1; j < wordParts2.size(); ++j)
					{
						if (wordParts2[j] == "full") rule->wordPosition |= FILTER_POSITION_FULL;
						else if (wordParts2[j] == "start") rule->wordPosition |= FILTER_POSITION_START;
						else if (wordParts2[j] == "part") rule->wordPosition |= FILTER_POSITION_PART;
					}
				}
				else if (wordParts2[0] == "ACTION")
				{
					for (std::vector<CString>::size_type j = 1; j < wordParts2.size(); ++j)
					{
						if (wordParts2[j] == "log") rule->action |= FILTER_ACTION_LOG;
						else if (wordParts2[j] == "tellrc") rule->action |= FILTER_ACTION_TELLRC;
						else if (wordParts2[j] == "replace") rule->action |= FILTER_ACTION_REPLACE;
						else if (wordParts2[j] == "warn") rule->action |= FILTER_ACTION_WARN;
						else if (wordParts2[j] == "jail") rule->action |= FILTER_ACTION_JAIL;
						else if (wordParts2[j] == "ban") rule->action |= FILTER_ACTION_BAN;
					}
				}
				else if (wordParts2[0] == "WARNMESSAGE")
				{
					rule->warnMessage = word2.remove(0, 12).trim();
				}

				++i;
			}

			// Make sure we have a valid rule.
			if (rule->check == 0 || rule->action == 0 || rule->wordPosition == 0)
			{
				delete rule;
				continue;
			}

			// Add the rule to the list.
			rules.push_back(rule);
		}
		else if (wordParts[0] == "WARNMESSAGE")
		{
			defaultWarnMessage = word.remove(0, 12).trim();
		}
		else if (wordParts[0] == "SHOWWORDSTORC")
		{
			if (wordParts.size() == 2 && wordParts[1] == "true")
				showWordsToRC = true;
		}
	}
}

int CWordFilter::apply(const TPlayer* player, CString& chat, int check)
{
	if (chat.isEmpty() || rules.size() == 0 || check == 0) return 0;

	CString out = chat;
	CString warnmessage;
	std::vector<CString> chatWords = chat.tokenize();
	std::vector<CString> wordsFound;
	int actionsFound = 0;

	for (std::vector<SWordFilterRule*>::iterator i = rules.begin(); i != rules.end(); ++i)
	{
		SWordFilterRule* rule = *i;

		// Check if we should use this rule.
		if ((check & rule->check) == 0) continue;

		// Start and full will check whole words.
		if (rule->wordPosition != FILTER_POSITION_PART)
		{
			// Loop through each word of the chat.
			for (std::vector<CString>::iterator j = chatWords.begin(); j != chatWords.end(); ++j)
			{
				CString* word = &(*j);

				// If we are checking for a full word and the words aren't the same length, go to the next word.
				if (rule->wordPosition == FILTER_POSITION_FULL && word->length() != rule->match.length()) continue;

				// See if it matches the rule.
				int wordsMatched = 0;
				bool failed = false;
				for (int chatpos = 0; chatpos < rule->match.length() && chatpos < word->length(); ++chatpos)
				{
					char letter = rule->match[chatpos];
					char wordletter = (*word)[chatpos];
					if (letter == '?')
					{
						wordsMatched++;
						continue;
					}
					if (isLower(letter) && letter == toLower(wordletter))
						wordsMatched++;
					else if (isUpper(letter))
					{
						if (toLower(letter) == toLower(wordletter)) wordsMatched++;
						else
						{
							failed = true;
							break;
						}
					}
				}
				if (failed) continue;

				// Check and see if we hit the limit.
				if (rule->precisionPercentage == false && wordsMatched < rule->precision) continue;
				if (rule->precisionPercentage == true && rule->precision > (int)(((float)wordsMatched / (float)rule->match.length()) * 100)) continue;

				// Add the word to the list of words found.
				wordsFound.push_back(*word);
				//wordsFound.push_back(rule->match);

				// Add the rule's actions to the total list of actions to take.
				actionsFound |= rule->action;

				// If it is a warning rule, we are altering the message.
				// We can abort and return it.
				if (rule->action & FILTER_ACTION_WARN)
				{
					warnmessage = rule->warnMessage;
					goto WordFilterActions;
				}

				// Censor the word.
				if (rule->action & FILTER_ACTION_REPLACE)
				{
					// Assemble the censor string.
					CString censor;
					for (int p = 0; p < word->length(); ++p) censor << "*";

					// Censor the output.
					out.replaceAllI(*word, censor);
				}
			}
		}
		else if (rule->wordPosition == FILTER_POSITION_PART)
		{
			for (int wordpos = 0; wordpos < chat.length(); ++wordpos)
			{
				int wordStart = wordpos;

				// See if it matches the rule.
				int wordsMatched = 0;
				bool failed = false;
				CString word;
				for (int chatpos = 0; chatpos < rule->match.length() && wordpos + chatpos < chat.length(); ++chatpos)
				{
					// Don't start on an empty space.
					if (wordpos + chatpos == wordStart)
					{
						bool found = false;
						for (int b = 0; b < sizeof(bypass); ++b)
						{
							if (chat[wordpos + chatpos] == bypass[b])
							{
								failed = true;
								found = true;
								break;
							}
						}
						if (found) break;
					}
					
					// Don't count empty spaces.
					while (true)
					{
						bool found = false;
						for (int b = 0; b < sizeof(bypass); ++b)
						{
							if (chat[wordpos + chatpos] == bypass[b])
							{
								found = true;
								word << bypass[b];
								++wordpos;
							}
						}
						if (!found) break;
					}

					// Check letter for match.
					char letter = rule->match[chatpos];
					char wordletter = chat[wordpos + chatpos];
					if (letter == '?')
					{
						word << wordletter;
						wordsMatched++;
						continue;
					}
					if (isLower(letter) && letter == toLower(wordletter))
						wordsMatched++;
					else if (isUpper(letter))
					{
						if (toLower(letter) == toLower(wordletter)) wordsMatched++;
						else
						{
							failed = true;
							break;
						}
					}
					word << wordletter;
				}
				wordpos = wordStart;
				if (failed) continue;

				// Check and see if we hit the limit.
				if (rule->precisionPercentage == false && wordsMatched < rule->precision) continue;
				if (rule->precisionPercentage == true && rule->precision > (int)(((float)wordsMatched / (float)rule->match.length()) * 100)) continue;

				// Trim the word.
				word.trimI();

				// Add the word to the list of words found.
				wordsFound.push_back(word);
				//wordsFound.push_back(rule->match);

				// Add the rule's actions to the total list of actions to take.
				actionsFound |= rule->action;

				// If it is a warning rule, we are altering the message.
				// We can abort and return it.
				if (rule->action & FILTER_ACTION_WARN)
				{
					warnmessage = rule->warnMessage;
					goto WordFilterActions;
				}

				// Censor the word.
				if (rule->action & FILTER_ACTION_REPLACE)
				{
					// Assemble the censor string.
					CString censor;
					for (int p = 0; p < word.length(); ++p) censor << "*";

					// Censor the output.
					out.replaceAllI(word, censor);
				}
			}
		}
	}

WordFilterActions:

	// If no words were found, exit now.
	if (wordsFound.size() == 0) return 0;

	// Assemble a list of the bad words.
	CString badwords;
	for (std::vector<CString>::iterator i = wordsFound.begin(); i != wordsFound.end(); ++i)
		badwords << *i << ", ";
	badwords.removeI(badwords.length() - 2);

	// Apply an action based on the word.
	if (actionsFound & FILTER_ACTION_LOG)
	{
		CLog wordfilter;
		wordfilter.setFilename(CString() << server->getServerPath() << "logs/serverlog.txt");
		wordfilter.setEnabled(true);
		wordfilter.out("[Word Filter] Player %s was caught using these words: %s\n", player->getAccountName().text(), badwords.text());
	}

	// Graal doesn't implement.  Should we?
	if (actionsFound & FILTER_ACTION_JAIL)
	{
	}

	// Graal doesn't implement.  Should we?
	if (actionsFound & FILTER_ACTION_BAN)
	{
	}

	// Tell RC what happened.
	if (showWordsToRC || actionsFound & FILTER_ACTION_TELLRC)
	{
		server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "Word Filter: Player " << player->getAccountName() << " was caught using these words: " << badwords);
	}

	// If it is a warning rule, we are altering the message.
	// If not, set the message to the filtered message.
	if (actionsFound & FILTER_ACTION_WARN)
	{
		if (warnmessage.isEmpty()) chat = defaultWarnMessage;
		else chat = warnmessage;
	}
	else chat = out;

	return actionsFound;
}
