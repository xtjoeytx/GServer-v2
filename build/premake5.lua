newoption {
	trigger		= "no-static",
	description	= "Don't compile as a static runtime."
}

newoption {
	trigger		= "no-upnp",
	description	= "Don't compile with universal plug and play support."
}

workspace "gserver2"
	configurations { "Debug", "Release" }
	platforms { "x32", "x64" }
	symbols "On"
	targetdir ( "Build/%{_ACTION}/bin/%{cfg.buildcfg}" )
	libdirs { "Build/%{_ACTION}/bin/%{cfg.buildcfg}" }

	configuration "vs*"
	defines { "_CRT_SECURE_NO_WARNINGS" }	

	if not _OPTIONS["no-static"] then flags { "StaticRuntime" } end

	filter "configurations:Debug"
	 	defines { "DEBUG" }
	 	targetsuffix "_d"
		editandcontinue "Off"
		optimize "Off"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"
		flags { "LinkTimeOptimization" }

	-- Set our platform architectures.
	filter "platforms:x32"
		architecture "x32"
	filter "platforms:x64"
		architecture "x64"

	-- C++17 support.
	-- filter { "language:C++", "toolset:clang*" }
	-- 	links { "c++experimental" }

	-- Windows defines.
	filter "system:windows"
		defines { "WIN32", "_WIN32" }
	filter { "system:windows", "platforms:x64" }
		defines { "WIN64", "_WIN64" }


project "gserver2"
	kind "ConsoleApp"
	-- kind "WindowedApp"
	language "C++"
	location "projects"
	targetdir "../bin"
	debugdir "../bin"
	targetname "gserver2"

	-- C++14 support
	cppdialect "C++14"

	vectorextensions "SSE2"

	files { "../server/include/**", "../server/src/**" }

	includedirs {
		"../server/include",
		"../dependencies",
		"../dependencies/bzip2",
		"../dependencies/zlib",
	}

	links {
		"bzip2",
		"zlib",
	}

	if not _OPTIONS["no-upnp"] then
		links { "miniupnpc" }
	end

	dependson { "bzip2", "zlib" }

	filter "system:linux"
		defines { "_BSD_SOURCE", "_POSIX_C_SOURCE=1" }
		links { "pthread" }

	filter "system:windows"
		links { "ws2_32", "Iphlpapi" }


project "bzip2"
	kind "StaticLib"
	language "C"
	location "projects"
	files { "../dependencies/bzip2/**.h", "../dependencies/bzip2/**.c" }
	includedirs { "../dependencies/bzip2/" }
	removefiles {
		"../dependencies/bzip2/bzip2.c",
		"../dependencies/bzip2/bzip2recover.c",
		"../dependencies/bzip2/dlltest.c",
		"../dependencies/bzip2/mk251.c",
		"../dependencies/bzip2/spewG.c",
		"../dependencies/bzip2/unzcrash.c"
	}

	-- I give up.
	filter "system:linux"
		kind "SharedLib"


project "zlib"
	kind "StaticLib"
	language "C"
	location "projects"
	files { "../dependencies/zlib/*.h", "../dependencies/zlib/*.c" }
	includedirs { "../dependencies/zlib/" }

	-- I give up.
	filter "system:linux"
		kind "SharedLib"


if not _OPTIONS["no-upnp"] then
project "miniupnpc"
	kind "StaticLib"
	language "C"
	location "projects"
	files { "../dependencies/miniupnpc/**" }
	includedirs { "../dependencies/" }
	defines { "UPNP" }
	if not _OPTIONS["no-static"] then
		defines { "STATICLIB" }  -- For the UPnP library.
	end
end
