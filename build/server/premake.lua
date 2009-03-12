--
-- client premake script
--

project_dynamic("gserver", "c++", "exe")

package.files =
{
    matchfiles(rootdir.."server/src/*.cpp"),
    matchfiles(rootdir.."server/include/*.h"),
}

-- Windows library includes.
if (windows) then
	include(rootdir.."dependencies/include")
	librarypath(rootdir.."dependencies")
end

-- Libraries to link to.
if (linux or target == "cb-gcc" or target == "gnu") then
	library("z")
	library("bz2")
	library("boost_thread")
else
	library("libz")
	library("libbz2")
end
if (windows) then library("ws2_32") end
