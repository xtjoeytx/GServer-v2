# GS2Emu GS1 NPC-Server

For generic NPC-Server information, see: [npcserver.md](npcserver.md)

## Generation

The GS1 npc-server has only been tested with the "newmain" generation.
It is possible that it will work with "modern", but there are no guarantees at this point.
Buyer beware.

## Considerations

Please review the lists below of implemented events, flags, functions, commands, and message codes.

---
## Events

    *    - Existed on official, but not serverside.
    [GR] - Never existed on official.

#### Implemented

      compusdied
      created
      exploded
      initialized
      itemdrop
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
      rcchat
      timeout
      triggeraction events
      updategani
      washit	(hitobjects)
      wasshot
      wasshooted
      wasthrown

#### Not implemented

    serverlistconnect
    washit	(anything not hitobjects)
    waspelt

#### Clientside only

    firedonhorse
    keypressed
    mousedown
    mouseup
    mousewheel
    playerendsreading
    weaponfired

---
## Flags

    *    - Existed on official, but not serverside.
    [GR] - Never existed on official.

#### Implemented

      canspin                   - ? (beta 7 or 1.32)
      carrying                  - around (1.20)
      carriesblackstone         - (beta 4)
      carriesbush               - (beta 4)
      carriesnpc                - around (1.20)
      carriessign               - (beta 4)
      carriesstone              - (beta 4)
      carriesvase               - (beta 4)
      compsdead                 - (beta 7)
      isleader                  - (1.22)
      isonmap                   - (1.38)
      issparringzone            - (2.02)
      isweapon                  - (1.34)
      nopkzone                  - (2.02)
      playerattached            - (2.04)
      playerisfemale            - (1.36)
      playerismale              - (1.36)
      playeronhorse             - around (1.20)
      playeronline
      playerpaused              - (2.14rev7)
      playerswimming            - around (1.32)
      shotbybaddy               - (beta 7)
    * shotbynpc
      shotbyplayer              - (beta 7)
      visible                   - (beta 5)
      weaponsenabled            - (beta 5)

#### Not implemented

    peltwithblackstone          - (beta 5)
    peltwithbush                - (beta 5)
    peltwithnpc                 - around (1.20)
    peltwithperson              - (beta 7) to around (1.20)
    peltwithsign                - (beta 5)
    peltwithstone               - (beta 5)
    peltwithvase                - (beta 5)
    playerkiller                - (beta 5) to (1.32)
    isinguild <guild>           - (beta 5) to (beta 7)
    playeringuild <guild>       - (beta 7) to around (1.25)

#### Clientside only

    followsplayer               - (beta 7)
    isfocused                   - (2.16rev5)
    leftmousebutton             - (2.14)
    lighteffectsenabled         - (2.03)
    middlemousebutton           - (2.14)
    playermap                   - (2.14rev7)
    playerreading               - (2.03)
    rightmousebutton            - (2.14)

---
## Variables

    *    - Existed on official, but not serverside.
    [GR] - Never existed on official.

