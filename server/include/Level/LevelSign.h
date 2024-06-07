#ifndef TLEVELSIGN_H
#define TLEVELSIGN_H

#include "CString.h"
#include <memory>
#include <vector>

#ifdef V8NPCSERVER
	#include "ScriptBindings.h"
#endif

class TPlayer;

class TLevelSign : public std::enable_shared_from_this<TLevelSign>
{
public:
	TLevelSign(const int pX, const int pY, const CString& pSign, bool encoded = false);

	// functions
	CString getSignStr(TPlayer* pPlayer = 0) const;

	// get private variables
	int getX() const { return x; }
	int getY() const { return y; }
	CString getText() const { return text; }
	CString getUText() const { return unformattedText; }

	void setX(int value = 0) { x = value; }
	void setY(int value = 0) { y = value; }
	void setText(const CString& value);
	void setUText(const CString& value);

#ifdef V8NPCSERVER
	inline IScriptObject<TLevelSign>* getScriptObject() const
	{
		return _scriptObject.get();
	}

	inline void setScriptObject(std::unique_ptr<IScriptObject<TLevelSign>> object)
	{
		_scriptObject = std::move(object);
	}
#endif

private:
	int x, y;
	CString text;
	CString unformattedText;

#ifdef V8NPCSERVER
	std::unique_ptr<IScriptObject<TLevelSign>> _scriptObject;
#endif
};

using TLevelSignPtr = std::shared_ptr<TLevelSign>;

#endif // TLEVELSIGN_H
