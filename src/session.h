/*
 *  session.h
 *  showeq
 *
 *  Created by Stephen Raub on 4/22/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __SESSION_H__
#define __SESSION_H__

#include "datalocationmgr.h"
#include "datetimemgr.h"
#include "eqstr.h"
#include "spells.h"
#include "xmlpreferences.h"
#include "guild.h"
#include "category.h"
#include "terminal.h"

#include <QString>

class Session 
{
	// GuildMgr
	
	
public:

};



class SessionManager : public QObject
{
	Q_OBJECT
	
private:
	DataLocationMgr*	m_dataLocationMgr;
	DateTimeMgr*		m_dateTimeMgr;
	XMLPreferences*		m_preferences;
	CategoryMgr*		m_categories;
	Terminal*			m_terminal;
	
	// Resources
	Spells*				m_spells;
	EQStr*				m_eqStrings;
	
	QString getUserDirectory();
	
public:
	SessionManager(QString configFile);
	virtual ~SessionManager();
	
	Session* newSession(class PacketSource*);

	XMLPreferences* preferences();
	DataLocationMgr* dataLocationMgr();
	DateTimeMgr* dateTimeMgr();
	EQStr* eqStrings();
	Spells* spells() { return m_spells; }
	CategoryMgr* categoryMgr() { return m_categories; }
	
	// Loads resources from file
	void loadFormatStrings();
	void loadSpells();
	
public slots:
	void savePrefs();
};

#endif // __SESSION_H__