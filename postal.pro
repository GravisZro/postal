QT=

DEFINES+="LOCALE=US"
DEFINES+="TARGET=POSTAL_1997"
DEFINES+="ALLOW_TWINSTICK"
#DEFINES+="ALLOW_JOYSTICK"
DEFINES+="DEBUG"
DEFINES+="_DEBUG"
DEFINES+="VERBOSE"
#DEFINES+="RELEASE"
#DEFINES+="FILE_VERBOSE"
DEFINES+="HIGH_SCORE_DLG"

DEFINES+="MULTIPLAYER_REMOVED"
#DEFINES+="MULTIPLAYER_DISABLED"
#DEFINES+="EDITOR_REMOVED"
#DEFINES+="LOADLEVEL_REMOVED"

#DEFINES+="STRICT"
#DEFINES+="PENDANT"

INCLUDEPATH += RSPiX
INCLUDEPATH += WishPiX
INCLUDEPATH += newpix


DISTFILES += \
    makeit.sh \
    build.sh \
    Makefile.gravis \
    postal1.sh \
    BLiT.vcproj \
    BLiT3D.vcproj \
    Blue Audio.vcproj \
    Blue Video.vcproj \
    Blue.vcproj \
    Cyan.vcproj \
    Green.vcproj \
    Japan Add On.vcproj \
    Orange.vcproj \
    Postal Common.vcproj \
    Postal Plus Demo.vcproj \
    Postal Plus MP.vcproj \
    Postal Plus.vcproj \
    Super Postal.vcproj \
    WishPiX.vcproj \
    BLiT.vcxproj \
    BLiT.vcxproj.filters \
    BLiT3D.vcxproj \
    BLiT3D.vcxproj.filters \
    Blue.vcxproj \
    Blue.vcxproj.filters \
    Cyan.vcxproj \
    Green.vcxproj \
    Green.vcxproj.filters \
    Orange.vcxproj \
    Orange.vcxproj.filters \
    Postal Common.vcxproj \
    Postal Common.vcxproj.filters \
    Postal Overview.txt \
    Postal Plus Demo.txt \
    Postal Plus.txt \
    Postal Plus.vcxproj \
    Postal Plus.vcxproj.filters \
    Postal.sln \
    WishPiX.vcxproj \
    WishPiX.vcxproj.filters

