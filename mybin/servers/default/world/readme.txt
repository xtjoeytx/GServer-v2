All levels, images, sounds, scripts, etc go in this folder or any sub folder.
If you create a sub folder while the gserver is running, you must restart the gserver or type
/cachesubdirs in RC for the newly created sub directory to be included.

**** NEW IN BUILD 56 (MUST READ) ****

- Folder Configuration is now implemented.  See the foldersconfig.txt
  file for a list of default options.  The folder configuration is
  mainly used to restrict files to certain directories, like
  restricting head images to world/heads/*.  This will prevent players
  from being able to do odd things like 'sethead pics1.png'.
  PLEASE MAKE SURE YOUR FOLDERSCONFIG.TXT FILE IS CORRECT BEFORE YOU
  SEEK HELP!  IF YOUR FOLDERSCONFIG.TXT FILE IS INCORRECT, IT WILL
  CAUSE MANY THINGS TO TO BREAK.


Here is default folder config in case you've lost it:
# Folder Configuration
body    bodies/*.png
head    heads/*
sword   swords/*
shield  shields/*
level   *.graal
level   *.nw
level   *.gmap
file    *.png
file    *.mng
file    *.gif
file    *.gani
file    *.wav
file    *.txt
file    *.gmap
file    images/*.png
file    images/*.gif
file    images/*.mng
