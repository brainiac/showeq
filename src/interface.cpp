/*
 * interface.cpp
 *
 *  ShowEQ Distributed under GPL
 *  http://seq.sourceforge.net/
 *
 *  Copyright 2000-2007 by the respective ShowEQ Developers
 */

#include "interface.h"
#include "util.h"
#include "main.h"
#include "editor.h"
#include "packet.h"
#include "zonemgr.h"
#include "compassframe.h"
#include "map.h"
#include "experiencelog.h"
#include "combatlog.h"
#include "filtermgr.h"
#include "spellshell.h"
#include "spawnlist.h"
#include "spelllist.h"
#include "player.h"
#include "skilllist.h"
#include "statlist.h"
#include "group.h"
#include "netdiag.h"
#include "spawnmonitor.h"
#include "spawnpointlist.h"
#include "spawnlist2.h"
#include "logger.h"
#include "spawnlog.h"
#include "packetlog.h"
#include "bazaarlog.h"
#include "category.h"
#include "guild.h"
#include "guildshell.h"
#include "guildlist.h"
#include "spells.h"
#include "datetimemgr.h"
#include "datalocationmgr.h"
#include "eqstr.h"
#include "messagefilter.h"
#include "messages.h"
#include "messageshell.h"
#include "messagewindow.h"
#include "terminal.h"
#include "filteredspawnlog.h"
#include "messagefilterdialog.h"
#include "diagnosticmessages.h"
#include "filternotifications.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <Q3FileDialog>
#include <Q3PtrList>
#include <Q3PopupMenu>

#include <QApplication>
#include <QFontDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QStatusBar>
#include <QWidgetAction>

using namespace Qt;

// this define is used to diagnose the order with which zone packets are rcvd
#define ZONE_ORDER_DIAG

/* The main interface widget */
EQInterface::EQInterface(DataLocationMgr* dlm, QWidget* parent, const char *name)
  : QMainWindow (parent, name),
	m_player(0),
	m_dataLocationMgr(dlm),
	m_mapMgr(0),
	m_spawnList(0),
	m_spawnList2(0),
	m_spellList(0),
	m_skillList(0),
	m_statList(0),
	m_spawnPointList(0),
	m_packet(0),
	m_zoneMgr(0),
	m_filterMgr(0),
	m_categoryMgr(0),
	m_spawnShell(0),
	m_spellShell(0),
	m_groupMgr(0),
	m_spawnMonitor(0),
	m_guildmgr(0),
	m_guildShell(0),
	m_dateTimeMgr(0),
	m_eqStrings(0),
	m_messageFilters(0),
	m_messages(0),
	m_messageShell(0),
	m_terminal(0),
	m_filteredSpawnLog(0),
	m_filterNotifications(0),
	m_spawnLogger(0),
	m_globalLog(0),
	m_worldLog(0),
	m_zoneLog(0),
	m_bazaarLog(0),
	m_unknownZoneLog(0),
	m_opcodeMonitorLog(0),
	m_selectedSpawn(0),
	m_windowsMenus(11),
	m_compass(0),
	m_expWindow(0),
	m_combatWindow(0),
	m_netDiag(0),
	m_messageFilterDialog(0),
	m_guildListWindow(0)
{
	// Finish some initialization
	for (int l = 0; l < maxNumMaps; l++)
		m_map[l] = 0;

	for (int l = 0; l < maxNumMessageWindows; l++)
		m_messageWindow[l] = 0;

	// make sure the windows menus list autodeletes
	m_windowsMenus.setAutoDelete(true);

	// indicate we're constructing the interface
	m_creating = true;

	// saved state version. change it if drastic changes happen.
	m_stateVersion = 1;

	// Create the date/time manager
	m_dateTimeMgr = new DateTimeMgr(this, "datetimemgr");

	// Create Message Filters object
	m_messageFilters = new MessageFilters(this, "messagefilters");

	// Create Messages storage
	m_messages = new Messages(m_dateTimeMgr, m_messageFilters, this, "messages");

	// Create the terminal object
	m_terminal = new Terminal(m_messages, this, "terminal");

	/***********************************************************************
	 * Create Packet Provider Object
	 **********************************************************************/

	/* get world opcodes file */
	QString fileName = pSEQPrefs->getPrefString("WorldOPCodes", "Network", "worldopcodes.xml");
	QFileInfo fileInfo = m_dataLocationMgr->findExistingFile(".", fileName);

	/* get zone opcodes file */
	QString fileName2 = pSEQPrefs->getPrefString("ZoneOPCodes", "Network", "zoneopcodes.xml");
	QFileInfo fileInfo2 = m_dataLocationMgr->findExistingFile(".", fileName2);

	// make packet object
	m_packet = new EQPacket(fileInfo.absFilePath(),
							fileInfo2.absFilePath(),
							pSEQPrefs->getPrefInt("ArqSeqGiveUp", "Network", 512),
							pSEQPrefs->getPrefString("Device", "Network", "eth0"),
							pSEQPrefs->getPrefString("IP", "Network", AUTOMATIC_CLIENT_IP),
							pSEQPrefs->getPrefString("MAC", "Network", "0"),
							pSEQPrefs->getPrefBool("RealTimeThread", "Network", false),
							pSEQPrefs->getPrefBool("SessionTracking", "Network", false),
							pSEQPrefs->getPrefBool("Record", "VPacket", false),
							pSEQPrefs->getPrefInt("Playback", "VPacket", PLAYBACK_OFF),
							pSEQPrefs->getPrefInt("PlaybackRate", "VPacket", false),
							pSEQPrefs->getPrefBool("Enabled", "RemotePacketServer", false),
							pSEQPrefs->getPrefUInt("Port", "RemotePacketServer", 8773),
							this, "packet");

	// initialize packet count
	m_initialcount = 0;
	m_packetStartTime = 0;

	// Retrieve last IP usd in previous session
	m_ipstr[0] = m_packet->ip();
	for (int i = 1; i < 5; i++)
		m_ipstr[i] = "0.0.0.0";

	// REtrieve last MAC used in previous session
	m_macstr[0] = m_packet->mac();
	for (int i = 1; i < 5; i++)
		m_macstr[i] = "00:00:00:00:00:00";


	/***********************************************************************
	 * Create ShowEQ Objects
	 **********************************************************************/

	// setup the user directory
	m_dataLocationMgr->setupUserDirectory();

	// Create the Spells object
	fileName = pSEQPrefs->getPrefString("SpellsFile", "Interface", "spells_us.txt");
	fileInfo = m_dataLocationMgr->findExistingFile(".", fileName);
	m_spells = new Spells(fileInfo.absFilePath());

	// Create the EQStr storage
	m_eqStrings = new EQStr(maxNumEQStr); // increase if the number of strings exeeds

	// Create the Zone Manager
	m_zoneMgr = new ZoneMgr(this, "zonemgr");

	// Create GuildMgr object
	fileName = pSEQPrefs->getPrefString("GuildsFile", "Interface", "guilds2.dat");
	fileInfo = m_dataLocationMgr->findWriteFile("tmp", fileName);
	m_guildmgr = new GuildMgr(fileInfo.absFilePath(), this, "guildmgr");

	// Create our player object
	m_player = new Player(this, m_zoneMgr, m_guildmgr);

	// Create the filter manager
	QString filterFile = pSEQPrefs->getPrefString("FilterFile", "Interface", "global.xml");
	bool isCaseSensitive = pSEQPrefs->getPrefBool("IsCaseSensitive", "Interface", false);
	m_filterMgr = new FilterMgr(m_dataLocationMgr, filterFile, isCaseSensitive);

	// if there is a short zone name already, try to load its filters
	QString shortZoneName = m_zoneMgr->shortZoneName();
	if (!shortZoneName.isEmpty())
		m_filterMgr->loadZone(shortZoneName);

	// Create the guild shell
	m_guildShell = new GuildShell(m_zoneMgr, this, "GuildShell");

	// Create the spawn shell
	m_spawnShell = new SpawnShell(*m_filterMgr, m_zoneMgr, m_player, m_guildmgr);

	// Create the Category manager
	m_categoryMgr = new CategoryMgr();

	// Create the map manager
	m_mapMgr = new MapMgr(m_dataLocationMgr, m_spawnShell, m_player, m_zoneMgr, this);

	// Create the spell shell
	m_spellShell = new SpellShell(m_player, m_spawnShell, m_spells);

	// Create the Spawn Monitor
	m_spawnMonitor = new SpawnMonitor(m_dataLocationMgr, m_zoneMgr, m_spawnShell);

	// Create the Group Manager
	m_groupMgr = new GroupMgr(m_spawnShell, m_player, this, "groupmgr");

	// Create the message shell
	m_messageShell = new MessageShell(m_messages, m_eqStrings, m_spells, m_zoneMgr, m_spawnShell, m_player, this, "messageshell");

	// Create the Filter Notifications object
	m_filterNotifications = new FilterNotifications(this, "filternotifications");

	createLogs();

	/***********************************************************************
	 * Create Main Interface Widgets
	 **********************************************************************/

	// set central widget to a QMainWindow so we can stack left and right
	// over the top and bottom...

	// TODO: Figure out how to handle main center gadget thing...
	m_filler = new QWidget(this, "filler");
	setCentralWidget(m_filler);

	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

	QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum, false);
	setSizePolicy(sizePolicy);

	// Shouldn't these be declared somewehere else...
	m_selectOnConsider = pSEQPrefs->getPrefBool("SelectOnCon", "Interface", false);
	m_selectOnTarget = pSEQPrefs->getPrefBool("SelectOnTarget", "Interface", false);

	// set the applications default font
	if (pSEQPrefs->isPreference("Font", "Interface"))
	{
		QFont appFont = pSEQPrefs->getPrefFont("Font", "Interface", qApp->font());
		qApp->setFont(appFont, true);
	}

	// create window menu (this gets used early)
	m_windowMenu = new Q3PopupMenu;

	createInterfaceWidgets();


	/***********************************************************************
	 * Create Main Menu Widgets
	 **********************************************************************/

	// Create the file menu
	createFileMenu();

	// Create the view menu
	createViewMenu();

	// Create the options menu
	createOptionsMenu();

	// Create the network menu
	createNetworkMenu();

	// Create the character menu
	createCharacterMenu();

	// Create the filters menu
	createFiltersMenu();

	// Create the interface menu
	createInterfaceMenu();

	// Create the window menu
	createWindowMenu();

	// Create the debug menu
	createDebugMenu();


	/***********************************************************************
	 * Create Status Bar Widget
	 **********************************************************************/

	createStatusBar();


	/***********************************************************************
	 * Connect Interface Signals
	 **********************************************************************/

	connectSignals();


	/***********************************************************************
	 * Configure Geometry and finalize setup
	 **********************************************************************/

	// set mainwindow Geometry
	QSize s = pSEQPrefs->getPrefSize("WindowSize", "Interface", size());
	resize(s);

	if (pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true))
	{
		QPoint p = pSEQPrefs->getPrefPoint("WindowPos", "Interface", pos());
		move(p);
	}

	QAction* toggleStatusBar = new QAction(this);
	toggleStatusBar->setShortcut(CTRL+ALT+Key_S);
	connect(toggleStatusBar, SIGNAL(triggered()), this, SLOT(toggle_view_statusbar()));
	this->addAction(toggleStatusBar);

	QAction* toggleMenuBar = new QAction(this);
	toggleMenuBar->setShortcut(CTRL+ALT+Key_T);
	connect(toggleMenuBar, SIGNAL(triggered()), this, SLOT(toggle_view_menubar()));
	this->addAction(toggleMenuBar);

	// load in the docking preferences if any have been saved
	QByteArray dockPrefs = pSEQPrefs->getPrefVariant("DockingInfo", "Interface", QVariant()).toByteArray();
	if (!dockPrefs.isEmpty())
	{
		if (!restoreState(dockPrefs, m_stateVersion))
		{
			seqDebug("Unable to restore state version %i", m_stateVersion);
		}
	}

	// Set main window title
	// TODO: Add % replacement values and a signal to update, for ip address currently being monitored.
	//setCaption(pSEQPrefs->getPrefString("Caption", "Interface", "ShowEQ " + QString(VERSION)));
	setCaption("ShowEQ " + QString(VERSION));

	// load the format strings for display
	loadFormatStrings();

	/* Start the packet capturing */
	m_packet->start(10);

	// TODO: Some windows are initialized but hidden.
	// TODO: When closing a window, its state should be saved for the session.
	m_creating = false;

	show();
} // end constructor
////////////////////

void EQInterface::createInterfaceWidgets()
{
	// Initialize the experience window
	setupExperienceWindow();

	// Initialize the combat window
	setupCombatWindow();

	// Create/display the Map widgets
	for (int i = 0; i < maxNumMaps; i++)
	{
		QString tmpPrefSuffix = "";
		if (i > 0)
			tmpPrefSuffix = QString::number(i + 1);

		// construct the preference name
		QString tmpPrefName = QString("DockedMap") + tmpPrefSuffix;

		// retrieve if the map should be docked
		m_isMapDocked[i] = pSEQPrefs->getPrefBool(tmpPrefName, "Interface", (i == 0));

		// construct the preference name
		tmpPrefName = QString("ShowMap") + tmpPrefSuffix;

		// and as appropriate, create the map
		if (pSEQPrefs->getPrefBool(tmpPrefName, "Interface", (i == 0)))
			showMap(i);
	}

	// Create/display the MessageWindow(s)
	for (int i = 0; i < maxNumMessageWindows; i++)
	{
		QString tmpPrefSuffix = "";
		if (i > 0)
			tmpPrefSuffix = QString::number(i + 1);

		// construct the preference name
		QString tmpPrefName = QString("DockedMessageWindow") + tmpPrefSuffix;

		// retrieve if the message window should be docked
		m_isMessageWindowDocked[i] = pSEQPrefs->getPrefBool(tmpPrefName, "Interface", false);

		// construct the preference name
		tmpPrefName = QString("ShowMessageWindow") + tmpPrefSuffix;

		// and as appropriate, create the message window
		if (pSEQPrefs->getPrefBool(tmpPrefName, "Interface", false))
			showMessageWindow(i);
	}

	// Create the Player Skills listview
	m_isSkillListDocked = pSEQPrefs->getPrefBool("DockedPlayerSkills", "Interface", true);
	if (pSEQPrefs->getPrefBool("ShowPlayerSkills", "Interface", true))
		showSkillList();

	// Create the Player Status listview
	m_isStatListDocked = pSEQPrefs->getPrefBool("DockedPlayerStats", "Interface", true);
	if (pSEQPrefs->getPrefBool("ShowPlayerStats", "Interface", true))
		showStatList();

	// Create the compass as required
	m_isCompassDocked = pSEQPrefs->getPrefBool("DockedCompass", "Interface", true);
	if (pSEQPrefs->getPrefBool("ShowCompass", "Interface", false))
		showCompass();

	// Create the spells listview as required (dynamic object)
	m_isSpellListDocked = pSEQPrefs->getPrefBool("DockedSpellList", "Interface", true);
	if (pSEQPrefs->getPrefBool("ShowSpellList", "Interface", false))
		showSpellList();

	// Create the Spawn List listview (always exists, just hidden if not specified)
	m_isSpawnListDocked = pSEQPrefs->getPrefBool("DockedSpawnList", "Interface", true);
	if (pSEQPrefs->getPrefBool("ShowSpawnList", "Interface", false))
		showSpawnList();

	// Create the Spawn List2 listview (always exists, just hidden if not specified)
	m_isSpawnList2Docked = pSEQPrefs->getPrefBool("DockedSpawnList2", "Interface", true);
	if (pSEQPrefs->getPrefBool("ShowSpawnList2", "Interface", true))
		showSpawnList2();

	// Create the Spawn List listview (always exists, just hidden if not specified)
	m_isSpawnPointListDocked = pSEQPrefs->getPrefBool("DockedSpawnPointList", "Interface", false);
	if (pSEQPrefs->getPrefBool("ShowSpawnPointList", "Interface", false))
		showSpawnPointList();

	// Create the Net Statistics window as required
	if (pSEQPrefs->getPrefBool("ShowNetStats", "Interface", false))
		showNetDiag();

	// Create the Guild member List window as required
	if (pSEQPrefs->getPrefBool("ShowGuildList", "Interface", false))
		showGuildList();
}

void EQInterface::createLogs()
{
	// Create log objects as necessary
	if (pSEQPrefs->getPrefBool("LogAllPackets", "PacketLogging", false))
		createGlobalLog();

	if (pSEQPrefs->getPrefBool("LogZonePackets", "PacketLogging", false))
		createZoneLog();

	if (pSEQPrefs->getPrefBool("LogBazaarPackets", "PacketLogging", false))
		createBazaarLog();

	if (pSEQPrefs->getPrefBool("LogWorldPackets", "PacketLogging", false))
		createWorldLog();

	if (pSEQPrefs->getPrefBool("LogUnknownZonePackets", "PacketLogging", false))
		createUnknownZoneLog();

	if (pSEQPrefs->getPrefBool("Enable", "OpCodeMonitoring", false))
		createOPCodeMonitorLog(pSEQPrefs->getPrefString("OpCodeList", "OpCodeMonitoring", ""));

	// create the filtered spawn log object if any filters are to be logged
	uint32_t filters = pSEQPrefs->getPrefInt("Log", "Filters", 0);
	if (filters)
	{
		// create the filtered spawn log object
		createFilteredSpawnLog();

		// set the filters to log
		m_filteredSpawnLog->setFilters(filters);
	}

	// if the user wants spawns logged, create the spawn logger
	if (pSEQPrefs->getPrefBool("LogSpawns", "Misc", false))
		createSpawnLog();
}

void EQInterface::createFileMenu()
{
	////////////////////////////////////////////////////////////////
	// File Menu
	QMenu* pFileMenu = menuBar()->addMenu("&File");
	QAction* action = NULL;

	// Save Preferences
	action = new QAction("&Save Preferences", this);
	action->setShortcut(CTRL + Key_S);
	connect(action, SIGNAL(triggered()), this, SLOT(savePrefs()));
	pFileMenu->addAction(action);

	// Open Map
	action = new QAction("Open &Map", this);
	action->setShortcut(Key_F1);
	connect(action, SIGNAL(triggered()), m_mapMgr, SLOT(loadMap()));
	pFileMenu->addAction(action);

	// Import Map
	action = new QAction("&Import Map", this);
	connect(action, SIGNAL(triggered()), m_mapMgr, SLOT(importMap()));
	pFileMenu->addAction(action);

	// Save Map
	action = new QAction("Sa&ve Map", this);
	action->setShortcut(Key_F2);
	connect(action, SIGNAL(triggered()), m_mapMgr, SLOT(saveMap()));
	pFileMenu->addAction(action);

	// Save SOE Map
	action = new QAction("Save SOE Map", this);
	connect(action, SIGNAL(triggered()), m_mapMgr, SLOT(saveSOEMap()));
	pFileMenu->addAction(action);

	// Reload Guilds File
	action = new QAction("Reload Guilds File", this);
	connect(action, SIGNAL(triggered()), m_guildmgr, SLOT(readGuildList()));
	pFileMenu->addAction(action);

	// Add Spawn Category
	action = new QAction("Add Spawn Category", this);
	action->setShortcut(ALT + Key_C);
	connect(action, SIGNAL(triggered()), this, SLOT(addCategory()));
	pFileMenu->addAction(action);

	// Rebuild Spawn List
	action = new QAction("Rebuild SpawnList", this);
	action->setShortcut(ALT + Key_R);
	connect(action, SIGNAL(triggered()), this, SLOT(rebuildSpawnList()));
	pFileMenu->addAction(action);

	// Reload Categories
	action = new QAction("Reload Categories", this);
	action->setShortcut(CTRL + Key_R);
	connect(action, SIGNAL(triggered()), this, SLOT(reloadCategories()));
	pFileMenu->addAction(action);

	// Select Next
	action = new QAction("Select Next", this);
	action->setShortcut(CTRL + Key_Right);
	connect(action, SIGNAL(triggered()), this, SLOT(selectNext()));
	pFileMenu->addAction(action);

	// Select Prev
	action = new QAction("Select Prev", this);
	action->setShortcut(CTRL + Key_Left);
	connect(action, SIGNAL(triggered()), this, SLOT(selectPrev()));
	pFileMenu->addAction(action);

	// Save Selected Spawns Path
	action = new QAction("Save Selected Spawns Path", this);
	connect(action, SIGNAL(triggered()), this, SLOT(saveSelectedSpawnPath()));
	pFileMenu->addAction(action);

	// Save NPC Spawn Paths
	action = new QAction("Save NPC Spawn Paths", this);
	connect(action, SIGNAL(triggered()), this, SLOT(saveSpawnPaths()));
	pFileMenu->addAction(action);

	if (m_packet->playbackPackets() != PLAYBACK_OFF && m_packet->playbackPackets() != PLAYBACK_REMOTE)
	{
		// Inc Playback Speed
		action = new QAction("Increase Playback Speed", this);
		action->setShortcut(CTRL + Key_X);
		connect(action, SIGNAL(triggered()), m_packet, SLOT(incPlayback()));
		pFileMenu->addAction(action);

		// Dec Playback Speed
		action = new QAction("Decrease Playback Speed", this);
		action->setShortcut(CTRL + Key_Z);
		connect(action, SIGNAL(triggered()), m_packet, SLOT(decPlayback()));
		pFileMenu->addAction(action);
	}

	// Quit
	action = new QAction("&Quit", this);
	connect(action, SIGNAL(triggered()), qApp, SLOT(quit()));
	pFileMenu->addAction(action);
}

