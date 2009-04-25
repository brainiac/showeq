/*
 *  session.cpp
 *  showeq
 *
 *  Created by Stephen Raub on 4/22/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "session.h"

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
	m_preferences = new XMLPreferences(defaultConfigFilePath, userConfigFilePath);
	
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
}

SessionManager::~SessionManager()
{
	delete m_eqStrings;
	delete m_spells;
	delete m_categories;
	
	delete m_dateTimeMgr;
	delete m_preferences;
	delete m_dataLocationMgr;
}

Session* SessionManager::newSession(class PacketSource*)
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
