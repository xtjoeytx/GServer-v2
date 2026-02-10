#ifndef WEAPON_H
#define WEAPON_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <ranges>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <CString.h>

#include <level/LevelItem.h>
#include <scripting/Script.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Events.h>
#include <utilities/std/generator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

// TODO: Weapon should probably just be inherited from NPC.
class Server;
class Player;

class Weapon
{
public:
	Weapon(LevelItemType itemType);
	Weapon(std::string_view name, std::string_view image, std::string_view script);
	~Weapon() = default;

public:
	static std::shared_ptr<Weapon> loadWeapon(const CString& pWeapon);

public:
	bool saveWeapon();
	Weapon& updateWeapon(std::string_view image, std::string_view script);

public:
	void registerWeaponWithPlayer(std::shared_ptr<Player> player) const;
	void sendByteCodeToPlayer(std::shared_ptr<Player> player) const;

public:
	std::string getJoinedClassesList() const;
	[[inline]] std::generator<std::shared_ptr<ScriptClass>> getJoinedClasses();
	void setJoinedClasses(std::string_view classes);
	void joinClass(std::string_view className);
	void leaveClass(std::string_view className);

protected:
	std::string getClientSideScript() const;
	void updateScriptClass(ScriptClass* scriptClass);

public:
	void executeEvents(ScriptEventQueue& events, ScriptObject source) const;

public:
	bool isDefault() const { return (m_weaponDefault != LevelItemType::INVALID); }
	LevelItemType getWeaponId() const { return m_weaponDefault; }
	Script& getScript() { return m_script; }
	const Script& getScript() const { return m_script; }

public:
	const std::string name;
	std::string image;
	clock::time_point modTime;
	ScriptContainer scripting;

protected:
	Server* m_server;
	LevelItemType m_weaponDefault;
	Script m_script;
	uint32_t m_checksum;
	std::string m_desKey;
	std::string m_header;
	std::string m_headerWithCRC;

	mutable std::vector<std::pair<EventHandle, std::weak_ptr<ScriptClass>>> m_joinedClasses;
};
using TWeaponPtr = std::shared_ptr<Weapon>;

//----------------------------

inline std::generator<std::shared_ptr<ScriptClass>> Weapon::getJoinedClasses()
{
	auto filter = m_joinedClasses
		| std::views::transform([](const auto& pair) { return pair.second.lock(); })
		| std::views::filter([](const auto& scriptClass) { return scriptClass != nullptr; });
	for (auto scriptClass : filter)
		co_yield scriptClass;
}

//----------------------------

namespace source
{
/// @brief Creates a ScriptObject from a Weapon by hashing the weapon's name.
ScriptObject FromWeapon(WeaponPtr weapon);
} // end namespace source

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // WEAPON_H
