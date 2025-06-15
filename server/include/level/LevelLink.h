#ifndef LEVELLINK_H
#define LEVELLINK_H

#include <memory>
#include <vector>

#include <CString.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class LevelLink : public std::enable_shared_from_this<LevelLink>
{
public:
	// constructor - destructor
	LevelLink() = default;
	LevelLink(const std::vector<CString>& pLink);

	// functions
	CString getLinkStr() const;
	void parseLinkStr(const std::vector<CString>& pLink);

	// get private variables
	inline CString getNewLevel() const;
	inline CString getNewX() const;
	inline CString getNewY() const;
	inline int getX() const;
	inline int getY() const;
	inline int getWidth() const;
	inline int getHeight() const;

	// set private variables
	inline void setNewLevel(const CString& _newLevel);
	inline void setNewX(const CString& _newX);
	inline void setNewY(const CString& _newY);
	inline void setX(int posX = 0);
	inline void setY(int posY = 0);
	inline void setWidth(int _width = 0);
	inline void setHeight(int _height = 0);

private:
	CString m_newLevel, m_newX, m_newY;
	int m_x = 0;
	int m_y = 0;
	int m_width = 0;
	int m_height = 0;
	bool m_constantX = false;
	bool m_constantY = false;
};

using LevelLinkPtr = std::shared_ptr<LevelLink>;

/*
	LevelLink: Get Private Variables
*/
inline CString LevelLink::getNewLevel() const
{
	return m_newLevel;
}

inline CString LevelLink::getNewX() const
{
	return m_newX;
}

inline CString LevelLink::getNewY() const
{
	return m_newY;
}

inline int LevelLink::getX() const
{
	return m_x;
}

inline int LevelLink::getY() const
{
	return m_y;
}

inline int LevelLink::getWidth() const
{
	return m_width;
}

inline int LevelLink::getHeight() const
{
	return m_height;
}

/*
	LevelLink: Set Private Variables
*/
inline void LevelLink::setNewLevel(const CString& _newLevel)
{
	m_newLevel = _newLevel;
}

inline void LevelLink::setNewX(const CString& _newX)
{
	m_newX = _newX;
}

inline void LevelLink::setNewY(const CString& _newY)
{
	m_newY = _newY;
}

inline void LevelLink::setX(int posX)
{
	m_x = posX;
}

inline void LevelLink::setY(int posY)
{
	m_y = posY;
}

inline void LevelLink::setWidth(int _width)
{
	m_width = _width;
}

inline void LevelLink::setHeight(int _height)
{
	m_height = _height;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELLINK_H
