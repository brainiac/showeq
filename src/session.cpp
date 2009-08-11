/*
 *  session.cpp
 *  showeq
 *
 *  Created by Stephen Raub on 4/22/09.
 *  Copyright 2009 Vexislogic. All rights reserved.
 *
 */

#include "session.h"

#include "datalocationmgr.h"
#include "datetimemgr.h"
#include "eqstr.h"
#include "spells.h"
#include "xmlpreferences.h"
#include "guild.h"
#include "category.h"
#include "terminal.h"
#include "player.h"
#include "zonemgr.h"
#include "filtermgr.h"
#include "guildshell.h"
#include "spawnshell.h"
#include "spellshell.h"
#include "map.h"
#include "spawnmonitor.h"
#include "group.h"
#include "messageshell.h"
#include "filternotifications.h"
#include "datasource.h"

#include <QFileInfo>

extern XMLPreferences* pSEQPrefs;

// Session Class

Session::Session(SessionManager* parent)
{
	m_zoneMgr = NULL;
	m_mapMgr = NULL;
	m_groupMgr = NULL;
	m_guildMgr = NULL;
	m_guildShell = NULL;
	m_player = NULL;
	m_spawnMonitor = NULL;
	m_spawnShell = NULL;
	m_spellShell = NULL;
	m_filterMgr = NULL;
	m_messages = NULL;
	m_messageFilters = NULL;
	m_messageShell = NULL;
	
	m_parent = parent;
	m_source = NULL;
	
	m_preferences = parent->preferences();
	
	/***********************************************************************
	 * Create Message Objects
	 **********************************************************************/
	
	// Create message filters object
	m_messageFilters = new MessageFilters(this, "messagefilters");
	
	// Create messages storage
	m_messages = new Messages(m_parent->dateTimeMgr(), m_messageFilters, this, "messages");
	
	// Create the terminal object
	m_terminal = new Terminal(m_messages, this, "terminal");
	
	
	/***********************************************************************
	 * Create ShowEQ Objects
	 **********************************************************************/
	
	// Create the Zone Manager
	m_zoneMgr = new ZoneMgr(this, "zonemgr");
	
	// Create the Guild Manager
	QString fileName = m_preferences->getPrefString("GuildsFile", "Interface", "guilds2.dat");
	QFileInfo fileInfo = m_parent->dataLocationMgr()->findWriteFile("tmp", fileName);
	m_guildMgr = new GuildMgr(fileInfo.absFilePath(), this, "guildmgr");
	
	// Create the Player Object
	m_player = new Player(this, m_zoneMgr, m_guildMgr);
	
	// if there is a short zone name already, try to load its filters
	// (there isn't one, this should be removed)
	QString shortZoneName = m_zoneMgr->shortZoneName();
	if (!shortZoneName.isEmpty() && shortZoneName != "unknown")
		m_filterMgr->loadZone(shortZoneName);
	
	
	// Create the Guild Shell
	m_guildShell = new GuildShell(m_zoneMgr, this, "GuildShell");
	
	// Create the Spawn Shell
	m_spawnShell = new SpawnShell(*m_filterMgr, m_zoneMgr, m_player, m_guildMgr);
	
	// Create the Map Manager
	m_mapMgr = new MapMgr(m_parent->dataLocationMgr(), m_spawnShell, m_player, m_zoneMgr);
	
	// Create the Spell Shell
	m_spellShell = new SpellShell(m_player, m_spawnShell, m_parent->spells());
	
	// Create the Spawn Monitor
	m_spawnMonitor = new SpawnMonitor(m_parent->dataLocationMgr(), m_zoneMgr, m_spawnShell);
	
	// Create the Group Manager
	m_groupMgr = new GroupMgr(m_spawnShell, m_player, this, "groupmgr");
	
	// Create the message shell
	m_messageShell = new MessageShell(m_messages, m_parent->eqStrings(), m_parent->spells(),
			m_zoneMgr, m_spawnShell, m_player, this, "messageshell");
	
	// Create the Filter Notification object
	m_filterNotifications = new FilterNotifications(this, "filternotifications");
	
	
}

Session::Session(SessionManager* parent, DataSource* source)
{
	m_zoneMgr = NULL;
	m_mapMgr = NULL;
	m_groupMgr = NULL;
	m_guildMgr = NULL;
	m_guildShell = NULL;
	m_player = NULL;
	m_spawnMonitor = NULL;
	m_spawnShell = NULL;
	m_spellShell = NULL;
	m_filterMgr = NULL;
	m_messages = NULL;
	m_messageFilters = NULL;
	m_messageShell = NULL;
	
	m_parent = parent;
	
	// TODO: Construct Objects
	
	assignSource(source);
}

