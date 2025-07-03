#ifndef GS1ERRORLISTENER_H
#define GS1ERRORLISTENER_H

#include <algorithm>
#include <cstdint>
#include <exception>
#include <format>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <BaseErrorListener.h>
#include <Recognizer.h>
#include <Token.h>

#include <BabyDI.h>
#include <Server.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

class GS1ErrorListener : public antlr4::BaseErrorListener
{
public:
	GS1ErrorListener(std::string_view name) : m_name(name) {}

	void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line, size_t charPositionInLine, const std::string& msg, std::exception_ptr e) override
	{
		std::vector<std::pair<uint8_t, std::string>> logbatch;
		logbatch.emplace_back(0_ui8, std::format("* GS1 script compilation failed for '{}':", m_name));
		logbatch.emplace_back(1_ui8, std::format("Line: {}, Column: {}", line + 1, charPositionInLine + 1));

		// If we have an offending token, log its details.
		if (offendingSymbol != nullptr)
		{
			logbatch.emplace_back(1_ui8, std::format("Offending token: '{}'", offendingSymbol->getText()));
		}

		// Log the error message.
		logbatch.emplace_back(1_ui8, std::format("Error: {}", msg));

		// Log the batch of messages.
		log::batch(log::script, logbatch);

		// Send the log messages to the server.
		auto server = BabyDI::Get<Server>();
		std::ranges::for_each(logbatch, [&server](const auto& kvp) { server->sendToNC(kvp.second); });
	}

protected:
	std::string m_name;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar

#endif // GS1ERRORLISTENER_H
