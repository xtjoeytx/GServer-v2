newoption {
	trigger		= "no-64bit",
	description	= "Don't add the 64-bit project configuration."
}

newoption {
	trigger		= "no-static",
	description	= "Don't compile as a static runtime."
}

newoption {
	trigger		= "no-boost",
	description = "Don't compile with boost.thread support."
}

solution "gserver2"
	configurations { "Debug", "Release" }
	platforms { "native", "x32" }
	if not _OPTIONS["no-64bit"] then platforms { "x64" } end
	flags { "Symbols", "Unicode" }
	if not _OPTIONS["no-static"] then flags { "StaticRuntime" } end
	
	project "gserver2"
		kind "ConsoleApp"
		-- kind "WindowedApp"
		language "C++"
		location "projects"
		targetdir "../bin"
		targetname "gserver2"
		files { "../server/include/**", "../server/src/**" }
		includedirs { "../server/include" }
		if not _OPTIONS["no-static"] then
			defines { "STATICLIB" }  -- For the UPnP library.
		end
		if _OPTIONS["no-boost"] then
			defines { "NO_BOOST" }
		end

		files { "../dependencies/miniupnpc/**" }
		files { "../dependencies/bzip2/**" }

	-- It bugs if I remove next line, but why? paul
		includedirs { "../dependencies" }
		
		includedirs { "../dependencies/bzip2" }

		files { "../dependencies/zlib/**" }
		includedirs { "../dependencies/zlib" }
		
		-- Libraries.
		configuration "windows"
			links { "ws2_32", "Iphlpapi" }
		if not _OPTIONS["no-boost"] then
			configuration { "linux or macosx or bsd or solaris" }
				links { "boost_thread", "boost_system" }
		end
		
		-- Windows defines.
		configuration "windows"
			defines { "WIN32", "_WIN32" }
			-- Miniunpnc need Windows XP and more recent (VINVER >=0x0501 )
			-- without it, MinGW would give linking error for freeaddrinfo
			defines ( "WINVER=0x0501")
		if not _OPTIONS["no-64bit"] then 
			configuration { "windows", "x64" }
				defines { "WIN64", "_WIN64" }
		end
		-- Dependencies (Windows only, we expect others to have those)

		-- Linux defines.
		configuration "Linux"
			defines { "_BSD_SOURCE", "_POSIX_C_SOURCE=1" }
		
		-- Debug options.
		configuration "Debug"
			defines { "DEBUG" }
			targetsuffix "_d"
			flags { "NoEditAndContinue" }
		
		-- Release options.
		configuration "Release"
			defines { "NDEBUG" }
			flags { "OptimizeSpeed" }