#### Implemented

      actionplayer              - (2.03)
      allplayers[]
      allplayerscount
      board[]                   - around (1.20)
      gravity                   - (2.22)
      groundheights[]           - possibly (2.12), revealed (???)
      levelorgx                 - (2.04)
      levelorgy                 - (2.04)
      npcscount                 - (1.38)
      nwday
      nwhour
      nwmin
      nwmonth
      nwtime
      nwweek
      nwweekday
      nwyear
      paramscount               - (2.03)
      playerscount              - around (1.20)
      tiles[x,y]    (no negative indexes)   - (2.10)
      timevar                   - (2.10)
      timevar2                  - (2.30)
      tokenscount               - (2.02)
      weaponscount              - (2.04)

      npcs[]                    - (1.38)
        .ap                     - (1.30)
        .bombs                  - (beta 5)
        .darts                  - (beta 5)
        .dir                    - (1.25)
        .glovepower             - around (1.20)
        .gralats                - sometime in (2.0x) or (2.1x)
      * .headset
        .hearts                 - (beta 5)
        .height                 - (1.38)
        .hp                     - possibly (2.12), revealed (???)
        .hurtdx                 - (1.27)
        .hurtdy                 - (1.27)
      * .hurtpower
        .id                     - (1.38)
        .rupees                 - (beta 5)
        .save[]                 - (1.27)
        .shieldpower            - around (1.20)
        .sprite                 - (1.25)
        .swordpower             - around (1.20)
        .timeout                - (beta 5)
        .width                  - (1.38)
        .x                      - (beta 5)
        .y                      - (beta 5)
        .z                      - possibly (2.12), revealed (???)
      npcs[] shorthand: e.g., hearts

      players[]                 - around (1.20)
        .ap                     - (1.30)
        .attachid               - (2.04)
        .attachtype             - (2.04)
        .bombs                  - (beta 5)
        .bombpower              - (beta 5) to ()
      * .carrysprite
        .darts                  - (beta 5)
        .deaths                 - possibly (1.39rev1)
        .dir                    - (beta 5)
        .fullhearts             - (beta 5)
        .glovepower             - (beta 5)
        .gralats                - sometime in (2.0x) or (2.1x)
        .headset                - (beta 5)
        .hearts                 - (beta 5)
        .hp                     - possibly (2.12), revealed (???)
        .hurtdx                 - (2.00)
        .hurtdy                 - (2.00)
        .hurtpower              - (2.01)
        .id                     - around (1.20)
        .kills                  - possibly (1.39rev1)
        .lastdead
        .logintime
        .maxhp                  - possibly (2.12), revealed (???)
        .mp                     - (1.22)
        .rating
        .ratingd
        .rupees                 - (beta 5)
        .saysnumber             - (1.21)
        .shieldpower            - (beta 5)
        .shootpower             - (beta 5)
        .sprite                 - (1.25)
        .swordpower             - (beta 5)
        .trial
        .x                      - (beta 5)
        .y                      - (beta 5)
        .z                      - possibly (2.12), revealed (???)
      players[] shorthand: e.g., playerhearts

    * arrowscount               - (1.36)
    * arrows[]                  - (1.36)
        .x                      - (1.36)
        .y                      - (1.36)
        .dx                     - (1.36)
        .dy                     - (1.36)
        .dir                    - (1.36)
        .type                   - (1.36)
        .from                   - (1.36)

    * compuscount               - around (1.20)
    * compus[]                  - around (1.20)
        .x                      - around (1.20)
        .y                      - around (1.20)
        .type                   - around (1.20)
        .dir                    - around (1.20)
        .headdir                - around (1.20)
        .power                  - around (1.20)
        .mode                   - around (1.20)

    * bombscount                - (1.36)
    * bombs[]                   - (1.36)
        .x                      - (1.36)
        .y                      - (1.36)
        .power                  - (1.36)
        .time                   - (1.36)

    * itemscount                - (1.36)
    * items[]                   - (1.36)
        .x                      - (1.36)
        .y                      - (1.36)
        .type                   - (1.36)
        .time                   - (1.36)

    * exploscount               - (1.36)
    * explos[]                  - (1.36)
        .x                      - (1.36)
        .y                      - (1.36)
        .power                  - (1.36)
        .time                   - (1.36)
        .dir                    - (1.36)

    * horsescount               - (1.36)
    * horses[]                  - (1.36)
        .x                      - (1.36)
        .y                      - (1.36)
        .dir                    - (1.36)
        .bushes                 - (1.36)
        .bombs          (always 0, client never sends)  - (1.36)
        .bombpower      (always 0, client never sends)  - (1.36)
        .type                   - (1.36)

      signscount                - (1.39rev2)
      signs[]                   - (1.39rev2)
        .x                      - (1.39rev2)
        .y                      - (1.39rev2)

#### Not implemented

    tiles[x,y]          (negative indexes)

