solution "gserver2"
	configurations { "Debug", "Release" }
	platforms { "native", "x32", "x64" }
	
	project "gserver2"
		kind "ConsoleApp"
		-- kind "WindowedApp"
		language "C++"
		location "projects"
		targetdir "../bin"
		targetname "gserver2"
		flags { "Symbols", "Unicode" }
		files { "../server/include/**", "../server/src/**" }
		includedirs { "../server/include" }
		
		-- Dependencies.
		files { "../dependencies/zlib/**" }
		files { "../dependencies/bzip2/**" }
		includedirs { "../dependencies/zlib" }
		includedirs { "../dependencies/bzip2" }
		
		-- Libraries.
		configuration "windows"
			links { "ws2_32" }
		configuration "linux"
			links { "boost_thread" }
		
		-- Windows defines.
		configuration "windows"
			defines { "WIN32", "_WIN32" }
		configuration { "windows", "x64" }
			defines { "WIN64", "_WIN64" }
		
		-- Debug options.
		configuration "Debug"
			defines { "DEBUG" }
			targetsuffix "_d"
			flags { "NoEditAndContinue" }
		
		-- Release options.
		configuration "Release"
			defines { "NDEBUG" }
			flags { "StaticRuntime", "OptimizeSpeed" }
