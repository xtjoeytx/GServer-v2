#ifndef TGAMEANI_H
#define TGAMEANI_H

#include <optional>
#include <string>

#include <CString.h>

class Server;

class GameAni
{
	enum AniFlags : uint8_t
	{
		Continous = 0x1,
		LoopAnimation = 0x2,
		SingleDirOnly = 0x4
	};

public:
	explicit GameAni(std::string aniName);

	// Move operations
	GameAni(GameAni&& o) noexcept;
	GameAni& operator=(GameAni&& o) noexcept;

	// Delete copy operations
	GameAni(const GameAni&) = delete;
	GameAni& operator=(const GameAni&) = delete;

	//! Get the animation filename
	//! \return animation filename
	const std::string& getName() const
	{
		return m_aniName;
	}

	//! Get the script's bytecode
	//! \return bytecode
	const CString& getByteCode() const
	{
		return m_bytecode;
	}

	//! Get the animation script
	//! \return gs1/gs2 script
	const std::string& getSource() const
	{
		return m_script;
	}

	//! Get the animation's setbackto gani
	//! \return setbackto gani
	const std::string& getSetBackTo() const
	{
		return m_setBackTo;
	}

	//! Is the animation continuous (doesn't restart the animation on subsequent calls)
	//! \return true/false
	bool isContinuous() const
	{
		return m_aniFlags & AniFlags::Continous;
	}

	//! Does the animation loop (restart after each last frame)
	//! \return true/false
	bool isLoop() const
	{
		return m_aniFlags & AniFlags::LoopAnimation;
	}

	//! Does the animation have only one direction (all directions use the same animation)
	//! \return true/false
	bool isSingleDir() const
	{
		return m_aniFlags & AniFlags::SingleDirOnly;
	}

	//! Get the bytecode packet to send to clients for the ani script
	//! \return bytecode packet
	CString getBytecodePacket() const;

	//! Load a GameAni from the filesystem
	//! \param server Global server pointer so we can fetch the correct filesystem
	//! \param name filename of the animation (ex: idle.gani)
	//! \return GameAni if it was successfully loaded, otherwise a nullopt
	static std::optional<GameAni> load(Server* const server, const std::string& name);

private:
	std::string m_aniName;
	std::string m_script;
	CString m_bytecode;
	std::string m_setBackTo;
	uint8_t m_aniFlags;
};

inline GameAni::GameAni(std::string aniName)
	: m_aniName(aniName), m_aniFlags(0)
{
}

inline GameAni::GameAni(GameAni&& o) noexcept
	: m_aniName(std::move(o.m_aniName)), m_script(std::move(o.m_script)),
	  m_bytecode(std::move(o.m_bytecode)), m_setBackTo(o.m_setBackTo),
	  m_aniFlags(o.m_aniFlags)
{
}

inline GameAni& GameAni::operator=(GameAni&& o) noexcept
{
	m_aniName = std::move(o.m_aniName);
	m_script = std::move(o.m_script);
	m_bytecode = std::move(o.m_bytecode);
	m_setBackTo = std::move(o.m_setBackTo);
	m_aniFlags = o.m_aniFlags;
	return *this;
}

#endif
