# First setup the includes and link paths
include_directories(
	${CMAKE_BINARY_DIR} 
	${CMAKE_SOURCE_DIR}/src
	)

set(seq_SOURCES 
	main.cpp spawn.cpp spawnshell.cpp spawnlist.cpp spellshell.cpp 
	spelllist.cpp vpacket.cpp editor.cpp filter.cpp packetfragment.cpp 	
	packetstream.cpp packetinfo.cpp packet.cpp packetcapture.cpp 
	packetformat.cpp interface.cpp compass.cpp map.cpp util.cpp 
	experiencelog.cpp combatlog.cpp player.cpp skilllist.cpp statlist.cpp 
	filtermgr.cpp mapcore.cpp category.cpp compassframe.cpp group.cpp 
	guild.cpp netdiag.cpp logger.cpp xmlconv.cpp xmlpreferences.cpp 
	seqwindow.cpp seqlistview.cpp zonemgr.cpp spawnmonitor.cpp 
	spawnpointlist.cpp spawnlistcommon.cpp spawnlist2.cpp spells.cpp 
	datetimemgr.cpp spawnlog.cpp packetlog.cpp datalocationmgr.cpp 
	eqstr.cpp messages.cpp message.cpp messagefilter.cpp messagewindow.cpp 
	messageshell.cpp terminal.cpp filteredspawnlog.cpp 
	messagefilterdialog.cpp diagnosticmessages.cpp mapicon.cpp 
	filternotifications.cpp netstream.cpp guildshell.cpp guildlist.cpp 
	bazaarlog.cpp
	)

# This is needed to be compatible with existent code
add_definitions("-DHAVE_CONFIG_H")

if(WITH_FLAG_WALL)
    add_definitions("-Wall")
endif(WITH_FLAG_WALL)

if(WITH_FLAG_ANSIPEDANTIC)
    add_definitions("-pedantic -ansi")
endif(WITH_FLAG_ANSIPEDANTIC)

if (HAVE_LIBPCAP)
    set(PCAP_LIBRARIES pcap)
endif (HAVE_LIBPCAP)

SET(seq_UI
    mapicondialog.ui
)

SET(QT_USE_QT3SUPPORT true)  
INCLUDE(${QT_USE_FILE}) 

#QT3_ADD_UI_FILES(seq_SOURCES ${seq_UI})

KDE4_ADD_UI3_FILES(seq_SOURCES ${seq_UI})

# Tell cmake to create .moc files for all files in the variable qtproject_SRCS
# that require such a file. note: this assumes that you use <<#include
# "header.moc">> in your files also note that you don't actually require kde
# to use this command
#kde3_automoc(${seq_SOURCES})
QT4_AUTOMOC(${seq_SOURCES})


add_executable(
    showeq 
    ${seq_SOURCES}
)

target_link_libraries(
    showeq 
    ${ZLIB_LIBRARIES}
    ${PCAP_LIBRARIES}
    ${QT_LIBRARIES}
)
