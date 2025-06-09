#include <math.h>
#include <IEnums.h>

#include <FileSystem.h>
#include <Server.h>
#include <level/Level.h>
#include <level/Map.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <scripting/ScriptContainers.h>
#include <scripting/SourceCode.h>
#include <utilities/Log.h>
#include <utilities/PropsContainer.h>
#include <utilities/StringUtils.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

static constexpr std::array<uint8_t, 10> savePackets = { 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };

static std::string_view toWeaponName(std::string_view code);
static std::string performClientSideJoinHack(std::string_view code, FileSystem* fs);

//----------------------------

#ifdef PACKETLOGGING
#define DO_PACKETLOG(LOG) LOG
#else
#define DO_PACKETLOG(LOG)
#endif

#define PRINT_NPCPROP(prop, ...) #prop ##sv,
constexpr std::array<std::string_view, NPCPROP_COUNT> npcPropNames =
{
	FOR_LIST_OF_NPC_PROPS(PRINT_NPCPROP)
};

//----------------------------

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

/// @brief A getter function for a property that gets its results from another getter function.
static GameVariable::func_get prop_get(ValidGameValueCallable auto getter)
{
	return [getter](std::string_view identifier) -> GameValue
	{
		return GameValue{ getter() };
	};
}

/// @brief A getter function for a property that gets its results from a property directly.
static GameVariable::func_get prop_get(auto& value)
{
	using V = std::remove_cvref_t<decltype(value)>;
	static_assert(std::integral<V> || std::floating_point<V> || string::StringVariant<V> || std::ranges::forward_range<V>,
				  "NPC prop_get called with an unsupported type. Supported types are integral, floats, string, or ranges.");

	// Number.
	if constexpr (std::integral<V> || std::floating_point<V>)
	{
		return [&value](std::string_view identifier) -> GameValue
		{
			return GameValue{ static_cast<double>(value) };
		};
	}
	// String.
	else if constexpr (string::StringVariant<V>)
	{
		return [&value](std::string_view identifier) -> GameValue
		{
			return GameValue{ std::string{ value } };
		};
	}
	// Array.
	else if constexpr (std::ranges::forward_range<V>)
	{
		return [&value](std::string_view identifier) -> GameValue
		{
			// Transform the range to a vector of doubles.
			return GameValue{ value | std::views::transform([](const auto& v) { return static_cast<double>(v); }) | std::ranges::to<std::vector<double>>() };
		};
	}

	throw std::invalid_argument("NPC prop_get called with an unsupported type.");
}

