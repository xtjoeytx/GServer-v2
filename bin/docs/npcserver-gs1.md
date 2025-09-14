# GS2Emu GS1 NPC-Server

For generic NPC-Server information, see: [npcserver.md](npcserver.md)

## Generation

The GS1 npc-server has only been tested with the "classic" generation.
It is possible that it will work with "newmain" and "modern", but there are no guarantees at this point.
Buyer beware.

## Considerations

Please review the lists below of implemented events, flags, functions, commands, and message codes.

---
## Events

    * - GR extension (not in official)
        Enable extensions in server options to use.

#### Implemented

      compusdied
      created
      exploded
      initialized
      movementfinished
      npcwarped
      playerchats
      playerdies
      playerenters
      playerlaysitem
      playerleaves
      playerlogin
      playerlogout
    * playertouchesme
    * playertouchesother
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

#### Clientside only

    firedonhorse
    keypressed
    mousedown
    mouseup
    mousewheel
    weaponfired

---
## Flags

    * - GR extension (not in official)
        Enable extensions in server options to use.

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
      shotbybaddy
    * shotbynpc
      shotbyplayer
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

#### Clientside only

    followsplayer
    isfocused
    leftmousebutton
    lighteffectsenabled
    middlemousebutton
    playeronline
    playermap
    playerreading
    rightmousebutton

---
## Variables

    * - GR extension (not in official)
        Enable extensions in server options to use.

#### Implemented

      actionplayer
      allplayers[]
      allplayerscount
      board[]
      levelorgx
      levelorgy
      npcscount
      paramscount
      players[]
      playerscount
      tiles[x,y]      (not gmap supported yet)
      timevar
      timevar2
      tokenscount
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
        .saysnumber
        .lastdead
        .logintime
      players[] shorthand: e.g., playerhearts

    * arrowscount
    * arrows[]
        .x
        .y
        .dx
        .dy
        .dir
        .type
        .from

    * compuscount
    * compus[]
        .x
        .y
        .type
        .dir
        .headdir
        .power
        .mode

    * bombscount
    * bombs[]
        .x
        .y
        .power
        .time

    * itemscount
    * items[]
        .x
        .y
        .type
        .time

    * exploscount
    * explos[]
        .x
        .y
        .power
        .time
        .dir

    * horsescount
    * horses[]
        .x
        .y
        .dir
        .bushes

    signscount
    signs[]
        .x
        .y

#### Not implemented

    * graalversion
      gravity
    * horses[]
        .bombs
        .bombpower
        .type
      nwday
      nwhour
      nwmin
      nwmonth
      nwtime
      nwweek
      nwweekday
      nwyear
    * playerhurtpower
      waterheight

#### Clientside only

    downloadpos
    downloadsize
    mousebuttons
    mousescreenx
    mousescreeny
    mousewheeldelta
    mousex
    mousey
    musicpos
    musiclen
    playerfreezetime
    screenheight
    screenwidth
    selectedweapon

---
## Commands

    * - GR extension (not in official)
        Enable extensions in server options to use.

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
    * carryobject
      changeimgcolors
    * changeimgmode
      changeimgpart
      changeimgvis
      changeimgzoom
      deletestring
      destroy
      detachplayer
      disableweapons
      dontblock
      drawoverplayer
    * drawovertrees
      drawunderplayer
      enableweapons
    * explodebomb
      freezeplayer2
      hide
      hideimg
    * hideimgs
    * hitnpc
      hitobjects
    * hitplayer
    * hurt
      insertstring
      join
    * lay
    * lay2
      message
      move
      noplayeronwall
      putbomb
    * putcomp
      putexplosion
      putexplosion2
    * puthorse
    * putnewcomp
    * putnpc
      putnpc2
    * removebomb
    * removecompus
      removeguild
      removeguildmember
    * removehorse
    * removeitem
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
    * setbody
      setcharani
      setchargender
      setcharprop
      setcoatcolor
      setgender
      setgif
      setgifpart
    * sethead
      setimg
      setimgpart
    * setlevel
      setlevel2
    * setmap
    * setminimap
    * setplayerdir
      setplayerprop
      setpm
      setshape
    * setshield
      setshoecolor
      setshootparams
      setskincolor
      setsleevecolor
      setstring
    * setsword
      shoot
    * shootarrow
    * shootball (gr extension - dir parameter)
    * shootfireball
    * shootfireblast
    * shootnuke
      show
      showani
      showani2
      showcharacter
      showimg
      showimg2
      showpoly
      showpoly2
      showtext
      showtext2
      sleep
    * spyfire
    * take
    * take2
    * takehorse
    * takeplayercarry
    * takeplayerhorse
    * throwcarry
    * timershow
      tokenize
      tokenize2
      triggeraction
      unfreezeplayer
      unset
      warpto

