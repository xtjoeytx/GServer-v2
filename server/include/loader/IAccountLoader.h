#ifndef IACCOUNTLOADER_H
#define IACCOUNTLOADER_H

#include <string>
#include <string_view>
#include <vector>

#include <Account.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class IAccountLoader
{
public:
	virtual bool loadAccount(std::string_view accountName, Account& account) = 0;
	virtual bool saveAccount(const Account& account) = 0;

public:
	virtual bool checkSearchConditions(std::string_view account, const std::vector<std::string>& searches) const = 0;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // IACCOUNTLOADER_H