/// @brief A setter function for a property that needs an additional setter function to write the values.
static GameVariable::func_set prop_set(NPC* who, std::optional<NPCProp> prop, std::function<void(const GameValue&, std::optional<size_t>)> setter)
{
	return [who, prop, setter](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
	{
		// Call the setter function.
		setter(value, index);

		// Record the modification time for the property.
		if (prop.has_value() && who != nullptr)
		{
			// Normal properties.
			if (prop.value() != NPCProp::SAVE0)
				who->modTime[PROPID(prop.value())] = currentTime();
			else if (index.has_value() && index.value() <= 9)
			{
				// Special handling for save properties since it is an array in script, but the props are stored individually.
				auto propIndex = PROPID(NPCProp::SAVE0) + index.value();
				who->modTime[propIndex] = currentTime();
			}
		}
	};
}

/// @brief A setter function for a property that can directly set to a value.
static GameVariable::func_set prop_set(NPC* who, std::optional<NPCProp> prop, auto& propvalue)
{
	using V = std::remove_cvref_t<decltype(propvalue)>;
	static_assert(std::integral<V> || std::floating_point<V> || string::StringVariant<V> || std::ranges::random_access_range<V>,
		"NPC prop_get called with an unsupported type. Supported types are integral, floats, string, or ranges.");

	// Number.
	if constexpr (std::integral<V> || std::floating_point<V>)
	{
		return [who, prop, &propvalue](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
		{
			propvalue = static_cast<V>(value.get<double>().value_or(0.0));
			if (prop.has_value())
				who->modTime[PROPID(prop.value())] = currentTime();
		};
	}
	// String.
	else if constexpr (string::StringVariant<V>)
	{
		return [who, prop, &propvalue](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
		{
			propvalue = value.get<std::string>().value_or({});
			if (prop.has_value())
				who->modTime[PROPID(prop.value())] = currentTime();
		};
	}
	// Array.
	else if constexpr (std::ranges::random_access_range<V>)
	{
		return [who, prop, &propvalue](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
		{
			size_t propvalue_size = std::ranges::size(propvalue);
			if (propvalue_size > 0)
			{
				using value_type = std::remove_cvref_t<decltype(propvalue[0])>;

				// Setting an individual index in an array.
				if (index.has_value() && index.value() < propvalue_size)
				{
					propvalue[index.value()] = static_cast<value_type>(value.get<double>().value_or(0.0));
					if (prop.has_value())
						who->modTime[PROPID(prop.value()) + index.value()] = currentTime();
				}
				// Setting the whole array.
				else if (!index.has_value())
				{
					auto vec = value.get<std::vector<double>>().value();
					copyToArrayAs<value_type>(vec, propvalue);
					if (prop.has_value())
						who->modTime[PROPID(prop.value())] = currentTime();
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
	modTime.fill(clock::time_point::min());

	// We need to alter the modTime of the following props as they should be always sent.
	// If we don't, they won't be sent until the prop gets modified.
	auto props = std::to_array({ NPCProp::IMAGE, NPCProp::SCRIPT, NPCProp::X, NPCProp::Y, NPCProp::Z, NPCProp::VISFLAGS, NPCProp::ID, NPCProp::SPRITE, NPCProp::MESSAGE, NPCProp::CURLEVEL, NPCProp::GMAPLEVELX, NPCProp::GMAPLEVELY, NPCProp::X2, NPCProp::Y2, NPCProp::Z2 });
	std::ranges::for_each(props, [this, now = currentTime()](const NPCProp& prop) { modTime[PROPID(prop)] = now; });

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
		prop_get(
			[this]()
			{
				return timeout.count() / 1000.0;
			}),
		prop_set(this, std::nullopt,
			[this](const GameValue& value, std::optional<size_t>)
			{
				timeout = std::chrono::milliseconds(static_cast<int>(value.get<double>().value_or(0.0) * 1000));
			})
		});
	scripting.variables.add(GameVariable{ "sprite",
		prop_get(character.sprite),
		prop_set(this, NPCProp::SPRITE,
			[this](const GameValue& value, std::optional<size_t>)
			{
				character.sprite = static_cast<uint8_t>(value.get<double>().value_or(0.0));

				auto server = BabyDI::Get<Server>();
				if (character.sprite >= 4 && server->Generation != ServerGeneration::ORIGINAL)
				{
					character.gani = std::format("def[{}]", character.sprite);
					//visFlags |= static_cast<uint8_t>(NPCVisFlags::UNKNOWNBIT5);
					modTime[PROPID(NPCProp::GANI)] = currentTime();
					//modTime[PROPID(NPCProp::VISFLAGS)] = currentTime();
				}
			})
		});
	scripting.variables.add(GameVariable{ "dir",
		prop_get([this]() { return static_cast<double>(character.direction); }),
		prop_set(this, NPCProp::SPRITE,
			[this](const GameValue& value, std::optional<size_t>)
			{
				character.direction = std::clamp(static_cast<uint8_t>(value.get<double>().value_or(0.0)), 0_ui8, 3_ui8);
			})
		});
}

//----------------------------

void NPC::setScript(std::string_view script)
{
	//auto profile = log::Profile(log::server, "NPC::setScript");

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

std::string NPC::getLevelName() const
{
	if (auto levelPtr = level.lock())
		return levelPtr->getLevelName().toString();
	return {};
}

//----------------------------

std::shared_ptr<PropertyBase> NPC::constructPropFor(NPCProp prop) const
{
	switch (prop)
	{
#define GENERATE_CONSTRUCTPROPFOR_CASE(prop, type, ...) case prop: return std::make_shared<type>();
		FOR_LIST_OF_NPC_PROPS(GENERATE_CONSTRUCTPROPFOR_CASE);
	}
	throw std::invalid_argument("Invalid NPCProp type in constructPropFor");
}

//----------------------------

std::shared_ptr<PropertyBase> NPC::getProp(NPCProp prop) const
{
	switch (prop)
	{
#define GENERATE_GETPROP_CASE(prop, type, ...) case prop: return std::make_shared<type>( __VA_ARGS__ );
		FOR_LIST_OF_NPC_PROPS(GENERATE_GETPROP_CASE);
	}

	throw std::invalid_argument("Invalid NPCProp type in getProp");
}

//----------------------------

SetResults NPC::setProp(NPCProp prop, SetBy setBy, std::shared_ptr<PropertyBase> base)
{
	PropertyBase* basePtr = base.get();
	if (basePtr != nullptr)
		return setProp(prop, setBy, basePtr);
	throw std::invalid_argument("setProp called with nullptr base pointer.");
}

SetResults NPC::setProp(NPCProp prop, SetBy setBy, PropertyBase* base)
{
	auto levelPtr = level.lock();
	bool canUpdatePosition = !m_blockPositionUpdates || setBy == props::SetBy::SERVER;

	props::SetResults result{ .propId = { PROPID(prop) } };
	result.resultFlags.set(props::SetResults::sendToLevel, true);
	result.resultFlags.set(props::SetResults::sendToSource, false);

	auto curTime = currentTime();
	modTime[PROPID(prop)] = curTime;

#define SETPROP_RETURN_ERROR do { result.resultFlags.set(SetResults::wasInvalid); return result; } while(false)

	switch (prop)
	{
		case NPCProp::IMAGE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			visFlags |= (uint8_t)NPCVisFlags::VISIBLE;
			result.resultPropIds.push_back(PROPID(NPCProp::VISFLAGS));

			if (strProp->value == "#c#" && image != "#c")
			{
				character.gani = "idle";
				result.resultPropIds.push_back(PROPID(NPCProp::GANI));
			}

			image = strProp->value;
			break;
		}

		case NPCProp::SCRIPT:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			// Only allow the server to set the script.
			if (setBy == props::SetBy::SERVER)
			{
				setScript(strProp->value);
			}
			break;
		}

		case NPCProp::X:
		{
			PropertyTileCoordinate* coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (!canUpdatePosition)
				break;

			character.pixelX = coordProp->pixelCoordinate;

			// Do collision testing.
			//doTouchTest = true;

			// Let 2.30+ clients see pre-2.30 movement.
			result.resultPropIds.push_back(PROPID(NPCProp::X2));
			break;
		}

		case NPCProp::Y:
		{
			PropertyTileCoordinate* coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (!canUpdatePosition)
				break;

			character.pixelY = coordProp->pixelCoordinate;

			// Do collision testing.
			//doTouchTest = true;

			// Let 2.30+ clients see pre-2.30 movement.
			result.resultPropIds.push_back(PROPID(NPCProp::Y2));
			break;
		}

		case NPCProp::Z:
		{
			PropertyTileCoordinateZ* zProp = dynamic_cast<PropertyTileCoordinateZ*>(base);
			if (zProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (!canUpdatePosition)
				break;

			character.pixelZ = zProp->pixelCoordinate;

			// Do collision testing.
			//doTouchTest = true;

			// Let 2.30+ clients see pre-2.30 movement.
			result.resultPropIds.push_back(PROPID(NPCProp::Z2));
			break;
		}

		case NPCProp::POWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.hitpointsInHalves = numProp->value;
			break;
		}

		case NPCProp::RUPEES:
		{
			PropertyNumeric<GBYTE3>* numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.gralats = numProp->value;
			break;
		}

		case NPCProp::ARROWS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.arrows = numProp->value;
			break;
		}

		case NPCProp::BOMBS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.bombs = numProp->value;
			break;
		}

		case NPCProp::GLOVEPOWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.glovePower = numProp->value;
			break;
		}

		case NPCProp::BOMBPOWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.bombPower = numProp->value;
			break;
		}

		case NPCProp::SWORDIMAGE:
		{
			PropertySwordPower* swordProp = dynamic_cast<PropertySwordPower*>(base);
			if (swordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (swordProp->power.has_value())
				character.swordPower = props::Limits::applySwordPower(swordProp->power.value_or(1));

			character.swordImage = props::Limits::apply(swordProp->image, props::Limits::SwordImageLength);
			break;
		}

		case NPCProp::SHIELDIMAGE:
		{
			PropertyShieldPower* shieldProp = dynamic_cast<PropertyShieldPower*>(base);
			if (shieldProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (shieldProp->power.has_value())
				character.shieldPower = props::Limits::applyShieldPower(shieldProp->power.value_or(1));

			character.shieldImage = props::Limits::apply(shieldProp->image, props::Limits::ShieldImageLength);
			break;
		}

		case NPCProp::GANI:
		{
			PropertyGaniOrBowGif* ganiProp = dynamic_cast<PropertyGaniOrBowGif*>(base);
			if (ganiProp == nullptr)
				SETPROP_RETURN_ERROR;

			// 1.x servers didn't have ganis.  This prop was used for the bow instead.
			if (m_server->Generation == ServerGeneration::ORIGINAL)
			{
				if (!ganiProp->bowGif.has_value())
					break;

				auto& [image, power] = ganiProp->bowGif.value();
				character.bowPower = props::Limits::apply(power, props::Limits::MaxBowPower);
				character.bowImage = image;
				if (!character.bowImage.empty() && !character.bowImage.contains('.'))
					character.bowImage += ".gif";
				break;
			}

			// Set the gani.
			std::string gani = ganiProp->gani.value_or("idle");
			character.gani = props::Limits::apply(gani, props::Limits::GaniLength);
			result.resultFlags.set(SetResults::getLatestOnSend);

			// If we are not in a legacy sprite gani and our sprite is not 0, reset the sprite.
			if (!character.gani.starts_with("def[") && character.sprite != 0)
			{
				character.sprite = 0;
				//visFlags &= ~static_cast<uint8_t>(NPCVisFlags::UNKNOWNBIT5);
				result.resultPropIds.push_back(PROPID(NPCProp::SPRITE));
				//result.resultPropIds.push_back(PROPID(NPCProp::VISFLAGS));
			}

			// Hack to allow spin to hurt things.
			if (character.gani == "spin")
			{
				float tX = static_cast<float>(character.pixelX / 16.0f);
				float tY = static_cast<float>(character.pixelY / 16.0f);
				m_server->hitObjectsAtPoint({ tX + 1.5f, tY + 2.0f }, character.swordPower, level, nullptr);
			}
			break;
		}

		case NPCProp::VISFLAGS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			visFlags = numProp->value;
			break;
		}

		case NPCProp::BLOCKFLAGS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			blockFlags = numProp->value;
			break;
		}

		case NPCProp::MESSAGE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.chatMessage = strProp->value;
			break;
		}

		case NPCProp::HURTDXDY:
		{
			PropertyHurtDxDy* hurtProp = dynamic_cast<PropertyHurtDxDy*>(base);
			if (hurtProp == nullptr)
				SETPROP_RETURN_ERROR;

			hurtX = hurtProp->hurtDX;
			hurtY = hurtProp->hurtDY;
			break;
		}

		case NPCProp::ID:
			break;

		case NPCProp::SPRITE:
		{
			PropertySprite* spriteProp = dynamic_cast<PropertySprite*>(base);
			if (spriteProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.direction = spriteProp->direction;
			character.sprite = spriteProp->sprite;

			// If we manually set a sprite, change the gani.
			auto server = BabyDI::Get<Server>();
			if (server->Generation != ServerGeneration::ORIGINAL && character.sprite != 0)
			{
				auto gani = std::format("def[{}]", character.sprite);
				//visFlags |= static_cast<uint8_t>(NPCVisFlags::UNKNOWNBIT5);
				result.resultPropIds.push_back(PROPID(NPCProp::GANI));
				//result.resultPropIds.push_back(PROPID(NPCProp::VISFLAGS));
			}

			result.resultFlags.set(SetResults::getLatestOnSend);
			break;
		}

		case NPCProp::COLORS:
		{
			PropertyColors* colorProp = dynamic_cast<PropertyColors*>(base);
			if (colorProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.colors = colorProp->values;
			break;
		}

		case NPCProp::NICKNAME:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.nickName = strProp->value;
			break;
		}

		case NPCProp::HORSEIMAGE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.horseImage = strProp->value;

			if (m_server->Generation == ServerGeneration::ORIGINAL && !character.horseImage.empty() && !character.horseImage.contains('.'))
				character.horseImage += ".gif";
			break;
		}

		case NPCProp::HEADIMAGE:
		{
			PropertyHeadGif* headProp = dynamic_cast<PropertyHeadGif*>(base);
			if (headProp == nullptr)
				SETPROP_RETURN_ERROR;

			std::string img;
			if (std::holds_alternative<uint8_t>(headProp->image))
				img = std::format("head{}.{}", std::get<uint8_t>(headProp->image), (m_server->Generation != ServerGeneration::ORIGINAL ? "png" : "gif"));
			else
				img = std::get<std::string>(headProp->image);

			if (m_server->Generation == ServerGeneration::ORIGINAL && !img.empty() && !img.contains('.'))
				img += ".gif";

			character.headImage = props::Limits::apply(img, props::Limits::HeadImageLength);
			result.resultFlags.set(SetResults::getLatestOnSend);
			break;
		}

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
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			auto index = PROPID(prop) - PROPID(NPCProp::SAVE0);
			saves[index] = numProp->value;
			break;
		}

		case NPCProp::ALIGNMENT:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.ap = numProp->value;
			break;
		}

		case NPCProp::IMAGEPART:
		{
			PropertyImagePart* imgPartProp = dynamic_cast<PropertyImagePart*>(base);
			if (imgPartProp == nullptr)
				SETPROP_RETURN_ERROR;

			imagePart = imgPartProp->imagePart;
			break;
		}

		case NPCProp::BODYIMAGE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.bodyImage = strProp->value;
			break;
		}

		case NPCProp::GMAPLEVELX:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (levelPtr == nullptr || !canUpdatePosition)
				break;

			if (warpRestrictions == NPCWarpRestrictions::NOTALLOWED)
			{
				// TODO(Nalin): Clamp the NPC to the level bounds.
				break;
			}

			// TODO(Nalin): The server needs a warp function for NPCs to handle leave props.
			if (auto cmap = levelPtr->getMap(); cmap && cmap->isGmap())
			{
				auto& newLevelName = cmap->getLevelAt(numProp->value, levelPtr->getMapY());
				if (auto newLevel = m_server->getLevel(newLevelName); newLevel != nullptr)
				{
					levelPtr->removeNPC(id);
					newLevel->addNPC(id);
					level = newLevel;
				}
			}
			break;
		}

		case NPCProp::GMAPLEVELY:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (levelPtr == nullptr || !canUpdatePosition)
				break;

			if (warpRestrictions == NPCWarpRestrictions::NOTALLOWED)
			{
				// TODO(Nalin): Clamp the NPC to the level bounds.
				break;
			}

			// TODO(Nalin): The server needs a warp function for NPCs to handle leave props.
			if (auto cmap = levelPtr->getMap(); cmap && cmap->isGmap())
			{
				auto& newLevelName = cmap->getLevelAt(numProp->value, levelPtr->getMapY());
				if (auto newLevel = m_server->getLevel(newLevelName); newLevel != nullptr)
				{
					levelPtr->removeNPC(id);
					newLevel->addNPC(id);
					level = newLevel;
				}
			}
			break;
		}

		case NPCProp::UNKNOWN48:
			break;

		case NPCProp::SCRIPTER:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;
			break;

			m_npcScripter = strProp->value;
		}

		case NPCProp::NAME:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			name = strProp->value;
			break;
		}

		case NPCProp::TYPE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_npcScriptType = strProp->value;
			break;
		}

		case NPCProp::CURLEVEL:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (!canUpdatePosition)
				break;

			// TODO: Level warp?
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
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			auto index = std::ranges::distance(NpcGaniAttrPackets.begin(), std::ranges::find(NpcGaniAttrPackets, PROPID(prop)));
			character.ganiAttributes[index] = strProp->value;
			break;
		}

		case NPCProp::CLASS:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_npcClass = strProp->value;
			break;
		}

		case NPCProp::X2:
		{
			PropertyPixelCoordinate* pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (!canUpdatePosition)
				break;

			character.pixelX = pixelProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(PlayerProp::X));

			//doTouchTest = true;
			break;
		}

		case NPCProp::Y2:
		{
			PropertyPixelCoordinate* pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (!canUpdatePosition)
				break;

			character.pixelY = pixelProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(PlayerProp::Y));

			//doTouchTest = true;
			break;
		}

		case NPCProp::Z2:
		{
			PropertyPixelCoordinate* pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (!canUpdatePosition)
				break;

			character.pixelZ = pixelProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(PlayerProp::Z));

			//doTouchTest = true;
			break;
		}
	}

	// If we are sending other ids, we need to update the mod time for them too.
	if (!result.resultPropIds.empty())
	{
		for (const auto& id : result.resultPropIds)
			modTime[id] = curTime;
	}

	return result;
}

