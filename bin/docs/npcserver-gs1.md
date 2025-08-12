# GS2Emu GS1 NPC-Server

For generic NPC-Server information, see: [npcserver.md](npcserver.md)

## Generation

The GS1 npc-server has only been tested with the "classic" generation.
It is possible that it will work with "newmain" and "modern", but there are no guarantees at this point.
Buyer beware.

## Considerations

Please review the lists below of implemented events, flags, functions, commands, and message codes.
Of particular note is that `sleep` IS NOT SUPPORTED.
Script suspend/resume will require a custom syntax tree walker to be developed and that time is better spent implementing things and fixing bugs.
Just use `timeout`.

---
## Events

#### Implemented

    compusdied
    created
    exploded
    initialized
    npcwarped
    playerchats
    playerdies
    playerenters
    playerlaysitem
    playerleaves
    playerlogin
    playerlogout
    playertouchesme
    playertouchesother
    playertouchsme
    playertouchsother
    pm
    timeout
    triggeraction events
    warped
    washit	(hitobjects)
    wasshot

#### Not implemented

    playerhurt
    serverlistconnect
    washit	(anything not hitobjects)
    waspelt
    wasthrown

---
## Flags

#### Implemented

    canspin
    compsdead
    isleader
    isonmap
    issparringzone
    isweapon
    nopkzone
    playerattached
    playerisfemale
    playerismale
    playeronhorse
    playerpaused
    visible
    weaponsenabled

#### Not implemented

    carriesblackstone
    carriesbush
    carriessign
    carriesstone
    carriesvase
    carrying
    peltwithblackstone
    peltwithbush
    peltwithnpc
    peltwithsign
    peltwithstone
    peltwithvase
    playerswimming
    playertrial
    shotbybaddy
    shotbyplayer

---
## Variables

#### Implemented

    actionplayer
    allplayers[]
    allplayerscount
    board[]
    levelorigx
    levelorigy
    npcscount
    paramscount
    players[]
    playerscount
    tiles[x,y]
    timevar
    timevar2
    weaponscount

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

    arrowscount
    arrows[]
        .x
        .y
        .dx
        .dy
        .dir
        .type
        .from

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

#### Not implemented

    horses[]
        .bombs
        .bombpower
        .type
    gravity
    waterheight
    playerhurtpower
    players[]
        .saysnumber

---
## Commands

#### Implemented

    addguildmember
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
    explodebomb
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
    removeguild
    removeguildmember
    removehorse
    removeitem
    removestring
    removeweapon
    replacestring
    savelog2
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
    setpm
    setshape
    setshield
    setshoecolor
    setshootparams
    setskincolor
    setsleevecolor
    setstring
    setsword
    shoot
    shootarrow
    shootball (gr extension - dir parameter)
    shootfireball
    shootfireblast
    shootnuke
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
    warpto

#### Not implemented

    carryobject
    copyflags
    copylevel
    copystrings
    deletelevel
    hitcompu
    hitnpc
    saveinfo
    savelog
    setz
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

---
## Functions

#### Implemented

    abs(value)
    aindexof(value, array)
    arctan(value)
    arraylen(array)
    ascii(string)
    cos(value)
    findnearestplayer(x, y)
    getangle(dx, dy)
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
    onwall(x, y)                    (old tileset layout only)
    onwall2(x, y, width, height)    (old tileset layout only)
    onwater(x, y)                   (old tileset layout only)
    onwater2(x, y, width, height)   (old tileset layout only)
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
    tiletype(x, y)                  (old tileset layout only)
    vecx(dir)
    vecy(dir)

#### Not implemented

    _(string)
    N_(string)
    base64decode(string)
    base64encode(string)
    findnearestplayers(x, y)
    getz(x, y)
    playersays(???)
    playersays2(???)
    tiletype(x, y) (new tileset layout)

---
## Message codes

#### Implemented

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
    #Q(guild_name, account_name)  [Read]
    #G | #G(index)                [Read]
    #b
    #e(start_index, length, string)
    #I(string_list, index)
    #R(string_list)
    #s(identifier)
    #T(string)
    #v(identifier)

#### Clientside only

    #D | #D(filename)
    #i(image) | #i(image, x, y, width, height)
    #K(key_index)
    #k(key_index)