void EQInterface::createViewMenu()
{
	QString section = "Interface";

	////////////////////////////////////////////////////////////////
	// View menu
	QMenu* viewMenu = menuBar()->addMenu("&View");

	// Create Experience Window Item
	m_viewExpWindow = new QAction("Experience Window", this);
	m_viewExpWindow->setCheckable(true);
	connect(m_viewExpWindow, SIGNAL(triggered()), this, SLOT(toggleExpWindow()));
	viewMenu->addAction(m_viewExpWindow);

	// Create Combat Window Item
	m_viewCombatWindow = new QAction("Combat Window", this);
	m_viewCombatWindow->setCheckable(true);
	connect(m_viewCombatWindow, SIGNAL(triggered()), this, SLOT(toggleCombatWindow()));
	viewMenu->addAction(m_viewCombatWindow);

	viewMenu->addSeparator();

	// Create Spawn List item
	m_viewSpawnList = new QAction("Spawn List", this);
	m_viewSpawnList->setCheckable(true);
	connect(m_viewSpawnList, SIGNAL(triggered()), this, SLOT(toggleSpawnList()));
	viewMenu->addAction(m_viewSpawnList);

	// Create Spawn List 2 item
	m_viewSpawnList2 = new QAction("Spawn List 2", this);
	m_viewSpawnList2->setCheckable(true);
	connect(m_viewSpawnList2, SIGNAL(triggered()), this, SLOT(toggleSpawnList2()));
	viewMenu->addAction(m_viewSpawnList2);

	// Create Spell List item
	m_viewSpellList = new QAction("Spell List", this);
	m_viewSpellList->setCheckable(true);
	connect(m_viewSpellList, SIGNAL(triggered()), this, SLOT(toggleSpellList()));
	viewMenu->addAction(m_viewSpellList);

	// Create Spawn Point List item
	m_viewSpawnPointList = new QAction("Spawn Point List", this);
	m_viewSpawnPointList->setCheckable(true);
	connect(m_viewSpawnPointList, SIGNAL(triggered()), this, SLOT(toggleSpawnPointList()));
	viewMenu->addAction(m_viewSpawnPointList);

	// Create player stats item
	m_viewPlayerStats = new QAction("Player Stats", this);
	m_viewPlayerStats->setCheckable(true);
	connect(m_viewPlayerStats, SIGNAL(triggered()), this, SLOT(togglePlayerStats()));
	viewMenu->addAction(m_viewPlayerStats);

	// Create player skills item
	m_viewPlayerSkills = new QAction("Player Skills", this);
	m_viewPlayerSkills->setCheckable(true);
	connect(m_viewPlayerSkills, SIGNAL(triggered()), this, SLOT(togglePlayerSkills()));
	viewMenu->addAction(m_viewPlayerSkills);

	// Create compass item
	m_viewCompass = new QAction("Compass", this);
	m_viewCompass->setCheckable(true);
	connect(m_viewCompass, SIGNAL(triggered()), this, SLOT(toggleCompass()));
	viewMenu->addAction(m_viewCompass);

	// Create Map submenu and items
	QMenu* subMenu = new QMenu("Maps", this);
	QString mapName;
	for (int i = 0; i < maxNumMaps; i++)
	{
		mapName.sprintf("Map %i", i + 1);

		m_viewMap[i] = new QAction(mapName, this);
		m_viewMap[i]->setCheckable(true);
		m_viewMap[i]->setData(i);
		subMenu->addAction(m_viewMap[i]);
	}
	connect(subMenu, SIGNAL(triggered(QAction*)), this, SLOT(toggleMap(QAction*)));
	viewMenu->addMenu(subMenu);

	// Create channel messages submenus and items
	subMenu = new QMenu("Channel Messages", this);
	QString messageWindowName;
	for (int i = 0; i < maxNumMessageWindows; i++)
	{
        messageWindowName.sprintf("Channel Messages %i", i + 1);

        m_viewMessageWindow[i] = new QAction(messageWindowName, this);
        m_viewMessageWindow[i]->setCheckable(true);
        m_viewMessageWindow[i]->setData(i);
        subMenu->addAction(m_viewMessageWindow[i]);
	}
	connect(subMenu, SIGNAL(triggered(QAction*)), this, SLOT(toggleChannelMsgs(QAction*)));
	viewMenu->addMenu(subMenu);

	// Create network diagnostic menu item
	m_viewNetDiag = new QAction("Network Diagnostics", this);
	m_viewNetDiag->setCheckable(true);
	connect(m_viewNetDiag, SIGNAL(triggered()), this, SLOT(toggleNetDiag()));
	viewMenu->addAction(m_viewNetDiag);

	// Create guild list menu item
	m_viewGuildList = new QAction("Guild Member List", this);
	m_viewGuildList->setCheckable(true);
	connect(m_viewGuildList, SIGNAL(triggered()), this, SLOT(toggleGuildList()));
	viewMenu->addAction(m_viewGuildList);


	viewMenu->addSeparator();


	/*
	 * Create Menu Entries for Player Stats
	 */
	struct {
		int Val;
		const char* Str;
	} statEntries[] = {
		{ LIST_HP, 		"Hit Points" },
		{ LIST_MANA, 	"Mana" },
		{ LIST_STAM, 	"Stamina" },
		{ LIST_EXP, 	"Experience" },
		{ LIST_ALTEXP, 	"Alt Experience" },
		{ LIST_FOOD, 	"Food" },
		{ LIST_WATR, 	"Water" },
		{ -1, 			NULL },
		{ LIST_STR, 	"Strength" },
		{ LIST_STA, 	"Stamina" },
		{ LIST_CHA, 	"Charisma" },
		{ LIST_DEX, 	"Dexterity" },
		{ LIST_INT, 	"Intelligence" },
		{ LIST_AGI, 	"Agility" },
		{ LIST_WIS, 	"Wisdom" },
		{ -1, 			NULL },
		{ LIST_MR, 		"Magic Res" },
		{ LIST_FR, 		"Fire Res" },
		{ LIST_CR, 		"Cold Res" },
		{ LIST_DR, 		"Disease Res" },
		{ LIST_PR,		"Poison Res" },
		{ -1, 			NULL },
		{ LIST_AC,		"Armor Class" },
		{ -2, 			NULL }
	};

	m_playerStatsMenu = new QMenu("&Player Stats", this);
	for (int idx = 0; statEntries[idx].Val != -2; idx++)
	{
		if (statEntries[idx].Val == -1)
		{
			m_playerStatsMenu->addSeparator();
			continue;
		}
		QAction* menuAction = new QAction(statEntries[idx].Str, this);
		menuAction->setCheckable(true);
		menuAction->setData(statEntries[idx].Val);
		m_playerStatsMenu->addAction(menuAction);
	}
	connect(m_playerStatsMenu, SIGNAL(triggered(QAction*)), this, SLOT(togglePlayerStat(QAction*)));
	m_playerStatsMenuAction = viewMenu->addMenu(m_playerStatsMenu);


	/*
	 * Create Menu Entries for Player Skills
	 */
	m_playerSkillsMenu = new QMenu("Player &Skills", this);
	m_playerSkillsMenu->setCheckable(true);

	m_playerSkillsLanguages = new QAction("&Languages", this);
	m_playerSkillsLanguages->setCheckable(true);
	m_playerSkillsLanguages->setData((int)0);
	m_playerSkillsMenu->addAction(m_playerSkillsLanguages);
	connect(m_playerSkillsMenu, SIGNAL(triggered(QAction*)), this, SLOT(togglePlayerSkill(QAction*)));
	m_playerSkillsMenuAction = viewMenu->addMenu(m_playerSkillsMenu);


	/*
	 * Create Menu Entries for Spawn List columns
	 */
    struct {
    	int Val;
    	const char* Str;
    } menuEntries[] = {
    	{ tSpawnColName, 		"&Name" },
    	{ tSpawnColLevel, 		"&Level" },
    	{ tSpawnColHP, 			"&HP" },
    	{ tSpawnColMaxHP,		"&Max HP" },
    	{ tSpawnColXPos,		"Coord &1" },
    	{ tSpawnColYPos,		"Coord &2" },
    	{ tSpawnColZPos,		"Coord &3" },
    	{ tSpawnColID,			"I&D", },
    	{ tSpawnColDist,		"&Distance" },
    	{ tSpawnColRace,		"&Race" },
    	{ tSpawnColClass,		"&Class" },
    	{ tSpawnColDeity,		"Deit&y" },
    	{ tSpawnColBodyType,	"&BodyType" },
    	{ tSpawnColGuildID,		"Guild Tag" },
    	{ tSpawnColInfo,		"&Info" },
    	{ tSpawnColSpawnTime, 	"Spawn &Time" },
    	{ -1, NULL }
    };

    m_spawnListMenu = new QMenu("Spawn &List", this);
    for (int idx = 0; menuEntries[idx].Val != -1; idx++)
    {
    	QAction* menuAction = new QAction(menuEntries[idx].Str, this);
    	menuAction->setCheckable(true);
    	menuAction->setData(menuEntries[idx].Val);
    	m_spawnListMenu->addAction(menuAction);
    }
    m_spawnListMenu->setCheckable(true);
    connect(m_spawnListMenu, SIGNAL(triggered(QAction*)), this, SLOT(toggleSpawnListCol(QAction*)));
    m_spawnListMenuAction = viewMenu->addMenu(m_spawnListMenu);
    viewMenu->addMenu(m_spawnListMenu);

    // Separator between spawn list columns and docking options
	viewMenu->insertSeparator(-1);


	/*
	 * Create Menu Entries for Docked Windows
	 */

	QAction* action = NULL;
	m_dockedWinMenu = new QMenu("&Docked", this);

	action = new QAction("Spawn &List", this);
	action->setData(menuSpawnList);
	action->setCheckable(true);
	action->setChecked(m_isSpawnListDocked);
	m_dockedWinMenu->addAction(action);

	action = new QAction("Spawn List &2", this);
	action->setData(menuSpawnList2);
	action->setCheckable(true);
	action->setChecked(m_isSpawnList2Docked);
	m_dockedWinMenu->addAction(action);

	action = new QAction("Spawn P&oint List", this);
	action->setData(menuSpawnPointList);
	action->setCheckable(true);
	action->setChecked(m_isSpawnPointListDocked);
	m_dockedWinMenu->addAction(action);

	action = new QAction("&Player Stats", this);
	action->setData(menuPlayerStats);
	action->setCheckable(true);
	action->setChecked(m_isStatListDocked);
	m_dockedWinMenu->addAction(action);

	action = new QAction("Player &Skills", this);
	action->setData(menuPlayerSkills);
	action->setCheckable(true);
	action->setChecked(m_isSkillListDocked);
	m_dockedWinMenu->addAction(action);

	action = new QAction("Sp&ell List", this);
	action->setData(menuSpellList);
	action->setCheckable(true);
	action->setChecked(m_isSpellListDocked);
	m_dockedWinMenu->addAction(action);

	action = new QAction("&Compass", this);
	action->setData(menuCompass);
	action->setCheckable(true);
	action->setChecked(m_isCompassDocked);
	m_dockedWinMenu->addAction(action);

	// insert Map docking options
	// NOTE: Always insert Map docking options at the end of the Docked menu
	for (int i = 0; i < maxNumMaps; i++)
	{
		QString mapName;
		mapName.sprintf("Map %i", i + 1);

		action = new QAction(mapName, this);
		action->setData(i + mapDockBase);
		action->setCheckable(true);
		action->setChecked(m_isMapDocked[i]);
		m_dockedWinMenu->addAction(action);
	}
	viewMenu->addMenu(m_dockedWinMenu);
	connect(m_dockedWinMenu, SIGNAL(triggered(QAction*)), this, SLOT(toggleWindowDocked(QAction*)));


	/*
	 * Create Menu Entries for Docked Windows
	 */
	m_dockableWinMenu = new QMenu("&Dockable", this);

	action = new QAction("Spawn &List", this);
	action->setCheckable(true);
	action->setData(menuSpawnList);
	action->setChecked(pSEQPrefs->getPrefBool("DockableSpawnList", "Interface", true));
	m_dockableWinMenu->addAction(action);

	action = new QAction("Spawn List &2", this);
	action->setCheckable(true);
	action->setData(menuSpawnList2);
	action->setChecked(pSEQPrefs->getPrefBool("DockablSspawnList2", "Interface", true));
	m_dockableWinMenu->addAction(action);

	action = new QAction("Spawn P&oint List", this);
	action->setCheckable(true);
	action->setData(menuSpawnPointList);
	action->setChecked(pSEQPrefs->getPrefBool("DockableSpawnPointList", "Interface", true));
	m_dockableWinMenu->addAction(action);

	action = new QAction("&Player Stats", this);
	action->setCheckable(true);
	action->setData(menuPlayerStats);
	action->setChecked(pSEQPrefs->getPrefBool("DockablePlayerStats", "Interface", true));
	m_dockableWinMenu->addAction(action);

	action = new QAction("Player &Skills", this);
	action->setCheckable(true);
	action->setData(menuPlayerSkills);
	action->setChecked(pSEQPrefs->getPrefBool("DockablePlayerSkills", "Interface", true));
	m_dockableWinMenu->addAction(action);

	action = new QAction("Sp&ell List", this);
	action->setCheckable(true);
	action->setData(menuSpellList);
	action->setChecked(pSEQPrefs->getPrefBool("DockableSpellList", "Interface", true));
	m_dockableWinMenu->addAction(action);

	action = new QAction("&Compass", this);
	action->setCheckable(true);
	action->setData(menuCompass);
	action->setChecked(pSEQPrefs->getPrefBool("DockableCompass", "Interface", true));
	m_dockableWinMenu->addAction(action);

	action = new QAction("E&xperience Window", this);
	action->setCheckable(true);
	action->setData(menuExperienceWindow);
	action->setChecked(pSEQPrefs->getPrefBool("DockableExperienceWindow", section, false));
	m_dockableWinMenu->addAction(action);

	action = new QAction("Com&bat Window", this);
	action->setCheckable(true);
	action->setData(menuCombatWindow);
	action->setChecked(pSEQPrefs->getPrefBool("DockableCombatWindow", section, false));
	m_dockableWinMenu->addAction(action);

	action = new QAction("&Guild List", this);
	action->setCheckable(true);
	action->setData(menuCombatWindow);
	action->setChecked(pSEQPrefs->getPrefBool("DockableGuildListWindow", section, true));
	m_dockableWinMenu->addAction(action);

	action = new QAction("&Net Diagnostics", this);
	action->setCheckable(true);
	action->setData(menuNetDiag);
	action->setChecked(pSEQPrefs->getPrefBool("DockableNetDiag", section, true));
	m_dockableWinMenu->addAction(action);

	// insert Map docking options
	subMenu = new QMenu("M&aps", this);
	for (int i = 0; i < maxNumMaps; i++)
	{
        QString mapName = "Map &";
		QString mapPrefName = "Map";
        if (i > 0)
		{
            mapName += QString::number(i + 1);
			mapPrefName + QString::number(i + 1);
		}
        action = new QAction(mapName, this);
        action->setCheckable(true);
        action->setData(i + mapDockBase);
        action->setChecked(pSEQPrefs->getPrefBool(QString("Dockable") + mapPrefName, section, true));
        subMenu->addAction(action);
	}
	m_dockableWinMenu->addMenu(subMenu);

	// insert Message Window docking options
	subMenu = new QMenu("Channel Messages", this);
	QString messagePrefName = "DockableMessageWindow";
	for (int i = 0; i < maxNumMessageWindows; i++)
	{
        QString messageWindowName = "Channel Messages &";
		if (i > 0)
			messageWindowName += QString::number(i + 1);

		action = new QAction(messageWindowName, this);
		action->setCheckable(true);
		action->setData(i + messageWindowDockBase);
		action->setChecked(pSEQPrefs->getPrefBool(messagePrefName + QString::number(i), section, false));
		subMenu->addAction(action);
	}
	m_dockableWinMenu->addMenu(subMenu);
	connect (m_dockableWinMenu, SIGNAL(triggered(QAction*)), this, SLOT(toggleWindowDockable(QAction*)));

	// view menu checks are set by init_view_menu
	viewMenu->addMenu(m_dockableWinMenu);
	connect(viewMenu, SIGNAL(aboutToShow()), this, SLOT(updateViewMenu()));
}

void EQInterface::createOptionsMenu()
{
	QMenu* optMenu = new QMenu("&Options", this);

	QAction* action = new QAction("Fast Machine", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->fast_machine);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleFastMachine(bool)));
	optMenu->addAction(action);

	action = new QAction("Select on Consider", this);
	action->setCheckable(true);
	action->setChecked(m_selectOnConsider);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleSelectOnConsider(bool)));
	optMenu->addAction(action);

	action = new QAction("Select on Target", this);
	action->setCheckable(true);
	action->setChecked(m_selectOnTarget);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleSelectOnTarget(bool)));
	optMenu->addAction(action);

	action = new QAction("Keep Selected Visible", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->keep_selected_visible);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleKeepSelectedVisible(bool)));
	optMenu->addAction(action);

	action = new QAction("Log Spawns", this);
	action->setCheckable(true);
	action->setChecked(m_spawnLogger != NULL);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleLogSpawns(bool)));
	optMenu->addAction(action);

	action = new QAction("Log Bazaar Searches", this);
	action->setCheckable(true);
	action->setChecked(m_bazaarLog != NULL);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleLogBazaarData(bool)));
	optMenu->addAction(action);

	action = new QAction("Reset Max Mana", this);
	connect(action, SIGNAL(triggered()), this, SLOT(resetMaxMana()));
	optMenu->addAction(action);

	action = new QAction("PvP Teams", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->pvp);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(togglePvPTeams(bool)));
	optMenu->addAction(action);

	action = new QAction("PvP Deity", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->deitypvp);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(togglePvPDeity(bool)));
	optMenu->addAction(action);

	action = new QAction("Create Unknown Spawns", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->createUnknownSpawns);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleCreateUnknownSpawns(bool)));
	optMenu->addAction(action);

	action = new QAction("Use EQ Retarded Coordinates", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->retarded_coords);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleRetardedCoords(bool)));
	optMenu->addAction(action);

	action = new QAction("Use System Time for Spawn Time", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->systime_spawntime);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleSystemSpawnTime(bool)));
	optMenu->addAction(action);

	action = new QAction("Record Spawn Walk Paths", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->walkpathrecord);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleRecordWalkPaths(bool)));
	optMenu->addAction(action);


	{
		// TODO: Get rid of the spin box, replace it with a dialog (or a preferences window!)
		QMenu* subMenu = new QMenu("Walk Path Length", this);;
		QSpinBox* spinBox = new QSpinBox(0, 8192, 1, subMenu);

		spinBox->setValue(showeq_params->walkpathlength);
		connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(setWalkPathLength(int)));

		QWidgetAction* pSpinBoxAction = new QWidgetAction(subMenu);
		pSpinBoxAction->setDefaultWidget(spinBox);
		subMenu->addAction(pSpinBoxAction);
		optMenu->addMenu(subMenu);
	}

	// SaveState SubMenu
	QMenu* saveStateMenu = new QMenu("&Save State", this);

	action = new QAction("&Player", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->savePlayerState);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleSavePlayerState(bool)));
	saveStateMenu->addAction(action);

	action = new QAction("&Zone", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->saveZoneState);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleSaveZoneState(bool)));
	saveStateMenu->addAction(action);

	action = new QAction("&Spawns", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->saveSpawns);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleSaveSpawnState(bool)));
	saveStateMenu->addAction(action);

	action = new QAction("Base &Filename...", this);
	connect(action, SIGNAL(triggered()), this, SLOT(setSaveBaseFilename()));
	saveStateMenu->addAction(action);

	saveStateMenu->addSeparator();
	{
		QMenu* subMenu = new QMenu("Spawn Save Frequency (s)", this);

		QSpinBox* spinBox = new QSpinBox(1, 320, 1, subMenu);
		spinBox->setValue(showeq_params->saveSpawnsFrequency / 1000);
		connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(setSpawnSaveFrequency(int)));

		QWidgetAction* pSpinBoxAction = new QWidgetAction(subMenu);
		pSpinBoxAction->setDefaultWidget(spinBox);
		subMenu->addAction(pSpinBoxAction);
		saveStateMenu->addMenu(subMenu);
	}
	optMenu->addMenu(saveStateMenu);

	action = new QAction("Clear Channel Messages", this);
	connect(action, SIGNAL(triggered()), this, SLOT(clearChannelMessages()));
	optMenu->addAction(action);

	// Con Color base menu
	QMenu* conColorMenu = new QMenu("Con &Colors", this);

	action = new QAction("Gray Spawn Base...", this);
	action->setData(tGraySpawn);
	conColorMenu->addAction(action);

	action = new QAction("Green Spawn Base...", this);
	action->setData(tGreenSpawn);
	conColorMenu->addAction(action);

	action = new QAction("Light Blue Spawn Base...", this);
	action->setData(tCyanSpawn);
	conColorMenu->addAction(action);

	action = new QAction("Blue Spawn Base...", this);
	action->setData(tBlueSpawn);
	conColorMenu->addAction(action);

	action = new QAction("Even Spawn...", this);
	action->setData(tEvenSpawn);
	conColorMenu->addAction(action);

	action = new QAction("Yellow Spawn Base...", this);
	action->setData(tYellowSpawn);
	conColorMenu->addAction(action);

	action = new QAction("Red Spawn Base...", this);
	action->setData(tRedSpawn);
	conColorMenu->addAction(action);

	action = new QAction("Unknown Spawn...", this);
	action->setData(tUnknownSpawn);
	conColorMenu->addAction(action);

	connect(conColorMenu, SIGNAL(triggered(QAction*)), this, SLOT(selectConColorBase(QAction*)));
	optMenu->addMenu(conColorMenu);

	optMenu->addSeparator();

	action = new QAction("Use EQ Update Radius", this);
	action->setCheckable(true);
	action->setChecked(showeq_params->useUpdateRadius);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleUseUpdateRadius(bool)));
	optMenu->addAction(action);

	menuBar()->addMenu(optMenu);
}

void EQInterface::createNetworkMenu()
{
	// Network Menu
	m_netMenu = new QMenu("&Network", this);

	// Monitor next eq client seen
	QAction* action = new QAction("Monitor &Next EQ Client Seen", this);
	connect(action, SIGNAL(triggered()), this, SLOT(set_net_monitor_next_client()));
	m_netMenu->addAction(action);

	// monitor eq client ip address
	action = new QAction("Monitor EQ Client &IP Address...", this);
	connect(action, SIGNAL(triggered()), this, SLOT(set_net_client_IP_address()));
	m_netMenu->addAction(action);

	// monitor eq client mac address
	action = new QAction("Monitor EQ Client &MAC Address...", this);
	connect(action, SIGNAL(triggered()), this, SLOT(set_net_client_MAC_address()));
	m_netMenu->addAction(action);

	// set net device
	action = new QAction("Set &Device...", this);
	connect(action, SIGNAL(triggered()), this, SLOT(set_net_device()));
	m_netMenu->addAction(action);

	// session tracking
	m_netSessionTracking = new QAction("Session Tracking", this);
	m_netSessionTracking->setChecked(m_packet->session_tracking());
	m_netSessionTracking->setCheckable(true);
	connect(m_netSessionTracking, SIGNAL(toggled(bool)), this, SLOT(toggle_net_session_tracking(bool)));
	m_netMenu->addAction(m_netSessionTracking);

	// real time thread
	m_netRealTimeThread = new QAction("&Real Time Thread", this);
	m_netRealTimeThread->setCheckable(true);
	m_netRealTimeThread->setChecked(m_packet->realtime());
	connect(m_netRealTimeThread, SIGNAL(triggered()), this, SLOT(toggle_net_real_time_thread()));
	m_netMenu->addAction(m_netRealTimeThread);

	m_netMenu->addSeparator();

	// Log sub-menu
	QMenu* pLogMenu = new QMenu("Lo&g", this);

	// All packets
	m_netLogAllPackets = new QAction("All Packets", this);
	m_netLogAllPackets->setShortcut(Key_F5);
	m_netLogAllPackets->setChecked(m_globalLog != NULL);
	m_netLogAllPackets->setCheckable(true);
	connect(m_netLogAllPackets, SIGNAL(triggered()), this, SLOT(toggleLogAllPackets()));
	pLogMenu->addAction(m_netLogAllPackets);

	// World data
	m_netLogWorldData = new QAction("World Data", this);
	m_netLogWorldData->setShortcut(Key_F6);
	m_netLogWorldData->setChecked(m_worldLog != NULL);
	m_netLogWorldData->setCheckable(true);
	connect(m_netLogWorldData, SIGNAL(triggered()), this, SLOT(toggleLogWorldData()));
	pLogMenu->addAction(m_netLogWorldData);

	// zone data
	m_netLogZoneData = new QAction("Zone Data", this);
	m_netLogZoneData->setShortcut(Key_F7);
	m_netLogZoneData->setChecked(m_zoneLog != NULL);
	m_netLogZoneData->setCheckable(true);
	connect(m_netLogZoneData, SIGNAL(triggered()), this, SLOT(toggleLogZoneData()));
	pLogMenu->addAction(m_netLogZoneData);

	// Unknown data
	m_netLogUnknownData = new QAction("Unknown Data", this);
	m_netLogUnknownData->setShortcut(Key_F8);
	m_netLogUnknownData->setChecked(m_unknownZoneLog != NULL);
	m_netLogUnknownData->setCheckable(true);
	connect(m_netLogUnknownData, SIGNAL(triggered()), this, SLOT(toggle_log_UnknownData()));
	pLogMenu->addAction(m_netLogUnknownData);

	m_netViewUnknownData = new QAction("View Unknown Data", this);
	m_netViewUnknownData->setShortcut(Key_F9);
	m_netViewUnknownData->setChecked(pSEQPrefs->getPrefBool("ViewUnknown", "PacketLogging", false));
	m_netViewUnknownData->setCheckable(true);
	connect(m_netViewUnknownData, SIGNAL(triggered()), this, SLOT(toggle_view_UnknownData()));
	pLogMenu->addAction(m_netViewUnknownData);

	// Raw Data
	m_netLogRawData = new QAction("Raw Data", this);
	m_netLogRawData->setShortcut(Key_F10);
	m_netLogRawData->setChecked(pSEQPrefs->getPrefBool("LogRawPackets", "PacketLogging", false));
	m_netLogRawData->setCheckable(true);
	connect(m_netLogRawData, SIGNAL(triggered()), this, SLOT(toggle_log_RawData()));
	pLogMenu->addAction(m_netLogRawData);


	// Filter Zone Data submenu
	m_filterZoneDataMenu = new QMenu("Filter Zone Data", this);
	m_netLogFilterZoneClient = new QAction("Client", this);
	connect(m_netLogFilterZoneClient, SIGNAL(triggered()), this, SLOT(toggle_log_Filter_ZoneData_Client()));
	m_filterZoneDataMenu->addAction(m_netLogFilterZoneClient);

	m_netLogFilterZoneServer = new QAction("Server", this);
	connect(m_netLogFilterZoneServer, SIGNAL(triggered()), this, SLOT(toggle_log_Filter_ZoneData_Server()));
	m_filterZoneDataMenu->addAction(m_netLogFilterZoneServer);
	pLogMenu->addMenu(m_filterZoneDataMenu);

	m_netMenu->addMenu(pLogMenu);

	// OpCode Monitor
	QMenu* pOpCodeMenu = new QMenu("OpCode Monitor", this);

	action = new QAction("&OpCode Monitoring", this);
	action->setCheckable(true);
	action->setShortcut(CTRL + ALT + Key_O);
	action->setChecked(m_opcodeMonitorLog != NULL);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleOpcodeMonitoring(bool)));
	pOpCodeMenu->addAction(action);

	action = new QAction("&Reload Monitored OpCode List...", this);
	action->setShortcut(CTRL + ALT + Key_R);
	connect(action, SIGNAL(triggered()), this, SLOT(set_opcode_monitored_list()));
	pOpCodeMenu->addAction(action);

	m_netOpcodeView = new QAction("&View Monitored Opcode Matches", this);
	m_netOpcodeView->setCheckable(true);
	m_netOpcodeView->setChecked(pSEQPrefs->getPrefBool("View", "OpCodeMonitoring", false));
	connect(m_netOpcodeView, SIGNAL(toggled(bool)), this, SLOT(toggle_opcode_view(bool)));
	pOpCodeMenu->addAction(m_netOpcodeView);

	m_netOpcodeLog = new QAction("&Log Monitored OpCode Matches", this);
	m_netOpcodeLog->setCheckable(true);
	m_netOpcodeLog->setChecked(pSEQPrefs->getPrefBool("Log", "OpCodeMonitoring", false));
	connect(m_netOpcodeLog, SIGNAL(toggled(bool)), this, SLOT(toggle_opcode_log(bool)));
	pOpCodeMenu->addAction(m_netOpcodeLog);

	action = new QAction("Log &Filename...", this);
	connect(action, SIGNAL(triggered()), this, SLOT(select_opcode_file()));
	pOpCodeMenu->addAction(action);

	m_netMenu->addMenu(pOpCodeMenu);

	m_netMenu->addSeparator();

	// Advanced menu
	QMenu* subMenu = new QMenu("Advanced", this);

	QMenu* subSubMenu = new QMenu("Arq Seq Give Up", subMenu);
	QSpinBox* spinBox = new QSpinBox(32, 1024, 8, subSubMenu);

	spinBox->setValue(m_packet->arqSeqGiveUp());
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(set_net_arq_giveup(int)));

	QWidgetAction* pWidgetAction = new QWidgetAction(subSubMenu);
	pWidgetAction->setDefaultWidget(spinBox);
	subSubMenu->addAction(pWidgetAction);

	subMenu->addMenu(subSubMenu);
	m_netMenu->addMenu(subMenu);

	menuBar()->addMenu(m_netMenu);
}

void EQInterface::createCharacterMenu()
{
#if 0
	int x;
	QString section = "Interface";

	const char* player_classes[] = {
		"Warrior", "Cleric", "Paladin", "Ranger",
		"Shadow Knight", "Druid", "Monk", "Bard",
		"Rogue", "Shaman", "Necromancer", "Wizard",
		"Magician", "Enchanter", "Beastlord",
		"Berserker"
	};

	const char* player_races[] = {
		"Human", "Barbarian", "Erudite", "Wood elf",
		"High Elf", "Dark Elf", "Half Elf", "Dwarf",
		"Troll", "Ogre", "Halfling", "Gnome", "Iksar",
		"Vah Shir", "Froglok"
	};


	// Character Menu
	m_charMenu = new Q3PopupMenu;
	menuBar()->insertItem("&Character", m_charMenu);
	int yx = m_charMenu->insertItem("Use Auto Detected Settings", this, SLOT(toggleAutoDetectPlayerSettings(int)));
	m_charMenu->setItemChecked(yx, m_player->useAutoDetectedSettings());

	// Character -> Level
	m_charLevelMenu = new Q3PopupMenu;
	m_charMenu->insertItem("Choose &Level", m_charLevelMenu);
	m_levelSpinBox = new QSpinBox(1, 80, 1, m_charLevelMenu, "m_levelSpinBox");
#ifndef QT3_SUPPORT
	m_charLevelMenu->insertItem( m_levelSpinBox );
#endif
	m_levelSpinBox->setWrapping( true );
	m_levelSpinBox->setButtonSymbols(QSpinBox::PlusMinus);
	m_levelSpinBox->setPrefix("Level: ");
	connect(m_levelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(SetDefaultCharacterLevel(int)));
	m_levelSpinBox->setValue(m_player->defaultLevel());

	// Character -> Class
	m_charClassMenu = new Q3PopupMenu;
	m_charMenu->insertItem("Choose &Class", m_charClassMenu);
	for (int i = 0; i < PLAYER_CLASSES; i++)
	{
		char_ClassID[i] = m_charClassMenu->insertItem(player_classes[i]);
		m_charClassMenu->setItemParameter(char_ClassID[i],i+1);
		if (i + 1 == m_player->defaultClass())
			m_charMenu->setItemChecked(char_ClassID[i], true);
	}
	connect (m_charClassMenu, SIGNAL(activated(int)), this, SLOT(SetDefaultCharacterClass(int)));

	// Character -> Race
	m_charRaceMenu = new Q3PopupMenu;
	m_charMenu->insertItem("Choose &Race", m_charRaceMenu);
	for( int i = 0; i < PLAYER_RACES; i++)
	{
		char_RaceID[i] = m_charRaceMenu->insertItem(player_races[i]);
		if(i != 12 || i != 13)
			m_charRaceMenu->setItemParameter(char_RaceID[i],i+1);
		if(i == 12)
			m_charRaceMenu->setItemParameter(char_RaceID[i],128);
		else if(i == 13)
			m_charRaceMenu->setItemParameter(char_RaceID[i],130);
		else if(i == 14)
			m_charRaceMenu->setItemParameter(char_RaceID[i],330);

		if (m_charRaceMenu->itemParameter(char_RaceID[i]) == m_player->defaultRace())
			m_charRaceMenu->setItemChecked(char_RaceID[i], true);
	}
	connect(m_charRaceMenu, SIGNAL(activated(int)), this, SLOT(SetDefaultCharacterRace(int)));
#endif
}

