#pragma once

#ifndef SOURCECODE_H
#define SOURCECODE_H

#include <string>
#include <string_view>

class SourceCode
{
public:
	SourceCode() : _gs2default(false) { }
	SourceCode(std::string src, bool gs2default = false) : _src(std::move(src)), _gs2default(gs2default) { init(); }
	SourceCode(SourceCode&& o) noexcept : _src(std::move(o._src)), _gs2default(o._gs2default) { init(); }

	SourceCode& operator=(SourceCode&& o) noexcept {
		_gs2default = o._gs2default;
		_src = std::move(o._src);
		_clientside = std::move(o._clientside);
		_serverside = std::move(o._serverside);
		_clientGS1 = std::move(o._clientGS1);
		_clientGS2 = std::move(o._clientGS2);
		return *this;
	}

	explicit operator bool() const {
		return !empty();
	}

	bool empty() const {
		return _src.empty();
	}

	const std::string& getSource() const {
		return _src;
	}

	std::string_view getServerSide() const {
		return _serverside;
	}

	std::string_view getClientSide() const {
		return _clientside;
	}

	std::string_view getClientGS1() const {
		return _clientGS1;
	}

	std::string_view getClientGS2() const {
		return _clientGS2;
	}

	void setServerSide(std::string_view sv) {
		_serverside = sv;
	}

private:
	bool _gs2default;
	std::string _src;
	std::string_view _clientside, _serverside;
	std::string_view _clientGS1, _clientGS2;

	void init()
	{
		auto clientSideSep = _src.find("//#CLIENTSIDE");
		if (clientSideSep != std::string::npos)
		{
			// Separate clientside and serverside
			_clientside = std::string_view{ _src.begin() + clientSideSep, _src.end() };
			_serverside = std::string_view{ _src.begin(), _src.begin() + clientSideSep };

			// Switch separator depending on if GS2 is set to default or not
			const char* gs2sep_char;
			if (_gs2default)
				gs2sep_char = "//#GS1";
			else
				gs2sep_char = "//#GS2";

			// Determine if this code is GS1 or GS2
			auto gs2Sep = _clientside.find(gs2sep_char);
			bool using_gs2 = (gs2Sep != std::string::npos) ^ _gs2default;

			if (using_gs2)
				_clientGS2 = _clientside;
			else
				_clientGS1 = _clientside;
		}
		else _serverside = std::string_view{ _src.begin(), _src.end() };
	}
};


#endif
