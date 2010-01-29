#
# ShowEQ Project File
#
TEMPLATE = app
TARGET = showeq

MOC_DIR = tmp
OBJECTS_DIR = tmp
UI_DIR = tmp
CONFIG += qt \
    thread \
    stl \
    warn_off \
    debug \
    uic3
QT += xml \
    qt3support \
    network
!win32:LIBS += -lpcap
DEFINES += PKGDATADIR="/usr/local/share/showeq"
DEFINES += QMAKEBUILD
DEFINES += VERSION="6.0"

# Include Paths
INCLUDEPATH += ..
INCLUDEPATH += /usr/include/pcap

# Interface form definitions
FORMS3 += mapicondialog.ui 

HEADERS += ../config.h

# CORE COMPONENTS
SOURCES += main.cpp \
    util.cpp \
    filter.cpp \
    filtermgr.cpp \
    category.cpp \
    filternotifications.cpp
SOURCES += datalocationmgr.cpp \
    eqstr.cpp \
    xmlconv.cpp \
    xmlpreferences.cpp \
    datetimemgr.cpp
SOURCES += spawnshell.cpp \
    spellshell.cpp \
    messageshell.cpp \
    guildshell.cpp
SOURCES += player.cpp \
    spawn.cpp \
    group.cpp \
    guild.cpp \
    zonemgr.cpp \
    spawnmonitor.cpp \
    spells.cpp
SOURCES += messages.cpp \
    message.cpp \
    messagefilter.cpp \
    terminal.cpp \
    diagnosticmessages.cpp
SOURCES += session.cpp
HEADERS += main.h \
    filter.h \
    spells.h \
    util.h \
    spawn.h \
    spawnshell.h \
    xmlconv.h \
    xmlpreferences.h
HEADERS += spellshell.h \
    crctab.h \
    diagnosticmessages.h \
    player.h
HEADERS += filtermgr.h \
    point.h \
    pointarray.h \
    category.h \
    group.h \
    guild.h \
    fixpt.h
HEADERS += zonemgr.h \
    spawnmonitor.h \
    datetimemgr.h \
    datalocationmgr.h \
    eqstr.h \
    messages.h
HEADERS += messagefilter.h \
    messageshell.h \
    terminal.h \
    filternotifications.h \
    guildshell.h \
    message.h
HEADERS += session.h

# LOGGERS
SOURCES += bazaarlog.cpp \
    filteredspawnlog.cpp \
    logger.cpp \
    spawnlog.cpp
HEADERS += logger.h \
    filteredspawnlog.h \
    spawnlog.h \
    bazaarlog.h

# INTERFACE COMPONENTS
SOURCES += seqwindow.cpp \
    seqlistview.cpp \
    spawnlist.cpp \
    spawnlist2.cpp \
    spawnlistcommon.cpp
SOURCES += spawnpointlist.cpp \
    statlist.cpp \
    spelllist.cpp \
    skilllist.cpp \
    compass.cpp \
    compassframe.cpp
SOURCES += map.cpp \
    mapcore.cpp \
    mapicon.cpp \
    experiencelog.cpp \
    combatlog.cpp \
    guildlist.cpp
SOURCES += editor.cpp \
    messagefilterdialog.cpp \
    messagewindow.cpp \
    interface.cpp
HEADERS += compass.h \
    interface.h \
    map.h \
    editor.h \
    experiencelog.h \
    combatlog.h \
    spawnlist.h
HEADERS += spelllist.h \
    skilllist.h \
    statlist.h \
    mapcore.h \
    mapicon.h \
    mapicondialog.ui.h
HEADERS += compassframe.h \
    seqwindow.h \
    seqlistview.h \
    spawnpointlist.h
HEADERS += spawnlistcommon.h \
    spawnlist2.h \
    messagewindow.h \
    guildlist.h \
    messagefilterdialog.h

# PACKET SOURCE COMPONENTS
!win32 { 
    SOURCES += vpacket.cpp \
        packetfragment.cpp \
        packetstream.cpp
    SOURCES += packetcapture.cpp \
        packetformat.cpp \
        packetlog.cpp
    SOURCES += netdiag.cpp
    HEADERS += vpacket.h \
        packetcapture.h \
        packetformat.h \
        packetlog.h
    HEADERS += packetstream.h \
        packetfragment.h \
        decode.h
    HEADERS += netdiag.h
}
SOURCES += remotepacket.cpp \
    packet.cpp \
    netstream.cpp \
    packetinfo.cpp
HEADERS += remotepacket.h \
    packetinfo.h \
    packetcommon.h \
    packet.h \
    netstream.h

# EVERQUEST DATA
HEADERS += races.h \
    skills.h \
    classes.h \
    languages.h \
    deity.h \
    typenames.h
HEADERS += everquest.h \
    s_everquest.h \
    staticspells.h \
    zones.h
HEADERS += weapons.h \
    weapons1.h \
    weapons27.h \
    weapons28.h \
    weapons29.h \
    weapons2a.h \
    weapons2b.h \
    weapons2c.h
FORMS += mapicondialog_qt4.ui
