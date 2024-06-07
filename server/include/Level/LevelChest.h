#ifndef TLEVELCHEST_H
#define TLEVELCHEST_H

#include "CString.h"
#include <memory>
#include <vector>

#ifdef V8NPCSERVER
	#include "ScriptBindings.h"
#endif

enum class LevelItemType;

class LevelChest : public std::enable_shared_from_this<LevelChest>
{
public:
	LevelChest(char nx, char ny, LevelItemType itemIdx, char signIdx)
		: itemIndex(itemIdx), signIndex(signIdx), x(nx), y(ny)
	{
	}

	LevelItemType getItemIndex() const
	{
		return itemIndex;
	}

	int getSignIndex() const
	{
		return signIndex;
	}

	int getX() const
	{
		return x;
	}

	int getY() const
	{
		return y;
	}

	void setItemIndex(int id)
	{
		itemIndex = (LevelItemType)id;
	}

	void setSignIndex(int id)
	{
		signIndex = id;
	}

	void setX(int xVal = 0)
	{
		x = xVal;
	}

	void setY(int yVal = 0)
	{
		y = yVal;
	}

#ifdef V8NPCSERVER
	inline IScriptObject<LevelChest>* getScriptObject() const
	{
		return _scriptObject.get();
	}

	inline void setScriptObject(std::unique_ptr<IScriptObject<LevelChest>> object)
	{
		_scriptObject = std::move(object);
	}
#endif

private:
	LevelItemType itemIndex;
	int signIndex, x, y;

#ifdef V8NPCSERVER
	std::unique_ptr<IScriptObject<LevelChest>> _scriptObject;
#endif
};

using TLevelChestPtr = std::shared_ptr<LevelChest>;

#endif // TLEVELCHEST_H