#### Clientside only

    downloadpos                 - (2.14)
    downloadsize                - (2.22)
    focusx                      - possibly (2.12), revealed (2.16)
    focusy                      - possibly (2.12), revealed (2.16)
    graalversion                - (2.10)
    mousebuttons                - (2.14)
    mousescreenx                - (2.14)
    mousescreeny                - (2.14)
    mousewheeldelta             - (2.14)
    mousex                      - (2.14)
    mousey                      - (2.14)
    musicpos                    - (2.16)
    musiclen                    - (2.16)
    playerfreezetime            - possibly around (1.20)
    screenheight                - (1.41)
    screenwidth                 - (1.41)
    selectedsword               - (newworld) (maybe 2.12)
    selectedweapon              - (2.04)
    waterheight                 - possibly (2.12), revealed (???)
    npcs[].anistep              - possibly (2.12), revealed (2.14)
    players[].anistep           - possibly (2.12), revealed (2.14)

---
## Commands

    *    - Existed on official, but not serverside.
    [GR] - Never existed on official.

#### Implemented

      addguildmember
      addstring                 - (2.10)
      addweapon
      attachplayertoobj         - (2.04)
      blockagain                - (beta 5)
      callnpc                   - (1.39rev2)
      canbecarried              - (beta 7)
      canbepulled               - (1.10)
      canbepushed               - (1.10)
      cannotbecarried           - (beta 7)
      cannotbepulled            - (1.10)
      cannotbepushed            - (1.10)
      cannotwarp
      canwarp
      canwarp2
    * carryobject               - (1.37)
      changeimgcolors           - (2.00)
    * changeimgmode             - (2.16)
      changeimgpart             - (1.38)
      changeimgvis              - (1.40)
      changeimgzoom             - (2.00)
      copylevel
      copystrings
      deletelevel
      deletestring              - (2.10)
      destroy                   - around (1.20)
      detachplayer              - (2.04)
    * disabledamagereactions    [GR]
      disableweapons            - (beta 5)
      dontblock                 - (beta 5)
      drawoverplayer            - (beta 5)
    * drawovertrees             - (newworld)
      drawunderplayer           - (beta 5)
    * enabledamagereactions     [GR]
      enableweapons             - (beta 5)
    * explodebomb               - (1.36)
      freezeplayer2
      hide                      - (beta 2)
      hideimg                   - around (1.20)
    * hideimgs                  - (2.17)
    * hitnpc                    - (1.38)
      hitobjects                - (2.01)
    * hitplayer                 - around (1.20)
    * hurt                      - (beta 2)
      insertstring              - (2.10)
      join
    * lay                       - (beta 2)
    * lay2                      - (1.36)
      message                   - (1.10)
      move                      - (2.03)
      noplayeronwall
      putbomb                   - (beta 5)
    * putcomp                   - (beta 4)
      putexplosion              - (beta 5)
      putexplosion2             - (1.40)
    * puthorse                  - (beta 7)
    * putnewcomp                - around (1.20)
    * putnpc                    - around (1.20)
      putnpc2
    * removearrow               - (1.36)
    * removebomb                - (1.36)
    * removecompus              - (beta 3)
    * removeexplo               - (1.36)
      removeguild
      removeguildmember
    * removehorse               - (1.36)
    * removeitem                - (1.36)
      removestring              - (2.10)
      removeweapon
      replacestring             - (2.10)
      savelog                   - possibly (2.12), revealed (???)
      savelog2
      say                       - (beta 2)
      say2                      - (2.16)
      sendpm
      sendrpgmessage
      sendtonc
      sendtorc
      serverwarp                - (2.14)
      set                       - (beta 2)
      setani                    - (2.00)
      setarray                  - around (1.20)
      setbeltcolor              - (beta 3)
    * setbody                   - (1.40)
      setcharani                - (2.00)
      setchargender             - (2.00)
      setcharprop               - (1.25)
      setcoatcolor              - (beta 3)
      setgender                 - (2.00)
      setgif                    - (beta 2)
      setgifpart                - (1.30)
    * sethead                   - around (1.20)
      setimg                    - (1.40)
      setimgpart                - (1.40)
    * setlevel                  - (beta 3)
      setlevel2                 - (1.38)
    * setmap                    - (1.23)
    * setminimap                - (1.23)
    * setplayerdir              - (beta 3)
      setplayerprop             - (1.25)
      setpm
      setshape                  - (2.04)
    * setshield                 - around (1.20)
      setshoecolor              - (beta 3)
      setshootparams            - revealed (2.14)
      setskincolor              - (beta 3)
      setsleevecolor            - (beta 3)
      setstring                 - (1.27)
    * setsword                  - around (1.20)
      shoot                     - revealed (2.14)
    * shootarrow                - (beta 5)
    * shootball                 - (beta 5) (gr extension - dir parameter)
    * shootfireball             - (beta 9)
    * shootfireblast            - (beta 9)
    * shootnuke                 - (beta 9)
      show                      - (beta 2)
      showani                   - (2.16)
      showani2                  - (2.16)
      showcharacter             - (1.25)
      showimg                   - around (1.20)
      showimg2                  - (2.15)
      showpoly                  - (2.16)
      showpoly2                 - (2.16)
      showtext                  - (2.16)
      showtext2                 - (2.16)
      sleep                     - (1.22)
    * spyfire                   - sometime in (2.0x)
    * take                      - (beta 2)
    * take2                     - (1.36)
    * takehorse                 - (1.36)
    * takeplayercarry           - (beta 4)
    * takeplayerhorse           - (1.36)
    * throwcarry                - (1.37)
    * timershow                 - (beta 5)
      tokenize                  - (2.02)
      tokenize2                 - (2.02)
      toweapons                 - (1.20)
      triggeraction             - (2.03)
      unfreezeplayer
      unset                     - (beta 2)
      updateboard               - around (1.20)
      updateboard2              - (2.20)
      warpto

