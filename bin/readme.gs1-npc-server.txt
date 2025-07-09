////////////////////////////////////////////////////////////////////////////////
// GS2Emu Beta 4 GS1 NPC-Server                                               //
////////////////////////////////////////////////////////////////////////////////

Oh neat.

//////////////////////////////////////////////////////////////////////
// Super simple idiot-proof setup instructions for existing servers //
//////////////////////////////////////////////////////////////////////

Add the following flags to your serveroptions.txt:
	serverside = true
	generation = classic

Your server will now be running the npc-server and in classic (2.x/3.x) mode.

///////////////////////////////////
// Generations                   //
///////////////////////////////////

Generations is a new concept for the gserver.  Since there have been some big changes over the years,
we have decided to restrict some things under a "generation" option.  This option can alter how data is stored,
how packets are sent to clients, what features are available, and so on.

There are currently four identified generations:
	original - 1.x
	classic  - 2.x/3.x
	newmain  - 4.x to 5.007
	modern   - 5.1+

The GS1 npc-server has only been tested with the "classic" generation.  It is possible that it will work with "newmain"
and "modern", but there are no guarantees at this point.  Buyer beware.

///////////////////////////////////
// Additional options            //
///////////////////////////////////

By default, the npc-server will not send signs and links to the player.  To change that, adjust the following options:
	clientsidesigns = true
	clientsidelinks = true

There is an optimization where script events will only be run on NPCs that test for the event flag.  To turn that off, enable:
	runallscriptevents = true

///////////////////////////////////
// allowedversions.txt           //
///////////////////////////////////

In order to support "generations", allowedversions.txt was altered to accept a new format.

If the file starts with "[generation-range]", then each generation has a list of accept client ranges.
Each generation contains a comma-separated list of allowed versions.  Putting a colon (:) between two versions creates a range.
See the examples already in the file.

///////////////////////////////////
// Considerations                //
///////////////////////////////////

Please review the lists below of available events, flags, functions, commands, and message codes.
Of particular note is that 'sleep' IS NOT SUPPORTED.  Script suspend/resume will require a custom syntax tree
walker to be developed and that time is better spent implementing things and fixing bugs.  Just use 'timeout'.

--------------------------------------------------------------------------------

///////////////////////////////////
// Implemented events            //
///////////////////////////////////

created
exploded
initialized
playerlogin
playerlogout
playerenters
playerleaves
playertouchsme
playertouchesme
playertouchsother
playertouchesother
playerlaysitem
playerchats
playerdies
compusdied
warped
washit	(hitobjects)
npcwarped
timeout
triggeraction events

//-------------------------------//
// Not implemented events        //
//-------------------------------//

playerendreading
playerhurt
weaponfired
firedonhorse
washit	(anything not hitobjects)
wasshot
waspelt
wasthrown
serverlistconnect

///////////////////////////////////
// Implemented flags             //
///////////////////////////////////

canspin
weaponsenabled
playerpaused
playerismale
playerisfemale
playeronhorse
playerattached
isleader
visible
issparringzone
nopkzone
isonmap
compsdead
isweapon

//-------------------------------//
// Not implemented flags         //
//-------------------------------//

carrying
carriesblackstone
carriesbush
carriessign
carriesstone
carriesvase
playerswimming
playertrial
followsplayer
peltwithblackstone
peltwithbush
peltwithnpc
peltwithsign
peltwithstone
peltwithvase
shotbybaddy
shotbyplayer

///////////////////////////////////
// Implemented variables         //
///////////////////////////////////

timevar
timevar2
allplayerscount
allplayers[]
weaponscount
playerscount
players[]
npcscount
board[]
paramscount
levelorigx
levelorigy

npcs[]
    .id
    .x
    .y
    .z
    .width
    .height
    .rupees
    .gralats
    .bombs
    .darts
    .hearts
    .glovepower
    .swordpower
    .shieldpower
    .ap
    .hurtdx
    .hurtdy
    .save[]
    .sprite
    .dir
    .timeout
npcs[] shorthand: e.g., hearts

players[]
    .id
    .x
    .y
    .z
    .rupees
    .gralats
    .bombs
    .darts
    .glovepower
    .swordpower
    .shieldpower
    .mp
    .ap
    .fullhearts
    .hearts
    .headset
    .sprite
    .dir
    .attachid
    .attachtype
    .lastdead
    .logintime
players[] shorthand: e.g., playerhearts

compuscount
compus[]
	.x
	.y
	.type
	.dir
	.headdir
	.power
	.mode

bombscount
bombs[]
	.x
	.y
	.power
	.time

itemscount
items[]
	.x
	.y
	.type
	.time

exploscount
explos[]
	.x
	.y
	.power
	.time
	.dir

horsescount
horses[]
	.x
	.y
	.dir
	.bushes

signscount
signs[]
	.x
	.y

//-------------------------------//
// Not implemented variables     //
//-------------------------------//

arrowscount
arrows[]
horses[]
	.bombs
	.bombpower
	.type
tiles[x,y]
gravity
actionplayer
playerhurtpower
players[]
    .saysnumber

