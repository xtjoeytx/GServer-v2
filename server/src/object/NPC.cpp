#include "object/NPC.h"

#include <math.h>
#include <IEnums.h>

#include "FileSystem.h"
#include "Server.h"
#include "level/Level.h"
#include "level/Map.h"
#include "npcserver/NPCServer.h"
#include <scripting/ScriptContainers.h>
#include "scripting/SourceCode.h"
#include "utilities/Log.h"

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

static constexpr std::array<uint8_t, 10> savePackets = { 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };

static std::string_view toWeaponName(std::string_view code);
static std::string performClientSideJoinHack(std::string_view code, FileSystem* fs);

template<typename T>
concept ValidGameValueCallable = requires(T t)
{
	{ t() } -> std::convertible_to<GameValue>;
};

template <typename T>
void copyToArrayAs(const auto& vec, auto& propvalue)
{
	size_t count = std::min(vec.size(), propvalue.size());
	auto it = std::begin(vec);
	for (size_t i = 0; i < count; ++i, ++it)
	{
		propvalue[i] = static_cast<T>(*it);
	}
}

static GameVariable::func_get prop_get(ValidGameValueCallable auto getter)
{
	return [getter](std::string_view identifier) -> GameValue
	{
		return GameValue{ getter() };
	};
}

static GameVariable::func_get prop_get(auto& value)
{
	using V = std::remove_cvref_t<decltype(value)>;
	static_assert(std::integral<V> || std::floating_point<V> || string::StringVariant<V> || std::ranges::forward_range<V>,
				  "NPC prop_get called with an unsupported type. Supported types are integral, floats, string, or ranges.");

	if constexpr (std::integral<V> || std::floating_point<V>)
	{
		return [&value](std::string_view identifier) -> GameValue
		{
			return GameValue{ static_cast<double>(value) };
		};
	}
	else if constexpr (string::StringVariant<V>)
	{
		return [&value](std::string_view identifier) -> GameValue
		{
			return GameValue{ std::string{ value } };
		};
	}
	else if constexpr (std::ranges::forward_range<V>)
	{
		return [&value](std::string_view identifier) -> GameValue
		{
			return GameValue{ value | std::views::transform([](const auto& v) { return static_cast<double>(v); }) | std::ranges::to<std::vector<double>>() };
		};
	}

	throw std::invalid_argument("NPC prop_get called with an unsupported type.");
}

