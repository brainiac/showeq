/*
 *  session.h
 *  showeq
 *
 *  Created by Stephen Raub on 4/22/09.
 *  Copyright 2009 Vexislogic. All rights reserved.
 *
 */

#ifndef __SESSION_H__
#define __SESSION_H__


#include <QObject>
#include <QString>

class SessionManager;
class XMLPreferences;

class ZoneMgr;
class MapMgr;
class GroupMgr;
class GuildMgr;
class GuildShell;
class Player;
class SpawnMonitor;
class SpawnShell;
class SpellShell;
class FilterMgr;
class Messages;
class MessageFilters;
class MessageShell;
class FilterNotifications;
class Terminal;
class DataSource;

class Session : public QObject
{
	Q_OBJECT
	
	// Session constructs
private:
	ZoneMgr*			m_zoneMgr;
	MapMgr*				m_mapMgr;
	
	GroupMgr*			m_groupMgr;
	GuildMgr*			m_guildMgr;
	GuildShell*			m_guildShell;
	
	Player*				m_player;
	SpawnMonitor*		m_spawnMonitor;
	SpawnShell*			m_spawnShell;
	SpellShell*			m_spellShell;
	
	FilterMgr*			m_filterMgr;
	Messages*			m_messages;
	MessageFilters*		m_messageFilters;
	MessageShell*		m_messageShell;
	Terminal*			m_terminal;
	FilterNotifications*	m_filterNotifications;
	
	DataSource*		m_source;
	SessionManager*		m_parent;
	
	XMLPreferences*		m_preferences;
	
public:
	Session(SessionManager*);
	Session(SessionManager*, DataSource*);
	
	virtual ~Session();
	
	void assignSource(DataSource*);

public:
	// Accessors
	ZoneMgr*			zoneMgr();
	MapMgr*				mapMgr();
	GroupMgr*			groupMgr();
	GuildMgr*			guildMgr(); 
	GuildShell*			guildShell();
	Player*				player();
	SpawnMonitor*		spawnMonitor();
	SpawnShell*			spawnShell();
	SpellShell*			spellShell();
	FilterMgr*			filterMgr();
	Messages*			messages();
	MessageFilters*		messageFilters();
	MessageShell*		messageShell();
};


class DataLocationMgr;
class DateTimeMgr;
class XMLPreferences;
class CategoryMgr;
class Terminal;
class Spells;
class EQStr;

class SessionManager : public QObject
{
	Q_OBJECT
	
private:
	DataLocationMgr*	m_dataLocationMgr;
	DateTimeMgr*		m_dateTimeMgr;
	XMLPreferences*		m_preferences;
	CategoryMgr*		m_categories;
	Terminal*			m_terminal;
	
	// Session Resources
	Spells*				m_spells;
	EQStr*				m_eqStrings;
	
	QString getUserDirectory();
	
	// Something to store sessions in
	
public:
	SessionManager(QString configFile);
	virtual ~SessionManager();
	
	/* I'm thinking right now, in order to create a new session, we'll require a
	 * data source which provides the flow of information necessary to create
	 * the session. */
	Session* newSession(DataSource*);

	XMLPreferences* preferences();
	DataLocationMgr* dataLocationMgr();
	DateTimeMgr* dateTimeMgr();
	EQStr* eqStrings();
	Spells* spells() { return m_spells; }
	CategoryMgr* categoryMgr() { return m_categories; }
	
	// Loads resources from file
	void loadFormatStrings();
	void loadSpells();
	
public slots:		/* Functions for Category Manager */
	void addCategory();
	void reloadCategories();
	
		
public slots:
	void savePrefs();
};

#endif // __SESSION_H__