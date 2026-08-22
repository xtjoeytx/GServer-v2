#ifndef SETTINGS_H
#define SETTINGS_H

#include <algorithm>
#include <concepts>
#include <filesystem>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <cassert>
#include <utilities/CommonTypes.h>
#include <utilities/Events.h>
#include <utilities/StringUtils.h>
#include <utilities/std/generator.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

struct append_list_t
{
	explicit append_list_t() = default;
};
inline constexpr append_list_t append_list{};

template<typename T>
class SettingCache;

//----------------------------

/// @brief Stores settings.
class Settings
{
public:
	/// @brief Event dispatch that passes the old value. The event is posted when a setting is updated.
	using SettingEventDispatch = EventDispatcher<>;

public:
	Settings() = default;
	~Settings()
	{
		m_settings.clear();
		for (auto& dispatch : m_settingUpdateEvents | std::views::values)
			dispatch.unsubscribeAll();
	}

public:
	/// @brief Loads settings from a file.
	/// @param file The path to the file to load settings from.
	void load(const std::filesystem::path& file);

	/// @brief Clears all settings and posts update events for all settings.
	void clear()
	{
		m_settings.clear();
		for (auto& dispatch : m_settingUpdateEvents | std::views::values)
			dispatch.post();
	}

public:
	/// @brief Tracks a setting cache. The setting cache will be updated when the setting is updated, and when this function is first called.
	/// @tparam T The contained types of the setting caches to track.
	/// @param cache The setting caches to track.
	template<typename... T>
	void track(SettingCache<T>&... cache)
	{
		(trackOne(cache), ...);
	}

public:
	/// @brief Checks if a setting exists.
	/// @param key The key of the setting to check for.
	/// @return True if the setting exists, false otherwise.
	bool exists(const std::string_view key) const
	{
		return m_settings.contains(key);
	}

public:
	/// @brief Sets a value to a setting.
	/// @tparam T The type of the value to set.
	/// @param key The key of the setting to set.
	/// @param value The value to set.
	template<typename T>
	void set(std::string_view key, const T& value)
	{
		m_settings.erase(key);

		if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>)
			m_settings.emplace(key, value);
		else if constexpr (std::same_as<T, bool>)
			m_settings.emplace(key, value ? "true" : "false");
		else
			m_settings.emplace(key, std::to_string(value));

		if (const auto eventIt = m_settingUpdateEvents.find(key); eventIt != m_settingUpdateEvents.end())
		{
			auto& event = eventIt->second;
			event.post();
		}
	}

	/// @brief Adds a value to a setting.
	/// @tparam T The type of the value to set.
	/// @param key The key of the setting to set.
	/// @param value The value to set.
	template<typename T>
	void set(append_list_t, std::string_view key, const T& value)
	{
		if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>)
			m_settings.emplace(key, value);
		else if constexpr (std::same_as<T, bool>)
			m_settings.emplace(key, value ? "true" : "false");
		else
			m_settings.emplace(key, std::to_string(value));

		if (const auto eventIt = m_settingUpdateEvents.find(key); eventIt != m_settingUpdateEvents.end())
		{
			auto& event = eventIt->second;
			event.post();
		}
	}

public:
	/// @brief Gets the value of a setting. If there are multiple values for the same key, the last value will be returned.
	/// @tparam T The type of the value to get.
	/// @param key The key of the setting to get.
	/// @return The value of the setting, or std::nullopt if the setting does not exist.
	template<typename T>
	std::optional<T> get(const std::string_view key) const
	{
		static_assert(false, "Settings::get called with a type that isn't handled. Make sure to provide a template specialization for the type you want to get.");
		return std::nullopt;
	}

	/// @brief Gets the value of a setting. If there are multiple values for the same key, the last value will be returned.
	/// @tparam T The type of the value to get.
	/// @param key The key of the setting to get.
	/// @return The value of the setting, or std::nullopt if the setting does not exist.
	template<string::StringVariant T = std::string>
	std::optional<T> get(const std::string_view key) const
	{
		auto range = m_settings.equal_range(key);

		// Not found.
		if (range.first == std::end(m_settings) || range.first->first != key)
			return std::nullopt;

		// One element.
		if (range.second == std::end(m_settings))
			return range.first->second;

		// Many elements.
		auto& last = range.second;
		--last;
		return last->second;
	}

	/// @brief Gets a list of values for a setting. The values will be split by commas. If there are multiple values for the same key, all values will be returned.
	/// @tparam T The type of the container to return.
	/// @param key The key of the setting to get.
	/// @return The container with the values of the setting, or std::nullopt if the setting does not exist.
	template<ContainerLikeNotString T>
	std::optional<T> get(const std::string_view key) const
	{
		using value_type = std::ranges::range_value_t<T>;

		if (!exists(key))
			return std::nullopt;

		std::optional<T> result = T{};

		for (auto item : getList(key))
		{
			if constexpr (std::integral<value_type>)
				result.value().emplace_back(string::toNumber<value_type>(item));
			else if constexpr (std::same_as<value_type, float>)
				result.value().emplace_back(string::toFloat(item));
			else if constexpr (std::same_as<value_type, double>)
				result.value().emplace_back(string::toDouble(item));
			else if constexpr (std::same_as<value_type, bool>)
				result.value().emplace_back(string::equalsi(item, "true"sv) ? true : false);
			else if constexpr (string::StringViewIshVariant<value_type>)
				result.value().emplace_back(item);
			else
				static_assert(false, "Settings::get<ContainerLike> called with a container that contains a data type that isn't handled.");
		}

		return result;
	}

	/// @brief Gets the value of a setting. If there are multiple values for the same key, the last value will be returned.
	/// @tparam T The type of the value to get.
	/// @param key The key of the setting to get.
	/// @return The value of the setting, or std::nullopt if the setting does not exist.
	template<std::integral T>
	std::optional<T> get(const std::string_view key) const
	{
		const auto value = get<std::string>(key);
		if (!value)
			return std::nullopt;

		if constexpr (std::same_as<T, bool>)
		{
			const std::string_view str = value.value();
			if (string::equalsi(str, "true"sv))
				return true;
			if (string::equalsi(str, "false"sv))
				return false;
			return std::nullopt;
		}
		else
		{
			return static_cast<T>(string::toNumber<T>(value.value()));
		}
	}

	/// @brief Gets the value of a setting. If there are multiple values for the same key, the last value will be returned.
	/// @tparam T The type of the value to get.
	/// @param key The key of the setting to get.
	/// @return The value of the setting, or std::nullopt if the setting does not exist.
	template<std::floating_point T>
	std::optional<float> get(const std::string_view key) const
	{
		const auto value = get<std::string>(key);
		if (!value)
			return std::nullopt;

		if constexpr (std::same_as<T, float>)
			return string::toFloat(value.value());
		else if constexpr (std::same_as<T, double>)
			return string::toDouble(value.value());
		else
			static_assert(false, "Settings::get<std::floating_point> called with a floating point type that isn't handled.");

		return std::nullopt;
	}

	/// @brief Gets a list of values for a setting. The values will be split by commas. If there are multiple values for the same key, all values will be returned.
	/// @param key The key of the setting to get.
	/// @return A generator that yields the values of the setting.
	std::generator<std::string_view> getList(const std::string_view key) const
	{
		auto range = m_settings.equal_range(key);
		if (range.first == std::end(m_settings))
			co_return;

		for (auto& it = range.first; it != range.second; ++it)
		{
			for (auto sv : string::split(it->second, ","sv, true))
				co_yield sv;
		}
	}

