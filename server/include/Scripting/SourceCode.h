#ifndef SOURCECODE_H
#define SOURCECODE_H

#include <string>
#include <string_view>

class SourceCode
{
public:
	SourceCode() : _gs2default(false) {}
	SourceCode(std::string src, bool gs2default = false) : _src(std::move(src)), _gs2default(gs2default) { init(); }
	SourceCode(SourceCode&& o) noexcept : _src(std::move(o._src)), _gs2default(o._gs2default) { init(); }

	SourceCode& operator=(SourceCode&& o) noexcept
	{
		_gs2default = o._gs2default;
		_src        = std::move(o._src);
		init();
		return *this;
	}

	explicit operator bool() const
	{
		return !empty();
	}

	bool empty() const
	{
		return _src.empty();
	}

	const std::string& getSource() const
	{
		return _src;
	}

	std::string_view getServerSide() const
	{
		return _serverside;
	}

	std::string_view getClientSide() const
	{
		return _clientside;
	}

	std::string_view getClientGS1() const
	{
		return _clientGS1;
	}

	std::string_view getClientGS2() const
	{
		return _clientGS2;
	}

	void clearServerSide()
	{
		_serverside = {};
	}

private:
	bool _gs2default;
	std::string _src;
	std::string_view _clientside, _serverside;
	std::string_view _clientGS1, _clientGS2;

	void init() noexcept
	{
		_clientside = _serverside = _clientGS1 = _clientGS2 = {};

#ifdef V8NPCSERVER
		auto clientSep = _src.find("//#CLIENTSIDE");
		if (clientSep != std::string::npos)
		{
			// Separate clientside and serverside
			_clientside = std::string_view{_src}.substr(clientSep);
			_serverside = std::string_view{_src}.substr(0, clientSep);
		}
		else
			_serverside = std::string_view{_src};
#else
		// For non-npcserver builds all code is considered clientside
		_clientside = std::string_view{_src};
#endif

		if (!_clientside.empty())
		{
			// Switch separator depending on if GS2 is set to default or not
			const char* gs2sep_char;
			if (_gs2default)
				gs2sep_char = "//#GS1";
			else
				gs2sep_char = "//#GS2";

			// Determine if this code is GS1 or GS2
			size_t codeSeparatorLoc = _clientside.find(gs2sep_char);
			if (codeSeparatorLoc != std::string::npos)
			{
				auto origCode  = _clientside.substr(0, codeSeparatorLoc);
				auto otherCode = _clientside.substr(codeSeparatorLoc);
				_clientGS2     = _gs2default ? origCode : otherCode;
				_clientGS1     = _gs2default ? otherCode : origCode;
			}
			else
			{
				if (_gs2default)
					_clientGS2 = _clientside;
				else
					_clientGS1 = _clientside;
			}
		}
	}
};

#endif
