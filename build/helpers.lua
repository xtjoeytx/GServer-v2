--
-- Premake helper functions
-- Heavily leeched from the CEGUI premake helper functions
--

rootdir = "../"

--
-- do a package easily
--
do
    local _tpkgdir = {}
    local _tpath = {}
    table.insert(_tpath, rootdir)

    local _dopackage = dopackage

    -- ugly path management :/
    function dopackage(name)
        table.insert(_tpath, string.gsub(name, "([^/]+)", "..").."/")
    	rootdir = table.concat(_tpath)

    	table.insert(_tpkgdir, name)
    	pkgdir = rootdir..table.concat(_tpkgdir).."/"
    	pkgparentdir = string.gsub(pkgdir, "([^/]+/)$", "")

        _dopackage(name)

        table.remove(_tpath)
    	table.remove(_tpkgdir)
    end
end

--
-- this functions initialises a default project
--
function project_dynamic(name, lang, kind)
	package.name = name
	package.kind = kind or "dll"
	package.language = lang or "c++"

	-- defaults
	package.buildflags =
	{
		"native-char",
		"unicode",
--		"no-rtti"
	}
	package.includepaths =
	{
		pkgdir.."include",
	}
	package.libpaths =
	{
	}
	package.defines =
	{
		"_CRT_SECURE_NO_DEPRECATE",
	}

	-- debug
	debug = package.config.Debug
	debug.target = name..(DEBUG_DLL_SUFFIX or "")
	debug.defines =
	{
		"_DEBUG"
	}
	debug.buildflags = {}


	-- release
	release = package.config.Release
	release.defines = {}
	release.buildflags =
	{
		"no-symbols",
		"optimize-speed",
		"no-frame-pointer",
	}
end

--
-- adds a library to the current package
--
function library(name, debugsuffix)
	tinsert(debug.links, name..(debugsuffix or ""))
	tinsert(release.links, name)
end

--
-- adds library path to the current package
--
function librarypath(path)
    tinsert(package.libpaths, path)
end

--
-- adds include path to the current package
--
function include(path)
	tinsert(package.includepaths, path)
end

--
-- adds defines to the current package
--
function define(def, conf)
    if conf then
        tinsert(package.config[conf].defines, def)
    else
        tinsert(package.defines, def)
    end
end

--
-- adds a dependency to the current package
--
function dependency(dep)
	tinsert(package.links, dep)
end

--
-- create the packages presented in table
--
function createpackages(list, prefix)
    local p = prefix or ""
	for k,v in pairs(list) do
		if type(v) ~= "string" then
			createpackages(v,p..k.."/")
		else
			dopackage(p..v)
		end
	end
end
