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

class EQPacket;
class EQPacketTypeDB;
class EQPacketOPCodeDB;

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
	
	SessionManager*		m_parent;
	QTimer*				m_timer;
	int					m_delay;

	XMLPreferences*		m_preferences;

private:

	DataSource*			m_dataSource;

	typedef QObject* Session::* ObjectMember;
	struct DataSourceRegistrationInfo
	{
		const char* opcodeName;
		uint8_t sp;
		uint8_t dir;
		const char* payload;
		uint8_t szt;
		ObjectMember receiver;	// TODO: This will change
		const char* member;
	};
	typedef void (*DataSourceRegistrationHandler)(DataSourceRegistrationInfo*, void* param);

	static void EnumerateDataSourceRegistration(DataSourceRegistrationHandler handler, void* param);
	static void ConnectNetworkSignalsCallback(DataSourceRegistrationInfo* dsri, void* param);

	void attachSignals();

public slots:
	//void begin();
	void updateSource();

public:
	Session(SessionManager*, DataSource* source = NULL);
	
	virtual ~Session();
	
	void assignSource(DataSource*);
	void initializeDataSource();

public:
	// Accessors
	ZoneMgr*			zoneMgr() { return m_zoneMgr; }
	MapMgr*				mapMgr() { return m_mapMgr; }
	GroupMgr*			groupMgr() { return m_groupMgr; }
	GuildMgr*			guildMgr() { return m_guildMgr; }
	GuildShell*			guildShell() { return m_guildShell; }
	Player*				player() { return m_player; }
	SpawnMonitor*		spawnMonitor() { return m_spawnMonitor; }
	SpawnShell*			spawnShell() { return m_spawnShell; }
	SpellShell*			spellShell() { return m_spellShell; }
	FilterMgr*			filterMgr() { return m_filterMgr; }
	Messages*			messages() { return m_messages; }
	MessageFilters*		messageFilters() { return m_messageFilters; }
	MessageShell*		messageShell() { return m_messageShell; }
	FilterNotifications* filterNotifications() { return m_filterNotifications; }
	Terminal*			terminal() { return m_terminal; }

signals:
	void DestroySession();
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

	// Common packet data
	EQPacketTypeDB*		m_packetTypeDB;
	EQPacketOPCodeDB*	m_worldOPCodeDB;
	EQPacketOPCodeDB*	m_zoneOPCodeDB;
	
public:
	SessionManager(QString configFile);
	virtual ~SessionManager();
	
	/* Calling newSession should trigger some sort of interface through 
	 * an interface controller type class, that enumerates interfaces for
	 * DataSourceFactory objects. */
	Session* newSession();

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
	//void addCategory(const Category* category);
	//void reloadCategories();

public slots:
	void savePrefs();
};

#endif // __SESSION_H__