void EQInterface::createFiltersMenu()
{
	// Filters Menu
	Q3PopupMenu* filterMenu = new Q3PopupMenu;
	menuBar()->insertItem("Fi&lters" , filterMenu);
	filterMenu->setCheckable(true);

	filterMenu->insertItem("&Reload Filters", m_filterMgr, SLOT(loadFilters()), Key_F3);
	filterMenu->insertItem("&Save Filters", m_filterMgr, SLOT(saveFilters()), Key_F4);
	filterMenu->insertItem("&Edit Filters", this, SLOT(launchFilterEditor()));
	filterMenu->insertItem("Select Fil&ter File", this, SLOT(select_filter_file()));

	filterMenu->insertItem("Reload &Zone Filters", m_filterMgr, SLOT(loadZoneFilters()), SHIFT+Key_F3);
	filterMenu->insertItem("S&ave Zone Filters", m_filterMgr, SLOT(saveZoneFilters()), SHIFT+Key_F4);
	filterMenu->insertItem("Edit Zone Fi&lters", this, SLOT(launchZoneFilterEditor()));

	filterMenu->insertItem("Re&filter Spawns", m_spawnShell, SLOT(refilterSpawns()));
	int x = filterMenu->insertItem("&Is Case Sensitive", this, SLOT(toggle_filter_Case(int)));
	filterMenu->setItemChecked(x, m_filterMgr->caseSensitive());
	x = filterMenu->insertItem("&Display Alert Info", this, SLOT(toggle_filter_AlertInfo(int)));
	filterMenu->setItemChecked(x,
							   pSEQPrefs->getPrefBool("AlertInfo", "Filters"));
	x = filterMenu->insertItem("&Use System Beep", this, SLOT(toggle_filter_UseSystemBeep(int)));
	filterMenu->setItemChecked(x, m_filterNotifications->useSystemBeep());
	x = filterMenu->insertItem("Use &Commands", this, SLOT(toggle_filter_UseCommands(int)));
	filterMenu->setItemChecked(x, m_filterNotifications->useCommands());

	uint32_t filters = pSEQPrefs->getPrefInt("Log", "Filters", 0);

	// Filter -> Log
	Q3PopupMenu* filterLogMenu = new Q3PopupMenu;
	filterLogMenu->setCheckable(true);
	filterMenu->insertItem("&Log", filterLogMenu);
	x = filterLogMenu->insertItem( "Alerts");
	filterLogMenu->setItemParameter(x, FILTER_FLAG_ALERT);
	filterLogMenu->setItemChecked(x, ((filters & FILTER_FLAG_ALERT) != 0));
	x = filterLogMenu->insertItem( "Locates");
	filterLogMenu->setItemParameter(x, FILTER_FLAG_LOCATE);
	filterLogMenu->setItemChecked(x, ((filters & FILTER_FLAG_LOCATE) != 0));
	x = filterLogMenu->insertItem( "Hunts");
	filterLogMenu->setItemParameter(x, FILTER_FLAG_HUNT);
	filterLogMenu->setItemChecked(x, ((filters & FILTER_FLAG_HUNT) != 0));
	x = filterLogMenu->insertItem( "Cautions");
	filterLogMenu->setItemParameter(x, FILTER_FLAG_CAUTION);
	filterLogMenu->setItemChecked(x, ((filters & FILTER_FLAG_CAUTION) != 0));
	x = filterLogMenu->insertItem( "Dangers");
	filterLogMenu->setItemParameter(x, FILTER_FLAG_DANGER);
	filterLogMenu->setItemChecked(x, ((filters & FILTER_FLAG_DANGER) != 0));
	connect(filterLogMenu, SIGNAL(activated(int)),
			this, SLOT(toggle_filter_Log(int)));

	// Filter -> Commands
	Q3PopupMenu* filterCmdMenu = new Q3PopupMenu;
	filterMenu->insertItem("&Audio Commands", filterCmdMenu);
	x = filterCmdMenu->insertItem( "Spawn...");
	filterCmdMenu->setItemParameter(x, 1);
	x = filterCmdMenu->insertItem( "DeSpawn...");
	filterCmdMenu->setItemParameter(x, 2);
	x = filterCmdMenu->insertItem( "Death...");
	filterCmdMenu->setItemParameter(x, 3);
	x = filterCmdMenu->insertItem( "Locate...");
	filterCmdMenu->setItemParameter(x, 4);
	x = filterCmdMenu->insertItem( "Caution...");
	filterCmdMenu->setItemParameter(x, 5);
	x = filterCmdMenu->insertItem( "Hunt...");
	filterCmdMenu->setItemParameter(x, 6);
	x = filterCmdMenu->insertItem( "Danger...");
	filterCmdMenu->setItemParameter(x, 7);
	connect(filterCmdMenu, SIGNAL(activated(int)), this, SLOT(set_filter_AudioCommand(int)));
}

void EQInterface::createInterfaceMenu()
{
	int x;
	QString section = "Interface";

	// Interface Menu
	Q3PopupMenu* pInterfaceMenu = new Q3PopupMenu;
	menuBar()->insertItem( "&Interface" , pInterfaceMenu);

	pInterfaceMenu->insertItem("Hide MenuBar", this, SLOT(toggle_view_menubar()));

	// Interface -> Style
	//pStyleMenu = new QPopupMenu;
#if 0
	Q3PopupMenu* pStyleMenu = new Q3PopupMenu;
	pInterfaceMenu->insertItem("&Style", pStyleMenu);
	pStyleMenu->setCheckable(TRUE);
	x = pStyleMenu->insertItem("Platinum (Macintosh)");
	pStyleMenu->setItemParameter(x, 1);
	IDList_StyleMenu.append(x);
	x = pStyleMenu->insertItem("Windows (Default)");
	pStyleMenu->setItemParameter(x, 2);
	IDList_StyleMenu.append(x);
	x = pStyleMenu->insertItem("CDE");
	pStyleMenu->setItemParameter(x, 3);
	IDList_StyleMenu.append(x);
	x = pStyleMenu->insertItem("CDE Polished");
	pStyleMenu->setItemParameter(x, 4);
	IDList_StyleMenu.append(x);
	x = pStyleMenu->insertItem("Motif");
	pStyleMenu->setItemParameter(x, 5);
	IDList_StyleMenu.append(x);
	x = pStyleMenu->insertItem("SGI");
	pStyleMenu->setItemParameter(x, 6);
	IDList_StyleMenu.append(x);
	connect (pStyleMenu, SIGNAL(activated(int)), this, SLOT(selectTheme(int)));

	setTheme(pSEQPrefs->getPrefInt("Theme", section, 2));
#endif

	// Interface -> Status Bar
	Q3PopupMenu* statusBarMenu = new Q3PopupMenu;
	statusBarMenu->setCheckable(true);
	pInterfaceMenu->insertItem("&Status Bar", statusBarMenu);
	statusBarMenu->insertItem("Show/Hide", this, SLOT(toggle_view_statusbar()));

	x = statusBarMenu->insertItem("Status");
	statusBarMenu->setItemParameter(x, 1);
	statusBarMenu->setItemChecked(x, pSEQPrefs->getPrefBool("ShowStatus", "Interface_StatusBar", false));

	x = statusBarMenu->insertItem("Zone");
	statusBarMenu->setItemParameter(x, 2);
	statusBarMenu->setItemChecked(x, pSEQPrefs->getPrefBool("ShowZone", "Interface_StatusBar", false));

	x = statusBarMenu->insertItem("Spawns");
	statusBarMenu->setItemParameter(x, 3);
	statusBarMenu->setItemChecked(x, pSEQPrefs->getPrefBool("ShowSpawns", "Interface_StatusBar", false));

	x = statusBarMenu->insertItem("Experience");
	statusBarMenu->setItemParameter(x, 4);
	statusBarMenu->setItemChecked(x, pSEQPrefs->getPrefBool("ShowExp", "Interface_StatusBar", false));

	x = statusBarMenu->insertItem("AA Experience");
	statusBarMenu->setItemParameter(x, 5);
	statusBarMenu->setItemChecked(x, pSEQPrefs->getPrefBool("ShowExpAA", "Interface_StatusBar", false));

	x = statusBarMenu->insertItem("Packet Counter");
	statusBarMenu->setItemParameter(x, 6);
	statusBarMenu->setItemChecked(x, pSEQPrefs->getPrefBool("ShowPacketCounter", "Interface_StatusBar", false));

	x = statusBarMenu->insertItem("EQ Time");
	statusBarMenu->setItemParameter(x, 7);
	statusBarMenu->setItemChecked(x, pSEQPrefs->getPrefBool("ShowEQTime", "Interface_StatusBar", false));
	x = statusBarMenu->insertItem("Run Speed");
	statusBarMenu->setItemParameter(x, 8);
	statusBarMenu->setItemChecked(x, pSEQPrefs->getPrefBool("ShowSpeed", "Interface_StatusBar", false));

	// ZEM code
	x = statusBarMenu->insertItem("ZEM");
	statusBarMenu->setItemParameter(x, 9);
	statusBarMenu->setItemChecked(x, pSEQPrefs->getPrefBool("ShowZEM", "Interface_StatusBar", false));

	connect(statusBarMenu, SIGNAL(activated(int)), this, SLOT(toggle_main_statusbar_Window(int)));

	m_terminalMenu = new Q3PopupMenu;
	pInterfaceMenu->insertItem("&Terminal", m_terminalMenu);

	m_terminalTypeFilterMenu = new Q3PopupMenu;
	m_terminalMenu->insertItem("MessageTypeFilter", m_terminalTypeFilterMenu);
	m_terminalTypeFilterMenu->insertItem("&Enable All", this, SLOT(enableAllTypeFilters()), 0, 64);
	m_terminalTypeFilterMenu->insertItem("&Disable All", this, SLOT(disableAllTypeFilters()), 0, 65);

	m_terminalTypeFilterMenu->insertSeparator(-1);

	QString typeName;
	uint64_t enabledTypes = m_terminal->enabledTypes();

	// iterate over the message types, filling in various menus and getting
	// font color preferences
	for (int i = MT_Guild; i <= MT_Max; i++)
	{
		typeName = MessageEntry::messageTypeString((MessageType)i);
		if (!typeName.isEmpty())
		{
			m_terminalTypeFilterMenu->insertItem(typeName, i);
			m_terminalTypeFilterMenu->setItemChecked(i, (((uint64_t(1) << i) & enabledTypes) != 0));
		}
	}

	connect(m_terminalTypeFilterMenu, SIGNAL(activated(int)), this, SLOT(toggleTypeFilter(int)));

	m_terminalShowUserFilterMenu = new Q3PopupMenu;
	m_terminalShowUserFilterMenu->insertItem("&Enable All", this, SLOT(enableAllShowUserFilters()), 0, 66);
	m_terminalShowUserFilterMenu->insertItem("&Disable All", this, SLOT(disableAllShowUserFilters()), 0, 67);
	m_terminalShowUserFilterMenu->insertSeparator(-1);
	m_terminalMenu->insertItem("User Message Filter - &Show", m_terminalShowUserFilterMenu);

	m_terminalHideUserFilterMenu = new Q3PopupMenu;
	m_terminalHideUserFilterMenu->insertItem("&Enable All", this, SLOT(enableAllHideUserFilters()), 0, 66);
	m_terminalHideUserFilterMenu->insertItem("&Disable All", this, SLOT(disableAllHideUserFilters()), 0, 67);
	m_terminalHideUserFilterMenu->insertSeparator(-1);
	m_terminalMenu->insertItem("User Message Filter - &Hide", m_terminalHideUserFilterMenu);

	uint32_t enabledShowUserFilters = m_terminal->enabledShowUserFilters();
	uint32_t enabledHideUserFilters = m_terminal->enabledHideUserFilters();
	const MessageFilter* filter;
	for(int i = 0; i < maxMessageFilters; i++)
	{
		filter = m_messageFilters->filter(i);
		if (filter)
		{
			m_terminalShowUserFilterMenu->insertItem(filter->name(), i);
			m_terminalShowUserFilterMenu->setItemChecked(i, (1 << i) & enabledShowUserFilters);

			m_terminalHideUserFilterMenu->insertItem(filter->name(), i);
			m_terminalHideUserFilterMenu->setItemChecked(i, (1 << i) & enabledHideUserFilters);
		}
	}

	connect(m_terminalShowUserFilterMenu, SIGNAL(activated(int)), this, SLOT(toggleShowUserFilter(int)));
	connect(m_terminalHideUserFilterMenu, SIGNAL(activated(int)), this, SLOT(toggleHideUserFilter(int)));

	m_terminalMenu->insertItem("Edit User &Message Filters...", this, SLOT(showMessageFilterDialog()));
	m_terminalMenu->insertSeparator(-1);

	x = m_terminalMenu->insertItem("&Display Type", this, SLOT(toggleDisplayType(int)));
	m_terminalMenu->setItemChecked(x, m_terminal->displayType());

	x = m_terminalMenu->insertItem("Display T&ime/Date", this, SLOT(toggleDisplayTime(int)));
	m_terminalMenu->setItemChecked(x, m_terminal->displayDateTime());

	x = m_terminalMenu->insertItem("Display &EQ Date/Time", this, SLOT(toggleEQDisplayTime(int)));
	m_terminalMenu->setItemChecked(x, m_terminal->displayEQDateTime());

	x = m_terminalMenu->insertItem("&Use Color", this, SLOT(toggleUseColor(int)));
	m_terminalMenu->setItemChecked(x, m_terminal->useColor());


	pInterfaceMenu->insertItem( "Formatted Messages File...", this, SLOT(select_main_FormatFile(int)));
	pInterfaceMenu->insertItem( "Spells File...", this, SLOT(select_main_SpellsFile(int)));
}

void EQInterface::createWindowMenu()
{
	int x;
	QString section = "Interface";

	// insert Window menu
	menuBar()->insertItem("&Window", m_windowMenu);

	// All of the Window menu items that don't automatically get inserted
	// have to be manually placed in the right positions.

	// Interface -> WindowCaption
	m_windowCaptionMenu = new Q3PopupMenu;
	m_windowMenu->insertItem( "Window &Caption", m_windowCaptionMenu, -1, 0);

	x = m_windowCaptionMenu->insertItem("&Main Window...");
	m_windowCaptionMenu->setItemParameter(x, 5);

	x = m_windowCaptionMenu->insertItem("Spawn &List...");
	m_windowCaptionMenu->setItemParameter(x, 0);

	x = m_windowCaptionMenu->insertItem("Spawn List &2...");
	m_windowCaptionMenu->setItemParameter(x, 10);

	x = m_windowCaptionMenu->insertItem("Spawn P&oint List...");
	m_windowCaptionMenu->setItemParameter(x, 9);

	x = m_windowCaptionMenu->insertItem("&Player Stats...");
	m_windowCaptionMenu->setItemParameter(x, 1);

	x = m_windowCaptionMenu->insertItem("Player &Skills...");
	m_windowCaptionMenu->setItemParameter(x, 2);

	x = m_windowCaptionMenu->insertItem("Spell L&ist...");
	m_windowCaptionMenu->setItemParameter(x, 3);

	x = m_windowCaptionMenu->insertItem("&Compass...");
	m_windowCaptionMenu->setItemParameter(x, 4);

	x = m_windowCaptionMenu->insertItem("&Experience Window...");
	m_windowCaptionMenu->setItemParameter(x, 6);

	x = m_windowCaptionMenu->insertItem("Comb&at Window...");
	m_windowCaptionMenu->setItemParameter(x, 7);

	x = m_windowCaptionMenu->insertItem("&Network Diagnostics...");
	m_windowCaptionMenu->setItemParameter(x, 8);

	// insert Map docking options
	// NOTE: Always insert Map docking options at the end of the Docked menu
	for (int i = 0; i < maxNumMaps; i++)
	{
        QString mapName = "Map";
        if (i > 0)
            mapName += QString::number(i + 1);
        x = m_windowCaptionMenu->insertItem(mapName);
        m_windowCaptionMenu->setItemParameter(x, i + mapCaptionBase);
	}

	connect (m_windowCaptionMenu, SIGNAL(activated(int)),
			 this, SLOT(set_main_WindowCaption(int)));

	// Interface -> Window Font
	Q3PopupMenu* windowFontMenu = new Q3PopupMenu;
	m_windowMenu->insertItem("&Font", windowFontMenu, -1, 1);

	windowFontMenu->insertItem("&Application Default...", this, SLOT(set_main_Font(int)));

	windowFontMenu->insertItem("Main Window Status Font...", this, SLOT(set_main_statusbar_Font(int)));
	//   x = windowFontMenu->insertItem("&Main Window");
	//   windowFontMenu->setItemParameter(x, 5);

	x = windowFontMenu->insertItem("Spawn &List...");
	windowFontMenu->setItemParameter(x, 0);

	x = windowFontMenu->insertItem("Spawn List &2...");
	windowFontMenu->setItemParameter(x, 10);

	x = windowFontMenu->insertItem("Spawn P&oint List...");
	windowFontMenu->setItemParameter(x, 9);

	x = windowFontMenu->insertItem("&Player Stats...");
	windowFontMenu->setItemParameter(x, 1);

	x = windowFontMenu->insertItem("Player &Skills...");
	windowFontMenu->setItemParameter(x, 2);

	x = windowFontMenu->insertItem("Spell L&ist...");
	windowFontMenu->setItemParameter(x, 3);

	x = windowFontMenu->insertItem("&Compass...");
	windowFontMenu->setItemParameter(x, 4);

	x = windowFontMenu->insertItem("&Experience Window...");
	windowFontMenu->setItemParameter(x, 6);

	x = windowFontMenu->insertItem("Comb&at Window...");
	windowFontMenu->setItemParameter(x, 7);

	x = windowFontMenu->insertItem("&Network Diagnostics...");
	windowFontMenu->setItemParameter(x, 8);

	// insert Map docking options
	// NOTE: Always insert Map docking options at the end of the Docked menu
	for (int i = 0; i < maxNumMaps; i++)
	{
        QString mapName = "Map";
        if (i > 0)
            mapName += QString::number(i + 1);
        x = m_windowCaptionMenu->insertItem(mapName);
        m_windowCaptionMenu->setItemParameter(x, i + mapCaptionBase);
	}

	connect (windowFontMenu, SIGNAL(activated(int)), this, SLOT(set_main_WindowFont(int)));


	x = m_windowMenu->insertItem("Save Window Sizes && Positions", this, SLOT(toggle_main_SavePosition(int)), 0, -1, 2);
	m_windowMenu->setItemChecked(x, pSEQPrefs->getPrefBool("SavePosition", "Interface", true));
	x = m_windowMenu->insertItem("Restore Window Positions", this, SLOT(toggle_main_UseWindowPos(int)), 0, -1, 3);
	m_windowMenu->setItemChecked(x, pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true));
	m_windowMenu->insertSeparator(4);
}

void EQInterface::createDebugMenu()
{
	////////////////////////////////////////////////////////////////
	// Debug menu
	QMenu* pDebugMenu = new QMenu;
	menuBar()->insertItem("&Debug", pDebugMenu);
	pDebugMenu->insertItem("List I&nterface", this, SLOT(listInterfaceInfo()));
	pDebugMenu->insertItem("List S&pawns", this, SLOT(listSpawns()), ALT+CTRL+Key_P);
	pDebugMenu->insertItem("List &Drops", this, SLOT(listDrops()), ALT+CTRL+Key_D);
	pDebugMenu->insertItem("List &Map Info", this, SLOT(listMapInfo()), ALT+CTRL+Key_M);
	pDebugMenu->insertItem("List G&uild Info", m_guildmgr, SLOT(listGuildInfo()));
	pDebugMenu->insertItem("List &Group", this, SLOT(listGroup()), ALT+CTRL+Key_G);
	pDebugMenu->insertItem("List Guild M&embers", this, SLOT(listGuild()), ALT+CTRL+Key_E);
	pDebugMenu->insertItem("Dump Spawns", this, SLOT(dumpSpawns()), ALT+SHIFT+CTRL+Key_P);
	pDebugMenu->insertItem("Dump Drops", this, SLOT(dumpDrops()), ALT+SHIFT+CTRL+Key_D);
	pDebugMenu->insertItem("Dump Map Info", this, SLOT(dumpMapInfo()), ALT+SHIFT+CTRL+Key_M);
	pDebugMenu->insertItem("Dump Guild Info", this , SLOT(dumpGuildInfo()));
	pDebugMenu->insertItem("Dump SpellBook Info", this , SLOT(dumpSpellBook()));
	pDebugMenu->insertItem("Dump Group", this, SLOT(dumpGroup()), ALT+CTRL+SHIFT+Key_G);
	pDebugMenu->insertItem("Dump Guild Members", this, SLOT(dumpGuild()), ALT+CTRL+SHIFT+Key_E);
	pDebugMenu->insertItem("List &Filters", m_filterMgr, SLOT(listFilters()), ALT+CTRL+Key_F);
	pDebugMenu->insertItem("List &Zone Filters", m_filterMgr, SLOT(listZoneFilters()));
}

void EQInterface::createStatusBar()
{
	QString statusBarSection = "Interface_StatusBar";
	int sts_widget_count = 0; // total number of widgets visible on status bar

	//Status widget
	m_stsbarStatus = new QLabel(statusBar(), "Status");
	m_stsbarStatus->setMinimumWidth(80);
	m_stsbarStatus->setText(QString("ShowEQ %1").arg(VERSION));
	statusBar()->addWidget(m_stsbarStatus, 8);

	//Zone widget
	m_stsbarZone = new QLabel(statusBar(), "Zone");
	m_stsbarZone->setText("Zone: [unknown]");
	statusBar()->addWidget(m_stsbarZone, 2);

	//Mobs widget
	m_stsbarSpawns = new QLabel(statusBar(), "Mobs");
	m_stsbarSpawns->setText("Mobs:");
	statusBar()->addWidget(m_stsbarSpawns, 1);

	//Exp widget
	m_stsbarExp = new QLabel(statusBar(), "Exp");
	m_stsbarExp->setText("Exp [unknown]");
	statusBar()->addWidget(m_stsbarExp, 2);

	//ExpAA widget
	m_stsbarExpAA = new QLabel(statusBar(), "ExpAA");
	m_stsbarExpAA->setText("ExpAA [unknown]");
	statusBar()->addWidget(m_stsbarExpAA, 2);

	//Pkt widget
	m_stsbarPkt = new QLabel(statusBar(), "Pkt");
	m_stsbarPkt->setText("Pkt 0");
	statusBar()->addWidget(m_stsbarPkt, 1);

	//EQTime widget
	m_stsbarEQTime = new QLabel(statusBar(), "EQTime");
	m_stsbarEQTime->setText("EQTime");
	statusBar()->addWidget(m_stsbarEQTime, 1);

	// Run Speed widget
	m_stsbarSpeed = new QLabel(statusBar(), "Speed");
	m_stsbarSpeed->setText("Run Speed:");
	statusBar()->addWidget(m_stsbarSpeed, 1);

	// ZEM code
	// Zone Exp Mult widget
	m_stsbarZEM = new QLabel(statusBar(), "ZEM");
	m_stsbarZEM->setText("ZEM: [unknown]");
	statusBar()->addWidget(m_stsbarZEM, 1);


		// setup the status fonts correctly
	restoreStatusFont();

	if (!pSEQPrefs->getPrefBool("ShowStatus", statusBarSection, true))
		m_stsbarStatus->hide();
	else
		sts_widget_count++;

	if (!pSEQPrefs->getPrefBool("ShowZone", statusBarSection, true))
		m_stsbarZone->hide();
	else
		sts_widget_count++;

	if (!pSEQPrefs->getPrefBool("ShowSpawns", statusBarSection, false))
		m_stsbarSpawns->hide();
	else
		sts_widget_count++;

	if (!pSEQPrefs->getPrefBool("ShowExp", statusBarSection, false))
		m_stsbarExp->hide();
	else
		sts_widget_count++;

	if (!pSEQPrefs->getPrefBool("ShowExpAA", statusBarSection, false))
		m_stsbarExpAA->hide();
	else
		sts_widget_count++;

	if (!pSEQPrefs->getPrefBool("ShowPacketCounter", statusBarSection, false))
		m_stsbarPkt->hide();
	else
		sts_widget_count++;

	if (!pSEQPrefs->getPrefBool("ShowEQTime", statusBarSection, true))
		m_stsbarEQTime->hide();
	else
		sts_widget_count++;

	if (!pSEQPrefs->getPrefBool("ShowSpeed", statusBarSection, false))
		m_stsbarSpeed->hide();
	else
		sts_widget_count++;

	// ZEM code
	if (!pSEQPrefs->getPrefBool("ShowZEM", statusBarSection, false))
		m_stsbarZEM->hide();
	else
		sts_widget_count++;


	//hide the statusbar if no visible widgets
	if (!sts_widget_count || !pSEQPrefs->getPrefBool("StatusBarActive", statusBarSection, 1))
		statusBar()->hide();
}