#### Not implemented

      copyflags
      copylevel
      copystrings
      deletelevel
    * hitcompu
      saveinfo
      savelog
      setz
    * showstats
      toweapons
      updateboard
      updateboard2
      updateterrain

#### Clientside only

    addtiledef
    addtiledef2
    blockagainlocal
    callweapon
    disabledefmovement
    disablemap
    disablepause
    disableselectweapons
    dontblocklocal
    drawaslight
    enabledefmovement
    enablefeatures
    enablemap
    enablepause
    enableselectweapons
    explodebomb
    followplayer
    freezeplayer
    hidelocal
    hideplayer
    hidesword
    loadmap
    noplayerkilling
    openurl
    openurl2
    play 
    play2
    playlooped
    putleaps
    putobject
    reflectarrow
    removetiledefs
    replaceani
    resetfocus
    setbackpal
    setcoloreffect
    setcursor 
    setcursor2
    seteffect 
    seteffectmode
    setfocus
    setletters
    setmusicvolume
    setshape2
    setspritesimage
    setstatusimage
    seturllevel
    setzoomeffect
    showfile
    showlocal
    stopmidi
    stopsound
    timereverywhere
    toinventory
    wraptext
    wraptext2

---
## Functions

    * - GR extension (not in official)
        Enable extensions in server options to use.

#### Implemented

      abs(value)
    * aindexof(value, array)
      arctan(value)
      arraylen(array)
    * ascii(string)
      cos(value)
      findnearestplayer(x, y)
      getangle(dx, dy)
    * getareanpcs(x, y, width, height)
    * getdir(dx, dy)
    * getnearestplayer(x, y)
    * getnearestplayers(x, y, flag)
      getnpc(name)
      getplayer(account)
      hasweapon(name)
      indexof(substring, string)
      int(value)
    * keycode(key)
      lindexof(string, list)
    * log(base, value)
    * max(value1, value2)
    * min(value1, value2)
      onmapx(level)
      onmapy(level)
      onwall(x, y)                    (old tileset layout only)
      onwall2(x, y, width, height)    (old tileset layout only)
      onwater(x, y)                   (old tileset layout only)
      onwater2(x, y, width, height)   (old tileset layout only)
    * playersays(index, text)
    * playersays(text)
    * playersays2(index, text)
    * playersays2(text)
      random(min, max)
      sarraylen(list)
      sin(value)
      startswith(prefix, string)
      strcontains(string, substring)
      strequals(string1, string2)
      strlen(string)
      strtofloat(string)
    * testbomb(x, y)
    * testcompu(x, y)
    * testexplo(x, y)
    * testhorse(x, y)
    * testitem(x, y)
      testnpc(x, y)
      testplayer(x, y)
      testsign(x, y)
    * tiletype(x, y)                  (old tileset layout only)
      vecx(dir)
      vecy(dir)

#### Not implemented

    _(string)
    N_(string)
    base64decode(string)
    base64encode(string)
    findnearestplayers(x, y)
    getz(x, y)
    tiletype(x, y) (new tileset layout)

#### Clientside only

    imgheight(file)
    imgwidth(file)
    keydown(key)
    keydown2(keycode, ignorecase)
    screenx(x, y)
    screeny(x, y)
    textheight(zoom, font, style)
    textwidth(zoom, font, style, text)
    worldx(x, y)
    worldy(x, y)

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
