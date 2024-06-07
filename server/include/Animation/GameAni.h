#ifndef TGAMEANI_H
#define TGAMEANI_H

#include "CString.h"
#include <optional>
#include <string>

class Server;

class GameAni
{
	enum AniFlags : uint8_t
	{
		Continous     = 0x1,
		LoopAnimation = 0x2,
		SingleDirOnly = 0x4
	};

public:
	explicit GameAni(std::string aniName);

	// Move operations
	GameAni(GameAni&& o) noexcept;
	GameAni& operator=(GameAni&& o) noexcept;

	// Delete copy operations
	GameAni(const GameAni&)            = delete;
	GameAni& operator=(const GameAni&) = delete;

	//! Get the animation filename
	//! \return animation filename
	const std::string& getName() const
	{
		return _aniName;
	}

	//! Get the script's bytecode
	//! \return bytecode
	const CString& getByteCode() const
	{
		return _bytecode;
	}

	//! Get the animation script
	//! \return gs1/gs2 script
	const std::string& getSource() const
	{
		return _script;
	}

	//! Get the animation's setbackto gani
	//! \return setbackto gani
	const std::string& getSetBackTo() const
	{
		return _setBackTo;
	}

	//! Is the animation continuous (doesn't restart the animation on subsequent calls)
	//! \return true/false
	bool isContinuous() const
	{
		return _aniFlags & AniFlags::Continous;
	}

	//! Does the animation loop (restart after each last frame)
	//! \return true/false
	bool isLoop() const
	{
		return _aniFlags & AniFlags::LoopAnimation;
	}

	//! Does the animation have only one direction (all directions use the same animation)
	//! \return true/false
	bool isSingleDir() const
	{
		return _aniFlags & AniFlags::SingleDirOnly;
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
	std::string _aniName;
	std::string _script;
	CString _bytecode;
	std::string _setBackTo;
	uint8_t _aniFlags;
};

inline GameAni::GameAni(std::string aniName)
	: _aniName(aniName), _aniFlags(0)
{
}

inline GameAni::GameAni(GameAni&& o) noexcept
	: _aniName(std::move(o._aniName)), _script(std::move(o._script)),
	  _bytecode(std::move(o._bytecode)), _setBackTo(o._setBackTo),
	  _aniFlags(o._aniFlags)
{
}

inline GameAni& GameAni::operator=(GameAni&& o) noexcept
{
	_aniName   = std::move(o._aniName);
	_script    = std::move(o._script);
	_bytecode  = std::move(o._bytecode);
	_setBackTo = std::move(o._setBackTo);
	_aniFlags  = o._aniFlags;
	return *this;
}

#endif