void EQInterface::connectSignals()
{
	// connect EQInterface slots to its own signals
	connect(this, SIGNAL(restoreFonts()), this, SLOT(restoreStatusFont()));

	// connect MapMgr slots to interface signals
	connect(this, SIGNAL(saveAllPrefs()), m_mapMgr, SLOT(savePrefs()));

	// connect CategoryMgr slots to interface signals
	connect(this, SIGNAL(saveAllPrefs()), m_categoryMgr, SLOT(savePrefs()));

	if (m_zoneMgr)
	{
		m_packet->connect2("OP_ZoneEntry", SP_Zone, DIR_Client, "ClientZoneEntryStruct", SZC_Match,
						   m_zoneMgr, SLOT(zoneEntryClient(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_PlayerProfile", SP_Zone, DIR_Server, "charProfileStruct", SZC_Match,
						   m_zoneMgr, SLOT(zonePlayer(const uint8_t*)));
		m_packet->connect2("OP_ZoneChange", SP_Zone, DIR_Client|DIR_Server, "zoneChangeStruct", SZC_Match,
						   m_zoneMgr, SLOT(zoneChange(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_NewZone", SP_Zone, DIR_Server, "newZoneStruct", SZC_Match,
						   m_zoneMgr, SLOT(zoneNew(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_SendZonePoints", SP_Zone, DIR_Server, "zonePointsStruct", SZC_None,
						   m_zoneMgr, SLOT(zonePoints(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_DzSwitchInfo", SP_Zone, DIR_Server, "dzSwitchInfo", SZC_None,
						   m_zoneMgr, SLOT(dynamicZonePoints(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_DzInfo", SP_Zone, DIR_Server, "dzInfo", SZC_Match,
						   m_zoneMgr, SLOT(dynamicZoneInfo(const uint8_t*, size_t, uint8_t)));
	}

	if (m_groupMgr)
	{
		connect(m_zoneMgr, SIGNAL(playerProfile(const charProfileStruct*)),
				m_groupMgr, SLOT(player(const charProfileStruct*)));
		m_packet->connect2("OP_GroupUpdate", SP_Zone, DIR_Server, "uint8_t", SZC_None,
						   m_groupMgr, SLOT(groupUpdate(const uint8_t*, size_t)));
		m_packet->connect2("OP_GroupFollow", SP_Zone, DIR_Server, "groupFollowStruct", SZC_Match,
						   m_groupMgr, SLOT(addGroupMember(const uint8_t*)));
		m_packet->connect2("OP_GroupDisband", SP_Zone, DIR_Server, "groupDisbandStruct", SZC_Match,
						   m_groupMgr, SLOT(removeGroupMember(const uint8_t*)));
		m_packet->connect2("OP_GroupDisband2", SP_Zone, DIR_Server, "groupDisbandStruct", SZC_Match,
						   m_groupMgr, SLOT(removeGroupMember(const uint8_t*)));

		// connect GroupMgr slots to SpawnShell signals
		connect(m_spawnShell, SIGNAL(addItem(const Item*)), m_groupMgr, SLOT(addItem(const Item*)));
		// connect GroupMgr slots to SpawnShell signals
		connect(m_spawnShell, SIGNAL(delItem(const Item*)), m_groupMgr, SLOT(delItem(const Item*)));
		// connect GroupMgr slots to SpawnShell signals
		connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)), m_groupMgr, SLOT(killSpawn(const Item*)));
	}

	if (m_dateTimeMgr)
	{
		// connect DateTimeMgr slots to EQPacket signals
		m_packet->connect2("OP_TimeOfDay", SP_Zone, DIR_Server,  "timeOfDayStruct", SZC_Match,
						   m_dateTimeMgr, SLOT(timeOfDay(const uint8_t*)));

		// connect interface slots to DateTimeMgr signals
		connect(m_dateTimeMgr, SIGNAL(updatedDateTime(const QDateTime&)), this, SLOT(updatedDateTime(const QDateTime&)));
		connect(m_dateTimeMgr, SIGNAL(syncDateTime(const QDateTime&)), this, SLOT(syncDateTime(const QDateTime&)));
	}

	if (m_filterMgr)
	{
		connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)), m_filterMgr, SLOT(loadZone(const QString&)));
		connect(m_zoneMgr, SIGNAL(zoneEnd(const QString&, const QString&)), m_filterMgr, SLOT(loadZone(const QString&)));
		connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)), m_filterMgr, SLOT(loadZone(const QString&)));
	}

	if (m_guildmgr)
	{
		m_packet->connect2("OP_GuildList", SP_World, DIR_Server, "worldGuildListStruct", SZC_Match,
						   m_guildmgr, SLOT(worldGuildList(const uint8_t*, size_t)));

		connect(this, SIGNAL(guildList2text(QString)), m_guildmgr, SLOT(guildList2text(QString)));
	}

	if (m_guildShell)
	{
		m_packet->connect2("OP_GuildMemberList", SP_Zone, DIR_Server, "uint8_t", SZC_None,
						   m_guildShell, SLOT(guildMemberList(const uint8_t*, size_t)));
		m_packet->connect2("OP_GuildMemberUpdate", SP_Zone, DIR_Server, "GuildMemberUpdate", SZC_Match,
						   m_guildShell, SLOT(guildMemberUpdate(const uint8_t*, size_t)));
	}

	if (m_messageShell)
	{
		m_packet->connect2("OP_CommonMessage", SP_Zone, DIR_Client|DIR_Server, "channelMessageStruct", SZC_None,
						   m_messageShell, SLOT(channelMessage(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_FormattedMessage", SP_Zone, DIR_Server, "formattedMessageStruct", SZC_None,
						   m_messageShell, SLOT(formattedMessage(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_SimpleMessage", SP_Zone, DIR_Server, "simpleMessageStruct", SZC_Match,
						   m_messageShell, SLOT(simpleMessage(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_SpecialMesg", SP_Zone, DIR_Server, "specialMessageStruct", SZC_None,
						   m_messageShell, SLOT(specialMessage(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_GuildMOTD", SP_Zone, DIR_Server, "guildMOTDStruct", SZC_None,
						   m_messageShell, SLOT(guildMOTD(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_RandomReq", SP_Zone, DIR_Client, "randomReqStruct", SZC_Match,
						   m_messageShell, SLOT(randomRequest(const uint8_t*)));
		m_packet->connect2("OP_RandomReply", SP_Zone, DIR_Server, "randomStruct", SZC_Match,
						   m_messageShell, SLOT(random(const uint8_t*)));
		m_packet->connect2("OP_ConsentResponse", SP_Zone, DIR_Server, "consentResponseStruct", SZC_Match,
						   m_messageShell, SLOT(consent(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_DenyResponse", SP_Zone, DIR_Server, "consentResponseStruct", SZC_Match,
						   m_messageShell, SLOT(consent(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_Emote", SP_Zone, DIR_Server|DIR_Client, "emoteTextStruct", SZC_None,
						   m_messageShell, SLOT(emoteText(const uint8_t*)));
		m_packet->connect2("OP_InspectAnswer", SP_Zone, DIR_Server, "inspectDataStruct", SZC_Match,
						   m_messageShell, SLOT(inspectData(const uint8_t*)));
		m_packet->connect2("OP_MoneyOnCorpse", SP_Zone, DIR_Server, "moneyOnCorpseStruct", SZC_Match,
						   m_messageShell, SLOT(moneyOnCorpse(const uint8_t*)));
		m_packet->connect2("OP_Logout", SP_Zone, DIR_Server, "none", SZC_Match,
						   m_messageShell, SLOT(logOut(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_NewZone", SP_Zone, DIR_Server, "newZoneStruct", SZC_Match,
						   m_messageShell, SLOT(zoneNew(const uint8_t*, size_t, uint8_t)));

		connect(m_zoneMgr, SIGNAL(zoneBegin(const ClientZoneEntryStruct*, size_t, uint8_t)),
				m_messageShell, SLOT(zoneEntryClient(const ClientZoneEntryStruct*)));
		connect(m_zoneMgr, SIGNAL(zoneChanged(const zoneChangeStruct*, size_t, uint8_t)),
				m_messageShell, SLOT(zoneChanged(const zoneChangeStruct*, size_t, uint8_t)));
		connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)),
				m_messageShell, SLOT(zoneBegin(const QString&)));
		connect(m_zoneMgr, SIGNAL(zoneEnd(const QString&, const QString&)),
				m_messageShell, SLOT(zoneEnd(const QString&, const QString&)));
		connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)),
				m_messageShell, SLOT(zoneChanged(const QString&)));

		m_packet->connect2("OP_MOTD", SP_World, DIR_Server, "worldMOTDStruct", SZC_None,
						   m_messageShell, SLOT(worldMOTD(const uint8_t*)));
		m_packet->connect2("OP_MemorizeSpell", SP_Zone, DIR_Server|DIR_Client, "memSpellStruct", SZC_Match,
						   m_messageShell, SLOT(handleSpell(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_BeginCast", SP_Zone, DIR_Server|DIR_Client, "beginCastStruct", SZC_Match,
						   m_messageShell, SLOT(beginCast(const uint8_t*)));
		m_packet->connect2("OP_BuffFadeMsg", SP_Zone, DIR_Server|DIR_Client, "spellFadedStruct", SZC_None,
						   m_messageShell, SLOT(spellFaded(const uint8_t*)));
		m_packet->connect2("OP_CastSpell", SP_Zone, DIR_Server|DIR_Client, "startCastStruct", SZC_Match,
						   m_messageShell, SLOT(startCast(const uint8_t*)));

		connect(m_zoneMgr, SIGNAL(playerProfile(const charProfileStruct*)), m_messageShell, SLOT(player(const charProfileStruct*)));

		m_packet->connect2("OP_SkillUpdate", SP_Zone, DIR_Server, "skillIncStruct", SZC_Match,
						   m_messageShell, SLOT(increaseSkill(const uint8_t*)));
		m_packet->connect2("OP_LevelUpdate", SP_Zone, DIR_Server, "levelUpUpdateStruct", SZC_Match,
						   m_messageShell, SLOT(updateLevel(const uint8_t*)));

		m_packet->connect2("OP_Consider", SP_Zone, DIR_Server, "considerStruct", SZC_Match,
						   m_messageShell, SLOT(consMessage(const uint8_t*, size_t, uint8_t)));

		connect(m_player, SIGNAL(setExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)),
				m_messageShell, SLOT(setExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)));
		connect(m_player, SIGNAL(newExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)),
				m_messageShell, SLOT(newExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)));
		connect(m_player, SIGNAL(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)),
				m_messageShell, SLOT(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)));
		connect(m_player, SIGNAL(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)),
				m_messageShell, SLOT(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)));

		connect(m_spawnShell, SIGNAL(addItem(const Item*)), m_messageShell, SLOT(addItem(const Item*)));
		connect(m_spawnShell, SIGNAL(delItem(const Item*)),	m_messageShell, SLOT(delItem(const Item*)));
		connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)), m_messageShell, SLOT(killSpawn(const Item*)));

		connect(m_dateTimeMgr, SIGNAL(syncDateTime(const QDateTime&)), m_messageShell, SLOT(syncDateTime(const QDateTime&)));

		// 9/3/2008 - Removed.  Serialized packet now.
		//      m_packet->connect2("OP_GroupUpdate", SP_Zone, DIR_Server,
		// 			"groupUpdateStruct", SZC_None,
		// 			m_messageShell, SLOT(groupUpdate(const uint8_t*, size_t, uint8_t)));
		m_packet->connect2("OP_GroupInvite", SP_Zone, DIR_Client, "groupInviteStruct", SZC_Match,
						   m_messageShell, SLOT(groupInvite(const uint8_t*)));
		m_packet->connect2("OP_GroupInvite", SP_Zone, DIR_Server, "groupAltInviteStruct", SZC_Match,
						   m_messageShell, SLOT(groupInvite(const uint8_t*)));
		m_packet->connect2("OP_GroupInvite2", SP_Zone, DIR_Client, "groupInviteStruct", SZC_Match,
						   m_messageShell, SLOT(groupInvite(const uint8_t*)));
		m_packet->connect2("OP_GroupFollow", SP_Zone, DIR_Server, "groupFollowStruct", SZC_Match,
						   m_messageShell, SLOT(groupFollow(const uint8_t*)));
		m_packet->connect2("OP_GroupFollow2", SP_Zone, DIR_Server, "groupFollowStruct", SZC_Match,
						   m_messageShell, SLOT(groupFollow(const uint8_t*)));
		m_packet->connect2("OP_GroupDisband", SP_Zone, DIR_Server, "groupDisbandStruct", SZC_Match,
						   m_messageShell, SLOT(groupDisband(const uint8_t*)));
		m_packet->connect2("OP_GroupDisband2", SP_Zone, DIR_Server, "groupDisbandStruct", SZC_Match,
						   m_messageShell, SLOT(groupDisband(const uint8_t*)));
		m_packet->connect2("OP_GroupCancelInvite", SP_Zone, DIR_Server|DIR_Client, "groupDeclineStruct", SZC_Match,
						   m_messageShell, SLOT(groupDecline(const uint8_t*)));
		m_packet->connect2("OP_GroupLeader", SP_Zone, DIR_Server, "groupLeaderChangeStruct", SZC_Match,
						   m_messageShell, SLOT(groupLeaderChange(const uint8_t*)));
	}

	if (m_filterNotifications)
	{
		connect(m_spawnShell, SIGNAL(addItem(const Item*)), m_filterNotifications, SLOT(addItem(const Item*)));
		connect(m_spawnShell, SIGNAL(delItem(const Item*)), m_filterNotifications, SLOT(delItem(const Item*)));
		connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)), m_filterNotifications, SLOT(killSpawn(const Item*)));
		connect(m_spawnShell, SIGNAL(changeItem(const Item*, uint32_t)), m_filterNotifications, SLOT(changeItem(const Item*, uint32_t)));
	}

	// connect interface slots to Packet signals
	m_packet->connect2("OP_TargetMouse", SP_Zone, DIR_Client|DIR_Server, "clientTargetStruct", SZC_Match,
					   this, SLOT(clientTarget(const uint8_t*)));
#if 0 // ZBTEMP
	connect(m_packet, SIGNAL(attack2Hand1(const uint8_t*, size_t, uint8_t)), this, SLOT(attack2Hand1(const uint8_t*)));
#endif
	m_packet->connect2("OP_Action2", SP_Zone, DIR_Client|DIR_Server, "action2Struct", SZC_Match,
					   this, SLOT(action2Message(const uint8_t*)));
	m_packet->connect2("OP_Death", SP_Zone, DIR_Server, "newCorpseStruct", SZC_Match,
					   this, SLOT(combatKillSpawn(const uint8_t*)));
#if 0 // ZBTEMP
	connect(m_packet, SIGNAL(interruptSpellCast(const uint8_t*, size_t, uint8_t)), this, SLOT(interruptSpellCast(const uint8_t*)));
	connect(m_packet, SIGNAL(moneyUpdate(const uint8_t*, size_t, uint8_t)), this, SLOT(moneyUpdate(const uint8_t*)));
	connect(m_packet, SIGNAL(moneyThing(const uint8_t*, size_t, uint8_t)), this, SLOT(moneyThing(const uint8_t*)));
