#ifndef WEAPON_H
#define WEAPON_H

#include <memory>
#include <time.h>
#include <vector>
#include <string>

#include <CString.h>
#include <BabyDI.h>

#include <level/LevelItem.h>
#include <scripting/Script.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Server;
class Weapon
{
public:
	// -- Constructor | Destructor -- //
	Weapon(LevelItemType itemType);
	Weapon(std::string pName, std::string pImage, std::string pScript, const time_t pModTime = 0, bool pSaveWeapon = false);
	~Weapon();

	// -- Functions -- //
	bool saveWeapon();
	void updateWeapon(std::string pImage, std::string pScript, const time_t pModTime = 0, bool pSaveWeapon = true);

	static std::shared_ptr<Weapon> loadWeapon(const CString& pWeapon);

	// Functions -> Inline Get-Functions
	CString getWeaponPacket(int clientVersion) const;
	bool isDefault() const { return (m_weaponDefault != LevelItemType::INVALID); }
	LevelItemType getWeaponId() const { return m_weaponDefault; }
	const Script& getSource() const { return m_source; }
	const std::string& getByteCodeFile() const { return m_bytecodeFile; }
	const std::string& getImage() const { return m_weaponImage; }
	const std::string& getName() const { return m_weaponName; }
	time_t getModTime() const { return m_modTime; }

	// Functions -> Set Variables
	void setModTime(time_t pModTime) { m_modTime = pModTime; }

protected:
	BabyDI_INJECT(Server, m_server);

	// Varaibles -> Weapon Data
	LevelItemType m_weaponDefault;
	time_t m_modTime;

	Script m_source;
	std::string m_bytecodeFile;

	std::string m_weaponImage;
	std::string m_weaponName;
	std::vector<std::string> m_joinedClasses;
};
using TWeaponPtr = std::shared_ptr<Weapon>;

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // WEAPON_H