private:
	template<typename T>
	[[a::inline]] void trackOne(SettingCache<T>& cache);

private:
	string_ordered_multimap<std::string> m_settings;
	string_map<SettingEventDispatch> m_settingUpdateEvents;
};

//----------------------------

template<typename T>
class SettingCache
{
public:
	friend class Settings;

public:
	SettingCache() = delete;

	/// @brief Creates a blank setting cache with the given key.
	/// @param key The key of the setting to cache.
	constexpr explicit SettingCache(const std::string_view key) : key(key) {}

	/// @brief Creates a setting cache with the given key and default value.
	/// @param key The key of the setting to cache.
	/// @param defaultValue The default value of the setting.
	constexpr SettingCache(const std::string_view key, const T& defaultValue)
		: key(key), value(defaultValue)
	{
		onRequireDefaultValue = [this, defaultValue]()
		{
			value = defaultValue;
		};
	}

	/// @brief Returns whether the setting cache has a value.
	/// @return True if the setting cache has a value, false otherwise.
	explicit operator bool() const
	{
		return value.has_value();
	}

public:
	/// @brief Binds the setting cache to a settings instance. The setting cache will be updated when the setting is updated and the onUpdate handler will be called.
	/// @param settings The settings instance to bind to.
	void bind(Settings& settings)
	{
		settings.track(*this);
	}

public:
	/// @brief Gets the value of the setting cache.
	/// @return The value of the setting cache.
	const std::optional<T>& get() const
	{
		return value;
	}

	/// @brief Gets the unwrapped value of the setting cache. If the value is std::nullopt, the behavior is undefined. Will throw if a default value was not given.
	/// @return The unwrapped value of the setting cache.
	const T& getValue() const
	{
		assert(onRequireDefaultValue);
		return value.value();
	}

public:
	/// @brief The key of the setting to cache.
	std::string key;

	/// @brief The value of the setting cache.
	std::optional<T> value = std::nullopt;

	/// @brief An event handler that is called when the setting is updated. The handler takes the new value and the old value as parameters.
	std::function<void(const std::optional<T>&, const std::optional<T>&)> onUpdate;

protected:
	std::function<void()> onRequireDefaultValue;

	void update(std::optional<T>&& newValue, const std::optional<T>& oldValue)
	{
		if (onRequireDefaultValue && !newValue.has_value())
		{
			onRequireDefaultValue();
			if (onUpdate)
				onUpdate(value, oldValue);
		}
		else
		{
			value = std::move(newValue);
			if (value != oldValue && onUpdate)
				onUpdate(value, oldValue);
		}
	}

protected:
	EventHandle m_updateHandle;
};

//----------------------------

template<typename T>
inline void Settings::trackOne(SettingCache<T>& cache)
{
	auto updateFunction = [&cache, this]()
	{
		std::optional<T> oldValueT = std::move(cache.value);
		cache.value.reset();
		cache.update(get<T>(cache.key), oldValueT);
	};

	SettingEventDispatch& dispatch = m_settingUpdateEvents[cache.key];
	cache.m_updateHandle = dispatch.subscribe(updateFunction);

	// Now that we are tracking this cache, update it with the current value of the setting.
	// We are usually calling this function before we even load the settings, so if we have no settings yet,
	// just call the update function with the existing value (most likely the default) so we aren't creating any unnecessary objects.

	if (m_settings.empty()) [[likely]]
	{
		if (cache.onUpdate)
			cache.onUpdate(cache.value, std::nullopt);
	}
	else
	{
		updateFunction();
	}
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SETTINGS_H
