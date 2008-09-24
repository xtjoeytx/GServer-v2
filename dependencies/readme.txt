The following dependencies are needed:

-------------
| LUA 5.0.3 |
-------------
[http]	http://www.lua.org/ftp/lua-5.0.3.tar.gz

-------------------------------------------------------------------------------

-
- FOLLOW THESE IN ORDER! -
-

-----------
| General |
-----------
Extract all of the dependencies to their own directories.

Make sure you provide the following paths to the build environment:
(Under MSVC 2005, Tools/Options/Projects and Solutions/VC++ directories)

[[ Include ]]
* lua/include

[[ Library ]]
* lua/lib

Alternatively, library files (.lib/.a) can be placed in this directory and they
will be linked to when the project is built.  However, you cannot do that with
include files yet.

-------
| LUA |
-------
* Replace the lua/include/lua.h file with the one from this directory.
* Create a new DLL project for LUA named "lua50" and add all of the lua/include/
  files and all of the lua/src/ files, EXCEPT for the lua/src/luac/ files.
* The project should be an empty DLL project.  Also, the LUA50_EXPORTS symbol
  must be defined.  If Visual Studio is used, it should automatically be set up
  to define the symbol.
* Have the project build both Debug and Release builds.  The Debug file should
  be named "lua50_d.dll" while the Release file should be named "lua50.dll".
* Place the "lua50_d.lib" and "lua50.lib" files into the lua/lib directory.