HEADERS += \
    res/resource.h \
    RSPiX/RSPiX.h \
    RSPiX/CompileOptions.h \
    RSPiX/BLUE/Blue.h \
    RSPiX/BLUE/BlueKeys.h \
    RSPiX/BLUE/System.h \
    RSPiX/BLUE/portable_endian.h \
    RSPiX/BLUE/stdint_msvc.h \
    RSPiX/CYAN/Cyan.h \
    RSPiX/GREEN/3D/pipeline.h \
    RSPiX/GREEN/3D/render.h \
    RSPiX/GREEN/3D/user3d.h \
    RSPiX/GREEN/3D/zbuffer.h \
    RSPiX/GREEN/Blit/_BlitInt.h \
    RSPiX/GREEN/Blit/AlphaBlit.h \
    RSPiX/GREEN/Blit/Blit.h \
    RSPiX/GREEN/Blit/Cfnt.h \
    RSPiX/GREEN/Blit/RPrint.h \
    RSPiX/GREEN/Hot/hot.h \
    RSPiX/GREEN/Image/Image.h \
    RSPiX/GREEN/Image/ImageAfp.h \
    RSPiX/GREEN/Image/ImageCon.h \
    RSPiX/GREEN/Image/ImageFile.h \
    RSPiX/GREEN/Image/ImageType.h \
    RSPiX/GREEN/Image/Pal.h \
    RSPiX/GREEN/Image/PalFile.h \
    RSPiX/GREEN/Image/SpecialType.h \
    RSPiX/GREEN/InputEvent/InputEvent.h \
    RSPiX/GREEN/Mix/mix.h \
    RSPiX/GREEN/Mix/MixBuf.h \
    RSPiX/GREEN/Sample/sample.h \
    RSPiX/GREEN/Snd/snd.h \
    RSPiX/GREEN/SndFx/SndFx.h \
    RSPiX/ORANGE/Attribute/attribute.h \
    RSPiX/ORANGE/CDT/advqueue.h \
    RSPiX/ORANGE/CDT/flist.h \
    RSPiX/ORANGE/CDT/fqueue.h \
    RSPiX/ORANGE/CDT/List.h \
    RSPiX/ORANGE/CDT/listbase.h \
    RSPiX/ORANGE/CDT/pixel.h \
    RSPiX/ORANGE/CDT/PQueue.h \
    RSPiX/ORANGE/CDT/Queue.h \
    RSPiX/ORANGE/CDT/slist.h \
    RSPiX/ORANGE/CDT/smrtarry.h \
    RSPiX/ORANGE/CDT/stack.h \
    RSPiX/ORANGE/Chips/chip.h \
    RSPiX/ORANGE/color/colormatch.h \
    RSPiX/ORANGE/color/dithermatch.h \
    RSPiX/ORANGE/Debug/profile.h \
    RSPiX/ORANGE/DirtRect/DirtRect.h \
    RSPiX/ORANGE/DynaLink/dynalink.h \
    RSPiX/ORANGE/File/file.h \
    RSPiX/ORANGE/GameLib/AnimSprite.h \
    RSPiX/ORANGE/GameLib/Pal.h \
    RSPiX/ORANGE/GameLib/Region.h \
    RSPiX/ORANGE/GameLib/Shapes.h \
    RSPiX/ORANGE/GameLib/Sprite.h \
    RSPiX/ORANGE/GUI/btn.h \
    RSPiX/ORANGE/GUI/dlg.h \
    RSPiX/ORANGE/GUI/edit.h \
    RSPiX/ORANGE/GUI/Frame.h \
    RSPiX/ORANGE/GUI/guiItem.h \
    RSPiX/ORANGE/GUI/ListBox.h \
    RSPiX/ORANGE/GUI/ListContents.h \
    RSPiX/ORANGE/GUI/MultiBtn.h \
    RSPiX/ORANGE/GUI/ProcessGui.h \
    RSPiX/ORANGE/GUI/PushBtn.h \
    RSPiX/ORANGE/GUI/scrollbar.h \
    RSPiX/ORANGE/GUI/txt.h \
    RSPiX/ORANGE/IFF/iff.h \
    RSPiX/ORANGE/ImageTools/lasso.h \
    RSPiX/ORANGE/Laymage/laymage.h \
    RSPiX/ORANGE/Meter/meter.h \
    RSPiX/ORANGE/MsgBox/MsgBox.h \
    RSPiX/ORANGE/MultiGrid/MultiGrid.h \
    RSPiX/ORANGE/MultiGrid/MultiGridIndirect.h \
    RSPiX/ORANGE/Parse/SimpleBatch.h \
    RSPiX/ORANGE/Props/Props.h \
    RSPiX/ORANGE/QuickMath/FixedPoint.h \
    RSPiX/ORANGE/QuickMath/Fractions.h \
    RSPiX/ORANGE/QuickMath/QuickMath.h \
    RSPiX/ORANGE/QuickMath/VectorMath.h \
    RSPiX/ORANGE/RString/rstring.h \
    RSPiX/ORANGE/str/str.h \
    WishPiX/Menu/menu.h \
    WishPiX/Prefs/prefline.h \
    WishPiX/Prefs/prefs.h \
    WishPiX/ResourceManager/resmgr.h \
    WishPiX/Spry/spry.h \
    alpha.h \
    AlphaAnimType.h \
    alphablitforpostal.h \
    Anim3D.h \
    AnimThing.h \
    Average.h \
    ball.h \
    band.h \
    barrel.h \
    bouy.h \
    BufQ.h \
    bulletFest.h \
    camera.h \
    CapFlag_Gskirts.h \
    character.h \
    chunk.h \
    collision.h \
    crawler.h \
    credits.h \
    CtrlBuf.h \
    cutscene.h \
    deathWad.h \
    demon.h \
    dispenser.h \
    doofus.h \
    dude.h \
    Econsite.h \
    encrypt.h \
    Eskirts.h \
    explode.h \
    fire.h \
    fireball.h \
    firebomb.h \
    flag.h \
    flagbase.h \
    game.h \
    gameedit.h \
    GameSettings.h \
    Gconsite.h \
    Goal_Gskirts.h \
    goaltimer.h \
    grenade.h \
    grip.h \
    Hconsite.h \
    heatseeker.h \
    hood.h \
    Hskirts.h \
    IdBank.h \
    input.h \
    InputSettings.h \
    InputSettingsDlg.h \
    item3d.h \
    keys.h \
    localize.h \
    Log.h \
    logtab.h \
    main.h \
    Mconsite.h \
    MemFileFest.h \
    menus.h \
    MenuSettings.h \
    MenuTrans.h \
    message.h \
    mine.h \
    Mpskirts.h \
    Mskirts.h \
    napalm.h \
    navnet.h \
    net.h \
    netbrowse.h \
    netclient.h \
    NetDlg.h \
    NetInput.h \
    netmsgr.h \
    netserver.h \
    organ.h \
    ostrich.h \
    Pconsite.h \
    person.h \
    personatorium.h \
    play.h \
    PostalAttrib.h \
    PowerUp.h \
    ProtoBSDIP.h \
    pylon.h \
    reality.h \
    realm.h \
    rocket.h \
    SampleMaster.h \
    scene.h \
    score.h \
    sentry.h \
    settings.h \
    smash.h \
    SndRelay.h \
    socket.h \
    SoundThing.h \
    sprites.h \
    StockPile.h \
    TexEdit.h \
    thing.h \
    Thing3d.h \
    title.h \
    toolbar.h \
    trigger.h \
    TriggerRegions.h \
    TriggerRgn.h \
    update.h \
    warp.h \
    weapon.h \
    yatime.h \
    newpix/resourcemanager.h \
    newpix/hashstring.h \
    newpix/filestream.h \
    newpix/sakarchive.h \
    newpix/3dtypes.h \
    newpix/animatedresource.h \
    newpix/resource.h \
    newpix/sharedarray.h

