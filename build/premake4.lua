newoption {
	trigger		= "no-64bit",
	description	= "Don't add the 64-bit project configuration."
}

newoption {
	trigger		= "no-static",
	description	= "Don't compile as a static runtime."
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
		
		-- Dependencies.
		files { "../dependencies/zlib/**" }
		files { "../dependencies/bzip2/**" }
		files { "../dependencies/miniupnpc/**" }
		includedirs { "../dependencies" }
		includedirs { "../dependencies/zlib" }
		includedirs { "../dependencies/bzip2" }
		
		-- Libraries.
		configuration "windows"
			links { "ws2_32", "Iphlpapi" }
		configuration "linux"
			links { "boost_thread" }
		
		-- Windows defines.
		configuration "windows"
			defines { "WIN32", "_WIN32" }
		if not _OPTIONS["no-64bit"] then 
			configuration { "windows", "x64" }
				defines { "WIN64", "_WIN64" }
		end
		
		-- Debug options.
		configuration "Debug"
			defines { "DEBUG" }
			targetsuffix "_d"
			flags { "NoEditAndContinue" }
		
		-- Release options.
		configuration "Release"
			defines { "NDEBUG" }
			flags { "OptimizeSpeed" }