#endif // ZBTEMP

	connect(m_packet, SIGNAL(toggle_session_tracking(bool)), this, SLOT(toggle_net_session_tracking(bool)));

	// connect EQInterface slots to ZoneMgr signals
	connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)), this, SLOT(zoneBegin(const QString&)));
	connect(m_zoneMgr, SIGNAL(zoneEnd(const QString&, const QString&)), this, SLOT(zoneEnd(const QString&, const QString&)));
	connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)), this, SLOT(zoneChanged(const QString&)));

	// connect the SpellShell slots to EQInterface signals
	connect(this, SIGNAL(spellMessage(QString&)), m_spellShell, SLOT(spellMessage(QString&)));

	// connect EQInterface slots to SpawnShell signals
	connect(m_spawnShell, SIGNAL(addItem(const Item*)), this, SLOT(addItem(const Item*)));
	connect(m_spawnShell, SIGNAL(delItem(const Item*)), this, SLOT(delItem(const Item*)));
	connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)), this, SLOT(killSpawn(const Item*)));
	connect(m_spawnShell, SIGNAL(changeItem(const Item*, uint32_t)), this, SLOT(changeItem(const Item*)));
	connect(m_spawnShell, SIGNAL(spawnConsidered(const Item*)), this, SLOT(spawnConsidered(const Item*)));

	// connect the SpawnShell slots to Packet signals
	m_packet->connect2("OP_GroundSpawn", SP_Zone, DIR_Server, "makeDropStruct", SZC_Match,
					   m_spawnShell, SLOT(newGroundItem(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_ClickObject", SP_Zone, DIR_Server, "remDropStruct", SZC_Match,
					   m_spawnShell, SLOT(removeGroundItem(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_SpawnDoor", SP_Zone, DIR_Server, "doorStruct", SZC_Modulus,
					   m_spawnShell, SLOT(newDoorSpawns(const uint8_t*, size_t, uint8_t)));
	// OP_NewSpawn is deprecated in the client
	//    m_packet->connect2("OP_NewSpawn", SP_Zone, DIR_Server,
	// 		      "spawnStruct", SZC_Match,
	// 		      m_spawnShell, SLOT(newSpawn(const uint8_t*)));
	m_packet->connect2("OP_ZoneEntry", SP_Zone, DIR_Server, "uint8_t", SZC_None,
					   m_spawnShell, SLOT(zoneEntry(const uint8_t*, size_t)));
	m_packet->connect2("OP_MobUpdate", SP_Zone, DIR_Server|DIR_Client, "spawnPositionUpdate", SZC_Match,
					   m_spawnShell, SLOT(updateSpawns(const uint8_t*)));
	m_packet->connect2("OP_WearChange", SP_Zone, DIR_Server|DIR_Client, "SpawnUpdateStruct", SZC_Match,
					   m_spawnShell, SLOT(updateSpawnInfo(const uint8_t*)));
	m_packet->connect2("OP_HPUpdate", SP_Zone, DIR_Server|DIR_Client, "hpNpcUpdateStruct", SZC_Match,
					   m_spawnShell, SLOT(updateNpcHP(const uint8_t*)));
	m_packet->connect2("OP_DeleteSpawn", SP_Zone, DIR_Server|DIR_Client, "deleteSpawnStruct", SZC_Match,
					   m_spawnShell, SLOT(deleteSpawn(const uint8_t*)));
	m_packet->connect2("OP_SpawnRename", SP_Zone, DIR_Server, "spawnRenameStruct", SZC_Match,
					   m_spawnShell, SLOT(renameSpawn(const uint8_t*)));
	m_packet->connect2("OP_Illusion", SP_Zone, DIR_Server|DIR_Client, "spawnIllusionStruct", SZC_Match,
					   m_spawnShell, SLOT(illusionSpawn(const uint8_t*)));
	m_packet->connect2("OP_SpawnAppearance", SP_Zone, DIR_Server|DIR_Client, "spawnAppearanceStruct", SZC_Match,
					   m_spawnShell, SLOT(updateSpawnAppearance(const uint8_t*)));
	m_packet->connect2("OP_Death", SP_Zone, DIR_Server, "newCorpseStruct", SZC_Match,
					   m_spawnShell, SLOT(killSpawn(const uint8_t*)));
	m_packet->connect2("OP_RespawnFromHover", SP_Zone, DIR_Server|DIR_Client, "uint8_t", SZC_None,
					   m_spawnShell, SLOT(respawnFromHover(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_Shroud", SP_Zone, DIR_Server, "spawnShroudSelf", SZC_None,
					   m_spawnShell, SLOT(shroudSpawn(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_RemoveSpawn", SP_Zone, DIR_Server|DIR_Client, "removeSpawnStruct", SZC_None,
					   m_spawnShell, SLOT(removeSpawn(const uint8_t*, size_t, uint8_t)));
#if 0 // ZBTEMP
	connect(m_packet, SIGNAL(spawnWearingUpdate(const uint8_t*, size_t, uint8_t)), m_spawnShell, SLOT(spawnWearingUpdate(const uint8_t*)));
#endif
	m_packet->connect2("OP_Consider", SP_Zone, DIR_Server|DIR_Client, "considerStruct", SZC_Match,
					   m_spawnShell, SLOT(consMessage(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_NpcMoveUpdate", SP_Zone, DIR_Server, "uint8_t", SZC_None,
					   m_spawnShell, SLOT(npcMoveUpdate(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_ClientUpdate", SP_Zone, DIR_Server, "playerSpawnPosStruct", SZC_Match,
					   m_spawnShell, SLOT(playerUpdate(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_CorpseLocResponse", SP_Zone, DIR_Server, "corpseLocStruct", SZC_Match,
					   m_spawnShell, SLOT(corpseLoc(const uint8_t*)));
#if 0 // No longer used as of 5-22-2008
	m_packet->connect2("OP_ZoneSpawns", SP_Zone, DIR_Server,
					   "spawnStruct", SZC_None,
					   m_spawnShell, SLOT(zoneSpawns(const uint8_t*, size_t)));
#endif

	// connect the SpellShell slots to ZoneMgr signals
	connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)), m_spellShell, SLOT(zoneChanged()));

	// connect the SpellShell slots to SpawnShell signals
	connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)), m_spellShell, SLOT(killSpawn(const Item*)));

	// connect the SpellShell slots to Player signals
	connect(m_player, SIGNAL(newPlayer()), m_spellShell, SLOT(clear()));
	connect(m_player, SIGNAL(buffLoad(const spellBuff *)), m_spellShell, SLOT(buffLoad(const spellBuff *)));

	// connect the SpellShell slots to EQPacket signals
	m_packet->connect2("OP_CastSpell", SP_Zone, DIR_Server|DIR_Client, "startCastStruct", SZC_Match,
					   m_spellShell, SLOT(selfStartSpellCast(const uint8_t*)));
	m_packet->connect2("OP_Buff", SP_Zone, DIR_Server|DIR_Client, "buffStruct", SZC_Match,
					   m_spellShell, SLOT(buff(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_Action", SP_Zone, DIR_Server|DIR_Client, "actionStruct", SZC_Match,
					   m_spellShell, SLOT(action(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_Action", SP_Zone, DIR_Server|DIR_Client, "actionAltStruct", SZC_Match,
					   m_spellShell, SLOT(action(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_SimpleMessage", SP_Zone, DIR_Server, "simpleMessageStruct", SZC_Match,
					   m_spellShell, SLOT(simpleMessage(const uint8_t*, size_t, uint8_t)));


	// connect Player slots to EQPacket signals
	connect(m_zoneMgr, SIGNAL(playerProfile(const charProfileStruct*)), m_player, SLOT(player(const charProfileStruct*)));
	m_packet->connect2("OP_SkillUpdate", SP_Zone, DIR_Server, "skillIncStruct", SZC_Match,
					   m_player, SLOT(increaseSkill(const uint8_t*)));
	m_packet->connect2("OP_ManaChange", SP_Zone, DIR_Server, "manaDecrementStruct", SZC_Match,
					   m_player, SLOT(manaChange(const uint8_t*)));
	m_packet->connect2("OP_ClientUpdate", SP_Zone, DIR_Server|DIR_Client, "playerSelfPosStruct", SZC_Match,
					   m_player, SLOT(playerUpdateSelf(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_ExpUpdate", SP_Zone, DIR_Server, "expUpdateStruct", SZC_Match,
					   m_player, SLOT(updateExp(const uint8_t*)));
	m_packet->connect2("OP_AAExpUpdate", SP_Zone, DIR_Server, "altExpUpdateStruct", SZC_Match,
					   m_player, SLOT(updateAltExp(const uint8_t*)));
	m_packet->connect2("OP_LevelUpdate", SP_Zone, DIR_Server, "levelUpUpdateStruct", SZC_Match,
					   m_player, SLOT(updateLevel(const uint8_t*)));
	m_packet->connect2("OP_HPUpdate", SP_Zone, DIR_Server|DIR_Client, "hpNpcUpdateStruct", SZC_Match,
					   m_player, SLOT(updateNpcHP(const uint8_t*)));
	m_packet->connect2("OP_WearChange", SP_Zone, DIR_Server|DIR_Client, "SpawnUpdateStruct", SZC_Match,
					   m_player, SLOT(updateSpawnInfo(const uint8_t*)));
	m_packet->connect2("OP_Stamina", SP_Zone, DIR_Server, "staminaStruct", SZC_Match,
					   m_player, SLOT(updateStamina(const uint8_t*)));
	m_packet->connect2("OP_Consider", SP_Zone, DIR_Server|DIR_Client, "considerStruct", SZC_Match,
					   m_player, SLOT(consMessage(const uint8_t*, size_t, uint8_t)));
	m_packet->connect2("OP_SwapSpell", SP_Zone, DIR_Server, "tradeSpellBookSlotsStruct", SZC_Match,
					   m_player, SLOT(tradeSpellBookSlots(const uint8_t*, size_t, uint8_t)));

	// interface statusbar slots
	connect (this, SIGNAL(newZoneName(const QString&)), m_stsbarZone, SLOT(setText(const QString&)));
	connect (m_packet, SIGNAL(stsMessage(const QString &, int)), this, SLOT(stsMessage(const QString &, int)));
	connect (m_spawnShell, SIGNAL(numSpawns(int)), this, SLOT(numSpawns(int)));
	connect (m_packet, SIGNAL(numPacket(int, int)), this, SLOT(numPacket(int, int)));
	connect (m_packet, SIGNAL(resetPacket(int, int)), this, SLOT(resetPacket(int, int)));
	connect (m_player, SIGNAL(newSpeed(double)), this, SLOT(newSpeed(double)));
	connect(m_player, SIGNAL(setExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)),
			this, SLOT(setExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)));
	connect(m_player, SIGNAL(newExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)),
			this, SLOT(newExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)));
	connect(m_player, SIGNAL(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)),
			this, SLOT(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)));
	connect(m_player, SIGNAL(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)),
			this, SLOT(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)));
	connect(m_player, SIGNAL(levelChanged(uint8_t)), this, SLOT(levelChanged(uint8_t)));

	if (m_expWindow != NULL)
	{
		// connect ExperienceWindow slots to Player signals
		connect(m_player, SIGNAL(newPlayer()), m_expWindow, SLOT(clear()));
		connect(m_player, SIGNAL(expGained(const QString &, int, long, QString )),
				m_expWindow, SLOT(addExpRecord(const QString &, int, long,QString )));

		// connect ExperienceWindow slots to EQInterface signals
		connect(this, SIGNAL(restoreFonts()), m_expWindow, SLOT(restoreFont()));
		connect(this, SIGNAL(saveAllPrefs()), m_expWindow, SLOT(savePrefs()));
	}

	if (m_combatWindow != NULL)
	{
		// connect CombatWindow slots to the signals
		connect(m_player, SIGNAL(newPlayer()), m_combatWindow, SLOT(clear()));
		connect(this, SIGNAL(combatSignal(int, int, int, int, int, QString, QString)),
				 m_combatWindow, SLOT(addCombatRecord(int, int, int, int, int, QString, QString)));
		connect (m_spawnShell, SIGNAL(spawnConsidered(const Item*)), m_combatWindow, SLOT(resetDPS()));
		connect(this, SIGNAL(restoreFonts()), m_combatWindow, SLOT(restoreFont()));
		connect(this, SIGNAL(saveAllPrefs()), m_combatWindow, SLOT(savePrefs()));
	}
}

EQInterface::~EQInterface()
{
	if (m_netDiag != 0)
		delete m_netDiag;

	if (m_spawnPointList != 0)
		delete m_spawnPointList;

	if (m_statList != 0)
		delete m_statList;

	if (m_guildListWindow != 0)
		delete m_guildListWindow;

	if (m_skillList != 0)
		delete m_skillList;

	if (m_spellList != 0)
		delete m_spellList;

	if (m_spawnList2 != 0)
		delete m_spawnList2;

	if (m_spawnList != 0)
		delete m_spawnList;

	for (int i = 0; i < maxNumMaps; i++)
	{
		if (m_map[i] != 0)
			delete m_map[i];
	}

	for (int i = 0; i < maxNumMessageWindows; i++)
	{
		if (m_messageWindow[i] != 0)
			delete m_messageWindow[i];
	}

	if (m_combatWindow != 0)
		delete m_combatWindow;

	if (m_expWindow != 0)
		delete m_expWindow;

	if (m_spawnLogger != 0)
		delete m_spawnLogger;

	if (m_filteredSpawnLog != 0)
		delete m_filteredSpawnLog;

	if (m_spawnMonitor != 0)
		delete m_spawnMonitor;

	if (m_groupMgr != 0)
		delete m_groupMgr;

	if (m_spellShell != 0)
		delete m_spellShell;

	if (m_spells != 0)
		delete m_spells;

	if (m_mapMgr != 0)
		delete m_mapMgr;

	if (m_spawnShell != 0)
		delete m_spawnShell;

	if (m_categoryMgr != 0)
		delete m_categoryMgr;

	if (m_filterMgr != 0)
		delete m_filterMgr;

	if (m_eqStrings != 0)
		delete m_eqStrings;

	if (m_player != 0)
		delete m_player;

	if (m_guildShell != 0)
		delete m_guildShell;

	if (m_zoneMgr != 0)
		delete m_zoneMgr;

	if (m_packet != 0)
		delete m_packet;

	if (m_dateTimeMgr != 0)
		delete m_dateTimeMgr;
}

void EQInterface::restoreStatusFont()
{
	QFont defFont;
	defFont.setPointSize(8);
	QFont statusFont = pSEQPrefs->getPrefFont("StatusFont", "Interface", defFont);

	int statusFixedHeight = statusFont.pointSize() + 6;

	// set the correct font information and sizes of the status bar widgets
	m_stsbarStatus->setFont(statusFont);
	m_stsbarStatus->setFixedHeight(statusFixedHeight);
	m_stsbarZone->setFont(statusFont);
	m_stsbarZone->setFixedHeight(statusFixedHeight);
	m_stsbarSpawns->setFont(statusFont);
	m_stsbarSpawns->setFixedHeight(statusFixedHeight);
	m_stsbarExp->setFont(statusFont);
	m_stsbarExp->setFixedHeight(statusFixedHeight);
	m_stsbarExpAA->setFont(statusFont);
	m_stsbarExpAA->setFixedHeight(statusFixedHeight);
	m_stsbarPkt->setFont(statusFont);
	m_stsbarPkt->setFixedHeight(statusFixedHeight);
	m_stsbarEQTime->setFont(statusFont);
	m_stsbarEQTime->setFixedHeight(statusFixedHeight);
	m_stsbarSpeed->setFont(statusFont);
	m_stsbarSpeed->setFixedHeight(statusFixedHeight);
	// ZEM code
	m_stsbarZEM->setFont(statusFont);
	m_stsbarZEM->setFixedHeight(statusFixedHeight);
}

void EQInterface::togglePlayerStat(QAction* action)
{
	if (action->data().isValid() && m_statList != NULL)
	{
		m_statList->statList()->enableStat(action->data().toInt(), action->isChecked());
	}
}

void EQInterface::togglePlayerSkill(QAction* action)
{
	if (m_playerSkillsLanguages == action && m_skillList != NULL)
	{
		m_skillList->skillList()->showLanguages(action->isChecked());
	}
}

void EQInterface::toggleSpawnListCol(QAction* action)
{
	int colnum = action->data().toInt();

	if (m_spawnList != NULL)
	{
		m_spawnList->spawnList()->setColumnVisible(colnum, action->isChecked());
	}
}

void EQInterface::toggleWindowDocked(QAction* action)
{
	SEQWindow* widget = NULL;
	int winnum = -1;
	QString preference;

	// get the window number parameter
	winnum = action->data().toInt();

	// get the new action state
	bool checked = action->isChecked();

	switch (winnum)
	{
		case menuSpawnList:
			m_isSpawnListDocked = checked;
			widget = m_spawnList;
			preference = "DockedSpawnList";
			break;

		case menuPlayerStats:
			m_isStatListDocked = checked;
			widget = m_statList;
			preference = "DockedPlayerStats";
			break;

		case menuPlayerSkills:
			m_isSkillListDocked = checked;
			widget = m_skillList;
			preference = "DockedPlayerSkills";
			break;

		case menuSpellList:
			m_isSpellListDocked = checked;
			widget = m_spellList;
			preference = "DockedSpellList";
			break;

		case menuCompass:
			m_isCompassDocked = checked;
			widget = m_compass;
			preference = "DockedCompass";
			break;

		case menuSpawnPointList:
			m_isSpawnPointListDocked = checked;
			widget = m_spawnPointList;
			preference = "DockedSpawnPointList";
			break;

		case menuSpawnList2:
			m_isSpawnList2Docked = !checked;
			widget = m_spawnList2;
			preference = "DockedSpawnList2";
			break;

		default:
			// use default for maps since the number of them can be changed via a
			// constant (maxNumMaps)
			if ((winnum >= mapDockBase) && (winnum < (mapDockBase + maxNumMaps)))
			{
				int i = winnum - mapDockBase;

				m_isMapDocked[i] = checked;
				widget = m_map[i];
				preference.sprintf("DockedMap%i", i+1);
			}
			break;
    };

	// save new setting
	pSEQPrefs->setPrefBool(preference, "Interface", checked);

	// attempt to undock the window
	if (widget)
	{
		if (checked)
			widget->dock();
		else
			widget->undock();

		// make the widget update it's geometry
		widget->updateGeometry();
	}
}

void EQInterface::toggleWindowDockable(QAction* action)
{
	// get the window number parameter
	int winnum = action->data().toInt();
	bool dockable = action->isChecked();

	// flip the menu item state
	SEQWindow* widget = NULL;
	QString preference;

	switch (winnum)
	{
		case menuSpawnList:
			widget = m_spawnList;
			preference = "DockableSpawnList";
			break;

		case menuPlayerStats:
			widget = m_statList;
			preference = "DockablePlayerStats";
			break;

		case menuPlayerSkills:
			widget = m_skillList;
			preference = "DockablePlayerSkills";
			break;

		case menuSpellList:
			widget = m_spellList;
			preference = "DockableSpellList";
			break;

		case menuCompass:
			widget = m_compass;
			preference = "DockableCompass";
			break;

		case menuSpawnPointList:
			widget = m_spawnPointList;
			preference = "DockableSpawnPointList";
			break;

		case menuSpawnList2:
			widget = m_spawnList2;
			preference = "DockableSpawnList2";
			break;

		case menuExperienceWindow:
			widget = m_expWindow;
			preference = "DockableExperienceWindow";
			break;

		case menuCombatWindow:
			widget = m_combatWindow;
			preference = "DockableCombatWindow";
			break;

		case menuGuildList:
			widget = m_guildListWindow;
			preference = "DockableGuildListWindow";
			break;

		case menuNetDiag:
			widget = m_netDiag;
			preference = "DockableNetDiag";
			break;

		default:
			// use default for maps since the number of them can be changed via a
			// constant (maxNumMaps)
			if ((winnum >= mapDockBase) && (winnum < (mapDockBase + maxNumMaps)))
			{
				int i = winnum - mapDockBase;
				widget = m_map[i];

				QString tmpPrefSuffix = "";
				if (i > 0)
					tmpPrefSuffix = QString::number(i + 1);
				preference = "DockableMap" + tmpPrefSuffix;
			}
			else if ((winnum >= messageWindowDockBase) &&
					 (winnum < (messageWindowDockBase + maxNumMessageWindows)))
			{
				int i = winnum - messageWindowDockBase;

				// reparent teh appropriate map
				widget = m_messageWindow[i];

				QString tmpPrefSuffix = "";
				tmpPrefSuffix = QString::number(i);

				// preference
				preference = "DockableMessageWindow" + tmpPrefSuffix;
			}
			break;
    };

	// save new setting
	pSEQPrefs->setPrefBool(preference, "Interface", dockable);

	// attempt to undock the window
	if (widget)
	{
		widget->setDockable(dockable);
		if (!dockable)
			widget->undock();
	}
}

void EQInterface::set_main_WindowCaption(int id)
{
	QWidget* widget = 0;
	int winnum;
	QString window;

	// get the window number parameter
	winnum = menuBar()->itemParameter(id);

	switch(winnum)
	{
		case 0: // Spawn List
			widget = m_spawnList;

			window = "Spawn List";
			break;
		case 1: // Player Stats
			widget = m_statList;

			window = "Player Stats";
			break;
		case 2: // Player Skills
			widget = m_skillList;

			window = "Player Skills";
			break;
		case 3: // Spell List
			widget = m_spellList;

			window = "Spell List";
			break;
		case 4: // Compass
			widget = m_compass;

			window = "Compass";
			break;
		case 5: // Interface
			widget = this;

			window = "Main Window";
			break;
		case 6: // Experience Window
			widget = m_expWindow;

			window = "Experience Window";
			break;
		case 7: // Combat Window
			widget = m_combatWindow;

			window = "Combat Window";
			break;
		case 8: // Network Diagnostics
			widget = m_netDiag;

			window = "Network Diagnostics";
			break;
		case 9: // Spawn Point List
			widget = m_spawnPointList;

			window = "Spawn Point List";
		case 10: // Spawn List
			widget = m_spawnList2;

			window = "Spawn List 2";
			break;
		default:
			// use default for maps since the number of them can be changed via a
			// constant (maxNumMaps)
			if ((winnum >= mapCaptionBase) && (winnum < (mapCaptionBase + maxNumMaps)))
			{
				int i = winnum - mapCaptionBase;

				widget = m_map[i];
			}

			break;
	};

	// attempt to undock the window
	if (widget != 0)
	{
		bool ok = false;
		QString caption = QInputDialog::getText("ShowEQ " + window + "Caption",
				"Enter caption for the " + window + ":", QLineEdit::Normal, widget->caption(), &ok, this);

		// if the user entered a caption and clicked ok, set the windows caption
		if (ok)
			widget->setCaption(caption);
	}
}

void EQInterface::set_main_WindowFont(int id)
{
	int winnum;

	// get the window number parameter
	winnum = menuBar()->itemParameter(id);

	bool ok = false;
	QFont newFont;
	SEQWindow* window = 0;
	QString title;

	//
	// NOTE: Yeah, this sucks for now, but until the architecture gets cleaned
	// up it will have to do
	switch(winnum)
	{
		case 0: // Spawn List
			title = "Spawn List";

			window = m_spawnList;
			break;
		case 1: // Player Stats
			title = "Player Stats";

			window = m_statList;
			break;
		case 2: // Player Skills
			title = "Player Skills";

			window = m_skillList;
			break;
		case 3: // Spell List
			title = "Spell List";

			window = m_spellList;
			break;
		case 4: // Compass
			title = "Compass";

			window = m_compass;
			break;
		case 5: // Interface
			// window = "Main Window";
			break;
		case 6: // Experience Window
			title = "Experience Window";

			window = m_expWindow;
			break;
		case 7: // Combat Window
			title = "Combat Window";

			window = m_combatWindow;
			break;
		case 8: // Network Diagnostics
			title = "Network Diagnostics";

			window = m_netDiag;
			break;
		case 9: // Spawn Point List
			title = "Spawn Point List";

			window = m_spawnPointList;
		case 10: // Spawn List
			title = "Spawn List 2";

			window = m_spawnList2;
			break;
		default:
			// use default for maps since the number of them can be changed via a
			// constant (maxNumMaps)
			if ((winnum >= mapCaptionBase) && (winnum < (mapCaptionBase + maxNumMaps)))
			{
				int i = winnum - mapCaptionBase;
				if (i)
					title.sprintf("Map %d", i);
				else
					title = "Map";

				window = m_map[i];
			}
			break;
	};

	if (window != 0)
	{
		// get a new font
		newFont = QFontDialog::getFont(&ok, window->font(),
									   this, "ShowEQ " + title + " Font");


		// if the user entered a font and clicked ok, set the windows font
		if (ok)
			window->setWindowFont(newFont);
	}
}

void EQInterface::set_main_Font(int id)
{
	QString name = "ShowEQ - Application Font";
	bool ok = false;

	// get a new application font
	QFont newFont;
	newFont = QFontDialog::getFont(&ok, QApplication::font(),
								   this, name);

	// if the user clicked ok and selected a valid font, set it
	if (ok)
	{
		// set the new application font
		qApp->setFont( newFont, true );

		// set the preference for future sessions
		pSEQPrefs->setPrefFont("Font", "Interface",
							   newFont);

		// make sure the windows that override the application font, do so
		emit restoreFonts();
	}
}

void EQInterface::select_main_FormatFile(int id)
{
	QString formatFile = pSEQPrefs->getPrefString("FormatFile", "Interface",
												  "eqstr_us.txt");

	QFileInfo fileInfo = m_dataLocationMgr->findExistingFile(".", formatFile);

	QString newFormatFile =
    Q3FileDialog::getOpenFileName(fileInfo.absFilePath(), "*.txt",
								  this, "FormatFile", "Select Format File");

	// if the newFormatFile name is not empty, then the user selected a file
	if (!newFormatFile.isEmpty())
	{
		// set the new format file to use
		pSEQPrefs->setPrefString("FormatFile", "Interface", newFormatFile);

		// reload the format strings
		loadFormatStrings();
	}
}

void EQInterface::select_main_SpellsFile(int id)
{
	QString spellsFile = pSEQPrefs->getPrefString("SpellsFile", "Interface",
												  "spells_us.txt");

	QFileInfo fileInfo = m_dataLocationMgr->findExistingFile(".", spellsFile);

	QString newSpellsFile =
    Q3FileDialog::getOpenFileName(fileInfo.absFilePath(), "*.txt",
								  this, "FormatFile", "Select Format File");

	// if the newFormatFile name is not empty, then the user selected a file
	if (!newSpellsFile.isEmpty())
	{
		// set the new format file to use
		pSEQPrefs->setPrefString("SpellsFile", "Interface", newSpellsFile);

		// reload the spells
		m_spells->loadSpells(newSpellsFile);
	}
}

void EQInterface::toggle_main_statusbar_Window(int id)
{
	QWidget* window = 0;
	QString preference;

	switch (menuBar()->itemParameter(id))
	{
		case 1:
			window = m_stsbarStatus;

			preference = "ShowStatus";
			break;
		case 2:
			window = m_stsbarZone;

			preference = "ShowZone";
			break;
		case 3:
			window = m_stsbarSpawns;

			preference = "ShowSpawns";
			break;
		case 4:
			window = m_stsbarExp;

			preference = "ShowExp";
			break;
		case 5:
			window = m_stsbarExpAA;

			preference = "ShowExpAA";
			break;
		case 6:
			window = m_stsbarPkt;

			preference = "ShowPacketCounter";
			break;
		case 7:
			window = m_stsbarEQTime;

			preference = "ShowEQTime";
			break;
		case 8:
			window = m_stsbarSpeed;

			preference = "ShowSpeed";
			break;
			// ZEM code
		case 9:
			window = m_stsbarZEM;

			preference = "ShowZEM";
			break;

		default:
			return;
	}

	if (window == 0)
		return;

	// should the window be visible
	bool show = !window->isVisible();

	// show or hide the window as necessary
	if (show)
		window->show();
	else
		window->hide();

	// check/uncheck the menu item
	menuBar()->setItemChecked(id, show);

	// set the preference for future sessions
	pSEQPrefs->setPrefBool(preference, "Interface_StatusBar", show);
}

void EQInterface::set_main_statusbar_Font(int id)
{
	QString name = "ShowEQ - Status Font";
	bool ok = false;

	// setup a default new status font
	QFont newFont = QApplication::font();
	newFont.setPointSize(8);

	// get new status font
	newFont = QFontDialog::getFont(&ok, pSEQPrefs->getPrefFont("StatusFont", "Interface", newFont), this, name);

	// if the user clicked ok and selected a valid font, set it
	if (ok)
	{
		// set the preference for future sessions
		pSEQPrefs->setPrefFont("StatusFont", "Interface", newFont);

		// make sure to reset the status font since the previous call may have changed it
		restoreStatusFont();
	}
}

void EQInterface::toggle_main_SavePosition(int id)
{
    pSEQPrefs->setPrefBool("SavePosition", "Interface", !pSEQPrefs->getPrefBool("SavePosition", "Interface"));
    m_windowMenu->setItemChecked(id, pSEQPrefs->getPrefBool("SavePosition", "Interface"));
}

void EQInterface::toggle_main_UseWindowPos(int id)
{
    pSEQPrefs->setPrefBool("UseWindowPos", "Interface", !pSEQPrefs->getPrefBool("UseWindowPos", "Interface"));
    m_windowMenu->setItemChecked(id, pSEQPrefs->getPrefBool("UseWindowPos", "Interface"));
}

//
// save prefs
//
void EQInterface::savePrefs()
{
	seqDebug("==> EQInterface::savePrefs()");

	if (isVisible())
	{
		seqDebug("\tisVisible()");
		QString section;
		QString interfaceSection = "Interface";
		QString tempStr;

		QString dockPrefs;
		Q3TextStream ts(&dockPrefs, QIODevice::WriteOnly);

		// save state info
		QByteArray info = saveState(m_stateVersion);
		if (!info.isEmpty())
		{
			pSEQPrefs->setPrefVariant("DockingInfo", interfaceSection, QVariant(info));
		}

		// send savePrefs signal out
		emit saveAllPrefs();

		section = "Interface";
		if (pSEQPrefs->getPrefBool("SavePosition", interfaceSection, true))
		{
			pSEQPrefs->setPrefPoint("WindowPos", section, topLevelWidget()->pos());
			pSEQPrefs->setPrefSize("WindowSize", section, topLevelWidget()->size());
		}

		// save prefs to file
		pSEQPrefs->save();
	}
}

#if 0
void EQInterface::saveDockAreaPrefs(Q3DockArea* a, Qt::DockWidgetArea edge)
{
	QList<Q3DockWindow*> l = a->dockWindowList();
	for (Q3DockWindow *dw = l.first(); dw; dw = l.next())
	{
		if (dw->inherits("SEQWindow"))
			pSEQPrefs->setPrefInt("Dock", ((SEQWindow*)dw)->preferenceName(), edge);
	}
}
#endif

void EQInterface::setCaption(const QString& text)
{
	QMainWindow::setCaption(text);
	pSEQPrefs->setPrefString("Caption", "Interface", caption());
}


void EQInterface::loadFormatStrings()
{
	// get the name of the format file
	QString formatFileName = pSEQPrefs->getPrefString("FormatFile", "Interface", "eqstr_us.txt");

	QFileInfo fileInfo = m_dataLocationMgr->findExistingFile(".", formatFileName);

	// load the strings
	m_eqStrings->load(fileInfo.absFilePath());
}

void EQInterface::select_filter_file()
{
	QString filterFile = Q3FileDialog::getOpenFileName(m_filterMgr->filterFile(),
													   QString("ShowEQ Filter Files (*.xml)"),
													   0, "Select Filter Config...");
	if (!filterFile.isEmpty())
		m_filterMgr->loadFilters(filterFile);
}

void EQInterface::toggle_filter_Case(int id)
{
	m_filterMgr->setCaseSensitive(!m_filterMgr->caseSensitive());
	menuBar()->setItemChecked(id, m_filterMgr->caseSensitive());
	pSEQPrefs->setPrefBool("IsCaseSensitive", "Filters", m_filterMgr->caseSensitive());
}

void EQInterface::toggle_filter_AlertInfo(int id)
{
	pSEQPrefs->setPrefBool("AlertInfo", "Filters", !pSEQPrefs->getPrefBool("AlertInfo", "Filters"));
	menuBar()->setItemChecked(id, pSEQPrefs->getPrefBool("AlertInfo", "Filters"));
}

void EQInterface::toggle_filter_UseSystemBeep(int id)
{
	m_filterNotifications->setUseSystemBeep(!m_filterNotifications->useSystemBeep());
	menuBar()->setItemChecked(id, m_filterNotifications->useSystemBeep());
}

void EQInterface::toggle_filter_UseCommands(int id)
{
	m_filterNotifications->setUseCommands(!m_filterNotifications->useCommands());
	menuBar()->setItemChecked(id, m_filterNotifications->useCommands());
}

void EQInterface::toggle_filter_Log(int id)
{
	if (!m_filteredSpawnLog)
		createFilteredSpawnLog();

	uint32_t filters = m_filteredSpawnLog->filters();
	uint32_t filter = menuBar()->itemParameter(id);

	if (filters & filter)
		filters &= ~filter;
	else
		filters |= filter;

	m_filteredSpawnLog->setFilters(filters);

	menuBar()->setItemChecked(id, ((filters & filter) != 0));
	pSEQPrefs->setPrefBool("Log", "Filters", filters);
}

void EQInterface::set_filter_AudioCommand(int id)
{
	QString property;
	QString prettyName;
	switch(menuBar()->itemParameter(id))
	{
		case 1:
			property = "SpawnAudioCommand";
			prettyName = "Spawn";
			break;
		case 2:
			property = "DeSpawnAudioCommand";
			prettyName = "DeSpawn";
			break;
		case 3:
			property = "DeathAudioCommand";
			prettyName = "Death";
			break;
		case 4:
			property = "LocateSpawnAudioCommand";
			prettyName = "Locate Spawn";
			break;
		case 5:
			property = "CautionSpawnAudioCommand";
			prettyName = "Caution Spawn";
			break;
		case 6:
			property = "HuntSpawnAudioCommand";
			prettyName = "Hunt Spawn";
			break;
		case 7:
			property = "DangerSpawnAudioCommand";
			prettyName = "Danger Spawn";
			break;
		default:
			return;
	}

	QString prefstring;
	prefstring.sprintf("/usr/bin/esdplay %s/spawn.wav &", PKGDATADIR);
	QString value = pSEQPrefs->getPrefString(property, "Filters", prefstring);

	bool ok = false;
	QString command =
    QInputDialog::getText("ShowEQ " + prettyName + "Command",
						  "Enter command line to use for " + prettyName + "'s:",
						  QLineEdit::Normal, value, &ok, this);

	if (ok)
		pSEQPrefs->setPrefString(property, "Filters", command);
}

void EQInterface::listSpawns()
{
#ifdef DEBUG
	debug ("listSpawns()");
#endif /* DEBUG */

	QString outText;

	// open the output data stream
	Q3TextStream out(&outText, QIODevice::WriteOnly);

	// dump the spawns
	m_spawnShell->dumpSpawns(tSpawn, out);

	seqInfo((const char*)outText);
}

void EQInterface::listDrops()
{
#ifdef DEBUG
	debug ("listDrops()");
#endif /* DEBUG */
	QString outText;

	// open the output data stream
	Q3TextStream out(&outText, QIODevice::WriteOnly);

	// dump the drops
	m_spawnShell->dumpSpawns(tDrop, out);

	seqInfo((const char*)outText);
}

void EQInterface::listMapInfo()
{
#ifdef DEBUG
	debug ("listMapInfo()");
#endif /* DEBUG */
	QString outText;

	// open the output data stream
	Q3TextStream out(&outText, QIODevice::WriteOnly);

	// dump map managers info
	m_mapMgr->dumpInfo(out);

	// iterate over all the maps
	for (int i = 0; i < maxNumMaps; i++)
	{
		// if this map has been instantiated, dump it's info
		if (m_map[i] != 0)
			m_map[i]->dumpInfo(out);
	}

	seqInfo((const char*)outText);
}

void EQInterface::listInterfaceInfo()
{
#ifdef DEBUG
	debug ("listMapInfo()");
#endif

	QString outText;

	// open the output data stream
	Q3TextStream out(&outText, QIODevice::WriteOnly);

	out << "Map window layout info:" << endl;
	out << "-----------------------" << endl;
	out << (QString)this->saveState();
	out << "-----------------------" << endl;


	seqInfo((const char*)outText);
}

void EQInterface::listGroup()
{
#ifdef DEBUG
	debug ("listGroup()");
#endif /* DEBUG */
	QString outText;

	// open the output data stream
	Q3TextStream out(&outText, QIODevice::WriteOnly);

	// dump the drops
	m_groupMgr->dumpInfo(out);

	seqInfo((const char*)outText);
}


void EQInterface::listGuild()
{
#ifdef DEBUG
	debug ("listGuild()");
#endif /* DEBUG */
	QString outText;

	// open the output data stream
	Q3TextStream out(&outText, QIODevice::WriteOnly);

	// dump the drops
	m_guildShell->dumpMembers(out);

	seqInfo((const char*)outText);
}

void EQInterface::dumpSpawns()
{
#ifdef DEBUG
	debug ("dumpSpawns()");
#endif /* DEBUG */

	QString logFile = pSEQPrefs->getPrefString("DumpSpawnsFilename", "Interface", "dumpspawns.txt");

	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

	// open the output data stream
	QFile file(logFileInfo.absFilePath());
	file.open(QIODevice::WriteOnly);
	Q3TextStream out(&file);

	// dump the spawns
	m_spawnShell->dumpSpawns(tSpawn, out);
}

void EQInterface::dumpDrops()
{
#ifdef DEBUG
	debug ("dumpDrops()");
#endif /* DEBUG */

	QString logFile = pSEQPrefs->getPrefString("DumpDropsFilename", "Interface", "dumpdrops.txt");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

	// open the output data stream
	QFile file(logFileInfo.absFilePath());
	file.open(QIODevice::WriteOnly);
	Q3TextStream out(&file);

	// dump the drops
	m_spawnShell->dumpSpawns(tDrop, out);
}

void EQInterface::dumpMapInfo()
{
#ifdef DEBUG
	debug ("dumpMapInfo()");
#endif /* DEBUG */

	QString logFile = pSEQPrefs->getPrefString("DumpMapInfoFilename", "Interface", "mapinfo.txt");

	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

	// open the output data stream
	QFile file(logFileInfo.absFilePath());
	file.open(QIODevice::WriteOnly);
	Q3TextStream out(&file);

	// dump map managers info
	m_mapMgr->dumpInfo(out);

	// iterate over all the maps
	for (int i = 0; i < maxNumMaps; i++)
	{
		// if this map has been instantiated, dump it's info
		if (m_map[i] != 0)
			m_map[i]->dumpInfo(out);
	}
}

void EQInterface::dumpGuildInfo()
{
	QString logFile = pSEQPrefs->getPrefString("GuildsDumpFile", "Interface", "guilds.txt");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

	emit guildList2text(logFileInfo.absFilePath());
}

void EQInterface::dumpSpellBook()
{
#ifdef DEBUG
	debug ("dumpSpellBook");
#endif /* DEBUG */

	QString logFile = pSEQPrefs->getPrefString("DumpSpellBookFilename", "Interface", "spellbook.txt");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

	// open the output data stream
	QFile file(logFileInfo.absFilePath());
	file.open(QIODevice::WriteOnly);
	Q3TextStream out(&file);
	QString txt;

	seqInfo("Dumping Spell Book to '%s'\n", (const char*)file.name().utf8());
	out << "Spellbook of " << m_player->name() << " a level "
		<< m_player->level() << " " << m_player->raceString()
		<< " " << m_player->classString()
		<< endl;

	uint8_t playerClass = m_player->classVal();

	uint32_t spellid;
	for (uint32_t i = 0; i < MAX_SPELLBOOK_SLOTS; i++)
	{
		spellid = m_player->getSpellBookSlot(i);
		if (spellid == 0xffffffff)
			continue;

		const Spell* spell = m_spells->spell(spellid);

		QString spellName;

		if (spell)
		{
			txt.sprintf("%.3d %.2d %.2d %#4.04x %02d\t%s",
						i, ((i / 8) + 1), ((i % 8) + 1),
						spellid, spell->level(playerClass),
						spell->name().latin1());
		}
		else
		{
			txt.sprintf("%.3d %.2d %.2d %#4.04x   \t%s",
						i, ((i / 8) + 1), ((i % 8) + 1),
						spellid,
						spell_name(spellid).latin1());
		}

		out << txt << endl;
	}
}

void EQInterface::dumpGroup()
{
#ifdef DEBUG
	debug ("dumpGroup()");
#endif /* DEBUG */

	QString logFile = pSEQPrefs->getPrefString("DumpGroupFilename", "Interface", "dumpgroup.txt");

	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

	// open the output data stream
	QFile file(logFileInfo.absFilePath());
	file.open(QIODevice::WriteOnly);
	Q3TextStream out(&file);

	// dump the drops
	m_groupMgr->dumpInfo(out);
}

void EQInterface::dumpGuild()
{
#ifdef DEBUG
	debug ("dumpGuild()");
#endif /* DEBUG */

	QString logFile = pSEQPrefs->getPrefString("DumpGuildFilename", "Interface", "dumpguild.txt");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("dumps", logFile);

	// open the output data stream
	QFile file(logFileInfo.absFilePath());
	file.open(QIODevice::WriteOnly);
	Q3TextStream out(&file);

	// dump the drops
	m_guildShell->dumpMembers(out);
}

void EQInterface::launchFilterEditor()
{
	EditorWindow * ew = new EditorWindow(m_filterMgr->filterFile());
	ew->setCaption(m_filterMgr->filterFile());
	ew->show();
}

void EQInterface::launchZoneFilterEditor()
{
	EditorWindow * ew = new EditorWindow(m_filterMgr->zoneFilterFile());
	ew->setCaption(m_filterMgr->zoneFilterFile());
	ew->show();
}

void EQInterface::toggleSelectOnConsider(bool state)
{
	m_selectOnConsider = state;
	pSEQPrefs->setPrefBool("SelectOnCon", "Interface", m_selectOnConsider);
}

void EQInterface::toggleSelectOnTarget(bool state)
{
	m_selectOnTarget = state;
	pSEQPrefs->setPrefBool("SelectOnTarget", "Interface", m_selectOnTarget);
}

void EQInterface::toggleFastMachine(bool state)
{
	showeq_params->fast_machine = state;
	pSEQPrefs->setPrefBool("FastMachine", "Misc", showeq_params->fast_machine);
}

void EQInterface::toggleKeepSelectedVisible(bool state)
{
	showeq_params->keep_selected_visible = state;
	pSEQPrefs->setPrefBool("KeepSelected", "Interface", showeq_params->keep_selected_visible);
}

void EQInterface::toggleUseUpdateRadius(bool state)
{
	showeq_params->useUpdateRadius = state;
	pSEQPrefs->setPrefBool("UseUpdateRadius", "Interface", showeq_params->useUpdateRadius);
}

/* Check and uncheck Log menu options & set EQPacket logging flags */
void EQInterface::toggleLogAllPackets()
{
	if (m_globalLog)
	{
		delete m_globalLog;
		m_globalLog = NULL;
	}
	else
	{
		createGlobalLog();
	}

	bool state = (m_globalLog != NULL);

	m_netLogAllPackets->setChecked(state);
	pSEQPrefs->setPrefBool("LogAllPackets", "PacketLogging", state);
}

void EQInterface::toggleLogWorldData()
{
	if (m_worldLog)
	{
		delete m_worldLog;
		m_worldLog = NULL;
	}
	else
	{
		createWorldLog();
	}

	bool state = (m_worldLog != NULL);
	m_netLogWorldData->setChecked(state);
	pSEQPrefs->setPrefBool("LogWorldPackets", "PacketLogging", state);
}

void EQInterface::toggleLogZoneData()
{
	if (m_zoneLog)
	{
		delete m_zoneLog;
		m_zoneLog = NULL;
	}
	else
	{
		createZoneLog();
	}

	bool state = (m_zoneLog != NULL);
	m_netLogZoneData->setChecked(state);
	pSEQPrefs->setPrefBool("LogZonePackets", "PacketLogging", state);
}

void EQInterface::toggle_log_Filter_ZoneData_Client()
{
	bool state = true;

	if (showeq_params->filterZoneDataLog == DIR_Client)
	{
		showeq_params->filterZoneDataLog = 0;
		state = false;
	}
	else
	{
		showeq_params->filterZoneDataLog = DIR_Client;
	}

	m_netLogFilterZoneClient->setChecked(state);
	m_netLogFilterZoneServer->setChecked(false);
}

void EQInterface::toggle_log_Filter_ZoneData_Server()
{
	bool state = true;

	if (showeq_params->filterZoneDataLog == DIR_Server)
	{
		showeq_params->filterZoneDataLog = 0;
		state = false;
	}
	else
	{
		showeq_params->filterZoneDataLog = DIR_Server;
	}

	m_netLogFilterZoneServer->setChecked(state);
	m_netLogFilterZoneClient->setChecked(false);
}

void EQInterface::toggleLogBazaarData(bool state)
{
	if (!state && m_bazaarLog)
	{
		disconnect(m_bazaarLog, 0, 0, 0);
		delete m_bazaarLog;
		m_bazaarLog = 0;
	}
	else if (state && !m_bazaarLog)
	{
		createBazaarLog();
	}

	pSEQPrefs->setPrefBool("LogBazaarPackets", "PacketLogging", state);
}

void EQInterface::toggle_log_UnknownData()
{
	if (m_unknownZoneLog)
	{
		delete m_unknownZoneLog;
		m_unknownZoneLog = NULL;
	}
	else
	{
		createUnknownZoneLog();
	}

	bool state = (m_unknownZoneLog != 0);
	m_netLogUnknownData->setChecked(state);
	pSEQPrefs->setPrefBool("LogUnknownZonePackets", "PacketLogging", state);
}

void EQInterface::toggle_log_RawData()
{
	bool state = !pSEQPrefs->getPrefBool("LogRawPackets", "PacketLogging", false);

	if (m_worldLog)
		m_worldLog->setRaw(state);

	if (m_zoneLog)
		m_zoneLog->setRaw(state);

	m_netLogRawData->setChecked(state);
	pSEQPrefs->setPrefBool("LogRawPackets", "PacketLogging", state);
}

/* Check and uncheck View menu options */
void EQInterface::toggleChannelMsgs(QAction* action)
{
	int id = action->data().toInt();
	int winNum = menuBar()->itemParameter(id);

	bool wasVisible = ((m_messageWindow[winNum] != 0) && (m_messageWindow[winNum]->isVisible()));

	if (!wasVisible)
		showMessageWindow(winNum);
	else
	{
		// save any preference changes
		m_messageWindow[winNum]->savePrefs();

		// hide it
		m_messageWindow[winNum]->hide();

		// remove its window menu
		removeWindowMenu(m_messageWindow[winNum]);

		// then delete it
		delete m_messageWindow[winNum];

		// make sure to clear it's variable
		m_messageWindow[winNum] = 0;
	}

	QString tmpPrefSuffix = "";
	if (winNum > 0)
		tmpPrefSuffix = QString::number(winNum + 1);

	QString tmpPrefName = QString("ShowMessageWindow") + tmpPrefSuffix;

	pSEQPrefs->setPrefBool(tmpPrefName, "Interface", !wasVisible);
}

void EQInterface::toggle_view_UnknownData ()
{
	bool state = !pSEQPrefs->getPrefBool("ViewUnknown", "PacketLogging", false);

	if (m_unknownZoneLog)
		m_unknownZoneLog->setView(state);

	m_netViewUnknownData->setChecked(state);
	pSEQPrefs->setPrefBool("ViewUnknown", "PacketLogging", state);
}

void EQInterface::toggleExpWindow()
{
    if (!m_expWindow->isVisible())
		m_expWindow->show();
    else
		m_expWindow->hide();

    pSEQPrefs->setPrefBool("ShowExpWindow", "Interface", m_expWindow->isVisible());
}

void EQInterface::toggleCombatWindow()
{
	if (!m_combatWindow->isVisible())
		m_combatWindow->show();
	else
		m_combatWindow->hide();

	pSEQPrefs->setPrefBool("ShowCombatWindow", "Interface", m_combatWindow->isVisible());
}

void EQInterface::toggleSpawnList()
{
	bool wasVisible = ((m_spawnList != 0) && m_spawnList->isVisible());

	if (!wasVisible)
	{
		showSpawnList();

		// enable it's options sub-menu
		m_spawnListMenuAction->setEnabled(true);
	}
	else
	{
		// save it's preferences
		m_spawnList->savePrefs();

		// hide it
		m_spawnList->hide();

		// disable it's options sub-menu
		m_spawnListMenuAction->setEnabled(true);

		// remove its window menu
		removeWindowMenu(m_spawnList);

		// delete the window
		delete m_spawnList;

		// make sure to clear it's variable
		m_spawnList = 0;
	}

	pSEQPrefs->setPrefBool("ShowSpawnList", "Interface", !wasVisible);
}

void EQInterface::toggleSpawnList2()
{
	bool wasVisible = ((m_spawnList2 != 0) && m_spawnList2->isVisible());

	if (!wasVisible)
		showSpawnList2();
	else
	{
		// save it's preferences
		m_spawnList2->savePrefs();

		// hide it
		m_spawnList2->hide();

		// remove its window menu
		removeWindowMenu(m_spawnList2);

		// delete the window
		delete m_spawnList2;

		// make sure to clear it's variable
		m_spawnList2 = 0;
	}

	pSEQPrefs->setPrefBool("ShowSpawnList2", "Interface", !wasVisible);
}

void EQInterface::toggleSpawnPointList()
{
	bool wasVisible = ((m_spawnPointList != 0) && m_spawnPointList->isVisible());

	if (!wasVisible)
		showSpawnPointList();
	else
	{
		// save it's preferences
		m_spawnPointList->savePrefs();

		// hide it
		m_spawnPointList->hide();

		// remove its window menu
		removeWindowMenu(m_spawnPointList);

		// delete the window
		delete m_spawnPointList;

		// make sure to clear it's variable
		m_spawnPointList = 0;
	}

	pSEQPrefs->setPrefBool("ShowSpawnPointList", "Interface", !wasVisible);
}

void EQInterface::toggleSpellList()
{
	bool wasVisible = ((m_spellList != 0) && (m_spellList->isVisible()));

	if (!wasVisible)
		showSpellList();
	else
	{
		// save it's preferences
		m_spellList->savePrefs();

		// hide it
		m_spellList->hide();

		// remove its window menu
		removeWindowMenu(m_spellList);

		// delete it
		delete m_spellList;

		// make sure to clear it's variable
		m_spellList = 0;
	}

	pSEQPrefs->setPrefBool("ShowSpellList", "Interface", !wasVisible);
}

void EQInterface::togglePlayerStats()
{
	bool wasVisible = ((m_statList != 0) && m_statList->isVisible());

	if (!wasVisible)
	{
		showStatList();

		// enable it's options sub-menu
		m_playerStatsMenuAction->setEnabled(true);
	}
	else
	{
		// save it's preferences
		m_statList->savePrefs();

		// hide it
		m_statList->hide();

		// disable it's options sub-menu
		m_playerStatsMenuAction->setEnabled(false);

		// remove its window menu
		removeWindowMenu(m_statList);

		// then delete it
		delete m_statList;

		// make sure to clear it's variable
		m_statList = NULL;
	}

	pSEQPrefs->setPrefBool("ShowPlayerStats", "Interface", !wasVisible);
}

void EQInterface::togglePlayerSkills()
{
	bool wasVisible = ((m_skillList != 0) && m_skillList->isVisible());

	if (!wasVisible)
	{
		showSkillList();
		m_playerSkillsMenuAction->setEnabled(true);
	}
	else
	{
		// save any preference changes
		m_skillList->savePrefs();

		// if it's not visible, hide it
		m_skillList->hide();

		// disable it's options sub-menu
		m_playerSkillsMenuAction->setEnabled(false);

		// remove its window menu
		removeWindowMenu(m_skillList);

		// then delete it
		delete m_skillList;

		// make sure to clear it's variable
		m_skillList = 0;
	}

	pSEQPrefs->setPrefBool("ShowPlayerSkills", "Interface", !wasVisible);
}

void EQInterface::toggleCompass()
{
	bool wasVisible = ((m_compass != 0) && (m_compass->isVisible()));

	if (!wasVisible)
		showCompass();
	else
	{
		// if it's not visible, hide it
		m_compass->hide();

		// remove its window menu
		removeWindowMenu(m_compass);

		// then delete it
		delete m_compass;

		// make sure to clear it's variable
		m_compass = 0;
	}

	pSEQPrefs->setPrefBool("ShowCompass", "Interface", !wasVisible);
}

void EQInterface::toggleMap(QAction* action)
{
	int id = action->data().toInt();
	int mapNum = menuBar()->itemParameter(id);

	bool wasVisible = ((m_map[mapNum] != 0) &&
					   (m_map[mapNum]->isVisible()));

	if (!wasVisible)
		showMap(mapNum);
	else
	{
		// save any preference changes
		m_map[mapNum]->savePrefs();

		// hide it
		m_map[mapNum]->hide();

		// remove its window menu
		removeWindowMenu(m_map[mapNum]);

		// then delete it
		delete m_map[mapNum];

		// make sure to clear it's variable
		m_map[mapNum] = 0;
	}

	QString tmpPrefSuffix = "";
	if (mapNum > 0)
		tmpPrefSuffix = QString::number(mapNum + 1);

	QString tmpPrefName = QString("ShowMap") + tmpPrefSuffix;

	pSEQPrefs->setPrefBool(tmpPrefName, "Interface", !wasVisible);
}

void EQInterface::toggleNetDiag()
{
	bool wasVisible = ((m_netDiag != 0) && (m_netDiag->isVisible()));

	if (!wasVisible)
		showNetDiag();
	else
	{
		// if it's not visible, hide it
		m_netDiag->hide();

		// remove its window menu
		removeWindowMenu(m_netDiag);

		// then delete it
		delete m_netDiag;

		// make sure to clear it's variable
		m_netDiag = 0;
	}

	pSEQPrefs->setPrefBool("ShowNetStats", "Interface", !wasVisible);
}

void EQInterface::toggleGuildList()
{
	bool wasVisible = ((m_guildListWindow != 0) && (m_guildListWindow->isVisible()));

	if (!wasVisible)
		showGuildList();
	else
	{
		// if it's not visible, hide it
		m_guildListWindow->hide();

		// remove its window menu
		removeWindowMenu(m_guildListWindow);

		// then delete it
		delete m_guildListWindow;

		// make sure to clear it's variable
		m_guildListWindow = 0;
	}

	pSEQPrefs->setPrefBool("ShowGuildList", "Interface", !wasVisible);
}

bool EQInterface::getMonitorOpCodeList(const QString& title, QString& opCodeList)
{
	bool ok = false;
	QString newMonitorOpCode_List =
    QInputDialog::getText(title,
						  "A list of OpCodes seperated by commas...\n"
						  "\n"
						  "Each Opcode has 4 arguments, only one of which is actually necessary...\n"
						  "They are:\n"
						  "OpCode:    16-bit HEX value of the OpCode\n"
						  "            (REQUIRED - No Default)\n"
						  "\n"
						  "Alias:     Name used when displaying the Opcode\n"
						  "            (DEFAULT: Monitored OpCode)\n"
						  "\n"
						  "Direction: 1 = Client ---> Server\n"
						  "           2 = Client <--- Server\n"
						  "           3 = Client <--> Server (BOTH)\n"
						  "            (DEFAULT: 3)\n"
						  "\n"
						  "Show known 1 = Show if OpCode is marked as known.\n"
						  "           0 = Ignore if OpCode is known.\n"
						  "            (DEFAULT: 0)\n"
						  "\n"
						  "The way which you include the arguments in the list of OpCodes is:\n"
						  "adding a ':' inbetween arguments and a ',' after the last OpCode\n"
						  "argument.\n"
						  "\n"
						  "(i.e. 7F21:Mana Changed:3:1, 7E21:Unknown Spell Event(OUT):1,\n"
						  "      7E21:Unknown Spell Event(IN):2 )\n",
						  QLineEdit::Normal,
						  opCodeList,
						  &ok, this);

	if (ok)
		opCodeList = newMonitorOpCode_List;

	return ok;
}

void EQInterface::toggleOpcodeMonitoring(bool state)
{
	if (state && !m_opcodeMonitorLog)
	{
		QString opCodeList = pSEQPrefs->getPrefString("OpCodeList", "OpCodeMonitoring", "");

		if (getMonitorOpCodeList("ShowEQ - Enable OpCode Monitor", opCodeList) && !opCodeList.isEmpty())
		{
			createOPCodeMonitorLog(opCodeList);

			// set the list of monitored opcodes
			pSEQPrefs->setPrefString("OpCodeList", "OpCodeMonitoring", opCodeList);
			seqInfo("OpCode monitoring is now ENABLED...\nUsing list:\t%s", (const char*)opCodeList);
		}
	}
	else if (!state && m_opcodeMonitorLog)
	{
		delete m_opcodeMonitorLog;
		m_opcodeMonitorLog = 0;

		seqInfo("OpCode monitoring has been DISABLED...");
	}

	pSEQPrefs->setPrefBool("Enable", "OpCodeMonitoring", state);
}

void EQInterface::set_opcode_monitored_list()
{
	QString opCodeList = pSEQPrefs->getPrefString("OpCodeList", "OpCodeMonitoring", "");

	if (getMonitorOpCodeList("ShowEQ - Reload OpCode Monitor", opCodeList) && m_opcodeMonitorLog)
	{
		m_opcodeMonitorLog->init(opCodeList);

		seqInfo("The monitored OpCode list has been reloaded...\nUsing list:\t%s", (const char*)opCodeList);

		// set the list of monitored opcodes
		pSEQPrefs->setPrefString("OpCodeList", "OpCodeMonitoring", opCodeList);
	}
}


void EQInterface::toggle_opcode_log(bool state)
{
	if (m_opcodeMonitorLog)
	{
		m_opcodeMonitorLog->setLog(state);
		state = m_opcodeMonitorLog->log();
	}

	m_netOpcodeLog->setChecked(state);
	pSEQPrefs->setPrefBool("Log", "OpCodeMonitoring", state);
}

void EQInterface::toggle_opcode_view(bool state)
{
	if (m_opcodeMonitorLog)
	{
		m_opcodeMonitorLog->setView(state);
	}

	m_netOpcodeView->setChecked(state);
	pSEQPrefs->setPrefBool("View", "OpCodeMonitoring", state);
}


void EQInterface::select_opcode_file()
{
	QString logFile = pSEQPrefs->getPrefString("LogFilename", "OpCodeMonitoring", "opcodemonitor.log");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);

	logFile = Q3FileDialog::getSaveFileName(logFileInfo.absFilePath(), "*.log", this, "ShowEQ - OpCode Log File");

	// set log filename
	if (!logFile.isEmpty())
	{
		pSEQPrefs->setPrefString("LogFilename", "OpCodeMonitoring", logFile);
	}
}

void EQInterface::resetMaxMana()
{
	if (m_statList != 0)
	{
		m_statList->statList()->resetMaxMana();
	}
}

void EQInterface::toggleLogSpawns(bool state)
{
    if (state && m_spawnLogger == NULL)
    {
		createSpawnLog();
    }
    else if (!state && m_spawnLogger != NULL)
    {
		// delete the spawn logger
		delete m_spawnLogger;

		// make sure to clear it's variable
		m_spawnLogger = NULL;
    }

    pSEQPrefs->setPrefBool("LogSpawns", "Misc", state);
}

void EQInterface::togglePvPTeams(bool state)
{
    showeq_params->pvp = state;
    pSEQPrefs->setPrefBool("PvPTeamColoring", "Interface", state);
}

void EQInterface::togglePvPDeity(bool state)
{
    showeq_params->deitypvp = state;
    pSEQPrefs->setPrefBool("DeityPvPTeamColoring", "Interface", state);
}

void EQInterface::toggleCreateUnknownSpawns(bool state)
{
    showeq_params->createUnknownSpawns = state;
    pSEQPrefs->setPrefBool("CreateUnknownSpawns", "Misc", state);
}

void EQInterface::toggleRecordWalkPaths(bool state)
{
    showeq_params->walkpathrecord = state;
    pSEQPrefs->setPrefBool("WalkPathRecording", "Misc", state);
}

void EQInterface::setWalkPathLength(int len)
{
	if ((len > 0) && (len <= 8192))
		showeq_params->walkpathlength = len;

    pSEQPrefs->setPrefInt("WalkPathLength", "Misc", showeq_params->walkpathlength);
}

void EQInterface::toggleRetardedCoords(bool state)
{
    showeq_params->retarded_coords = state;
    pSEQPrefs->setPrefBool("RetardedCoords", "Interface", state);
}

void EQInterface::toggleSystemSpawnTime(bool state)
{
    showeq_params->systime_spawntime = state;
    pSEQPrefs->setPrefBool("SystimeSpawntime", "Interface", state);
}

void EQInterface::selectConColorBase(QAction* action)
{
	ColorLevel level = (ColorLevel)action->data().toInt();

	// get the current color
	QColor color = m_player->conColorBase(level);

	// get the new color
	QColor newColor = QColorDialog::getColor(color, this, "ShowEQ - Con Color");

	// only set if the user selected a valid color and clicked ok
	if (newColor.isValid())
	{
		// set the new con color
		m_player->setConColorBase(level, newColor);

		// force the spawn lists to get rebuilt with the new colors
		rebuildSpawnList();
	}
}

void EQInterface::setExp(uint32_t totalExp, uint32_t totalTick,
						 uint32_t minExpLevel, uint32_t maxExpLevel,
						 uint32_t tickExpLevel)
{
	if (m_stsbarExp)
	{
		char expperc[5];
		sprintf(expperc, "%.2f", totalTick*100.0/330.0);

		m_stsbarExp->setText(QString("Exp: %1 (%2/330, %3%)")
							 .arg(Commanate(totalExp)).arg(totalTick).arg(expperc));
	}
}

void EQInterface::newExp(uint32_t newExp, uint32_t totalExp,
						 uint32_t totalTick,
						 uint32_t minExpLevel, uint32_t maxExpLevel,
						 uint32_t tickExpLevel)
{
	uint32_t leftExp = maxExpLevel - totalExp;

	if (newExp)
	{
		uint32_t needKills = leftExp / newExp;
		// format a string for the status bar
		if (m_stsbarStatus)
			m_stsbarStatus->setText(QString("Exp: %1; %2 (%3/330); %4 left [~ %5 kills]")
									.arg(Commanate(newExp))
									.arg(Commanate(totalExp - minExpLevel))
									.arg(totalTick)
									.arg(Commanate(leftExp))
									.arg(needKills));

		if (m_stsbarExp)
		{
			char expperc[5];
			sprintf(expperc, "%.2f", totalTick*100.0/330.0);

			m_stsbarExp->setText(QString("Exp: %1 (%2/330, %3%)")
								 .arg(Commanate(totalExp)).arg(totalTick).arg(expperc));
		}
	}
	else
	{
		if (m_stsbarStatus)
			m_stsbarStatus->setText(QString("Exp: <%1; %2 (%3/330); %4 left")
									.arg(Commanate(tickExpLevel))
									.arg(Commanate(totalExp - minExpLevel))
									.arg(totalTick).arg(Commanate(leftExp)));

		if (m_stsbarExp)
		{
			char expperc[5];
			sprintf(expperc, "%.2f", totalTick*100.0/330.0);

			m_stsbarExp->setText(QString("Exp: %1 (%2/330, %3%)")
								 .arg(Commanate(totalExp)).arg(totalTick).arg(expperc));
		}
	}
}

void EQInterface::setAltExp(uint32_t totalExp, uint32_t maxExp, uint32_t tickExp, uint32_t aapoints)
{
	if (m_stsbarExpAA)
		m_stsbarExpAA->setText(QString("ExpAA: %1").arg(totalExp));
}

void EQInterface::newAltExp(uint32_t newExp, uint32_t totalExp, uint32_t totalTick, uint32_t maxExp, uint32_t tickExp, uint32_t aapoints)
{
	if (m_stsbarExpAA)
	{
		char aaperc[5];
		sprintf(aaperc, "%.2f", totalTick*100.0/330.0);

		m_stsbarExpAA->setText(QString("ExpAA: %1 (%2/330, %3%)").arg(Commanate(totalExp)).arg(totalTick).arg(aaperc));
	}
}

void EQInterface::levelChanged(uint8_t level)
{
	QString tempStr;
	tempStr.sprintf("New Level: %u", level);
	if (m_stsbarStatus)
		m_stsbarStatus->setText(tempStr);
}

//
// TODO:  clear after timeout miliseconds
//
void EQInterface::stsMessage(const QString &string, int timeout)
{
	if (m_stsbarStatus)
		m_stsbarStatus->setText(string);
}

void EQInterface::numSpawns(int num)
{
	// only update once per sec
	static int lastupdate = 0;
	if ( (mTime() - lastupdate) < 1000)
		return;
	lastupdate = mTime();

	QString tempStr;
	tempStr.sprintf("Mobs: %d", num);
	m_stsbarSpawns->setText(tempStr);
}

void EQInterface::newSpeed(double speed)
{
	// update twice per sec
	static int lastupdate = 0;
	if ( (mTime() - lastupdate) < 500)
		return;
	lastupdate = mTime();

	QString tempStr;
	tempStr.sprintf("Run Speed: %3.1f", speed);
	m_stsbarSpeed->setText(tempStr);
}

void EQInterface::resetPacket(int num, int stream)
{
	//if (stream != (int)zone2client);
	// if passed 0 reset the average
	m_packetStartTime = mTime();
	m_initialcount = num;
}

void EQInterface::numPacket(int num, int stream)
{

	if(stream != (int)zone2client)
		return;
	// start the timer of not started
	if (!m_packetStartTime)
		m_packetStartTime = mTime();

	// only update once per sec
	static int lastupdate = 0;
	if ( (mTime() - lastupdate) < 1000)
		return;
	lastupdate = mTime();


	QString tempStr;
	int delta = mTime() - m_packetStartTime;
	num -= m_initialcount;
	if (num && delta)
		tempStr.sprintf("Pkt: %d (%2.1f)", num, (float) (num<<10) / (float) delta);
	else
		tempStr.sprintf("Pkt: %d", num);

	m_stsbarPkt->setText(tempStr);
}

// TODO: Move attack2Hand1 out of EQInterface
void EQInterface::attack2Hand1(const uint8_t* data)
{
	// const attack2Struct * atk2 = (const attack2Struct*)data;
}

// TODO: Move action2Message out of EQInterface
void EQInterface::action2Message(const uint8_t* data)
{
	action2Struct *action2 = (action2Struct*)data;
	const Item* target = m_spawnShell->findID(tSpawn, action2->target);
	const Item* source = m_spawnShell->findID(tSpawn, action2->source);
	emit combatSignal(action2->target, action2->source, action2->type, action2->spell, action2->damage,
					  (target != 0) ? target->name() : QString("Unknown"), (source != 0) ? source->name() : QString("Unknown"));
}

// belith - combatKillSpawn, fix for the combat window
//          this displays a killing shot on a mob in combat records
// TODO: Move combatKillSpawn out of EQInterface
void EQInterface::combatKillSpawn(const uint8_t* data)
{
	const newCorpseStruct *deadspawn = (const newCorpseStruct *)data;
	// only show my kills
	if (deadspawn && deadspawn->killerId == m_player->id())
	{
		const Item* target = m_spawnShell->findID(tSpawn, deadspawn->spawnId);
		const Item* source = m_spawnShell->findID(tSpawn, deadspawn->killerId);

		emit combatSignal(deadspawn->spawnId, deadspawn->killerId, (deadspawn->type == -25) ? 231 : deadspawn->type,
						  deadspawn->spellId, deadspawn->damage, (target != 0) ? target->name() : QString("Unknown"),
						  (source != 0) ? source->name() : QString("Unknown"));
	}
}

void EQInterface::updatedDateTime(const QDateTime& dt)
{
	m_stsbarEQTime->setText(dt.toString(pSEQPrefs->getPrefString("DateTimeFormat", "Interface", "ddd MMM dd,yyyy - hh:mm ap")));
}

void EQInterface::syncDateTime(const QDateTime& dt)
{
	QString dateString = dt.toString(pSEQPrefs->getPrefString("DateTimeFormat", "Interface", "ddd MMM dd,yyyy - hh:mm ap"));
	m_stsbarEQTime->setText(dateString);
}

void EQInterface::zoneBegin(const QString& shortZoneName)
{
	emit newZoneName(shortZoneName);
	float percentZEM = ((float)(m_zoneMgr->zoneExpMultiplier()-0.75)/0.75)*100;
	QString tempStr;
	tempStr.sprintf("ZEM: %3.2f%%", percentZEM);
	if (m_stsbarZEM)
		m_stsbarZEM->setText(tempStr);
}

void EQInterface::zoneEnd(const QString& shortZoneName, const QString& longZoneName)
{
	emit newZoneName(longZoneName);
	stsMessage("");
	float percentZEM = ((float)(m_zoneMgr->zoneExpMultiplier()-0.75)/0.75)*100;
	QString tempStr;
	tempStr.sprintf("ZEM: %3.2f%%", percentZEM);
	if (m_stsbarZEM)
		m_stsbarZEM->setText(tempStr);
}

void EQInterface::zoneChanged(const QString& shortZoneName)
{
	QString tempStr;
	stsMessage("- Busy Zoning -");
	emit newZoneName(shortZoneName);
	float percentZEM = ((float)(m_zoneMgr->zoneExpMultiplier()-0.75)/0.75)*100;
	tempStr.sprintf("ZEM: %3.2f%%", percentZEM);
	if (m_stsbarZEM)
		m_stsbarZEM->setText(tempStr);
}

void EQInterface::clientTarget(const uint8_t* data)
{
	if (!m_selectOnTarget)
		return;

	const clientTargetStruct* cts = (const clientTargetStruct*)data;

	// try to find the targeted spawn in the spawn shell
	const Item* item = m_spawnShell->findID(tSpawn, cts->newTarget);

	// if found, make it the currently selected target
	if (item)
	{
		// note the new selection
		m_selectedSpawn = item;

		// notify others of the new selected spawn
		emit selectSpawn(m_selectedSpawn);

		// update the spawn status
		updateSelectedSpawnStatus(m_selectedSpawn);
	}
}

void EQInterface::spawnSelected(const Item* item)
{
	if (item == 0)
		return;

	// note the new selection
	m_selectedSpawn = item;

	// notify others of the new selected spawn
	emit selectSpawn(m_selectedSpawn);

	// update the spawn status
	updateSelectedSpawnStatus(m_selectedSpawn);
}

void EQInterface::spawnConsidered(const Item* item)
{
	if (item == 0)
		return;

	if (!m_selectOnConsider)
		return;

	// note the new selection
	m_selectedSpawn = item;

	// notify others of the new selected spawn
	emit selectSpawn(m_selectedSpawn);

	// update the spawn status
	updateSelectedSpawnStatus(m_selectedSpawn);
}

void EQInterface::addItem(const Item* item)
{
	uint32_t filterFlags = item->filterFlags();

	if (filterFlags & FILTER_FLAG_LOCATE)
	{
		// note the new selection
		m_selectedSpawn = item;

		// notify others of the new selected spawn
		emit selectSpawn(m_selectedSpawn);

		// update the spawn status
		updateSelectedSpawnStatus(m_selectedSpawn);
	} // End LOCATE Filter alerting
}


void EQInterface::delItem(const Item* item)
{
	// if this is the selected spawn, then there isn't a selected spawn anymore
	if (m_selectedSpawn == item)
	{
		m_selectedSpawn = 0;

		// notify others of the new selected spawn
		emit selectSpawn(m_selectedSpawn);
	}
}

void EQInterface::killSpawn(const Item* item)
{
	if (m_selectedSpawn != item)
		return;

	// update status message, notifying that selected spawn has died
	QString string = m_selectedSpawn->name() + " died";

	stsMessage(string);
}

void EQInterface::changeItem(const Item* item)
{
	// if this isn't the selected spawn, nothing more to do
	if (item != m_selectedSpawn)
		return;

	updateSelectedSpawnStatus(item);
}

void EQInterface::updateSelectedSpawnStatus(const Item* item)
{
	if (item == 0)
		return;

	const Spawn* spawn = 0;

	if ((item->type() == tSpawn) || (item->type() == tPlayer))
		spawn = (const Spawn*)item;

	// construct a message for the status message display
	QString string("");
	if (spawn != 0)
		string.sprintf("%d: %s:%d (%d/%d) Pos:", // "%d/%d/%d (%d) %s %s Item:%s",
					   item->id(),
					   (const char*)item->name().utf8(),
					   spawn->level(), spawn->HP(),
					   spawn->maxHP());
	else
		string.sprintf("%d: %s: Pos:", // "%d/%d/%d (%d) %s %s Item:%s",
					   item->id(),
					   (const char*)item->name().utf8());

	if (showeq_params->retarded_coords)
		string += QString::number(item->y()) + "/"
		+ QString::number(item->x()) + "/"
		+ QString::number(item->z());
	else
		string += QString::number(item->x()) + "/"
		+ QString::number(item->y()) + "/"
		+ QString::number(item->z());

	string += QString(" (")
    + QString::number(item->calcDist(m_player->x(),
									 m_player->y(),
									 m_player->z()))
    + ") " + item->raceString() + " " + item->classString();

	// just call the status message method
	stsMessage(string);
}

void EQInterface::addCategory()
{
	if (m_categoryMgr)
		m_categoryMgr->addCategory();
}

void EQInterface::reloadCategories()
{
	if (m_categoryMgr)
		m_categoryMgr->reloadCategories();
}

void EQInterface::rebuildSpawnList()
{
	if (m_spawnList)
		m_spawnList->spawnList()->rebuildSpawnList();

	if (m_spawnList2)
		m_spawnList2->rebuildSpawnList();
}

void EQInterface::selectNext()
{
	if (m_spawnList)
		m_spawnList->spawnList()->selectNext();
}

void EQInterface::selectPrev()
{
	if (m_spawnList)
		m_spawnList->spawnList()->selectPrev();
}

void EQInterface::saveSelectedSpawnPath()
{
	QString fileName;
	fileName.sprintf("%s_mobpath.map",
					 (const char*)m_zoneMgr->shortZoneName());

	QFileInfo fileInfo = m_dataLocationMgr->findWriteFile("maps", fileName,
														  false);

	QFile mobPathFile(fileInfo.absFilePath());
	if (mobPathFile.open(QIODevice::Append | QIODevice::WriteOnly))
	{
		Q3TextStream out(&mobPathFile);
		// append the selected spawns path to the end
		saveSpawnPath(out, m_selectedSpawn);

		seqInfo("Finished appending '%s'!\n", (const char*)fileName);
	}
}

void EQInterface::saveSpawnPaths()
{
	QString fileName;
	fileName.sprintf("%s_mobpath.map", (const char*)m_zoneMgr->shortZoneName());

	QFileInfo fileInfo = m_dataLocationMgr->findWriteFile("maps", fileName, false);

	QFile mobPathFile(fileInfo.absFilePath());
	if (mobPathFile.open(QIODevice::Truncate | QIODevice::WriteOnly))
	{
		Q3TextStream out(&mobPathFile);
		// map header line
		out << m_zoneMgr->longZoneName() << ","
			<< m_zoneMgr->shortZoneName() << ",0,0" << endl;

		// iterate over the spawns adding their paths to the file
		ItemConstIterator it(m_spawnShell->getConstMap(tSpawn));
		const Item* item;
		for (item = it.current(); item != 0; item = ++it)
		{
			if ((item->NPC() == SPAWN_NPC) ||
				(item->NPC() == SPAWN_NPC_CORPSE) ||
				(item->NPC() == SPAWN_NPC_UNKNOWN))
				saveSpawnPath(out, it.current());
		}

		seqInfo("Finished writing '%s'!\n", (const char*)fileName);
	}
}

void EQInterface::saveSpawnPath(Q3TextStream& out, const Item* item)
{
	if (item == 0)
		return;

	const Spawn* spawn = spawnType(item);

	if (spawn == 0)
		return;

	const SpawnTrackList& trackList = spawn->trackList();
	SpawnTrackListIterator trackIt(spawn->trackList());
	int cnt = trackList.count();

	// only make a line if there is more then one point
	if (cnt < 2)
		return;

	const SpawnTrackPoint* trackPoint;

	out << "M," << spawn->realName() << ",blue," << trackList.count();
	//iterate over the track, writing out the points
	for (trackPoint = trackIt.current();
		 trackPoint;
		 trackPoint = ++trackIt)
	{
		out << "," << trackPoint->x()
		<< "," <<  trackPoint->y()
		<< "," << trackPoint->z();
	}
	out << endl;
}

void EQInterface::toggle_net_real_time_thread()
{
	bool realtime = !m_packet->realtime();

	m_packet->setRealtime(realtime);
	m_netRealTimeThread->setChecked(realtime);
	pSEQPrefs->setPrefBool("RealTimeThread", "Network", realtime);
}

void EQInterface::set_net_monitor_next_client()
{
	// start monitoring the next client seen
	m_packet->monitorNextClient();

	// set it as the address to monitor next session
	pSEQPrefs->setPrefString("IP", "Network", m_packet->ip());
}

void EQInterface::set_net_client_IP_address()
{
	QStringList iplst;
	for( int l = 0; l < 5; l++)
		iplst += m_ipstr[l];
	bool ok = false;
	QString address =
	QInputDialog::getItem("ShowEQ - EQ Client IP Address",
						  "Enter IP address of EQ client",
						  iplst, 0, TRUE, &ok, this );
	if (ok)
	{
		for (int i = 4; i > 0; i--)
			m_ipstr[i] = m_ipstr[ i - 1 ];
		m_ipstr[0] = address;
		// start monitoring the new address
		m_packet->monitorIPClient(address);

		// set it as the address to monitor next session
		pSEQPrefs->setPrefString("IP", "Network", m_packet->ip());
	}
}

void EQInterface::set_net_client_MAC_address()
{
	QStringList maclst;
	for( int l = 0; l < 5; l++)
		maclst += m_macstr[l];
	bool ok = false;
	QString address =
	QInputDialog::getItem("ShowEQ - EQ Client MAC Address",
						  "Enter MAC address of EQ client",
						  maclst, 0, TRUE, &ok, this );
	if (ok)
	{
		if (address.length() != 17)
		{
			seqWarn("Invalid MAC Address (%s)! Ignoring!",
					(const char*)address);
			return;
		}
		for (int i = 4; i > 0; i--)
			m_macstr[i] = m_macstr[ i - 1 ];
		m_macstr[0] = address;
		// start monitoring the new address
		m_packet->monitorMACClient(address);

		// set it as the address to monitor next session
		pSEQPrefs->setPrefString("MAC", "Network", m_packet->mac());
	}
}

void EQInterface::set_net_device()
{
	bool ok = false;
	QString dev =
    QInputDialog::getText("ShowEQ - Device",
						  "Enter the device to sniff for EQ Packets:",
						  QLineEdit::Normal, m_packet->device(),
						  &ok, this);

	if (ok)
	{
		// start monitoring the device
		m_packet->monitorDevice(dev);

		// set it as the device to monitor next session
		pSEQPrefs->setPrefString("Device", "Network", m_packet->device());
	}
}

void EQInterface::set_net_arq_giveup(int giveup)
{
	// set the Arq Seq Give Up length
	m_packet->setArqSeqGiveUp(uint16_t(giveup));

	// set it as the value to use next session
	pSEQPrefs->setPrefInt("ArqSeqGiveUp", "Network", m_packet->arqSeqGiveUp());
}

void EQInterface::toggle_net_session_tracking(bool enable)
{
	m_netSessionTracking->setChecked(enable);
	m_packet->session_tracking(enable);

	pSEQPrefs->setPrefBool("SessionTracking", "Network", enable);
}

void EQInterface::toggleAutoDetectPlayerSettings (int id)
{
	m_player->setUseAutoDetectedSettings(!m_player->useAutoDetectedSettings());
	menuBar()->setItemChecked (id, m_player->useAutoDetectedSettings());
}

/* Choose the character's level */
void EQInterface::SetDefaultCharacterLevel(int level)
{
	m_player->setDefaultLevel(level);
}

/* Choose the character's class */
void EQInterface::SetDefaultCharacterClass(int id)
{
	for (int i = 0; i < PLAYER_CLASSES; i++)
		m_charClassMenu->setItemChecked(char_ClassID[i], char_ClassID[i] == id);
	m_player->setDefaultClass(m_charClassMenu->itemParameter(id));
}

/* Choose the character's race */
void EQInterface::SetDefaultCharacterRace(int id)
{
	for (int i = 0; i < PLAYER_RACES; i++)
		m_charRaceMenu->setItemChecked(char_RaceID[i], char_RaceID[i] == id);
	m_player->setDefaultRace(m_charRaceMenu->itemParameter(id));
}

void EQInterface::toggle_view_menubar()
{
	if (menuBar()->isVisible())
		menuBar()->hide();
	else
		menuBar()->show();
}

void EQInterface::toggle_view_statusbar()
{
	if (statusBar()->isVisible())
		statusBar()->hide();
	else
		statusBar()->show();
	pSEQPrefs->setPrefBool("StatusBarActive", "Interface_StatusBar", statusBar()->isVisible());
}

void EQInterface::updateViewMenu()
{
	// set the checkmarks for windows that are always created, but not always visible
	m_viewExpWindow->setChecked((m_expWindow != NULL) && m_expWindow->isVisible());
	m_viewCombatWindow->setChecked((m_combatWindow != NULL) && m_combatWindow->isVisible());

	m_viewPlayerSkills->setChecked((m_skillList != NULL) && m_skillList->isVisible());
	m_viewPlayerStats->setChecked((m_statList != NULL) && m_statList->isVisible());
	m_viewSpawnList->setChecked((m_spawnList != NULL) && m_spawnList->isVisible());
	m_viewSpawnList2->setChecked((m_spawnList2 != NULL) && m_spawnList2->isVisible());
	m_viewSpawnPointList->setChecked((m_spawnPointList != NULL) && m_spawnPointList->isVisible());
	m_viewCompass->setChecked((m_compass != NULL) && m_compass->isVisible());
	m_viewSpellList->setChecked((m_spellList != NULL) && m_spellList->isVisible());
	m_viewNetDiag->setChecked((m_netDiag != NULL) && m_netDiag->isVisible());
	m_viewGuildList->setChecked((m_guildListWindow != NULL) && m_guildListWindow->isVisible());

	// loop over the maps
	for (int i = 0; i < maxNumMaps; i++)
		m_viewMap[i]->setChecked((m_map[i] != NULL) && m_map[i]->isVisible());

	// loop over the message windows
	for (int i = 0; i < maxNumMessageWindows; i++)
		m_viewMessageWindow[i]->setChecked((m_messageWindow[i] != NULL) && m_messageWindow[i]->isVisible());

	// set initial view options for the spawn list menu
	if (m_spawnList != NULL && m_spawnList->isVisible())
	{

		SEQListView* spawnList = m_spawnList->spawnList();

		QListIterator<QAction*> it(m_spawnListMenu->actions());
		while (it.hasNext())
		{
			QAction* action = it.next();
			QVariant val = action->data();
			if (val.isValid())
			{
				action->setChecked(spawnList->columnVisible(val.toInt()));
			}
		}
	}
	else
	{
		m_spawnListMenuAction->setEnabled(false);
	}

	// set view status for the player stats menu
	if (m_statList != NULL && m_statList->isVisible())
	{
		StatList* statList = m_statList->statList();

		QListIterator<QAction*> it(m_playerStatsMenu->actions());
		while (it.hasNext())
		{
			QAction* action = it.next();
			QVariant val = action->data();
			if (val.isValid())
			{
				action->setChecked(statList->statShown(val.toInt()));
			}
		}
	}
	else
	{
		m_playerStatsMenuAction->setEnabled(false);
	}

	// set view status for the skill list menu
	if (m_skillList != NULL && m_skillList->isVisible())
	{
		// make sure the proper menu items are checked
		m_playerSkillsLanguages->setChecked(m_skillList->skillList()->showLanguages());
	}
	else
	{
		m_playerSkillsMenuAction->setEnabled(false);
	}
}

void EQInterface::toggleSavePlayerState(bool state)
{
	showeq_params->savePlayerState = state;
	pSEQPrefs->setPrefBool("PlayerState", "SaveState", state);
}

void EQInterface::toggleSaveZoneState(bool state)
{
	showeq_params->saveZoneState = state;
	pSEQPrefs->setPrefBool("ZoneState", "SaveState", state);
}

void EQInterface::toggleSaveSpawnState(bool state)
{
	showeq_params->saveSpawns = state;
	pSEQPrefs->setPrefBool("Spawns", "SaveState", state);

	if (state)
		m_spawnShell->saveSpawns();
}

void EQInterface::setSpawnSaveFrequency(int frequency)
{
	showeq_params->saveSpawnsFrequency = frequency * 1000;
	pSEQPrefs->setPrefInt("SpawnsFrequency", "SaveState", showeq_params->saveSpawnsFrequency);
}

void EQInterface::setSaveBaseFilename()
{
	QString fileName =
    Q3FileDialog::getSaveFileName(showeq_params->saveRestoreBaseFilename, QString::null, this, "SaveBaseFilename", "Save State Base Filename");
	if (!fileName.isEmpty())
	{
		// set it to be the new base filename
		showeq_params->saveRestoreBaseFilename = fileName;

		// set preference to use for next session
		pSEQPrefs->setPrefString("BaseFilename", "SaveState", showeq_params->saveRestoreBaseFilename);
	}
}

void EQInterface::clearChannelMessages()
{
	m_messages->clear();
}


void EQInterface::showMessageFilterDialog()
{
	// create the filter dialog, if necessary
	if (!m_messageFilterDialog)
		m_messageFilterDialog = new MessageFilterDialog(m_messageFilters, "ShowEQ Message Filters", this, "messagefilterdialog");

	// show the message filter dialog
	m_messageFilterDialog->show();
}

void EQInterface::toggleTypeFilter(int id)
{
	uint64_t enabledTypes = m_terminal->enabledTypes();

	if (((uint64_t(1) << id) & enabledTypes) != 0)
		enabledTypes &= ~(uint64_t(1) << id);
	else
		enabledTypes |= (uint64_t(1) << id);

	m_terminal->setEnabledTypes(enabledTypes);

	// (un)check the appropriate menu item
	m_terminalTypeFilterMenu->setItemChecked(id, ((enabledTypes & (uint64_t(1) << id))));
}

void EQInterface::disableAllTypeFilters()
{
	m_terminal->setEnabledTypes(0);
	// make sure the All menu items are unchecked
	m_terminalTypeFilterMenu->setItemChecked(64, false);
	m_terminalTypeFilterMenu->setItemChecked(65, false);

	// uncheck all the menu items
	QString typeName;
	for (int i = MT_Guild; i <= MT_Max; i++)
	{
		typeName = MessageEntry::messageTypeString((MessageType)i);
		if (!typeName.isEmpty())
			m_terminalTypeFilterMenu->setItemChecked(i, false);
	}
}

void EQInterface::enableAllTypeFilters()
{
	m_terminal->setEnabledTypes(0xFFFFFFFFFFFFFFFFULL);

	// make sure the All menu items are unchecked
	m_terminalTypeFilterMenu->setItemChecked(64, false);
	m_terminalTypeFilterMenu->setItemChecked(65, false);

	// check all the menu items
	QString typeName;
	for (int i = MT_Guild; i <= MT_Max; i++)
	{
		typeName = MessageEntry::messageTypeString((MessageType)i);
		if (!typeName.isEmpty())
			m_terminalTypeFilterMenu->setItemChecked(i, true);
	}
}

void EQInterface::toggleShowUserFilter(int id)
{
	uint32_t enabledShowUserFilters = m_terminal->enabledShowUserFilters();
	// toggle whether the filter is enabled/disabled
	if (((1 << id) & enabledShowUserFilters) != 0)
		enabledShowUserFilters &= ~(1 << id);
	else
		enabledShowUserFilters |= (1 << id);

	m_terminal->setEnabledShowUserFilters(enabledShowUserFilters);

	// (un)check the appropriate menu item
	m_terminalShowUserFilterMenu->setItemChecked(id, ((enabledShowUserFilters & (1 << id)) != 0));
}

void EQInterface::disableAllShowUserFilters()
{
	// set and save all filters disabled setting
	m_terminal->setEnabledShowUserFilters(0);

	// make sure the All menu items are unchecked
	m_terminalShowUserFilterMenu->setItemChecked(66, false);
	m_terminalShowUserFilterMenu->setItemChecked(67, false);

	// uncheck all the menu items
	QString typeName;
	for (int i = 0; i <= maxMessageFilters; i++)
	{
		if (m_messageFilters->filter(i))
			m_terminalShowUserFilterMenu->setItemChecked(i, false);
	}
}

void EQInterface::enableAllShowUserFilters()
{
	// set and save all filters enabled flag
	m_terminal->setEnabledShowUserFilters(0xFFFFFFFF);

	// make sure the All menu items are unchecked
	m_terminalShowUserFilterMenu->setItemChecked(66, false);
	m_terminalShowUserFilterMenu->setItemChecked(67, false);

	// check all the menu items
	QString typeName;
	for (int i = 0; i <= maxMessageFilters; i++)
	{
		if (m_messageFilters->filter(i))
			m_terminalShowUserFilterMenu->setItemChecked(i, true);
	}
}

void EQInterface::toggleHideUserFilter(int id)
{
	uint32_t enabledHideUserFilters = m_terminal->enabledHideUserFilters();

	// toggle whether the filter is enabled/disabled
	if (((1 << id) & enabledHideUserFilters) != 0)
		enabledHideUserFilters &= ~(1 << id);
	else
		enabledHideUserFilters |= (1 << id);

	m_terminal->setEnabledHideUserFilters(enabledHideUserFilters);

	// (un)check the appropriate menu item
	m_terminalHideUserFilterMenu->setItemChecked(id, ((enabledHideUserFilters & (1 << id)) != 0));
}

void EQInterface::disableAllHideUserFilters()
{
	// set and save all filters disabled setting
	m_terminal->setEnabledHideUserFilters(0);

	// make sure the All menu items are unchecked
	m_terminalHideUserFilterMenu->setItemChecked(66, false);
	m_terminalHideUserFilterMenu->setItemChecked(67, false);

	// uncheck all the menu items
	QString typeName;
	for (int i = 0; i <= maxMessageFilters; i++)
	{
		if (m_messageFilters->filter(i))
			m_terminalHideUserFilterMenu->setItemChecked(i, false);
	}
}

void EQInterface::enableAllHideUserFilters()
{
	// set and save all filters enabled flag
	m_terminal->setEnabledHideUserFilters(0xFFFFFFFF);

	// make sure the All menu items are unchecked
	m_terminalHideUserFilterMenu->setItemChecked(66, false);
	m_terminalHideUserFilterMenu->setItemChecked(67, false);

	// check all the menu items
	QString typeName;
	for (int i = 0; i <= maxMessageFilters; i++)
	{
		if (m_messageFilters->filter(i))
			m_terminalHideUserFilterMenu->setItemChecked(i, true);
	}
}

void EQInterface::toggleDisplayType(int id)
{
	// toggle the display of message types
	m_terminal->setDisplayType(m_terminal->displayType());
	m_terminalMenu->setItemChecked(id, m_terminal->displayType());
}

void EQInterface::toggleDisplayTime(int id)
{
	// toggle the display of message time
	m_terminal->setDisplayDateTime(!m_terminal->displayDateTime());
	m_terminalMenu->setItemChecked(id, m_terminal->displayDateTime());
}

void EQInterface::toggleEQDisplayTime(int id)
{
	m_terminal->setDisplayEQDateTime(!m_terminal->displayEQDateTime());
	m_terminalMenu->setItemChecked(id, m_terminal->displayEQDateTime());
}

void EQInterface::toggleUseColor(int id)
{
	m_terminal->setUseColor(!m_terminal->useColor());
	m_terminalMenu->setItemChecked(id, m_terminal->useColor());
}

int EQInterface::setTheme(int id)
{
#if 0
    static QFont OrigFont = qApp->font();
    static QPalette OrigPalette = qApp->palette();;

    MenuIDList::Iterator iter;
    for ( iter = IDList_StyleMenu.begin(); iter != IDList_StyleMenu.end(); ++iter)
		menuBar()->setItemChecked( (*iter), false );

    menuBar()->setItemChecked( id, true );
    int theme = menuBar()->itemParameter(id);

    switch ( theme )
    {
		case 1: // platinum
		{
			QPalette p( QColor( 239, 239, 239 ) );
			qApp->setStyle("platinum");
			qApp->setPalette( p, TRUE );
		}
			break;
		case 2: // windows
		{
			qApp->setStyle("windows");
			qApp->setFont( OrigFont, TRUE );
			qApp->setPalette( OrigPalette, TRUE );
		}
			break;
		case 3: // cde
		case 4: // cde polished
		{
			QPalette p( QColor( 75, 123, 130 ) );
			qApp->setStyle("cde");
			p.setColor( QPalette::Active, QColorGroup::Base, QColor( 55, 77, 78 ) );
			p.setColor( QPalette::Inactive, QColorGroup::Base, QColor( 55, 77, 78 ) );
			p.setColor( QPalette::Disabled, QColorGroup::Base, QColor( 55, 77, 78 ) );
			p.setColor( QPalette::Active, QColorGroup::Highlight, Qt::white );
			p.setColor( QPalette::Active, QColorGroup::HighlightedText, QColor( 55, 77, 78 ) );
			p.setColor( QPalette::Inactive, QColorGroup::Highlight, Qt::white );
			p.setColor( QPalette::Inactive, QColorGroup::HighlightedText, QColor( 55, 77, 78 ) );
			p.setColor( QPalette::Disabled, QColorGroup::Highlight, Qt::white );
			p.setColor( QPalette::Disabled, QColorGroup::HighlightedText, QColor( 55, 77, 78 ) );
			p.setColor( QPalette::Active, QColorGroup::Foreground, Qt::white );
			p.setColor( QPalette::Active, QColorGroup::Text, Qt::white );
			p.setColor( QPalette::Active, QColorGroup::ButtonText, Qt::white );
			p.setColor( QPalette::Inactive, QColorGroup::Foreground, Qt::white );
			p.setColor( QPalette::Inactive, QColorGroup::Text, Qt::white );
			p.setColor( QPalette::Inactive, QColorGroup::ButtonText, Qt::white );
			p.setColor( QPalette::Disabled, QColorGroup::Foreground, Qt::lightGray );
			p.setColor( QPalette::Disabled, QColorGroup::Text, Qt::lightGray );
			p.setColor( QPalette::Disabled, QColorGroup::ButtonText, Qt::lightGray );
			qApp->setPalette( p, TRUE );
			qApp->setFont( QFont( "times", OrigFont.pointSize() ), TRUE );
		}
			break;
		case 5: // motif
		{
			QPalette p( QColor( 192, 192, 192 ) );
			qApp->setStyle("motif");
			qApp->setPalette( p, TRUE );
			qApp->setFont( OrigFont, TRUE );
		}
			break;
		case 6: // SGI
		{
			//QPalette p( QColor( 192, 192, 192 ) );
			qApp->setStyle("sgi");
			qApp->setPalette( OrigPalette, TRUE );
			qApp->setFont( OrigFont, TRUE );
		}
			break;
		default: // system default
		{
			QPalette p( QColor( 192, 192, 192 ) );
			qApp->setStyle("motif");
			qApp->setPalette( p, TRUE );
			qApp->setFont( OrigFont, TRUE );
			theme = 2;
		}
			break;
    }

    // make sure the windows that override the application font, do so
    emit restoreFonts();

    return theme;
#else
	return 0;
#endif

}

void EQInterface::selectTheme(int id)
{
	int theme = setTheme(id);
	pSEQPrefs->setPrefInt("Theme", "Interface", theme);
}

void EQInterface::showMap(int i)
{
	if ((i > maxNumMaps) || (i < 0))
		return;

	// if it doesn't exist, create it
	if (m_map[i] == 0)
	{
		int mapNum = i + 1;
		QString mapPrefName = "Map";
		QString mapName = QString("map") + QString::number(mapNum);
		QString mapCaption = "Map ";

		if (i != 0)
		{
			mapPrefName += QString::number(mapNum);
			mapCaption += QString::number(mapNum);
		}

		m_map[i] = new MapFrame(m_filterMgr, m_mapMgr, m_player, m_spawnShell, m_zoneMgr,
								m_spawnMonitor, mapPrefName, mapCaption, mapName, this);

		if (i != 0)
		{
			setDockEnabled(m_map[i], pSEQPrefs->getPrefBool(QString("Dockable") + mapPrefName, "Interface", true));
			Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_map[i]->preferenceName(), Qt::LeftDockWidgetArea);
			addDockWidget(edge, m_map[i]);

			if (!m_isMapDocked[i])
				m_map[i]->undock();
		}
		else
		{
			m_map[i]->setFeatures(QDockWidget::NoDockWidgetFeatures);
			m_map[i]->setTitleBarWidget(new QWidget());
			setCentralWidget(m_map[i]);
		}
		connect(this, SIGNAL(saveAllPrefs()), m_map[i], SLOT(savePrefs()));
		connect(this, SIGNAL(restoreFonts()), m_map[i], SLOT(restoreFont()));

		// Get the map...
		Map* map = m_map[i]->map();

		// supply the Map slots with signals from EQInterface
		connect(this, SIGNAL(selectSpawn(const Item*)), map, SLOT(selectSpawn(const Item*)));

		// supply EQInterface slots with signals from Map
		connect(map, SIGNAL(spawnSelected(const Item*)), this, SLOT(spawnSelected(const Item*)));

		m_map[i]->restoreSize();

		// restore it's position if necessary and practical
		if (pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true))
			m_map[i]->restorePosition();

		// insert its menu into the window menu
		insertWindowMenu(m_map[i]);
	}

	// make sure it's visible
	if (!m_creating)
		m_map[i]->show();
}

void EQInterface::showMessageWindow(int i)
{
	if ((i > maxNumMessageWindows) || (i < 0))
		return;

	// if it doesn't exist, create it
	if (m_messageWindow[i] == 0)
	{
		int winNum = i + 1;
		QString prefName = "MessageWindow" + QString::number(winNum);
		QString name = QString("messageWindow") + QString::number(winNum);
		QString caption = "Channel Messages ";

		if (i != 0)
			caption += QString::number(winNum);

		m_messageWindow[i] = new MessageWindow(m_messages, m_messageFilters, prefName, caption, this, name);

		setDockEnabled(m_messageWindow[i], pSEQPrefs->getPrefBool(QString("Dockable") + prefName, "Interface", false));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_messageWindow[i]->preferenceName(), Qt::LeftDockWidgetArea);
		addDockWidget(edge, m_messageWindow[i]);

		if (!m_isMessageWindowDocked[i])
			m_messageWindow[i]->undock();

		connect(this, SIGNAL(saveAllPrefs()), m_messageWindow[i], SLOT(savePrefs()));
		connect(this, SIGNAL(restoreFonts()), m_messageWindow[i], SLOT(restoreFont()));

		m_messageWindow[i]->restoreSize();

		// restore it's position if necessary and practical
		if (pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true))
			m_messageWindow[i]->restorePosition();

		// insert its menu into the window menu
		insertWindowMenu(m_messageWindow[i]);
	}

	// make sure it's visible
	if (!m_creating)
		m_messageWindow[i]->show();
}

void EQInterface::showSpawnList()
{
	// if it doesn't exist, create it.
	if (m_spawnList == NULL)
	{
		m_spawnList = new SpawnListWindow (m_player, m_spawnShell, m_categoryMgr, this, "spawnlist");

		setDockEnabled(m_spawnList, pSEQPrefs->getPrefBool("DockableSpawnList", "Interface", true));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_spawnList->preferenceName(), Qt::LeftDockWidgetArea);
		addDockWidget(edge, m_spawnList);

		if (m_isSpawnListDocked)
			m_spawnList->undock();

		// restore the size of the spawn list
		m_spawnList->restoreSize();

		// only do this move stuff if the spawn list isn't docked
		// and the user set the option to do so.
		if (!m_isSpawnListDocked && pSEQPrefs->getPrefBool("UseWindowPos", "Interface", false))
			m_spawnList->restorePosition();

		// connections from spawn list to interface
		connect(m_spawnList->spawnList(), SIGNAL(spawnSelected(const Item*)), this, SLOT(spawnSelected(const Item*)));

		// connections from interface to spawn list
		connect(this, SIGNAL(selectSpawn(const Item*)), m_spawnList->spawnList(), SLOT(selectSpawn(const Item*)));
		connect(this, SIGNAL(saveAllPrefs()), m_spawnList, SLOT(savePrefs()));
		connect(this, SIGNAL(restoreFonts()), m_spawnList, SLOT(restoreFont()));

		// insert its menu into the window menu
		insertWindowMenu(m_spawnList);
	}

	// make sure it's visible
	if (!m_creating)
		m_spawnList->show();
}

void EQInterface::showSpawnList2()
{
	// if it doesn't exist, create it.
	if (m_spawnList2 == NULL)
	{
		m_spawnList2 = new SpawnListWindow2(m_player, m_spawnShell, m_categoryMgr, this, "spawnlist");

		setDockEnabled(m_spawnList2, pSEQPrefs->getPrefBool("DockableSpawnList2", "Interface", true));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_spawnList2->preferenceName(), Qt::LeftDockWidgetArea);
		addDockWidget(edge, m_spawnList2);

		if (!m_isSpawnList2Docked)
			m_spawnList2->undock();

		// restore the size of the spawn list
		m_spawnList2->restoreSize();

		// only do this move stuff if the spawn list isn't docked
		// and the user set the option to do so.
		if (!m_isSpawnList2Docked && pSEQPrefs->getPrefBool("UseWindowPos", "Interface", 0))
			m_spawnList2->restorePosition();

		// connections from spawn list to interface
		connect(m_spawnList2, SIGNAL(spawnSelected(const Item*)), this, SLOT(spawnSelected(const Item*)));

		// connections from interface to spawn list
		connect(this, SIGNAL(selectSpawn(const Item*)), m_spawnList2, SLOT(selectSpawn(const Item*)));
		connect(this, SIGNAL(saveAllPrefs()), m_spawnList2, SLOT(savePrefs()));
		connect(this, SIGNAL(restoreFonts()), m_spawnList2, SLOT(restoreFont()));

		// insert its menu into the window menu
		insertWindowMenu(m_spawnList2);
	}

	// make sure it's visible
	if (!m_creating)
		m_spawnList2->show();
}

