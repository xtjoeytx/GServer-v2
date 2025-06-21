#ifndef WEAPON_H
#define WEAPON_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <string>
#include <vector>

#include <CString.h>

#include <level/LevelItem.h>
#include <scripting/Script.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

// TODO: Weapon should probably just be inherited from NPC.
class Server;
class Weapon
{
public:
	Weapon(LevelItemType itemType) : name(LevelItem::getItemName(itemType)), modTime(clock::now()), m_weaponDefault(itemType), m_checksum(0) {}
	Weapon(std::string_view name, std::string_view image, std::string_view script);
	~Weapon() = default;

private:
	Weapon() = default;

public:
	static std::shared_ptr<Weapon> loadWeapon(const CString& pWeapon);

public:
	bool saveWeapon();
	Weapon& updateWeapon(std::string_view image, std::string_view script);

public:
	CString getAddWeaponPacket() const;
	CString getWeaponByteCodePacket() const;

public:
	std::string getJoinedClasses() const;
	void setJoinedClasses(std::string_view classes);
	void joinClass(std::string_view className);
	void leaveClass(std::string_view className);

public:
	void executeEvents(ScriptEventQueue& events, ScriptObjectSource source) const;

public:
	bool isDefault() const { return (m_weaponDefault != LevelItemType::INVALID); }
	LevelItemType getWeaponId() const { return m_weaponDefault; }
	const Script& getScript() const { return m_script; }

public:
	const std::string name;
	std::string image;
	clock::time_point modTime;
	ScriptContainer scripting;

protected:
	LevelItemType m_weaponDefault;
	Script m_script;
	uint32_t m_checksum;
	std::string m_desKey;
	std::string m_header;

	std::vector<std::weak_ptr<ScriptClass>> m_joinedClasses;
};
using TWeaponPtr = std::shared_ptr<Weapon>;

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // WEAPON_H