//----------------------------

void NPC::sendPropsFromResults(PropertySendResults& results, PlayerPtr source)
{
	CString sendAll, sendLevel, sendSource;

	collectPacketsFromResults(results, sendAll, sendLevel, sendSource, [this](uint8_t propId)
	{
		return this->getProp((NPCProp)propId);
	});

	// Send the buffers out.
	if (sendAll.length() > 0)
		m_server->sendPacketToAll(CString() >> (char)PLO_NPCPROPS >> (int)id << sendAll);

	PlayerID exclude = 0;
	if (source != nullptr)
		exclude = source->getId();

	if (sendLevel.length() > 0 && !level.expired())
		m_server->sendPacketToLevelArea(CString() >> (char)PLO_NPCPROPS >> (int)id << sendLevel, level, { exclude });

	if (sendSource.length() > 0 && source != nullptr)
		source->sendPacket(CString() >> (char)PLO_NPCPROPS >> (int)id << sendSource);
}

//----------------------------

void NPC::setPropsFromPacket(CString& packet, PlayerPtr source)
{
	DO_PACKETLOG(log::printBlock(log::networkdump, "NPC::setPropsFromPacket:\n"));

	PropertySendResults results;
	auto setBy = (source != nullptr ? SetBy::CLIENT : SetBy::SERVER);

	while (packet.bytesLeft() > 0)
	{
		NPCProp propId = (NPCProp)packet.readGUChar();

		DO_PACKETLOG(size_t oldPos = packet.readPos());

		auto prop = constructPropFor(propId);
		prop->deserialize(packet);

#ifdef PACKETLOGGING
		size_t currentPos = packet.readPos();
		CString rawData = packet.subString(oldPos, currentPos - oldPos);

		log::printBlock(log::networkdump, "  {}: {} |", npcPropNames[PROPID(propId)], prop);
		for (size_t i = 0; i < rawData.length(); ++i)
		{
			log::printBlock(log::networkdump, " {:02x}", (unsigned char)rawData[i]);
		}
		log::printBlock(log::networkdump, "\n");
#endif

		results.emplace_back(setProp(propId, setBy, prop), prop);
	}

	sendPropsFromResults(results, source);

	DO_PACKETLOG(log::print(log::networkdump, "\n"));
}

