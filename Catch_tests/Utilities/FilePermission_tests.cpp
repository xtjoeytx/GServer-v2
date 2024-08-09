#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"
#include <utilities/FilePermissions.h>

SCENARIO("FilePermissions") {

	GIVEN("Some folder rights") {
		FilePermissions permManager;
		std::string permissionsInput =
			"r accounts/*\n"
			"r config/*\n"
			"r documents/*\n"
			"rw guilds/*\n"
			"r logs/*\n"
			"rw npcprops/*\n"
			"rw translations/*\n"
			"r weapons/*\n"
			"rw world/*\n"
			"rw world/levels/*\n"
			"rw world/bodies/*\n"
			"rw world/ganis/*\n"
			"rw world/global/*\n"
			"rw world/global/heads/*\n"
			"rw world/global/bodies/*\n"
			"rw world/global/swords/*\n"
			"rw world/global/shields/*\n"
			"rw world/hats/*\n"
			"rw world/heads/*\n"
			"rw world/images/*\n"
			"rw world/shields/*\n"
			"rw world/swords/*\n"
			"rw world/sounds/*\n"
			"-r logs/serverlog.txt\n";
		permManager.loadPermissions(permissionsInput);

		WHEN("user tries to read weapons folder") {
			THEN("user should have permission") {
				REQUIRE(permManager.hasPermission("weapons/file.txt", FilePermissions::Type::Read) == true);
			}
		}

		WHEN("user tries to write to weapons folder") {
			THEN("user should not have permission") {
				REQUIRE(permManager.hasPermission("weapons/file.txt", FilePermissions::Type::Write) == false);
			}
		}

		WHEN("user tries to read serverlog file") {
			THEN("user should not have permission due to negative permission") {
				REQUIRE(permManager.hasPermission("logs/serverlog.txt", FilePermissions::Type::Read) == false);
			}
		}

		WHEN("user tries to read a file without any access") {
			THEN("user should not have permission") {
				REQUIRE(permManager.hasPermission("", FilePermissions::Type::Read) == false);
				REQUIRE(permManager.hasPermission("", FilePermissions::Type::Write) == false);
				REQUIRE(permManager.hasPermission("somefolder/somefile.txt", FilePermissions::Type::Read) == false);
				REQUIRE(permManager.hasPermission("somefolder/somefile.txt", FilePermissions::Type::Write) == false);
			}
		}
	}

	GIVEN("an odd set of folder rights") {
		FilePermissions permManager;
		std::string permissionsInput =
			"rw weapons/*\n"
			"rw world/*\n"
			"rw world/*/*\n"
			"rw guilds/*\n"
			"rw levels/*\n"
			"noperms\n"
			"rw\n"
			"-rw world/test.txt\n";
		permManager.loadPermissions(permissionsInput);

		THEN("the file permissions should be correctly evaluated") {
			REQUIRE(permManager.hasPermission("world/test.txt", FilePermissions::Type::Read) == false);
			REQUIRE(permManager.hasPermission("world/test2.txt", FilePermissions::Type::Read) == true);
			REQUIRE(permManager.hasPermission("", FilePermissions::Type::Read) == false);
			REQUIRE(permManager.hasPermission("", FilePermissions::Type::Write) == false);
			REQUIRE(permManager.hasPermission(" ", FilePermissions::Type::Read) == false);
			REQUIRE(permManager.hasPermission(" ", FilePermissions::Type::Write) == false);
			REQUIRE(permManager.hasPermission("	", FilePermissions::Type::Read) == false);
			REQUIRE(permManager.hasPermission("	", FilePermissions::Type::Write) == false);
			REQUIRE(permManager.hasPermission("noperms", FilePermissions::Type::Read) == false);
			REQUIRE(permManager.hasPermission("noperms", FilePermissions::Type::Write) == false);
		}
	}
}