SOURCES += \
    RSPiX/RSPiX.cpp \
    RSPiX/GREEN/3D/pipeline.cpp \
    RSPiX/GREEN/3D/render.cpp \
    RSPiX/GREEN/3D/zbuffer.cpp \
    RSPiX/GREEN/Blit/AlphaBlit.cpp \
    RSPiX/GREEN/Blit/Blit.cpp \
    RSPiX/GREEN/Blit/BlitInit.cpp \
    RSPiX/GREEN/Blit/BlitT.cpp \
    RSPiX/GREEN/Blit/Cfnt.cpp \
    RSPiX/GREEN/Blit/Fspr1.cpp \
    RSPiX/GREEN/Blit/Fspr8.cpp \
    RSPiX/GREEN/Blit/line.cpp \
    RSPiX/GREEN/Blit/mono.cpp \
    RSPiX/GREEN/Blit/Rotate96.cpp \
    RSPiX/GREEN/Blit/RPrint.cpp \
    RSPiX/GREEN/Blit/ScaleFlat.cpp \
    RSPiX/GREEN/Hot/hot.cpp \
    RSPiX/GREEN/Image/Image.cpp \
    RSPiX/GREEN/Image/ImageCon.cpp \
    RSPiX/GREEN/Image/ImageFile.cpp \
    RSPiX/GREEN/Image/Pal.cpp \
    RSPiX/GREEN/Image/PalFile.cpp \
    RSPiX/GREEN/InputEvent/InputEvent.cpp \
    RSPiX/GREEN/Mix/mix.cpp \
    RSPiX/GREEN/Mix/MixBuf.cpp \
    RSPiX/GREEN/Sample/sample.cpp \
    RSPiX/GREEN/Snd/snd.cpp \
    RSPiX/GREEN/SndFx/SndFx.cpp \
    RSPiX/ORANGE/Attribute/attribute.cpp \
    RSPiX/ORANGE/CDT/List.cpp \
    RSPiX/ORANGE/CDT/Queue.cpp \
    RSPiX/ORANGE/Chips/chip.cpp \
    RSPiX/ORANGE/color/colormatch.cpp \
    RSPiX/ORANGE/color/dithermatch.cpp \
    RSPiX/ORANGE/Debug/profile.cpp \
    RSPiX/ORANGE/DirtRect/DirtRect.cpp \
    RSPiX/ORANGE/File/file.cpp \
    RSPiX/ORANGE/GameLib/AnimSprite.cpp \
    RSPiX/ORANGE/GameLib/Region.cpp \
    RSPiX/ORANGE/GameLib/Shapes.cpp \
    RSPiX/ORANGE/GameLib/Sprite.cpp \
    RSPiX/ORANGE/GUI/btn.cpp \
    RSPiX/ORANGE/GUI/dlg.cpp \
    RSPiX/ORANGE/GUI/edit.cpp \
    RSPiX/ORANGE/GUI/guiItem.cpp \
    RSPiX/ORANGE/GUI/ListBox.cpp \
    RSPiX/ORANGE/GUI/ListContents.cpp \
    RSPiX/ORANGE/GUI/MultiBtn.cpp \
    RSPiX/ORANGE/GUI/ProcessGui.cpp \
    RSPiX/ORANGE/GUI/PushBtn.cpp \
    RSPiX/ORANGE/GUI/scrollbar.cpp \
    RSPiX/ORANGE/GUI/txt.cpp \
    RSPiX/ORANGE/IFF/iff.cpp \
    RSPiX/ORANGE/ImageTools/lasso.cpp \
    RSPiX/ORANGE/Laymage/laymage.cpp \
    RSPiX/ORANGE/Meter/meter.cpp \
    RSPiX/ORANGE/MsgBox/MsgBox.cpp \
    RSPiX/ORANGE/MultiGrid/MultiGrid.cpp \
    RSPiX/ORANGE/MultiGrid/MultiGridIndirect.cpp \
    RSPiX/ORANGE/Parse/SimpleBatch.cpp \
    RSPiX/ORANGE/QuickMath/FixedPoint.cpp \
    RSPiX/ORANGE/QuickMath/QuickMath.cpp \
    RSPiX/ORANGE/RString/rstring.cpp \
    RSPiX/ORANGE/str/str.cpp \
    WishPiX/Menu/menu.cpp \
    WishPiX/Prefs/prefline.cpp \
    WishPiX/Prefs/prefs.cpp \
    WishPiX/ResourceManager/resmgr.cpp \
    WishPiX/Spry/spry.cpp \
    aivars.cpp \
    alpha.cpp \
    alphablitforpostal.cpp \
    Anim3D.cpp \
    AnimThing.cpp \
    ball.cpp \
    band.cpp \
    barrel.cpp \
    bouy.cpp \
    BufQ.cpp \
    bulletFest.cpp \
    camera.cpp \
    character.cpp \
    chunk.cpp \
    crawler.cpp \
    credits.cpp \
    cutscene.cpp \
    deathWad.cpp \
    demon.cpp \
    dispenser.cpp \
    doofus.cpp \
    dude.cpp \
    encrypt.cpp \
    explode.cpp \
    fire.cpp \
    fireball.cpp \
    firebomb.cpp \
    flag.cpp \
    flagbase.cpp \
    game.cpp \
    gameedit.cpp \
    GameSettings.cpp \
    goaltimer.cpp \
    grenade.cpp \
    grip.cpp \
    heatseeker.cpp \
    hood.cpp \
    IdBank.cpp \
    input.cpp \
    InputSettings.cpp \
    InputSettingsDlg.cpp \
    item3d.cpp \
    keys.cpp \
    localize.cpp \
    Log.cpp \
    logtab.cpp \
    main.cpp \
    MemFileFest.cpp \
    menus.cpp \
    MenuSettings.cpp \
    MenuTrans.cpp \
    mine.cpp \
    napalm.cpp \
    navnet.cpp \
    net.cpp \
    NetBrowse.cpp \
    NetClient.cpp \
    NetDlg.cpp \
    netmsgr.cpp \
    NetServer.cpp \
    organ.cpp \
    ostrich.cpp \
    person.cpp \
    Personatorium.cpp \
    play.cpp \
    PowerUp.cpp \
    ProtoBSDIP.cpp \
    pylon.cpp \
    realm.cpp \
    rocket.cpp \
    SampleMaster.cpp \
    scene.cpp \
    score.cpp \
    sentry.cpp \
    settings.cpp \
    smash.cpp \
    SndRelay.cpp \
    socket.cpp \
    SoundThing.cpp \
    StockPile.cpp \
    TexEdit.cpp \
    thing.cpp \
    Thing3d.cpp \
    title.cpp \
    toolbar.cpp \
    trigger.cpp \
    TriggerRegions.cpp \
    update.cpp \
    warp.cpp \
    weapon.cpp \
    yatime.cpp \
    newpix/resourcemanager.cpp \
    newpix/sakarchive.cpp \
    newpix/filestream.cpp \
    newpix/3dtypes.cpp