void EQInterface::showSpawnPointList()
{
	// if it doesn't exist, create it.
	if (m_spawnPointList == NULL)
	{
		m_spawnPointList = new SpawnPointWindow(m_spawnMonitor, this, "spawnlist");

		setDockEnabled(m_spawnPointList, pSEQPrefs->getPrefBool("DockableSpawnPointList", "Interface", true));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_spawnPointList->preferenceName(), Qt::LeftDockWidgetArea);
		addDockWidget(edge, m_spawnPointList);

		if (!m_isSpawnPointListDocked)
			m_spawnPointList->undock();

		// restore the size of the spawn list
		m_spawnPointList->restoreSize();

		// only do this move stuff iff the spawn list isn't docked
		// and the user set the option to do so.
		if (!m_isSpawnPointListDocked && pSEQPrefs->getPrefBool("UseWindowPos", "Interface", false))
			m_spawnPointList->restorePosition();

		// connections from interface to spawn list
		connect(this, SIGNAL(saveAllPrefs()), m_spawnPointList, SLOT(savePrefs()));
		connect(this, SIGNAL(restoreFonts()), m_spawnPointList, SLOT(restoreFont()));

		// insert its menu into the window menu
		insertWindowMenu(m_spawnPointList);
	}

	// make sure it's visible
	if (!m_creating)
		m_spawnPointList->show();
}

