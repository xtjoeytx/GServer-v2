#ifndef TLEVELSIGN_H
#define TLEVELSIGN_H

#include <memory>
#include <vector>

#include <CString.h>

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

private:
	int m_x, m_y;
	CString m_text;
	CString m_unformattedText;
};

using LevelSignPtr = std::shared_ptr<LevelSign>;

#endif // TLEVELSIGN_H