!dos:CONFIG += sdl2


new_config {
HEADERS += \
    inifile.h \
    pdtk/cxxutils/configmanip.h

SOURCES += \
    inifile.cpp \
    pdtk/cxxutils/configmanip.cpp
}

sdl {
QMAKE_CXXFLAGS+= -std=c++11
LIBS += -L/usr/lib/x86_64-linux-gnu -lSDL
SOURCES += \
    RSPiX/CYAN/sdl/uColors.cpp \
    RSPiX/CYAN/sdl/uDialog.cpp \
    RSPiX/CYAN/sdl/uPath.cpp \
    RSPiX/BLUE/sdl/Bdebug.cpp \
    RSPiX/BLUE/sdl/Bdisp.cpp \
    RSPiX/BLUE/sdl/Bjoy.cpp \
    RSPiX/BLUE/sdl/Bkey.cpp \
    RSPiX/BLUE/sdl/Bmain.cpp \
    RSPiX/BLUE/sdl/Bmouse.cpp \
    RSPiX/BLUE/sdl/Bsound.cpp \
    RSPiX/BLUE/sdl/Btime.cpp
}

sdl2 {
QMAKE_CXXFLAGS+= -std=c++11
LIBS += -lSDL2
SOURCES += \
    RSPiX/CYAN/sdl2/uColors.cpp \
    RSPiX/CYAN/sdl2/uDialog.cpp \
    RSPiX/CYAN/sdl2/uPath.cpp \
    RSPiX/BLUE/sdl2/Bdebug.cpp \
    RSPiX/BLUE/sdl2/Bdisp.cpp \
    RSPiX/BLUE/sdl2/Bjoy.cpp \
    RSPiX/BLUE/sdl2/Bkey.cpp \
    RSPiX/BLUE/sdl2/Bmain.cpp \
    RSPiX/BLUE/sdl2/Bmouse.cpp \
    RSPiX/BLUE/sdl2/Bsound.cpp \
    RSPiX/BLUE/sdl2/Btime.cpp
}