void EQInterface::showStatList()
{
	// if it doesn't exist, create it
	if (m_statList == NULL)
	{
		m_statList = new StatListWindow(m_player, this, "stats");

		setDockEnabled(m_statList, pSEQPrefs->getPrefBool("DockablePlayerStats", "Interface", true));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_statList->preferenceName(), Qt::LeftDockWidgetArea);
		addDockWidget(edge, m_statList);

		if (!m_isStatListDocked)
			m_statList->undock();

		// connect stat list slots to interface signals
		connect(this, SIGNAL(saveAllPrefs()), m_statList, SLOT(savePrefs()));
		connect(this, SIGNAL(restoreFonts()), m_statList, SLOT(restoreFont()));

		// restore the size of the spawn list
		m_statList->restoreSize();

		// only do this move stuff iff the spawn list isn't docked
		// and the user set the option to do so.
		if (!m_isStatListDocked && pSEQPrefs->getPrefBool("UseWindowPos", "Interface", false))
			m_statList->restorePosition();

		// insert its menu into the window menu
		insertWindowMenu(m_statList);
	}

	// make sure it's visible
	if (!m_creating)
		m_statList->show();
}

void EQInterface::showSkillList()
{
	// if it doesn't exist, create it
	if (m_skillList == NULL)
	{
		m_skillList = new SkillListWindow(m_player, this, "skills");

		setDockEnabled(m_skillList, pSEQPrefs->getPrefBool("DockablePlayerSkills", "Interface", true));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_skillList->preferenceName(), Qt::LeftDockWidgetArea);
		addDockWidget(edge, m_skillList);

		if (!m_isSkillListDocked)
			m_skillList->undock();

		// connect skill list slots to interfaces signals
		connect(this, SIGNAL(saveAllPrefs()), m_skillList, SLOT(savePrefs()));
		connect(this, SIGNAL(restoreFonts()), m_skillList, SLOT(restoreFont()));

		// restore the size of the spawn list
		m_skillList->restoreSize();

		// only do this move stuff iff the spawn list isn't docked
		// and the user set the option to do so.
		if (!m_isSkillListDocked &&	pSEQPrefs->getPrefBool("UseWindowPos", "Interface", false))
			m_skillList->restorePosition();

		// insert its menu into the window menu
		insertWindowMenu(m_skillList);
	}

	// make sure it's visible
	if (!m_creating)
		m_skillList->show();
}

