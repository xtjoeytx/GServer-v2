#ifndef TLEVELSIGN_H
#define TLEVELSIGN_H

#include "CString.h"
#include <memory>
#include <vector>

#ifdef V8NPCSERVER

	#include "ScriptBindings.h"

#endif

class Player;

class LevelSign : public std::enable_shared_from_this<LevelSign>
{
public:
	LevelSign(const int pX, const int pY, const CString& pSign, bool encoded = false);

	// functions
	CString getSignStr(Player* pPlayer = 0) const;

	// get private variables
	int getX() const { return m_x; }

	int getY() const { return m_y; }

	CString getText() const { return m_text; }

	CString getUText() const { return m_unformattedText; }

	void setX(int value = 0) { m_x = value; }

	void setY(int value = 0) { m_y = value; }

	void setText(const CString& value);

	void setUText(const CString& value);

#ifdef V8NPCSERVER

	inline IScriptObject<LevelSign>* getScriptObject() const
	{
		return m_scriptObject.get();
	}

	inline void setScriptObject(std::unique_ptr<IScriptObject<LevelSign>> object)
	{
		m_scriptObject = std::move(object);
	}

#endif

private:
	int m_x, m_y;
	CString m_text;
	CString m_unformattedText;

#ifdef V8NPCSERVER
	std::unique_ptr<IScriptObject<LevelSign>> m_scriptObject;
#endif
};

using LevelSignPtr = std::shared_ptr<LevelSign>;

#endif // TLEVELSIGN_H