//----------------------------

CString NPC::getModifiedPropsPacket() const
{
	DO_PACKETLOG(bool printedHeader = false);

	CString result;
	for (auto i = 0; i < NPCPROP_COUNT; ++i)
	{
		if (modTime[i] != m_savedModTime[i])
		{
			DO_PACKETLOG(if (!printedHeader) { printedHeader = true; log::printBlock(log::networkdump, "NPC::getModifiedPropsPacket:\n"); });

			if (i == PROPID(NPCProp::GANI) && !isCharacter())
			{
				DO_PACKETLOG(log::printBlock(log::networkdump, "  NPCProp::GANI: (empty)\n"));
				result >> (char)i >> (char)0;
			}
			else
			{
#ifdef PACKETLOGGING
				auto prop = getProp((NPCProp)i);
				CString data = prop->serialize();

				log::printBlock(log::networkdump, "  {}: {}", npcPropNames[i], prop);
				if ((NPCProp)i != NPCProp::SCRIPT)
				{
					log::printBlock(log::networkdump, " |");
					for (size_t i = 0; i < data.length(); ++i)
						log::printBlock(log::networkdump, " {:02x}", (unsigned char)data[i]);
				}
				log::printBlock(log::networkdump, "\n");

				result >> (char)i << data;
#else
				result >> (char)i << getProp((NPCProp)i)->serialize();
#endif
			}
		}
	}

	DO_PACKETLOG(if (printedHeader) log::print(log::networkdump, "\n"));
	return result;
}

