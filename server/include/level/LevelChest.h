#ifndef TLEVELCHEST_H
#define TLEVELCHEST_H

#include <memory>
#include <vector>

#include <CString.h>

enum class LevelItemType;

class LevelChest : public std::enable_shared_from_this<LevelChest>
{
public:
	LevelChest(char nx, char ny, LevelItemType itemIdx, char signIdx)
		: m_itemIndex(itemIdx), m_signIndex(signIdx), m_x(nx), m_y(ny)
	{
	}

	LevelItemType getItemIndex() const
	{
		return m_itemIndex;
	}

	int getSignIndex() const
	{
		return m_signIndex;
	}

	int getX() const
	{
		return m_x;
	}

	int getY() const
	{
		return m_y;
	}

	void setItemIndex(int id)
	{
		m_itemIndex = (LevelItemType)id;
	}

	void setSignIndex(int id)
	{
		m_signIndex = id;
	}

	void setX(int xVal = 0)
	{
		m_x = xVal;
	}

	void setY(int yVal = 0)
	{
		m_y = yVal;
	}

private:
	LevelItemType m_itemIndex;
	int m_signIndex, m_x, m_y;
};

using LevelChestPtr = std::shared_ptr<LevelChest>;

#endif // TLEVELCHEST_H
