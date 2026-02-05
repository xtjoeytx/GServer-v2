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
      playerhurt
      playerhurted
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
      wasshooted

#### Not implemented

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
      carrying
      carriesblackstone
      carriesbush
      carriesnpc
      carriessign
      carriesstone
      carriesvase
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
      playeronline
      playerpaused
      playerswimming
      shotbybaddy
    * shotbynpc
      shotbyplayer
      visible
      weaponsenabled

#### Not implemented

    peltwithblackstone
    peltwithbush
    peltwithnpc
    peltwithsign
    peltwithstone
    peltwithvase
    playertrial

#### Clientside only

    followsplayer
    isfocused
    leftmousebutton
    lighteffectsenabled
    middlemousebutton
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
      gravity
      groundheights[]
      levelorgx
      levelorgy
      npcscount
      nwday
      nwhour
      nwmin
      nwmonth
      nwtime
      nwweek
      nwweekday
      nwyear
      paramscount
      players[]
      playerscount
      tiles[x,y]
      timevar
      timevar2
      tokenscount
      weaponscount

      npcs[]
        .ap
        .bombs
        .darts
        .dir
        .glovepower
        .gralats
        .headset
        .hearts
        .height
        .hp
        .hurtdx
        .hurtdy
      * .hurtpower
        .id
        .rupees
        .save[]
        .shieldpower
        .sprite
        .swordpower
        .timeout
        .width
        .x
        .y
        .z
      npcs[] shorthand: e.g., hearts

      players[]
        .ap
        .attachid
        .attachtype
        .bombs
      * .carrysprite
        .darts
        .dir
        .fullhearts
        .glovepower
        .gralats
        .headset
        .hearts
        .hp
        .hurtpower
        .id
        .lastdead
        .logintime
        .maxhp
        .mp
        .rupees
        .saysnumber
        .shieldpower
        .sprite
        .swordpower
        .upgradestatus
        .x
        .y
        .z
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
        .bombs          (always 0, client never sends)
        .bombpower      (always 0, client never sends)
        .type

      signscount
      signs[]
        .x
        .y

#### Not implemented

    players[]
        .shootpower

#### Clientside only

    downloadpos
    downloadsize
    focusx
    focusy
    graalversion
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
    selectedsword
    selectedweapon
    waterheight

    npcs[].anistep

    players[]
        .anistep

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
      copylevel
      copystrings
      deletelevel
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
      toweapons
      triggeraction
      unfreezeplayer
      unset
      updateboard
      updateboard2
      warpto

#### Not implemented

    * hitcompu
      saveinfo
    * savelevel
      savelog
    * showstats

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
    setbacktile
    setbacktile2
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
    setz
    showfile
    showlocal
    stopmidi
    stopsound
    timereverywhere
    toinventory
    updateterrain
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
      base64decode(string)
      base64encode(string)
      cos(value)
      findnearestplayer(x, y)
      getangle(dx, dy)
    * getareanpcs(x, y, width, height)
    * getdir(dx, dy)
    * getnearestplayer(x, y)
    * getnearestplayers(x, y, flag)
      getnpc(name)
      getplayer(account)
      getz(x, y)
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
      onwall(x, y)
      onwall2(x, y, width, height)
      onwater(x, y)
      onwater2(x, y, width, height)
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
    * tiletype(x, y)
      vecx(dir)
      vecy(dir)

#### Not implemented

    exp(r,r)
    findnearestplayers(x, y)
    getflagkeys()

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
    #f | #f(index)  [Read]
    #p(index)       [Read / Write]
    #t(index)       [Read / Write]
    #F              [Read]
    #L              [Read]
    #C0 - #C4  | #C0(index) - #C4(index)   [Read / Write]
    #P1 - #P30 | #P1(index) - #P30(index)  [Read / Write]
    #Q(guild_name, account_name)  [Read]
    #G | #G(index)                [Read]
    #b
    #e(start_index, length, string)
    #I(string_list, index)
    #K(ascii_number)
    #R(string_list)
    #s(identifier)
    #T(string)
    #U(string)
    #v(identifier)

#### Not implemented

    #C5 - #C7 | #C5(index) - #C7(index)

#### Clientside only

    #D | #D(filename)
    #E
    #i(image) | #i(image, x, y, width, height)
    #k(key_index)
