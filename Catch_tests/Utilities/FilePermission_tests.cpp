#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"
#include <utilities/FilePermissions.h>

SCENARIO("FilePermissions tests", "[FilePermissions]") {

	GIVEN("Default folder rights") {
		FilePermissions permManager;
		std::string permissionsInput = R"(
r accounts/*
r config/*
r documents/*
rw guilds/*
r logs/*
rw npcprops/*
rw translations/*
r weapons/*
rw world/*
rw world/levels/*
rw world/bodies/*
rw world/ganis/*
rw world/global/*
rw world/global/heads/*
rw world/global/bodies/*
rw world/global/swords/*
rw world/global/shields/*
rw world/hats/*
rw world/heads/*
rw world/images/*
rw world/shields/*
rw world/swords/*
rw world/sounds/*
		)";

		WHEN("the permissions are loaded") {
			permManager.loadPermissions(permissionsInput);

			THEN("the permissions should be correctly evaluated") {
				REQUIRE(permManager.hasPermission("weapons/file.txt", FilePermissions::Type::Read) == true);
				REQUIRE(permManager.hasPermission("WEAPONS/file.txt", FilePermissions::Type::Read) == false);
				REQUIRE(permManager.hasPermission("world/file.txt", FilePermissions::Type::Read) == true);
				REQUIRE(permManager.hasPermission("guilds/file.txt", FilePermissions::Type::Read) == true);
				REQUIRE(permManager.hasPermission("logs/server.log", FilePermissions::Type::Read) == true);
				REQUIRE(permManager.hasPermission("logs/server.log", FilePermissions::Type::Write) == false);
				REQUIRE(permManager.hasPermission("world/levels/onlinestartlocal.nw", FilePermissions::Type::Read) == true);
				REQUIRE(permManager.hasPermission("world/levels/onlinestartlocal.nw", FilePermissions::Type::Write) == true);
			}
		}
	}

    GIVEN("A FilePermissions manager and a set of permissions loaded from a string") {
        FilePermissions permManager;

        std::string permissionsInput =
            "rw WEAPONS/*\n"
            "rw world/*\n"
            "rw world/*/*\n"
            "rw guilds/*\n"
            "rw levels/*\n"
            "rw levels/some/somethin*\n"
            "rw levels/test/file*/*\n"
            "rw explicit/file\n"
            "rw partial/extension/test*.txt\n"
            "rw partial/extension/file*.e*\n"
            "rw logs/*\n"
            "-w logs/*\n"
            "w writeperm/file.txt\n"
            "-w world/hello.txt\n"
            "-rw world/test\n";

        WHEN("the permissions are loaded") {
            permManager.loadPermissions(permissionsInput);

            THEN("the permissions should be correctly evaluated") {
                REQUIRE(permManager.hasPermission("WEAPONS/file.txt", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("weapons/file.txt", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("world/file.txt", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("world/folder1/file.txt", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("world/folder1/subfolder/file.txt", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("guilds/file.txt", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("levels/file.txt", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("levels/folder1/file.txt", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("levels/some/something", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("levels/some/somethen", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("levels/test/randomtest/file", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("logs/server.log", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("logs/server.log", FilePermissions::Type::Write) == false);
                REQUIRE(permManager.hasPermission("explicit/file", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("explicit/f*le", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("explicit/filename", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("partial/extension", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("partial/extension/test123.txt", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("partial/extension/test*.txt", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("partial/extension/file1.ext", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("writeperm/file.txt", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("writeperm/file.txt", FilePermissions::Type::Write) == true);
                REQUIRE(permManager.hasPermission("world/test", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("world/test", FilePermissions::Type::Write) == false);
                REQUIRE(permManager.hasPermission("world/test2", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("world/test2", FilePermissions::Type::Write) == true);
                REQUIRE(permManager.hasPermission("world/hello.txt", FilePermissions::Type::Read) == true);
                REQUIRE(permManager.hasPermission("world/hello.txt", FilePermissions::Type::Write) == false);
                REQUIRE(permManager.hasPermission("", FilePermissions::Type::Read) == false);
                REQUIRE(permManager.hasPermission("", FilePermissions::Type::Write) == false);
            }
        }
    }

    GIVEN("A FilePermissions manager and a different set of permissions loaded from a string") {
        FilePermissions permManager;

        std::string permissionsInput =
            "rw WEAPONS/*\n"
            "rw world/*\n"
            "rw world/*/*\n"
            "rw guilds/*\n"
            "rw levels/*\n"
            "noperms\n"
            "rw\n"
            "-rw world/test.txt\n";

        WHEN("the permissions are loaded") {
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
}


//TEST_CASE("PermissionManager tests", "[PermissionManager]") {
//	FilePermissions permManager;
//
//	SECTION("Loading permissions from string") {
//		std::string permissionsInput =
//			"rw WEAPONS/*\n"
//			"rw world/*\n"
//			"rw world/*/*\n"
//			"rw guilds/*\n"
//			"rw levels/*\n"
//			"rw levels/some/somethin*\n"
//			"rw levels/test/file*/*\n"
//			"rw explicit/file\n"
//			"rw partial/extension/test*.txt\n"
//			"rw partial/extension/file*.e*\n"
//			"rw logs/*\n"
//			"-w logs/*\n"
//			"w writeperm/file.txt\n"
//			"-w world/hello.txt\n"
//			"-rw world/test\n";
//		permManager.loadPermissions(permissionsInput);
//
//		REQUIRE(permManager.hasPermission("WEAPONS/file.txt", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("weapons/file.txt", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("world/file.txt", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("world/folder1/file.txt", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("world/folder1/subfolder/file.txt", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("guilds/file.txt", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("levels/file.txt", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("levels/folder1/file.txt", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("levels/some/something", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("levels/some/somethen", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("levels/some/somethen", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("levels/test/randomtest/file", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("logs/server.log", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("logs/server.log", FilePermissions::Type::Write) == false);
//		REQUIRE(permManager.hasPermission("explicit/file", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("explicit/f*le", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("explicit/filename", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("partial/extension", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("partial/extension/test123.txt", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("partial/extension/test*.txt", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("partial/extension/file1.ext", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("writeperm/file.txt", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("writeperm/file.txt", FilePermissions::Type::Write) == true);
//		REQUIRE(permManager.hasPermission("world/test", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("world/test", FilePermissions::Type::Write) == false);
//		REQUIRE(permManager.hasPermission("world/test2", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("world/test2", FilePermissions::Type::Write) == true);
//		REQUIRE(permManager.hasPermission("world/hello.txt", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("world/hello.txt", FilePermissions::Type::Write) == false);
//		REQUIRE(permManager.hasPermission("", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("", FilePermissions::Type::Write) == false);
//	}
//
//	SECTION("Test file permissions") {
//		std::string permissionsInput =
//			"rw WEAPONS/*\n"
//			"rw world/*\n"
//			"rw world/*/*\n"
//			"rw guilds/*\n"
//			"rw levels/*\n"
//			"noperms\n"
//			"rw\n"
//			"-rw world/test.txt\n";
//		permManager.loadPermissions(permissionsInput);
//
//
//		REQUIRE(permManager.hasPermission("world/test.txt", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("world/test2.txt", FilePermissions::Type::Read) == true);
//		REQUIRE(permManager.hasPermission("", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("", FilePermissions::Type::Write) == false);
//		REQUIRE(permManager.hasPermission(" ", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission(" ", FilePermissions::Type::Write) == false);
//		REQUIRE(permManager.hasPermission("	", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("	", FilePermissions::Type::Write) == false);
//		REQUIRE(permManager.hasPermission("noperms", FilePermissions::Type::Read) == false);
//		REQUIRE(permManager.hasPermission("noperms", FilePermissions::Type::Write) == false);
//	}
//}
