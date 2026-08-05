#ifndef WEAPON_H
#define WEAPON_H

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <ranges>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

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
	explicit Weapon(const LevelItemType itemType);
	Weapon(const std::string_view name, const std::string_view image, const std::string_view script);
	~Weapon() = default;

public:
	static std::shared_ptr<Weapon> loadWeapon(const std::filesystem::path& fileName);

public:
	bool saveWeapon();
	Weapon& updateWeapon(const std::string_view newImage, const std::string_view newScript);

public:
	void registerWeaponWithPlayer(const std::shared_ptr<Player>& player) const;
	void sendByteCodeToPlayer(const std::shared_ptr<Player>& player) const;

public:
	std::string getJoinedClassesList() const;
	[[a::inline]] std::generator<std::shared_ptr<ScriptClass>> getJoinedClasses() const;
	void setJoinedClasses(std::string_view classes);
	void joinClass(const std::string_view className);
	void leaveClass(const std::string_view className);

protected:
	std::string getClientSideScript() const;
	void calculateHeaderChecksum();
	void updateScriptClass(ScriptClass* scriptClass);

public:
	void executeEvents(ScriptEventQueue& events, const ScriptObject& source) const;

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
	uint32_t m_checksum = 0;
	std::string m_desKey;
	std::string m_header;
	std::string m_headerWithCRC;

	mutable std::vector<std::pair<EventHandle, std::weak_ptr<ScriptClass>>> m_joinedClasses;
};
using TWeaponPtr = std::shared_ptr<Weapon>;

//----------------------------

inline std::generator<std::shared_ptr<ScriptClass>> Weapon::getJoinedClasses() const
{
	// ReSharper disable once CppLocalVariableMayBeConst
	auto filter = m_joinedClasses
		| std::views::transform([](const auto& pair) { return pair.second.lock(); })
		| std::views::filter([](const auto& scriptClass) { return scriptClass != nullptr; });
	for (const auto& scriptClass : filter)
		co_yield scriptClass;
}

//----------------------------

namespace source
{
/// @brief Creates a ScriptObject from a Weapon by hashing the weapon's name.
ScriptObject FromWeapon(const WeaponPtr& weapon);
} // end namespace source

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // WEAPON_H