dos {
QMAKE_CXXFLAGS+= -std=gnu++11
DEFINES += RSP_DEBUG_OUT_FILE

HEADERS += \
    RSPiX/BLUE/dos/platform.h \
    RSPiX/BLUE/dos/ps2.h \
    RSPiX/BLUE/dos/keyboard.h \
    RSPiX/BLUE/dos/mouse.h \
    RSPiX/BLUE/dos/video.h \
    RSPiX/BLUE/dos/video/vgamodes.h \
    RSPiX/BLUE/dos/video/vid_dos.h \
    RSPiX/BLUE/dos/video/vregset.h \
    RSPiX/BLUE/dos/video/vid.h \
    RSPiX/BLUE/dos/system/sys.h \
    RSPiX/BLUE/dos/system/sys_dosa.s \
    RSPiX/BLUE/dos/system/dosasm.s
SOURCES += \
    RSPiX/CYAN/dos/uColors.cpp \
    RSPiX/CYAN/dos/uDialog.cpp \
    RSPiX/CYAN/dos/uPath.cpp \
    RSPiX/BLUE/dos/Bdebug.cpp \
    RSPiX/BLUE/dos/Bdisp.cpp \
    RSPiX/BLUE/dos/Bjoy.cpp \
    RSPiX/BLUE/dos/Bkey.cpp \
    RSPiX/BLUE/dos/Bmain.cpp \
    RSPiX/BLUE/dos/Bmouse.cpp \
    RSPiX/BLUE/dos/Bsound.cpp \
    RSPiX/BLUE/dos/Btime.cpp \
    RSPiX/BLUE/dos/platform.cpp

# \
#    RSPiX/BLUE/dos/video/vid_dos.c \
#    RSPiX/BLUE/dos/video/vid_ext.c \
#    RSPiX/BLUE/dos/video/vregset.c \
#    RSPiX/BLUE/dos/video/vid_vga.c \
#    RSPiX/BLUE/dos/system/sys_dos.c

}

dreamcast {
QMAKE_CXXFLAGS+= -std=c++11
SOURCES += \
    RSPiX/CYAN/dreamcast/uColors.cpp \
    RSPiX/CYAN/dreamcast/uDialog.cpp \
    RSPiX/CYAN/dreamcast/uPath.cpp \
    RSPiX/BLUE/dreamcast/Bdebug.cpp \
    RSPiX/BLUE/dreamcast/Bdisp.cpp \
    RSPiX/BLUE/dreamcast/Bjoy.cpp \
    RSPiX/BLUE/dreamcast/Bkey.cpp \
    RSPiX/BLUE/dreamcast/Bmain.cpp \
    RSPiX/BLUE/dreamcast/Bmouse.cpp \
    RSPiX/BLUE/dreamcast/Bsound.cpp \
    RSPiX/BLUE/dreamcast/Btime.cpp
}

saturn {
QMAKE_CXXFLAGS+= -std=c++11
SOURCES += \
    RSPiX/CYAN/saturn/uColors.cpp \
    RSPiX/CYAN/saturn/uDialog.cpp \
    RSPiX/CYAN/saturn/uPath.cpp \
    RSPiX/BLUE/saturn/Bdebug.cpp \
    RSPiX/BLUE/saturn/Bdisp.cpp \
    RSPiX/BLUE/saturn/Bjoy.cpp \
    RSPiX/BLUE/saturn/Bkey.cpp \
    RSPiX/BLUE/saturn/Bmain.cpp \
    RSPiX/BLUE/saturn/Bmouse.cpp \
    RSPiX/BLUE/saturn/Bsound.cpp \
    RSPiX/BLUE/saturn/Btime.cpp
}