CString NPC::getAllPropsPacket(clock::time_point newTime) const
{
	DO_PACKETLOG(log::printBlock(log::networkdump, "NPC::getAllPropsPacket:\n"));

	bool oldcreated = m_server->getSettings().getBool("oldcreated", "false");
	CString retVal;
	int pmax = NPCPROP_COUNT;

	for (int i = 0; i < pmax; i++)
	{
		if (modTime[i] != clock::time_point::min() && modTime[i] >= newTime)
		{
			/*
			if (oldcreated && i == PROPID(NPCProp::VISFLAGS) && newTime == clock::time_point::min())
			{
				retVal >> (char)i >> (char)(visFlags | (uint8_t)NPCVisFlags::VISIBLE);
			}
			else*/ if (i == PROPID(NPCProp::GANI) && !isCharacter())
			{
				DO_PACKETLOG(log::printBlock(log::networkdump, "  NPCProp::GANI: (empty)\n"));
				retVal >> (char)i >> (char)0;
			}
			else
			{
#ifdef PACKETLOGGING
				auto prop = getProp((NPCProp)i);
				CString data = prop->serialize();

				log::printBlock(log::networkdump, "  {}: {}", npcPropNames[i], prop);
				if ((NPCProp)i != NPCProp::SCRIPT)
				{
					log::printBlock(log::networkdump, " |");
					for (size_t i = 0; i < data.length(); ++i)
						log::printBlock(log::networkdump, " {:02x}", (unsigned char)data[i]);
				}
				log::printBlock(log::networkdump, "\n");

				retVal >> (char)i << data;
#else
				retVal >> (char)i << getProp((NPCProp)i)->serialize();
#endif
			}
		}
	}

	DO_PACKETLOG(log::print(log::networkdump, "\n"));
	return retVal;
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