#### Not implemented

    hitcompu                    - around (1.20)
    savelevel           [GR]
    setbow                      - around (1.20) to (2.00)
    setx                        - (beta 2) to (beta 5)
    sety                        - (beta 2) to (beta 5)
    setplayerx                  - (beta 3) to (beta 5)
    setplayery                  - (beta 3) to (beta 5)
    reducerupees                - (beta 3) to (beta 5)
    reducebombs                 - (beta 3) to (beta 5)
    reducedarts                 - (beta 3) to (beta 5)

#### Clientside only

    addtiledef                  - (2.04)
    addtiledef2                 - (2.04)
    blockagainlocal             - (1.34)
    callweapon                  - possibly (2.04)
    disabledefmovement          - revealed (2.02)
    disablemap                  - (2.14rev7)
    disablepause                - (2.14rev7)
    disableselectweapons        - (2.04)
    dontblocklocal              - (1.34)
    drawaslight                 - (2.00)
    enabledefmovement           - revealed (2.02)
    enablefeatures              - (2.16)
    enablemap                   - (2.14rev7)
    enablepause                 - (2.14rev7)
    enableselectweapons         - (2.04)
    followplayer                - (beta 5)
    freezeplayer                - around (1.20)
    hidelocal                   - (1.34)
    hideplayer                  - (1.22)
    hidesword                   - around (1.20)
    loadmap                     - possibly (2.12), revealed (???)
    noplayerkilling             - (1.00)
    openurl                     - (beta 5)
    openurl2                    - (2.00)
    play                        - (beta 2)
    play2                       - (2.10)
    playlooped                  - (1.37)
    putleaps                    - (beta 5)
    putobject                   - around (1.20) to (???)
    reflectarrow                - (1.36)
    removetiledefs              - (2.04)
    replaceani                  - (2.04)
    resetfocus                  - possibly (2.12), revealed (2.16)
    setbackpal                  - (1.27)
    setbacktile                 - (newworld)
    setbacktile2                - (newworld)
    setcoloreffect              - (2.00)
    setcursor                   - possibly (2.12), revealed (???)
    setcursor2                  - possibly (2.12), revealed (???)
    seteffect                   - around (1.20) to (1.33rev1)
    seteffect                   - (2.00)
    seteffectmode               - (2.16)
    setfocus                    - possibly (2.12), revealed (2.16)
    setletters                  - (1.27)
    setmusicvolume              - (2.00)
    setshape2                   - (2.04)
    setspritesimage             - (5.00rev6)
    setstatusimage              - (5.00rev6)
    seturllevel                 - (beta 5)
    setzoomeffect               - (2.00)
    setz                        - possibly (2.12), revealed (???)
    showfile                    - (1.26rev2)
    showlocal                   - (1.34)
    showstats                   - (1.41)
    stopmidi                    - (beta 5)
    stopsound                   - (1.37)
    timereverywhere             - (1.22)
    toinventory                 - (beta 5)
    unseteffect                 - around (1.20) to (1.33rev1)
    updateterrain               - possibly (2.17), revealed (???)
    wraptext                    - (2.12)
    wraptext2                   - (2.12)

