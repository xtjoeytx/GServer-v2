#pragma once

#ifndef SOURCECODE_H
#define SOURCECODE_H

#include <string>
#include <string_view>

class SourceCode
{
public:
	SourceCode() { }
	SourceCode(const std::string& src) : _src(src) { init(); }
	SourceCode(std::string&& src) noexcept : _src(std::move(src)) { init(); }
	SourceCode(SourceCode&& o) noexcept : _src(std::move(o._src)) { init(); }
	
	SourceCode& operator=(SourceCode&& o) noexcept {
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

			// Separate GS2 section in clientside
			auto gs2Sep = _clientside.find("//#GS2");
			if (gs2Sep != std::string::npos)
			{
				_clientGS1 = std::string_view{ _clientside.begin(), _clientside.begin() + gs2Sep };
				_clientGS2 = std::string_view{ _clientside.begin() + gs2Sep, _clientside.end() };
			}
			else _clientGS1 = _clientside;
		}
		else _serverside = std::string_view{ _src.begin(), _src.end() };
	}
};


#endif