void EQInterface::showSpellList()
{
	// if it doesn't exist, create it
	if (m_spellList == NULL)
	{
		m_spellList = new SpellListWindow(m_spellShell, this, "spelllist");

		setDockEnabled(m_spellList, pSEQPrefs->getPrefBool("DockableSpellList", "Interface", true));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_spellList->preferenceName(), Qt::LeftDockWidgetArea);
		addDockWidget(edge, m_spellList);

		if (!m_isSpellListDocked)
			m_spellList->undock();

		SpellList* spellList = m_spellList->spellList();

		// connect SpellShell to SpellList
		connect(m_spellShell, SIGNAL(addSpell(const SpellItem *)), spellList, SLOT(addSpell(const SpellItem *)));
		connect(m_spellShell, SIGNAL(delSpell(const SpellItem *)), spellList, SLOT(delSpell(const SpellItem *)));
		connect(m_spellShell, SIGNAL(changeSpell(const SpellItem *)), spellList, SLOT(changeSpell(const SpellItem *)));
		connect(m_spellShell, SIGNAL(clearSpells()), spellList, SLOT(clear()));
		connect(this, SIGNAL(saveAllPrefs()), m_spellList, SLOT(savePrefs()));
		connect(this, SIGNAL(restoreFonts()), m_spellList, SLOT(restoreFont()));

		// restore the size of the spell list
		m_spellList->restoreSize();

		// only do this move stuff iff the spell list isn't docked
		// and the user set the option to do so.
		if (!m_isSpellListDocked && pSEQPrefs->getPrefBool("UseWindowPos", "Interface", false))
			m_spellList->restorePosition();

		// insert its menu into the window menu
		insertWindowMenu(m_spellList);
	}

	// make sure it's visible
	if (!m_creating)
		m_spellList->show();
}

void EQInterface::showCompass()
{
	// if it doesn't exist, create it.
	if (m_compass == 0)
	{
		m_compass = new CompassFrame(m_player, this, "compass");

		setDockEnabled(m_compass, pSEQPrefs->getPrefBool("DockableCompass", "Interface", true));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_compass->preferenceName(), Qt::LeftDockWidgetArea);
		addDockWidget(edge, m_compass);

		if (!m_isCompassDocked)
			m_compass->undock();

		// supply the compass slots with EQInterface signals
		connect(this, SIGNAL(selectSpawn(const Item*)), m_compass, SLOT(selectSpawn(const Item*)));
		connect(this, SIGNAL(restoreFonts()), m_compass, SLOT(restoreFont()));
		connect(this, SIGNAL(saveAllPrefs()), m_compass, SLOT(savePrefs()));

		m_compass->restoreSize();

		// move window to new position
		if (pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true))
			m_compass->restorePosition();

		// insert its menu into the window menu
		insertWindowMenu(m_compass);
	}

	// make sure it's visible
	if (!m_creating)
		m_compass->show();
}

void EQInterface::showNetDiag()
{
	if (m_netDiag == 0)
	{
		m_netDiag = new NetDiag(m_packet, this, "NetDiag");

		setDockEnabled(m_netDiag, pSEQPrefs->getPrefBool("DockableNetDiag", "Interface", true));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_netDiag->preferenceName(), Qt::BottomDockWidgetArea);
		addDockWidget(edge, m_netDiag);

		m_netDiag->undock();

		connect(this, SIGNAL(restoreFonts()), m_netDiag, SLOT(restoreFont()));
		connect(this, SIGNAL(saveAllPrefs()), m_netDiag, SLOT(savePrefs()));

		m_netDiag->restoreSize();

		// move window to new position
		if (pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true))
			m_netDiag->restorePosition();

		// insert its menu into the window menu
		insertWindowMenu(m_netDiag);
	}

	// make sure it's visible
	if (!m_creating)
		m_netDiag->show();
}

void EQInterface::showGuildList()
{
	if (!m_guildListWindow)
	{
		m_guildListWindow = new GuildListWindow(m_player, m_guildShell, this, "GuildList");

		setDockEnabled(m_guildListWindow, pSEQPrefs->getPrefBool("DockableGuildListWindow", "Interface", true));
		Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_guildListWindow->preferenceName(), Qt::BottomDockWidgetArea);
		addDockWidget(edge, m_guildListWindow);

		m_guildListWindow->undock();

		connect(this, SIGNAL(restoreFonts()), m_guildListWindow, SLOT(restoreFont()));
		connect(this, SIGNAL(saveAllPrefs()), m_guildListWindow, SLOT(savePrefs()));

		m_guildListWindow->restoreSize();

		// move window to new position
		if (pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true))
			m_guildListWindow->restorePosition();

		// insert its menu into the window menu
		insertWindowMenu(m_guildListWindow);
	}

	// make sure it's visible
	if (!m_creating)
		m_guildListWindow->show();
}

void EQInterface::createFilteredSpawnLog()
{
	if (m_filteredSpawnLog)
		return;

	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", "filtered_spawns.log");

	m_filteredSpawnLog = new FilteredSpawnLog(m_dateTimeMgr, m_filterMgr, logFileInfo.absFilePath());

	connect(m_spawnShell, SIGNAL(addItem(const Item*)), m_filteredSpawnLog, SLOT(addItem(const Item*)));
	connect(m_spawnShell, SIGNAL(delItem(const Item*)), m_filteredSpawnLog, SLOT(delItem(const Item*)));
	connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)), m_filteredSpawnLog, SLOT(killSpawn(const Item*)));
}

void EQInterface::createSpawnLog()
{
	// if the spawnLogger already exists, then nothing to do...
	if (m_spawnLogger)
		return;

	QString logFile = pSEQPrefs->getPrefString("SpawnLogFilename", "Misc", "spawnlog.txt");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
	logFile = logFileInfo.absFilePath();

	// create the spawn logger
	m_spawnLogger = new SpawnLog(m_dateTimeMgr, logFile);

	// initialize it with the current state
	QString shortZoneName = m_zoneMgr->shortZoneName();
	if (!shortZoneName.isEmpty())
		m_spawnLogger->logNewZone(shortZoneName);

	// Connect SpawnLog slots to ZoneMgr signals
	connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)), m_spawnLogger, SLOT(logNewZone(const QString&)));

	// No longer used as of 5-22-2008
#if 0
	// Connect SpawnLog slots to EQPacket signals
	m_packet->connect2("OP_ZoneSpawns", SP_Zone, DIR_Server, "spawnStruct", SZC_Modulus, m_spawnLogger, SLOT(logZoneSpawns(const uint8_t*, size_t)));
#endif

	// OP_NewSpawn is deprecated in the client
	//    m_packet->connect2("OP_NewSpawn", SP_Zone, DIR_Server,
	// 		      "spawnStruct", SZC_Match,
	// 		      m_spawnLogger, SLOT(logNewSpawn(const uint8_t*)));

	// Connect SpawnLog slots to SpawnShell signals
	connect(m_spawnShell, SIGNAL(delItem(const Item*)),	m_spawnLogger, SLOT(logDeleteSpawn(const Item *)));
	connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)), m_spawnLogger, SLOT(logKilledSpawn(const Item *, const Item*, uint16_t)));
}

void EQInterface::createGlobalLog()
{
	if (m_globalLog)
		return;

	QString logFile = pSEQPrefs->getPrefString("GlobalLogFilename", "PacketLogging", "global.log");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);

	m_globalLog = new PacketLog(*m_packet, logFileInfo.absFilePath(), this, "GlobalLog");

	connect(m_packet, SIGNAL(newPacket(const EQUDPIPPacketFormat&)), m_globalLog, SLOT(logData(const EQUDPIPPacketFormat&)));
}

void EQInterface::createWorldLog()
{
	if (m_worldLog)
		return;

	QString logFile = pSEQPrefs->getPrefString("WorldLogFilename", "PacketLogging", "world.log");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);

	m_worldLog = new PacketStreamLog(*m_packet, logFileInfo.absFilePath(), this, "WorldLog");
	m_worldLog->setRaw(pSEQPrefs->getPrefBool("LogRawPackets", "PacketLogging", false));

	connect(m_packet, SIGNAL(rawWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t)),
			m_worldLog, SLOT(rawStreamPacket(const uint8_t*, size_t, uint8_t, uint16_t)));
	connect(m_packet, SIGNAL(decodedWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)),
			m_worldLog, SLOT(decodedStreamPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)));
}

void EQInterface::createZoneLog()
{
	if (m_zoneLog)
		return;

	QString logFile = pSEQPrefs->getPrefString("ZoneLogFilename", "PacketLogging", "zone.log");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);

	m_zoneLog = new PacketStreamLog(*m_packet, logFileInfo.absFilePath(), this, "ZoneLog");
	m_zoneLog->setRaw(pSEQPrefs->getPrefBool("LogRawPackets", "PacketLogging", false));
	m_zoneLog->setDir(0);

	connect(m_packet, SIGNAL(rawZonePacket(const uint8_t*, size_t, uint8_t, uint16_t)),
			m_zoneLog, SLOT(rawStreamPacket(const uint8_t*, size_t, uint8_t, uint16_t)));
	connect(m_packet, SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)),
			m_zoneLog, SLOT(decodedStreamPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*)));
}

void EQInterface::createBazaarLog()
{
	if (m_bazaarLog)
		return;

	QString logFile = pSEQPrefs->getPrefString("BazaarLogFilename", "PacketLogging", "bazaar.log");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);

	m_bazaarLog = new BazaarLog(*m_packet, logFileInfo.absFilePath(), this, *m_spawnShell, "BazaarLog");
	m_packet->connect2("OP_BazaarSearch", SP_Zone, DIR_Server, "bazaarSearchResponseStruct", SZC_Modulus,
					   m_bazaarLog, SLOT(bazaarSearch(const uint8_t*, size_t, uint8_t)));
}

void EQInterface::createUnknownZoneLog()
{
	if (m_unknownZoneLog)
		return;

	QString section = "PacketLogging";
	QString logFile = pSEQPrefs->getPrefString("UnknownZoneLogFilename", section, "unknownzone.log");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
	logFile = logFileInfo.absFilePath();

	m_unknownZoneLog = new UnknownPacketLog(*m_packet, logFile, this, "UnknownLog");
	m_unknownZoneLog->setView(pSEQPrefs->getPrefBool("ViewUnknown", section, false));

	connect(m_packet, SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
			m_unknownZoneLog, SLOT(packet(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
	connect(m_packet, SIGNAL(decodedWorldPacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
			m_unknownZoneLog, SLOT(packet(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
}

void EQInterface::createOPCodeMonitorLog(const QString& opCodeList)
{
	if (m_opcodeMonitorLog)
		return;

	QString section = "OpCodeMonitoring";
	QString logFile = pSEQPrefs->getPrefString("LogFilename", section, "opcodemonitor.log");
	QFileInfo logFileInfo = m_dataLocationMgr->findWriteFile("logs", logFile);
	logFile = logFileInfo.absFilePath();

	m_opcodeMonitorLog = new OPCodeMonitorPacketLog(*m_packet, logFile, this, "OpCodeMonitorLog");
	m_opcodeMonitorLog->init(opCodeList);
	m_opcodeMonitorLog->setLog(pSEQPrefs->getPrefBool("Log", section, false));
	m_opcodeMonitorLog->setView(pSEQPrefs->getPrefBool("View", section, false));

	connect(m_packet, SIGNAL(decodedZonePacket(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)),
			m_opcodeMonitorLog, SLOT(packet(const uint8_t*, size_t, uint8_t, uint16_t, const EQPacketOPCode*, bool)));
}

void EQInterface::insertWindowMenu(SEQWindow* window)
{
	QMenu* menu = window->menu();
	if (menu)
	{
		// insert the windows menu into the window menu
		int id = m_windowMenu->insertItem(window->caption(), menu);

		// insert it into the window to window menu id dictionary
		m_windowsMenus.insert((void*)window, new int(id));
	}
}

void EQInterface::removeWindowMenu(SEQWindow* window)
{
	// find the windows menu id
	int* id = m_windowsMenus.find((void*)window);

	// if the window had a menu, then remove it
	if (id)
	{
		m_windowMenu->removeItem(*id);

		// remove the item from the list
		m_windowsMenus.remove(window);
	}
}

void EQInterface::setDockEnabled(SEQWindow* dw, bool enable)
{
	dw->setDockable(enable);
}

void EQInterface::setupExperienceWindow()
{
	// Initialize the experience window;
	m_expWindow = new ExperienceWindow(m_dataLocationMgr, m_player, m_groupMgr, m_zoneMgr, this, "ExperienceWindow");

	setDockEnabled(m_expWindow, pSEQPrefs->getPrefBool("DockableExperienceWindow", "Interface", false));
	//Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_expWindow->preferenceName(), Qt::NoDockWidgetArea);
	Qt::DockWidgetArea edge = Qt::BottomDockWidgetArea;
	addDockWidget(edge, m_expWindow);

	m_expWindow->undock();
	m_expWindow->restoreSize();

	// move window to new position
	if (pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true))
		m_expWindow->restorePosition();

	//if (pSEQPrefs->getPrefBool("ShowExpWindow", "Interface", false))
	//		m_expWindow->show();

	// insert its menu into the window menu
	insertWindowMenu(m_expWindow);
}

void EQInterface::setupCombatWindow()
{
	QString section = "Interface";

	// Initialize the combat window
	m_combatWindow = new CombatWindow(m_player, this, "CombatWindow");

	m_combatWindow->setDockable(pSEQPrefs->getPrefBool("DockableCombatWindow", section, false));
	//Qt::DockWidgetArea edge = (Qt::DockWidgetArea)pSEQPrefs->getPrefInt("Dock", m_combatWindow->preferenceName(), Qt::NoDockWidgetArea);
	Qt::DockWidgetArea edge = Qt::TopDockWidgetArea;
	addDockWidget(edge, m_combatWindow);

	m_combatWindow->undock();
	m_combatWindow->restoreSize();

	// move window to new position
	if (pSEQPrefs->getPrefBool("UseWindowPos", "Interface", true))
		m_combatWindow->restorePosition();

	//if (pSEQPrefs->getPrefBool("ShowCombatWindow", section, false))
	//	m_combatWindow->show();

    // insert its menu into the window menu
	insertWindowMenu(m_combatWindow);
}

#ifndef QMAKEBUILD
#include "interface.moc"
#endif
