#ifndef TLEVELHORSE_H
#define TLEVELHORSE_H

#include <CString.h>
#include <CTimeout.h>

class Server;
class LevelHorse
{
public:
	LevelHorse(int horselife, const CString& pImage, float pX, float pY, char pDir = 0, char pBushes = 0)
		: m_lifetime(horselife), m_image(pImage), m_x(pX), m_y(pY), m_dir(pDir), m_bushes(pBushes)
	{
		timeout.setTimeout(m_lifetime);
	}

	CString getHorseStr();

	// get private variables
	CString getImage() const { return m_image; }
	float getX() const { return m_x; }
	float getY() const { return m_y; }
	char getDir() const { return m_dir; }
	char getBushes() const { return m_bushes; }

	CTimeout timeout;

private:
	CString m_image;
	CString m_horsePacket;
	float m_x, m_y;
	char m_dir, m_bushes;
	int m_lifetime;
};

inline CString LevelHorse::getHorseStr()
{
	if (m_horsePacket.isEmpty())
	{
		char dir_bush = (m_bushes << 2) | (m_dir & 0x03);
		m_horsePacket = CString() << (char)(m_x * 2) >> (char)(m_y * 2) >> (char)dir_bush << m_image;
	}

	return m_horsePacket;
}

#endif // TLEVELHORSE_H
