#ifndef TLEVELLINK_H
#define TLEVELLINK_H

#include <vector>
#include <memory>
#include "CString.h"

#ifdef V8NPCSERVER
#include "ScriptBindings.h"
#endif

class TLevelLink : public std::enable_shared_from_this<TLevelLink>
{
	public:
		// constructor - destructor
		TLevelLink() : x(0), y(0), width(0), height(0) { }
		TLevelLink(const std::vector<CString>& pLink);

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

#ifdef V8NPCSERVER
		inline IScriptObject<TLevelLink> * getScriptObject() const {
			return _scriptObject.get();
		}

		inline void setScriptObject(std::unique_ptr<IScriptObject<TLevelLink>> object) {
			_scriptObject = std::move(object);
		}
#endif

	private:
		CString newLevel, newX, newY;
		int x, y, width, height;

#ifdef V8NPCSERVER
		std::unique_ptr<IScriptObject<TLevelLink>> _scriptObject;
#endif
};

using TLevelLinkPtr = std::shared_ptr<TLevelLink>;

/*
	TLevelLink: Get Private Variables
*/
inline CString TLevelLink::getNewLevel() const
{
	return newLevel;
}

inline CString TLevelLink::getNewX() const
{
	return newX;
}

inline CString TLevelLink::getNewY() const
{
	return newY;
}

inline int TLevelLink::getX() const
{
	return x;
}

inline int TLevelLink::getY() const
{
	return y;
}

inline int TLevelLink::getWidth() const
{
	return width;
}

inline int TLevelLink::getHeight() const
{
	return height;
}

/*
	TLevelLink: Set Private Variables
*/
inline void TLevelLink::setNewLevel(const CString& _newLevel) {
	newLevel = _newLevel;
}

inline void TLevelLink::setNewX(const CString& _newX) {
	newX = _newX;
}

inline void TLevelLink::setNewY(const CString& _newY) {
	newY = _newY;
}

inline void TLevelLink::setX(int posX) {
	x = posX;
}

inline void TLevelLink::setY(int posY) {
	y = posY;
}

inline void TLevelLink::setWidth(int _width) {
	width = _width;
}

inline void TLevelLink::setHeight(int _height) {
	height = _height;
}

#endif // TLEVELLINK_H