///////////////////////////////////
// Implemented commands          //
///////////////////////////////////

addstring
addweapon
attachplayertoobj
blockagain
callnpc
canbecarried
canbepulled
canbepushed
cannotbecarried
cannotbepulled
cannotbepushed
cannotwarp
canwarp
canwarp2
deletestring
destroy
detachplayer
disableweapons
dontblock
drawoverplayer
drawovertrees
drawunderplayer
enableweapons
freezeplayer2
hide
hitobjects
hitplayer
hurt
insertstring
join
lay
lay2
message
move
noplayeronwall
putbomb
putcomp
putexplosion
putexplosion2
puthorse
putnewcomp
putnpc
putnpc2
removebomb
removecompus
removehorse
removeitem
removestring
removeweapon
replacestring
say
say2
sendpm
sendrpgmessage
sendtonc
sendtorc
serverwarp
set
setani
setarray
setbeltcolor
setbody
setcharani
setchargender
setcharprop
setcoatcolor
setgender
setgif
sethead
setimg
setimgpart
setlevel
setlevel2
setmap
setminimap
setplayerdir
setplayerprop
setshape
setshield
setshoecolor
setshootparams
setskincolor
setsleevecolor
setstring
setsword
shoot (sending projectiles to the client)
show
showcharacter
spyfire
take
take2
takeplayerhorse
tokenize
tokenize2
unfreezeplayer
unset

//-------------------------------//
// Not implemented commands      //
//-------------------------------//

addguildmember
carryobject
copyflags
copylevel
copystrings
deletelevel
explodebomb
hitcompu
hitnpc
removearrow
removeguild
removeguildmember
saveinfo
savelog
savelog2
setpm
setz
shoot (serverside processing of projectiles)
shootarrow
shootball
shootfireball
shootfireblast
shootnuke
showani
showani2
showimg
showimg2
showstats
sleep
takehorse
takeplayercarry
throwcarry
timershow
updateboard
updateboard2
updateterrain

///////////////////////////////////
// Implemented functions         //
///////////////////////////////////

abs(value)
aindexof(value, array)
arctan(value)
arraylen(array)
ascii(string)
cos(value)
findnearestplayer(x, y)
getareanpcs(x, y, width, height)
getdir(dx, dy)
getnearestplayer(x, y)
getnearestplayers(x, y, flag)
getnpc(name)
getplayer(account)
hasweapon(name)
indexof(substring, string)
int(value)
keycode(key)
lindexof(string, list)
log(value)
max(value1, value2)
min(value1, value2)
onmapx(level)
onmapy(level)
onwall(x, y)
onwall2(x, y, width, height)
onwater(x, y)
random(min, max)
sarraylen(list)
sin(value)
startswith(prefix, string)
strcontains(string, substring)
strequals(string1, string2)
strlen(string)
strtofloat(string)
testbomb(x, y)
testcompu(x, y)
testexplo(x, y)
testhorse(x, y)
testitem(x, y)
testnpc(x, y)
testplayer(x, y)
testsign(x, y)
vecx(dir)
vecy(dir)

//-------------------------------//
// Not implemented functions     //
//-------------------------------//

_(string)
N_(string)
base64decode(string)
base64encode(string)
findnearestplayers(x, y)
getangle(dx, dy)
getz(x, y)
imgheight(image)
imgwidth(image)
keydown(key)
keydown2(keycode, ignorecase)
playersays(???)
playersays2(???)
screenx(x, y)
screeny(x, y)
textheight(zoom, font, style)
textwidth(zoom, font, style, text)
tiletype(x, y)
worldx(x, y)
worldy(x, y)

///////////////////////////////////
// Implemented message codes     //
///////////////////////////////////

#1 | #1(index)  [Read / Write]
#2 | #2(index)  [Read / Write]
#3 | #3(index)  [Read / Write]
#5 | #5(index)  [Read / Write]
#6 | #6(index)  [Read]
#7 | #7(index)  [Read / Write]
#8 | #8(index)  [Read / Write]
#a | #a(index)  [Read]
#c | #c(index)  [Read / Write]
#g | #g(index)  [Read]
#m | #m(index)  [Read / Write]
#n | #n(index)  [Read / Write]
#N | #N(index)  [Read]
#W | #W(index)  [Read]
#w | #w(index)  [Read]
#p(index)       [Read / Write]
#t(index)       [Read / Write]
#F              [Read]
#f              [Read]
#L              [Read]
#C0 - #C4  | #C0(index) - #C4(index)   [Read / Write]
#P1 - #P30 | #P1(index) - #P30(index)  [Read / Write]
#b
#e(start_index, length, string)
#I(string_list, index)
#R(string_list)
#s(identifier)
#T(string)
#v(identifier)

//-------------------------------//
// Not implemented message codes //
//-------------------------------//

#Q(guild_name, account_name)  [Read]
#G | #G(index)                [Read]
#D | #D(filename)
#i(image) | #i(image, x, y, width, height)
#K(key_index)
#k(key_index)
