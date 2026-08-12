#ifndef FLATFILEACCOUNTLOADER_H
#define FLATFILEACCOUNTLOADER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Account.h>
#include <loader/IAccountLoader.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

using flagPair = std::pair<std::string, std::string>;
using chestPair = std::pair<std::string, LocalWholeTilePosition>;

class FlatFileAccountLoader : public IAccountLoader
{
public:
	bool loadAccount(std::string_view accountName, Account& account) override;
	bool saveAccount(const Account& account) override;

public:
	[[nodiscard]] bool checkSearchConditions(std::string_view account, const std::vector<std::string>& searches) const override;

protected:
	[[nodiscard]] static flagPair decomposeFlag(const std::string& flag);
	[[nodiscard]] static chestPair decomposeChest(const std::string& chest);
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // FLATFILEACCOUNTLOADER_H