#### Won't implement

    * saveinfo      (internal command to control an official database, won't be implemented)

---
## Functions

    *    - Existed on official, but not serverside.
    [GR] - Never existed on official.

#### Implemented

      abs(value)                        - around (1.20)
    * aindexof(value, array)            - (2.16)
      arctan(value)                     - around (1.20)
      arraylen(array)                   - around (1.20)
    * ascii(string)                     - (2.10)
      base64decode(string)
      base64encode(string)
      cos(value)                        - around (1.20)
      exp(value)
      findnearestplayer(x, y)           - (5.00rev6)
      getangle(dx, dy)                  - (2.10)
      getareanpcs(x, y, width, height)
    * getdir(dx, dy)                    - (2.16)
      getflagkeys(prefix)               - possibly (2.12)
      getnearestplayer(x, y)            - (5.00rev6)
      getnearestplayers(x, y, flag)     - (5.00rev6)
      getnpc(name)
      getplayer(account)
      getz(x, y)                        - possibly (2.12), revealed (???)
      hasweapon(name)                   - (1.37)
      indexof(substring, string)        - (2.02)
      int(value)                        - around (1.20)
    * keycode(key)                      - (2.14)
      lindexof(string, list)            - (2.10)
    * log(base, value)                  - (2.16)
    * max(value1, value2)               - (2.16)
    * min(value1, value2)               - (2.16)
      onmapx(level)                     - (2.03)
      onmapy(level)                     - (2.03)
      onwall(x, y)                      - (beta 5)
      onwall2(x, y, width, height)      - (2.30)
      onwater(x, y)                     - (1.38)
      onwater2(x, y, width, height)     - (2.30)
      passwordmatches(encrypted, test)
    * playersays(index, text)           - (1.21)
    * playersays(text)                  - (1.21)
    * playersays2(index, text)          - (1.21)
    * playersays2(text)                 - (1.21)
      random(min, max)                  - (beta 5)
      sarraylen(list)                   - (2.10)
      sin(value)                        - around (1.20)
      startswith(prefix, string)        - (2.02)
      strcontains(string, substring)    - (1.27)
      strequals(string1, string2)       - (1.24)
      strlen(string)                    - (2.02)
      strtofloat(string)                - (1.27)
    * testbomb(x, y)                    - (1.38)
    * testcompu(x, y)                   - (1.38)
    * testexplo(x, y)                   - (1.38)
    * testhorse(x, y)                   - (1.38)
    * testitem(x, y)                    - (1.38)
      testnpc(x, y)                     - (1.38)
      testplayer(x, y)                  - (1.38)
      testsign(x, y)                    - (1.39rev2)
    * tiletype(x, y)                    - possibly (2.12), revealed (???)
      vecx(dir)                         - (2.03)
      vecy(dir)                         - (2.03)

#### Not implemented

    hasright(rw,path)

#### Clientside only

    imgheight(file)                     - (2.13)
    imgwidth(file)                      - (2.13)
    keydown(key)                        - (1.39rev2)
    keydown2(keycode, ignorecase)       - (2.14)
    screenx(x, y)                       - (2.16)
    screeny(x, y)                       - (2.16)
    textheight(zoom, font, style)       - (2.20)
    textwidth(zoom, font, style, text)  - (2.19)
    worldx(x, y)                        - (2.16)
    worldy(x, y)                        - (2.16)

---
## Message codes

#### Implemented

    #1 | #1(index)  [RW]    - Sword image                       - (1.24-1.25)
    #2 | #2(index)  [RW]    - Shield image                      - (1.24-1.25)
    #3 | #3(index)  [RW]    - Head image                        - (1.24-1.25)
    #5 | #5(index)  [RW]    - Horse image                       - (1.24-1.25)
    #6 | #6(index)  [R]     - Carried NPC image                 - (1.24-1.25)
    #7 | #7(index)  [RW]    - Bow image (1.x)                   - (1.24-1.25) to (2.00)
    #8 | #8(index)  [RW]    - Body image (2.x+)                 - readable around (1.34), writeable (1.40)
    #a | #a(index)  [R]     - Player account name               - (1.30)
    #c | #c(index)  [RW]    - Chat text                         - (1.24-1.25)
    #g | #g(index)  [R]     - Guild name                        - (1.24-1.25)
    #m | #m(index)  [RW]    - Animation                         - (2.00)
    #n | #n(index)  [RW]    - Nickname                          - (1.24-1.25)
    #N | #N(index)  [R]     - Database NPC name
    #f | #f(index)  [R]     - NPC image                         - non-indexed (1.24-1.25), indexed (2.12)
    #W(index)       [R]     - Weapon image                      - (2.04)
    #w(index)       [R]     - Weapon name                       - (2.04)
    #p(index)       [RW]    - Action parameter (triggeraction)  - (2.03)
    #t(index)       [RW]    - Token (tokenize)                  - (2.02)
    #F              [R]     - Level of the player
    #L              [R]     - Level of the source NPC           - (1.24)
    #C0 - #C4(index)  [RW]  - Body colors                       - (1.24-1.25)
    #C5 - #C7(index)  [RW]  - Newworld body colors (NW only)    - (newworld)
    #P1 - #P30(index) [RW]  - Gani attributes                   - [1-5] (2.02), [6-9] (2.13), [10-30] (2.16)
    #Q(guild_name, account_name)  [R]  - Nickname of a guild member
    #G | #G(index)                [R]  - Player's account level (e.g., gold, classic, trial, etc.)
    #e(start_index, length, string)    - Extracts a substring from the given string         - (2.02)
    #E(string)              - Password hashes the given string
    #I(string_list, index)  - Returns the string at the given index from the string list    - (2.10)
    #K(ascii_number)        - The character represented by the given ASCII code             - (2.00)
    #R(string_list)         - Randomly selects a string from the given string list          - (2.19)
    #s(identifier)          - The string value of a variable                                - (1.27)
    #T(string)              - Trims the string                                              - (2.02)
    #U(string)              - Replaces the string with a translated version of it
    #v(identifier)          - The value of an number variable as a string                   - (1.24-1.25)
    #b                      - Line break                                                    - (2.16)

#### Clientside only

    #D | #D(filename)       - Current file being downloaded | The download position of the specified file       - (2.14)
    #E                      - The current emoticon character being displayed by the player
    #i(image) | #i(image, x, y, width, height)  - Displays an image or part of an image when used in a sign     - (2.02)
    #k(key_index)           - The description of the specified key (in client language/key assignments)
    #W                      - The image of the player's currently selected weapon                               - possibly (2.04)
    #w                      - The name of the player's currently selected weapon                                - (1.24-1.25)
    #S                      - The player's currently selected sword                                             - (newworld)