static GameVariable::func_set prop_set(NPC* who, std::optional<NPCProp> prop, std::function<void(const GameValue&, std::optional<size_t>)> setter)
{
	return [who, prop, setter](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
	{
		setter(value, index);
		if (prop.has_value() && who != nullptr)
		{
			if (prop.value() != NPCProp::SAVE0)
				who->modTime[PROPID(prop.value())] = currentTimeInSeconds();
			else if (index.has_value() && index.value() <= 9)
			{
				auto propIndex = PROPID(NPCProp::SAVE0) + index.value();
				who->modTime[propIndex] = currentTimeInSeconds();
			}
		}
	};
}

static GameVariable::func_set prop_set(NPC* who, std::optional<NPCProp> prop, auto& propvalue)
{
	using V = std::remove_cvref_t<decltype(propvalue)>;
	static_assert(std::integral<V> || std::floating_point<V> || string::StringVariant<V> || std::ranges::random_access_range<V>,
		"NPC prop_get called with an unsupported type. Supported types are integral, floats, string, or ranges.");

	if constexpr (std::integral<V> || std::floating_point<V>)
	{
		return [who, prop, &propvalue](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
		{
			propvalue = static_cast<V>(value.get<double>().value_or(0.0));
			if (prop.has_value())
				who->modTime[PROPID(prop.value())] = currentTimeInSeconds();
		};
	}
	else if constexpr (string::StringVariant<V>)
	{
		return [who, prop, &propvalue](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
		{
			propvalue = value.get<std::string>().value_or({});
			if (prop.has_value())
				who->modTime[PROPID(prop.value())] = currentTimeInSeconds();
		};
	}
	else if constexpr (std::ranges::random_access_range<V>)
	{
		return [who, prop, &propvalue](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
		{
			size_t propvalue_size = std::ranges::size(propvalue);
			if (propvalue_size > 0)
			{
				using value_type = std::remove_cvref_t<decltype(propvalue[0])>;
				if (index.has_value() && index.value() < propvalue_size)
				{
					propvalue[index.value()] = static_cast<value_type>(value.get<double>().value_or(0.0));
					if (prop.has_value())
						who->modTime[PROPID(prop.value()) + index.value()] = currentTimeInSeconds();
				}
				else if (!index.has_value())
				{
					auto vec = value.get<std::vector<double>>().value();
					copyToArrayAs<value_type>(vec, propvalue);
					if (prop.has_value())
						who->modTime[PROPID(prop.value())] = currentTimeInSeconds();
				}
			}
		};
	}

	throw std::invalid_argument("NPC prop_set called with an unsupported type.");
}

//----------------------------

NPC::NPC(NPCID id, NPCType type)
	: id(id), type(type), m_savedModTime()
{
	saves.fill(0);
	modTime.fill(0);

	// We need to alter the modTime of the following props as they should be always sent.
	// If we don't, they won't be sent until the prop gets modified.
	auto props = std::to_array({ NPCProp::IMAGE, NPCProp::SCRIPT, NPCProp::X, NPCProp::Y, NPCProp::VISFLAGS, NPCProp::ID, NPCProp::SPRITE, NPCProp::MESSAGE, NPCProp::GMAPLEVELX, NPCProp::GMAPLEVELY, NPCProp::X2, NPCProp::Y2 });
	std::ranges::for_each(props, [this, now = currentTimeInSeconds()](const NPCProp& prop) { modTime[PROPID(prop)] = now; });

	m_savedModTime = modTime;

	// Create variable store links.
	scripting.variables.add(GameVariable{ "id", prop_get([this]() { return static_cast<double>(this->id); }), {} });
	scripting.variables.add(GameVariable{ "width", prop_get([this]() { return static_cast<double>(imageSize.width()); }), {} });
	scripting.variables.add(GameVariable{ "height", prop_get([this]() { return static_cast<double>(imageSize.height()); }), {} });
	scripting.variables.add(GameVariable{ "rupees", prop_get(character.gralats), prop_set(this, NPCProp::RUPEES, character.gralats) });
	scripting.variables.add(GameVariable{ "gralats", prop_get(character.gralats), prop_set(this, NPCProp::RUPEES, character.gralats) });
	scripting.variables.add(GameVariable{ "bombs", prop_get(character.bombs), prop_set(this, NPCProp::BOMBS, character.bombs) });
	scripting.variables.add(GameVariable{ "darts", prop_get(character.arrows), prop_set(this, NPCProp::ARROWS, character.arrows) });
	scripting.variables.add(GameVariable{ "glovepower", prop_get(character.glovePower), prop_set(this, NPCProp::GLOVEPOWER, character.glovePower) });
	scripting.variables.add(GameVariable{ "swordpower", prop_get(character.swordPower), prop_set(this, NPCProp::SWORDIMAGE, character.swordPower) });
	scripting.variables.add(GameVariable{ "shieldpower", prop_get(character.shieldPower), prop_set(this, NPCProp::SHIELDIMAGE, character.shieldPower) });
	scripting.variables.add(GameVariable{ "sprite", prop_get(character.sprite), prop_set(this, NPCProp::SPRITE, character.sprite) });
	scripting.variables.add(GameVariable{ "ap", prop_get(character.ap), prop_set(this, NPCProp::ALIGNMENT, character.ap) });
	scripting.variables.add(GameVariable{ "hurtdx", prop_get(hurtX), prop_set(this, NPCProp::HURTDXDY, hurtX) });
	scripting.variables.add(GameVariable{ "hurtdy", prop_get(hurtY), prop_set(this, NPCProp::HURTDXDY, hurtY) });
	scripting.variables.add(GameVariable{ "save", prop_get(saves), prop_set(this, NPCProp::SAVE0, saves) });
	scripting.variables.add(GameVariable{ "x",
		prop_get([this]() { return character.pixelX / 16.0; }),
		prop_set(this, NPCProp::X2, [this](const GameValue& value, std::optional<size_t>) { character.pixelX = value.get<double>().value_or(0.0) * 16; }) });
	scripting.variables.add(GameVariable{ "y",
		prop_get([this]() { return character.pixelY / 16.0; }),
		prop_set(this, NPCProp::Y2, [this](const GameValue& value, std::optional<size_t>) { character.pixelY = value.get<double>().value_or(0.0) * 16; }) });
	scripting.variables.add(GameVariable{ "z",
		prop_get([this]() { return character.pixelZ / 16.0; }),
		prop_set(this, NPCProp::Z2, [this](const GameValue& value, std::optional<size_t>) { character.pixelZ = value.get<double>().value_or(0.0) * 16; }) });
	scripting.variables.add(GameVariable{ "timeout",
		prop_get([this]() { return timeout.count() / 1000.0; }),
		prop_set(this, std::nullopt, [this](const GameValue& value, std::optional<size_t>) { timeout = std::chrono::milliseconds(static_cast<int>(value.get<double>().value_or(0.0) * 1000)); }) });
	scripting.variables.add(GameVariable{ "dir",
		prop_get([this]() { return static_cast<double>(character.sprite % 4); }),
		prop_set(this, NPCProp::SPRITE, [this](const GameValue& value, std::optional<size_t>) { character.sprite = static_cast<uint8_t>(value.get<double>().value_or(0.0)) % 4; }) });
}

//----------------------------

void NPC::setScript(std::string_view script)
{
	m_script = std::move(SourceCode{ script });

	auto clientside = m_script.getClientSide();

	// Check for position update blocking.
	if (m_server->isNpcServerEnabled() || clientside.contains("//#BLOCKPOSITIONUPDATES"))
		m_blockPositionUpdates = true;

	// If there is no npc-server, emulate script joins.
	if (!m_server->isNpcServerEnabled() && m_server->Generation == ServerGeneration::CLASSIC)
	{
		auto joinedScript = performClientSideJoinHack(clientside, m_server->getFileSystem());
		m_script.setModifiedSource(joinedScript);
		clientside = m_script.getClientSide();
	}

	// If we have no npc-server, we support toweapons, so extract the weapon name.
	if (!m_server->isNpcServerEnabled())
	{
		m_weaponName = toWeaponName(clientside);
	}

	// If we have an npc-server, compile the scripts.
	if (m_server->isNpcServerEnabled())
	{
		auto npcServer = m_server->getNpcServer();
		if (m_server->Generation == ServerGeneration::CLASSIC)
		{
			m_script.setServerCompiledScript(npcServer->scripting.getCompiledServerScript(ScriptType::CLASS, name, m_script.getServerSide()));
		}
		else if (m_server->Generation == ServerGeneration::NEWMAIN || m_server->Generation == ServerGeneration::MODERN)
		{
			m_script.setClientCompiledScript(npcServer->scripting.getCompiledClientScript(ScriptType::CLASS, name, m_script.getClientSide()));
			m_script.setServerCompiledScript(npcServer->scripting.getCompiledServerScript(ScriptType::CLASS, name, m_script.getServerSide()));
		}
	}

	// Just a little warning for people who don't know.
	if (m_script.getClientByteCode().empty() && m_script.getClientSide().length() > 0x705F)
		log::printLine(log::server, "WARNING: Clientside script of NPC ({}) exceeds the limit of 28767 bytes.", (image.length() != 0 ? image : std::to_string(id)));
}

//----------------------------

CString NPC::getModifiedPropsPacket(int clientVersion) const
{
	CString result;
	for (auto i = 0; i < NPCPROP_COUNT; ++i)
	{
		if (modTime[i] != m_savedModTime[i])
			result >> (char)i << getPropPacket((NPCProp)i, clientVersion);
	}
	return result;
}

CString NPC::getAllPropsPacket(time_t newTime, int clientVersion) const
{
	bool oldcreated = m_server->getSettings().getBool("oldcreated", "false");
	CString retVal;
	int pmax = NPCPROP_COUNT;
	if (clientVersion < CLVER_2_1) pmax = 36;

	for (int i = 0; i < pmax; i++)
	{
		if (modTime[i] != 0 && modTime[i] >= newTime)
		{
			if (oldcreated && i == PROPID(NPCProp::VISFLAGS) && newTime == 0)
				retVal >> (char)i >> (char)(visFlags | (uint8_t)NPCVisFlags::VISIBLE);
			else
				retVal >> (char)i << getPropPacket((NPCProp)i, clientVersion);
		}
	}
	if (clientVersion > CLVER_1_411)
	{
		if (modTime[PROPID(NPCProp::GANI)] == 0 && image == "#c#")
			retVal >> (char)NPCProp::GANI >> (char)4 << "idle";
	}

	return retVal;
}

//----------------------------

CString NPC::getPropPacket(NPCProp pId, int clientVersion) const
{
	switch (pId)
	{
		case NPCProp::IMAGE:
			return CString() >> (char)image.length() << image;

		case NPCProp::SCRIPT:
		{
			// Modern sends scripts in a different way.
			if (m_server->Generation == ServerGeneration::MODERN)
				return CString() >> (short)0;

			// We have bytecode set so send it.
			if (const auto& client = m_script.getClientByteCode(); !client.empty())
			{
				// TODO: Proper handling of bytecode that is too large.
				assert(client.size() <= 0x705F);
				return CString() >> (short)client.size() << std::string_view{ reinterpret_cast<const char*>(client.data()), client.size() };
			}

			// Fallback to sending the script itself.
			auto clientside = m_script.getClientSide();
			return CString() >> (short)(clientside.length() > 0x705F ? 0x705F : clientside.length()) << clientside.substr(0, 0x705F);
		}

		case NPCProp::X:
			return CString() >> (char)(character.pixelX / 8);

		case NPCProp::Y:
			return CString() >> (char)(character.pixelY / 8);

		case NPCProp::Z:
			// range: -25 to 85
			return CString() >> (char)(std::min(85 * 2, std::max(-25 * 2, (character.pixelZ / 8))) + 50);

		case NPCProp::POWER:
			return CString() >> (char)(character.hitpointsInHalves);

		case NPCProp::RUPEES:
			return CString() >> (int)character.gralats;

		case NPCProp::ARROWS:
			return CString() >> (char)character.arrows;

		case NPCProp::BOMBS:
			return CString() >> (char)character.bombs;

		case NPCProp::GLOVEPOWER:
			return CString() >> (char)character.glovePower;

		case NPCProp::BOMBPOWER:
			return CString() >> (char)character.bombPower;

		case NPCProp::SWORDIMAGE:
			if (character.swordPower == 0)
				return CString() >> (char)0;
			else
				return CString() >> (char)(character.swordPower + 30) >> (char)character.swordImage.length() << character.swordImage;

		case NPCProp::SHIELDIMAGE:
			if (character.shieldPower + 10 > 10)
				return CString() >> (char)(character.shieldPower + 10) >> (char)character.shieldImage.length() << character.shieldImage;
			else
				return CString() >> (char)0;

		case NPCProp::GANI:
			if (clientVersion < CLVER_2_1)
			{
				if (character.bowPower < 10)
					return CString() >> (char)character.bowPower;
				else
					return CString() >> (char)(character.bowImage.length() + 10) << character.bowImage;
			}
			if (isCharacter())
				return CString() >> (char)character.gani.length() << character.gani;
			else return CString() >> (char)0;

		case NPCProp::VISFLAGS:
			return CString() >> (char)visFlags;

		case NPCProp::BLOCKFLAGS:
			return CString() >> (char)blockFlags;

		case NPCProp::MESSAGE:
			return CString() >> (char)character.chatMessage.length() << character.chatMessage;

		case NPCProp::HURTDXDY:
			return CString() >> (char)((hurtX * 32) + 32) >> (char)((hurtY * 32) + 32);

		case NPCProp::ID:
			return CString() >> (int)id;

		// Sprite is deprecated and has been replaced by def.gani.
		// Sprite now holds the direction of the npc.  sprite % 4 gives backwards compatibility.
		case NPCProp::SPRITE:
		{
			if (clientVersion < CLVER_2_1)
				return CString() >> (char)character.sprite;
			else
				return CString() >> (char)(character.sprite % 4);
		}

		case NPCProp::COLORS:
			return CString() >> (char)character.colors[0] >> (char)character.colors[1] >> (char)character.colors[2] >> (char)character.colors[3] >> (char)character.colors[4];

		case NPCProp::NICKNAME:
			return CString() >> (char)character.nickName.length() << character.nickName;

		case NPCProp::HORSEIMAGE:
			return CString() >> (char)character.horseImage.length() << character.horseImage;

		case NPCProp::HEADIMAGE:
			return CString() >> (char)(character.headImage.length() + 100) << character.headImage;

		case NPCProp::SAVE0:
		case NPCProp::SAVE1:
		case NPCProp::SAVE2:
		case NPCProp::SAVE3:
		case NPCProp::SAVE4:
		case NPCProp::SAVE5:
		case NPCProp::SAVE6:
		case NPCProp::SAVE7:
		case NPCProp::SAVE8:
		case NPCProp::SAVE9:
		{
			auto index = static_cast<size_t>(PROPID(pId)) - PROPID(NPCProp::SAVE0);
			return CString() >> (char)saves[index];
		}

		case NPCProp::ALIGNMENT:
			return CString() >> (char)character.ap;

		case NPCProp::IMAGEPART:
			return CString() >> (short)imagePart.position.x() >> (short)imagePart.position.y() >> (char)imagePart.size.width() >> (char)imagePart.size.height();

		case NPCProp::BODYIMAGE:
			return CString() >> (char)character.bodyImage.length() << character.bodyImage;

		case NPCProp::GMAPLEVELX:
		{
			auto lvl = level.lock();
			return CString() >> (char)(lvl ? lvl->getGmapX() : 0);
		}

		case NPCProp::GMAPLEVELY:
		{
			auto lvl = level.lock();
			return CString() >> (char)(lvl ? lvl->getGmapY() : 0);
		}

		case NPCProp::SCRIPTER:
			return CString() >> (char)m_npcScripter.length() << m_npcScripter;

		case NPCProp::NAME:
			return CString() >> (char)name.length() << name;

		case NPCProp::TYPE:
			return CString() >> (char)m_npcScriptType.length() << m_npcScriptType;

		case NPCProp::CURLEVEL:
		{
			auto lvl = level.lock();
			CString tmpLevelName = (lvl ? lvl->getLevelName() : "");
			return CString() >> (char)tmpLevelName.length() << tmpLevelName;
		}

		case NPCProp::CLASS:
		{
			CString classList;

			if (!classList.isEmpty())
				classList.removeI(classList.length() - 1);
			return CString() >> (short)classList.length() << classList;
		}

		case NPCProp::X2:
		{
			uint16_t val = ((uint16_t)std::abs(character.pixelX)) << 1;
			if (character.pixelX < 0)
				val |= 0x0001;
			return CString().writeGShort(val);
		}

		case NPCProp::Y2:
		{
			uint16_t val = ((uint16_t)std::abs(character.pixelY)) << 1;
			if (character.pixelY < 0)
				val |= 0x0001;
			return CString().writeGShort(val);
		}

		case NPCProp::Z2:
		{
			// range: -25 to 85
			uint16_t val = std::min<int16_t>(85 * 16, std::max<int16_t>(-25 * 16, character.pixelZ));
			val = std::abs(val) << 1;
			if (character.pixelZ < 0)
				val |= 0x0001;
			return CString().writeGShort(val);
		}

		case NPCProp::GATTRIB1:
		case NPCProp::GATTRIB2:
		case NPCProp::GATTRIB3:
		case NPCProp::GATTRIB4:
		case NPCProp::GATTRIB5:
		case NPCProp::GATTRIB6:
		case NPCProp::GATTRIB7:
		case NPCProp::GATTRIB8:
		case NPCProp::GATTRIB9:
		case NPCProp::GATTRIB10:
		case NPCProp::GATTRIB11:
		case NPCProp::GATTRIB12:
		case NPCProp::GATTRIB13:
		case NPCProp::GATTRIB14:
		case NPCProp::GATTRIB15:
		case NPCProp::GATTRIB16:
		case NPCProp::GATTRIB17:
		case NPCProp::GATTRIB18:
		case NPCProp::GATTRIB19:
		case NPCProp::GATTRIB20:
		case NPCProp::GATTRIB21:
		case NPCProp::GATTRIB22:
		case NPCProp::GATTRIB23:
		case NPCProp::GATTRIB24:
		case NPCProp::GATTRIB25:
		case NPCProp::GATTRIB26:
		case NPCProp::GATTRIB27:
		case NPCProp::GATTRIB28:
		case NPCProp::GATTRIB29:
		case NPCProp::GATTRIB30:
		{
			auto index = std::ranges::distance(npcGaniAttrPackets.begin(), std::ranges::find(npcGaniAttrPackets, PROPID(pId)));
			return CString() >> (char)character.ganiAttributes[index].length() << character.ganiAttributes[index];
		}
	}

	return CString();
}

CString NPC::setPropsFromPacket(CString& pProps, int clientVersion, bool pForward)
{
	bool hasMoved = false;

	// TODO(joey): Most of these props will eventually be ignored

	CString ret;
	int len = 0;
	while (pProps.bytesLeft() > 0)
	{
		NPCProp propId = (NPCProp)pProps.readGUChar();
		CString oldProp = getPropPacket(propId);
		//printf( "propId: %d\n", propId );
		switch (propId)
		{
			case NPCProp::IMAGE:
				visFlags |= (uint8_t)NPCVisFlags::VISIBLE;
				image = pProps.readChars(pProps.readGUChar()).text();
				if (!image.empty() && clientVersion < CLVER_2_1 && getExtension(image).isEmpty())
					image.append(".gif");
				break;

			case NPCProp::SCRIPT:
				pProps.readChars(pProps.readGUShort());

				// TODO(joey): is this used for putnpcs?
				//clientScript = pProps.readChars(pProps.readGUShort());
				break;

			case NPCProp::X:
				if (m_blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				character.pixelX = pProps.readGChar() * 8;
				hasMoved = true;
				break;

			case NPCProp::Y:
				if (m_blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				character.pixelY = pProps.readGChar() * 8;
				hasMoved = true;
				break;
				
			case NPCProp::Z:
				if (m_blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				character.pixelZ = (pProps.readGChar() - 50) * 8;
				hasMoved = true;
				break;

			case NPCProp::POWER:
				character.hitpointsInHalves = pProps.readGUChar();
				break;

			case NPCProp::RUPEES:
				character.gralats = pProps.readGUInt();
				break;

			case NPCProp::ARROWS:
				character.arrows = pProps.readGUChar();
				break;

			case NPCProp::BOMBS:
				character.bombs = pProps.readGUChar();
				break;

			case NPCProp::GLOVEPOWER:
				character.glovePower = pProps.readGUChar();
				break;

			case NPCProp::BOMBPOWER:
				character.bombPower = pProps.readGUChar();
				break;

			case NPCProp::SWORDIMAGE:
			{
				int sp = pProps.readGUChar();
				if (sp <= 4)
					character.swordImage = (CString() << "sword" << CString(sp) << (clientVersion < CLVER_2_1 ? ".gif" : ".png")).toString();
				else
				{
					sp -= 30;
					len = pProps.readGUChar();
					if (len > 0)
					{
						character.swordImage = pProps.readChars(len).toString();
						if (!character.swordImage.empty() && clientVersion < CLVER_2_1 && getExtension(character.swordImage).isEmpty())
							character.swordImage += ".gif";
					}
					else
						character.swordImage = "";
					//character.swordPower = clip(sp, ((settings->getBool("healswords", false) == true) ? -(settings->getInt("swordlimit", 3)) : 0), settings->getInt("swordlimit", 3));
				}
				character.swordPower = sp;
				break;
			}

			case NPCProp::SHIELDIMAGE:
			{
				int sp = pProps.readGUChar();
				if (sp <= 3)
					character.shieldImage = (CString() << "shield" << CString(sp) << (clientVersion < CLVER_2_1 ? ".gif" : ".png")).toString();
				else
				{
					sp -= 10;
					len = pProps.readGUChar();
					if (len > 0)
					{
						character.shieldImage = pProps.readChars(len).toString();
						if (!character.shieldImage.empty() && clientVersion < CLVER_2_1 && getExtension(character.shieldImage).isEmpty())
							character.shieldImage += ".gif";
					}
					else
						character.shieldImage = "";
				}
				character.shieldPower = std::min<uint8_t>(sp, 3);
				break;
			}

			case NPCProp::GANI:
			{
				if (clientVersion < CLVER_2_1)
				{
					// Older clients don't use ganis.  This is the bow power and image instead.
					character.bowPower = pProps.readGUChar();
					if (character.bowPower >= 10)
					{
						character.bowImage = pProps.readChars(character.bowPower - 10).toString();
						if (!character.bowImage.empty() && clientVersion < CLVER_2_1 && getExtension(character.bowImage).isEmpty())
							character.bowImage += ".gif";
					}
					break;
				}
				character.gani = pProps.readChars(pProps.readGUChar()).text();
				break;
			}

			case NPCProp::VISFLAGS:
				visFlags = pProps.readGUChar();
				break;

			case NPCProp::BLOCKFLAGS:
				blockFlags = pProps.readGUChar();
				break;

			case NPCProp::MESSAGE:
				character.chatMessage = pProps.readChars(pProps.readGUChar()).text();
				break;

			case NPCProp::HURTDXDY:
				hurtX = ((float)(pProps.readGUChar() - 32)) / 32;
				hurtY = ((float)(pProps.readGUChar() - 32)) / 32;
				break;

			case NPCProp::ID:
				pProps.readGUInt();
				break;

			case NPCProp::SPRITE:
			{
				auto sprite = pProps.readGUChar();
				if (clientVersion < CLVER_2_1)
					character.sprite = sprite;
				else character.sprite = sprite % 4;
				break;
			}

			case NPCProp::COLORS:
				for (int i = 0; i < 5; i++)
					character.colors[i] = pProps.readGUChar();
				break;

			case NPCProp::NICKNAME:
				character.nickName = pProps.readChars(pProps.readGUChar()).text();
				break;

			case NPCProp::HORSEIMAGE:
				character.horseImage = pProps.readChars(pProps.readGUChar()).toString();
				if (!character.horseImage.empty() && clientVersion < CLVER_2_1 && getExtension(character.horseImage).isEmpty())
					character.horseImage += ".gif";
				break;

			case NPCProp::HEADIMAGE:
				len = pProps.readGUChar();
				if (len < 100)
					character.headImage = (CString() << "head" << CString(len) << (clientVersion < CLVER_2_1 ? ".gif" : ".png")).toString();
				else
				{
					character.headImage = pProps.readChars(len - 100).toString();
					if (!character.headImage.empty() && clientVersion < CLVER_2_1 && getExtension(character.headImage).isEmpty())
						character.headImage += ".gif";
				}
				break;

			case NPCProp::ALIGNMENT:
				character.ap = pProps.readGUChar();
				character.ap = clip(character.ap, 0, 100);
				break;

			case NPCProp::IMAGEPART:
			{
				Position<uint16_t> pos = { pProps.readGUShort(), pProps.readGUShort() };
				Dimension<uint8_t> size = { pProps.readGUChar(), pProps.readGUChar() };
				imagePart = Rectangle<uint16_t, uint8_t>(pos, size);
				break;
			}

			case NPCProp::BODYIMAGE:
				character.bodyImage = pProps.readChars(pProps.readGUChar()).toString();
				break;

			case NPCProp::GMAPLEVELX:
				pProps.readGUChar();
				break;

			case NPCProp::GMAPLEVELY:
				pProps.readGUChar();
				break;

			case NPCProp::SCRIPTER:
				m_npcScripter = pProps.readChars(pProps.readGUChar());
				break;

			case NPCProp::NAME:
				name = pProps.readChars(pProps.readGUChar()).text();
				break;

			case NPCProp::TYPE:
				m_npcScriptType = pProps.readChars(pProps.readGUChar());
				break;

			case NPCProp::CURLEVEL:
				pProps.readChars(pProps.readGUChar());
				break;

			case NPCProp::CLASS:
				pProps.readChars(pProps.readGShort());
				break;

				// Location, in pixels, of the npc on the level in 2.3+ clients.
				// Bit 0x0001 controls if it is negative or not.
				// Bits 0xFFFE are the actual value.
			case NPCProp::X2:
				if (m_blockPositionUpdates)
				{
					pProps.readGUShort();
					continue;
				}

				len = pProps.readGUShort();
				character.pixelX = (len >> 1);

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					character.pixelX = -character.pixelX;

				hasMoved = true;
				break;

			case NPCProp::Y2:
				if (m_blockPositionUpdates)
				{
					pProps.readGUShort();
					continue;
				}

				len = pProps.readGUShort();
				character.pixelY = (len >> 1);

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					character.pixelY = -character.pixelY;

				hasMoved = true;
				break;

			case NPCProp::Z2:
				if (m_blockPositionUpdates)
				{
					pProps.readGUShort();
					continue;
				}

				len = pProps.readGUShort();
				character.pixelZ = (len >> 1);

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					character.pixelZ = -character.pixelZ;

				hasMoved = true;
				break;

			case NPCProp::SAVE0:
			case NPCProp::SAVE1:
			case NPCProp::SAVE2:
			case NPCProp::SAVE3:
			case NPCProp::SAVE4:
			case NPCProp::SAVE5:
			case NPCProp::SAVE6:
			case NPCProp::SAVE7:
			case NPCProp::SAVE8:
			case NPCProp::SAVE9:
			{
				int index = PROPID(propId) - PROPID(NPCProp::SAVE0);
				saves[index] = pProps.readGUChar();
				break;
			}

			case NPCProp::GATTRIB1:
			case NPCProp::GATTRIB2:
			case NPCProp::GATTRIB3:
			case NPCProp::GATTRIB4:
			case NPCProp::GATTRIB5:
			case NPCProp::GATTRIB6:
			case NPCProp::GATTRIB7:
			case NPCProp::GATTRIB8:
			case NPCProp::GATTRIB9:
			case NPCProp::GATTRIB10:
			case NPCProp::GATTRIB11:
			case NPCProp::GATTRIB12:
			case NPCProp::GATTRIB13:
			case NPCProp::GATTRIB14:
			case NPCProp::GATTRIB15:
			case NPCProp::GATTRIB16:
			case NPCProp::GATTRIB17:
			case NPCProp::GATTRIB18:
			case NPCProp::GATTRIB19:
			case NPCProp::GATTRIB20:
			case NPCProp::GATTRIB21:
			case NPCProp::GATTRIB22:
			case NPCProp::GATTRIB23:
			case NPCProp::GATTRIB24:
			case NPCProp::GATTRIB25:
			case NPCProp::GATTRIB26:
			case NPCProp::GATTRIB27:
			case NPCProp::GATTRIB28:
			case NPCProp::GATTRIB29:
			case NPCProp::GATTRIB30:
			{
				auto index = std::ranges::distance(npcGaniAttrPackets.begin(), std::ranges::find(npcGaniAttrPackets, PROPID(propId)));
				character.ganiAttributes[index] = pProps.readChars(pProps.readGUChar()).toString();
				break;
			}

			default:
			{
				printf("NPC %ud (%.2f, %.2f): ", id, (float)character.pixelX / 16.0f, (float)character.pixelY / 16.0f);
				printf("Unknown prop: %ud, readPos: %d\n", propId, pProps.readPos());
				for (int i = 0; i < pProps.length(); ++i)
					printf("%02x ", (unsigned char)pProps[i]);
				printf("\n");
				return ret;
			}
		}

		// If a prop changed, adjust its mod time.
		if ((int)propId < NPCPROP_COUNT)
		{
			if (oldProp != getPropPacket(propId))
				modTime[PROPID(propId)] = time(0);
		}

		// Add to ret.
		ret >> (char)propId << getPropPacket(propId, clientVersion);
	}

	if (pForward)
	{
		// Send the props.
		m_server->sendPacketToLevelArea(CString() >> (char)PLO_NPCPROPS >> (int)id << ret, level);
	}

	return ret;
}

/*
void NPC::setPropModTime(NPCProp pid, time_t time)
{
	if (PROPID(pid) >= NPCPROP_COUNT)
		return;
	modTime[PROPID(pid)] = time;
}
*/

//----------------------------

prop_access NPC::getPropAccess(NPCProp prop)
{
	static uint32_t prevent_access_int = 0;
	static float prevent_access_float = 0.0f;
	static std::string prevent_access_string;

	switch (prop)
	{
		case NPCProp::IMAGE:
			return &image;
		case NPCProp::SCRIPT:
			return &prevent_access_string;
		case NPCProp::X:
			throw std::invalid_argument("NPC::getPropAccess: use X2 instead of X");
		case NPCProp::Y:
			throw std::invalid_argument("NPC::getPropAccess: use Y2 instead of Y");
		case NPCProp::POWER:
			return &character.hitpointsInHalves;
		case NPCProp::RUPEES:
			return &character.gralats;
		case NPCProp::ARROWS:
			return &character.arrows;
		case NPCProp::BOMBS:
			return &character.bombs;
		case NPCProp::GLOVEPOWER:
			return &character.glovePower;
		case NPCProp::BOMBPOWER:
			return &character.bombPower;
		case NPCProp::SWORDIMAGE:
			return &character.swordImage;
		case NPCProp::SHIELDIMAGE:
			return &character.shieldImage;
		case NPCProp::GANI:
			return &character.gani;
		case NPCProp::VISFLAGS:
			return &visFlags;
		case NPCProp::BLOCKFLAGS:
			return &blockFlags;
		case NPCProp::MESSAGE:
			return &character.chatMessage;
		case NPCProp::HURTDXDY:
			return std::make_pair(&hurtX, &hurtY);
		case NPCProp::ID:
			return &prevent_access_int;
		case NPCProp::SPRITE:
			return &character.sprite;
		case NPCProp::COLORS:
			return &character.colors.at(0);
		case NPCProp::NICKNAME:
			return &character.nickName;
		case NPCProp::HORSEIMAGE:
			return &character.horseImage;
		case NPCProp::HEADIMAGE:
			return &character.headImage;
		case NPCProp::SAVE0:
		case NPCProp::SAVE1:
		case NPCProp::SAVE2:
		case NPCProp::SAVE3:
		case NPCProp::SAVE4:
		case NPCProp::SAVE5:
		case NPCProp::SAVE6:
		case NPCProp::SAVE7:
		case NPCProp::SAVE8:
		case NPCProp::SAVE9:
			return &saves[static_cast<size_t>(PROPID(prop)) - PROPID(NPCProp::SAVE0)];
		case NPCProp::ALIGNMENT:
			return &character.ap;
		case NPCProp::IMAGEPART:
			throw std::invalid_argument("NPC::getPropAccess: IMAGEPART is not implemented, is this required?");
		case NPCProp::BODYIMAGE:
			return &character.bodyImage;
		case NPCProp::GMAPLEVELX:
			return &prevent_access_int;
		case NPCProp::GMAPLEVELY:
			return &prevent_access_int;
		case NPCProp::Z:
			throw std::invalid_argument("NPC::getPropAccess: use Z2 instead of Z");
		case NPCProp::UNKNOWN48:
			throw std::invalid_argument("NPC::getPropAccess: UNKNOWN48 is not implemented, is this required?");
		case NPCProp::SCRIPTER:
			return &m_npcScripter;
		case NPCProp::NAME:
			return &name;
		case NPCProp::TYPE:
			return &m_npcScriptType;
		case NPCProp::CURLEVEL:
			return &prevent_access_string;
		case NPCProp::CLASS:
			return &m_npcClass;
		case NPCProp::X2:
			return &character.pixelX;
		case NPCProp::Y2:
			return &character.pixelY;
		case NPCProp::Z2:
			return &character.pixelZ;
		case NPCProp::GATTRIB1:
		case NPCProp::GATTRIB2:
		case NPCProp::GATTRIB3:
		case NPCProp::GATTRIB4:
		case NPCProp::GATTRIB5:
		case NPCProp::GATTRIB6:
		case NPCProp::GATTRIB7:
		case NPCProp::GATTRIB8:
		case NPCProp::GATTRIB9:
		case NPCProp::GATTRIB10:
		case NPCProp::GATTRIB11:
		case NPCProp::GATTRIB12:
		case NPCProp::GATTRIB13:
		case NPCProp::GATTRIB14:
		case NPCProp::GATTRIB15:
		case NPCProp::GATTRIB16:
		case NPCProp::GATTRIB17:
		case NPCProp::GATTRIB18:
		case NPCProp::GATTRIB19:
		case NPCProp::GATTRIB20:
		case NPCProp::GATTRIB21:
		case NPCProp::GATTRIB22:
		case NPCProp::GATTRIB23:
		case NPCProp::GATTRIB24:
		case NPCProp::GATTRIB25:
		case NPCProp::GATTRIB26:
		case NPCProp::GATTRIB27:
		case NPCProp::GATTRIB28:
		case NPCProp::GATTRIB29:
		case NPCProp::GATTRIB30:
			return &character.ganiAttributes[std::ranges::distance(npcGaniAttrPackets.begin(), std::ranges::find(npcGaniAttrPackets, PROPID(prop)))];
	};

	return &prevent_access_int; // Should never be reached, but prevents compiler warnings.
}

//----------------------------

std::string_view toWeaponName(std::string_view code)
{
	constexpr size_t notFound = std::string_view::npos;

	size_t name_start = code.find("toweapons");
	if (name_start == notFound)
		return {};

	name_start += 9; // 9 = strlen("toweapons")

	size_t name_end[2] = { code.find(";", name_start), code.find("}", name_start) };
	if (name_end[0] == notFound && name_end[1] == notFound)
		return {};

	size_t name_pos = name_end[0];
	if (name_end[1] != notFound && name_end[1] < name_end[0])
		name_pos = name_end[1];

	if (name_pos == notFound)
		return {};

	return string::trim(code.substr(name_start, name_pos - name_start));
}

std::string performClientSideJoinHack(std::string_view code, FileSystem* fs)
{
	std::string result;
	std::vector<std::string_view> joins;

	size_t start = 0, end = 0;
	while (start < code.length())
	{
		// Find the next join.
		// If we don't find one, copy the rest of the code and break.
		end = code.find("join ", start);
		if (end == std::string_view::npos)
		{
			result += code.substr(start);
			break;
		}

		// Look for a newline or the start of a code block so we don't capture the word join in a string.
		bool join_is_start_of_block = true;
		if (end != 0)
		{
			size_t block_start = end - 1;
			while (block_start > 0)
			{
				// Skip any whitespace before the join.
				if (code[block_start] == ' ' || code[block_start] == '\t')
				{
					--block_start;
					continue;
				}
				// Look for the start of a block or a newline.
				else if (!(code[block_start] == '\n' || code[block_start] == '\xa7' || code[block_start] == '{'))
				{
					join_is_start_of_block = false;
					break;
				}

				// We found a new line or a block start.
				break;
			}
			if (!join_is_start_of_block)
			{
				result += code.substr(start, end);
				start = end + 5; // 5 = strlen("join ")
				continue;
			}
		}

		// Copy the code before the join.
		// Then, add a semi-colon.  We are going to remove the join entirely.
		result += code.substr(start, end - start);
		result += ";";

		// Get the name of the join.
		start = end + 5; // 5 = strlen("join ")
		end = code.find(";", start);
		if (end == std::string_view::npos)
			break;

		// Save the join to the list of joins.
		std::string_view join = string::trim(code.substr(start, end - start));
		if (!join.empty())
			joins.push_back(join);

		start = end + 1;
	}

	// Load the files and append them to the result.
	CString c;
	for (const auto& fileName : joins)
	{
		c = removeComments(fs->load(std::format("{}.txt", fileName)));
		c.removeAllI("\r");
		result += "\n";
		result += c.toStringView();
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
