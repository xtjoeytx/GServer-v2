--
-- client premake script
--

project_dynamic("gserver", "c++", "exe")

package.files =
{
    matchfiles(rootdir.."server/src/*.cpp"),
    matchfiles(rootdir.."server/include/*.h"),
	matchfiles(rootdir.."dependencies/bzip2/*.c"),
	matchfiles(rootdir.."dependencies/bzip2/*.h"),
	matchfiles(rootdir.."dependencies/zlib/*.c"),
	matchfiles(rootdir.."dependencies/zlib/*.h"),
}

-- Library includes.
include(rootdir.."dependencies/bzip2")
include(rootdir.."dependencies/zlib")

-- Libraries to link to.
if (linux or target == "cb-gcc" or target == "gnu") then
	library("boost_thread")
end
if (windows) then library("ws2_32") end