Session::~Session()
{
	if (m_zoneMgr != NULL)
		delete m_zoneMgr;
	if (m_mapMgr != NULL)
		delete m_mapMgr;
	if (m_groupMgr != NULL)
		delete m_groupMgr;
	if (m_guildMgr != NULL)
		delete m_guildMgr;
	if (m_guildMgr != NULL)
		delete m_guildMgr;
	if (m_player != NULL)
		delete m_player;
	if (m_spawnMonitor != NULL)
		delete m_spawnMonitor;
	if (m_spawnShell != NULL)
		delete m_spawnShell;
	if (m_spellShell != NULL)
		delete m_spellShell;
	if (m_filterMgr != NULL)
		delete m_filterMgr;
	if (m_messages != NULL)
		delete m_messages;
	if (m_messageFilters != NULL)
		delete m_messageFilters;
	if (m_messageShell != NULL)
		delete m_messageShell;
	
	m_parent = NULL;
	
	if (m_source != NULL)
		delete m_source;
}


void Session::assignSource(DataSource* source)
{
	m_source = source;
}


// Session Manager Class

SessionManager::SessionManager(QString configFile)
	: QObject(NULL, "SessionManager")
{
	// Begin by creating the data location manager
	QString userDir = getUserDirectory();
	m_dataLocationMgr = new DataLocationMgr(userDir);
	m_dataLocationMgr->setupUserDirectory();
	
	// Set up preferences
	QFileInfo defaultConfigFile = m_dataLocationMgr->findExistingFile(".", "seqdef.xml", true, false);
	
	/* [brainiac] I'm not sure why this has to exist. I think it should be
	 * permissible to load without a default config file. Defaults should be
	 * hardcoded somewhere IMO. */
	if (!defaultConfigFile.exists()) {
		fprintf(stderr, "Fatal: Couldn't find seqdef.xml!\n"
				"\tDid you remember to do 'make install'?\n");
		exit(-1);
	}
	QString defaultConfigFilePath = defaultConfigFile.absFilePath();
	
	// Get user config file, or use the specified file
	QString userConfigFilePath;
	if (configFile.isEmpty()) {
		QFileInfo userConfigFile = m_dataLocationMgr->findExistingFile(".", "showeq.xml", true, true);

		/* deal with funky border case since we may be running setuid */
		if (userConfigFile.dir() != QDir::root())
			userConfigFilePath = userConfigFile.absFilePath();
		else
			userConfigFilePath = QFileInfo(m_dataLocationMgr->userDataDir(".").absPath(), "showeq.xml").absFilePath();
	} else {
		/* TODO: Validate the path provided by the user */
		userConfigFilePath = configFile;
	}	
	printf("Using config file '%s'\n", (const char*)configFile);
	
	// Create the preferences object
	pSEQPrefs = m_preferences = new XMLPreferences(defaultConfigFilePath, userConfigFilePath);
	
	/* [brainiac] Ok now that preferences are created, we can set up the rest of
	 * the system. */
	
	// Create the date/time manager object
	m_dateTimeMgr = new DateTimeMgr();
	
	// Create and load the eqstr_us.txt file
	m_eqStrings = new EQStr();
	loadFormatStrings();
	
	// Create and load the spells_us.txt file
	m_spells = new Spells();
	loadSpells();
	
	// Load categories
	m_categories = new CategoryMgr();
	
	// Connect some signals
	connect(this, SLOT(savePrefs()), m_categories, SLOT(savePrefs()));
}

SessionManager::~SessionManager()
{
	if (m_eqStrings != NULL)
		delete m_eqStrings;
	if (m_spells != NULL)
		delete m_spells;
	if (m_categories != NULL)
		delete m_categories;
	
	if (m_dateTimeMgr != NULL)
		delete m_dateTimeMgr;
	if (m_preferences != NULL)
		delete m_preferences;
	if (m_dataLocationMgr)
		delete m_dataLocationMgr;
}

Session* SessionManager::newSession(DataSource*)
{
}

QString SessionManager::getUserDirectory()
{
	return ".showeq";
}

// Loads the format strings stored in eqstr_us.txt
void SessionManager::loadFormatStrings()
{
	QString formatFileName = m_preferences->getPrefString("FormatFile", "Resources", "eqstr_us.txt");
	QFileInfo fileInfo = m_dataLocationMgr->findExistingFile(".", formatFileName);
	
	// load the strings
	m_eqStrings->load(fileInfo.absFilePath());
}

void SessionManager::loadSpells()
{
	QString fileName = m_preferences->getPrefString("SpellsFile", "Resources", "spells_us.txt");
	QFileInfo fileInfo = m_dataLocationMgr->findExistingFile(".", fileName);
	
	// load the spells
	m_spells->loadSpells(fileInfo.absFilePath());
}

// "Getters"
XMLPreferences* SessionManager::preferences()
{
	return m_preferences;
}

DataLocationMgr* SessionManager::dataLocationMgr()
{
	return m_dataLocationMgr;
}

DateTimeMgr* SessionManager::dateTimeMgr()
{
	return m_dateTimeMgr;
}

EQStr* SessionManager::eqStrings()
{
	return m_eqStrings;
}

void SessionManager::savePrefs()
{
}


// Category Functions
void SessionManager::addCategory()
{
	if (m_categories)
		m_categories->addCategory();
}

void SessionManager::reloadCategories()
{
	if (m_categories)
		m_categories->reloadCategories();
}
