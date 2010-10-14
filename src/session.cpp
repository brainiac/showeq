/*
 *  session.cpp
 *  showeq
 *
 *  Created by Stephen Raub on 4/22/09.
 *  Copyright 2009 Vexislogic. All rights reserved.
 *
 */

#include "pch.h"

#include "diagnosticmessages.h"
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

#include "remotepacket.h"

extern XMLPreferences* pSEQPrefs;

// Session Class

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
	m_preferences = parent->preferences();

	m_timer = new QTimer(this);
	m_delay = 1000;
	m_dataSource = 0;
	
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
	
	// Create the filter manager
	QString filterFile = pSEQPrefs->getPrefString("FilterFile", "Interface", "global.xml");
	bool isCaseSensitive = pSEQPrefs->getPrefBool("IsCaseSensitive", "Interface", false);
	m_filterMgr = new FilterMgr(m_parent->dataLocationMgr(), filterFile, isCaseSensitive);

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

	attachSignals();

	assignSource(source);
}
	
void Session::attachSignals()
{
	// zoneManager -> group Manager
	connect(m_zoneMgr, SIGNAL(playerProfile(const charProfileStruct*)),
			m_groupMgr, SLOT(player(const charProfileStruct*)));
	
	// spawnShell -> group Manager
	connect(m_spawnShell, SIGNAL(addItem(const Item*)),
			m_groupMgr, SLOT(addItem(const Item*)));
	connect(m_spawnShell, SIGNAL(delItem(const Item*)),
			m_groupMgr, SLOT(delItem(const Item*)));
	connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
			m_groupMgr, SLOT(killSpawn(const Item*)));
	
	// zone Manager -> filter Manager
	connect(m_zoneMgr, SIGNAL(zoneBegin(const QString&)),
			m_filterMgr, SLOT(loadZone(const QString&)));
	connect(m_zoneMgr, SIGNAL(zoneEnd(const QString&)),
			m_filterMgr, SLOT(loadZone(const QString&)));
	connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)),
			m_filterMgr, SLOT(loadZone(const QString&)));
	
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
	connect(m_zoneMgr, SIGNAL(playerProfile(const charProfileStruct*)),
			m_messageShell, SLOT(player(const charProfileStruct*)));
	
	connect(m_player, SIGNAL(setExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)),
			m_messageShell, SLOT(setExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)));
	connect(m_player, SIGNAL(newExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)),
			m_messageShell, SLOT(newExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)));
	connect(m_player, SIGNAL(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)),
			m_messageShell, SLOT(setAltExp(uint32_t, uint32_t, uint32_t, uint32_t)));
	connect(m_player, SIGNAL(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)),
			m_messageShell, SLOT(newAltExp(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)));

	connect(m_spawnShell, SIGNAL(addItem(const Item*)),
			m_messageShell, SLOT(addItem(const Item*)));
	connect(m_spawnShell, SIGNAL(delItem(const Item*)),
			m_messageShell, SLOT(delItem(const Item*)));
	connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
			m_messageShell, SLOT(killSpawn(const Item*)));
	
	// connect the SpellShell slots to ZoneMgr signals
	connect(m_zoneMgr, SIGNAL(zoneChanged(const QString&)),
			m_spellShell, SLOT(zoneChanged()));
	
	// connect the SpellShell slots to SpawnShell signals
	connect(m_spawnShell, SIGNAL(killSpawn(const Item*, const Item*, uint16_t)),
			m_spellShell, SLOT(killSpawn(const Item*)));
	
	// connect the SpellShell slots to Player signals
	connect(m_player, SIGNAL(newPlayer()),
			m_spellShell, SLOT(clear()));
	connect(m_player, SIGNAL(buffLoad(const spellBuff *)),
			m_spellShell, SLOT(buffLoad(const spellBuff *)));
	
	connect(m_zoneMgr, SIGNAL(playerProfile(const charProfileStruct*)), m_player, SLOT(player(const charProfileStruct*)));
}

void Session::assignSource(DataSource* source)
{
	// connects signals from the source to the objects that consume packets
	m_dataSource = source;
	
	if (m_dataSource != NULL)
	{	
		EnumerateDataSourceRegistration(&Session::ConnectNetworkSignalsCallback, this);
		
		connect(m_timer, SIGNAL(timeout()), this, SLOT(updateSource()));
		m_timer->start(m_delay);

		m_dataSource->start();
	}
}

void Session::updateSource()
{
	if (m_dataSource)
		m_dataSource->update();
}

void Session::ConnectNetworkSignalsCallback(DataSourceRegistrationInfo* dsri, void* param)
{
	Session* pThis = (Session*)param;

	if (pThis->*(dsri->receiver))
	{
		pThis->m_dataSource->connectReceiver(dsri->opcodeName, dsri->payload,
			(EQStreamPairs)dsri->sp, dsri->dir, 
			(EQSizeCheckType)dsri->szt,	pThis->*(dsri->receiver), dsri->member);
	}
}

void Session::initializeDataSource()
{
	/***********************************************************************
	 * Create Packet Provider Object
	 **********************************************************************/

	// make packet object
#if 0
	m_packet = new EQPacket(worldOpcodes.absFilePath(),
		zoneOpcodes.absFilePath(),
		pSEQPrefs->getPrefInt("ArqSeqGiveUp", "Network", 512),
		pSEQPrefs->getPrefString("Device", "Network", "eth0"),
		pSEQPrefs->getPrefString("IP", "Network", AUTOMATIC_CLIENT_IP),
		pSEQPrefs->getPrefString("MAC", "Network", "0"),
		pSEQPrefs->getPrefBool("RealTimeThread", "Network", false),
		pSEQPrefs->getPrefBool("SessionTracking", "Network", false),
		pSEQPrefs->getPrefBool("Record", "VPacket", false),
		pSEQPrefs->getPrefInt("Playback", "VPacket", PLAYBACK_OFF),
		pSEQPrefs->getPrefInt("PlaybackRate", "VPacket", false),
		this, "packet");
	// initialize packet count
	m_initialcount = 0;
	m_packetStartTime = 0;

	// Retrieve last IP usd in previous session
	m_ipstr[0] = m_packet->ip();
	for (int i = 1; i < 5; i++)
		m_ipstr[i] = "0.0.0.0";

	// Retrieve last MAC used in previous session
	m_macstr[0] = m_packet->mac();
	for (int i = 1; i < 5; i++)
		m_macstr[i] = "00:00:00:00:00:00";

	// Create the network menu
	//createNetworkMenu();
#endif

	// connect network signals
	EnumerateDataSourceRegistration(&Session::ConnectNetworkSignalsCallback, this);

	// TODO: dateTimeMgr connections
	if (m_parent->dateTimeMgr())
	{
		// connect DateTimeMgr slots to EQPacket signals
		m_dataSource->connectReceiver("OP_TimeOfDay", "timeOfDayStruct", SP_Zone, DIR_Server, SZC_Match,
			m_parent->dateTimeMgr(), SLOT(timeOfDay(const uint8_t*)));
	}

	// connect interface slots to Packet signals
	m_dataSource->connectReceiver("OP_TargetMouse", "clientTargetStruct", SP_Zone, DIR_Client | DIR_Server, SZC_Match,
		this, SLOT(clientTarget(const uint8_t*)));
	m_dataSource->connectReceiver("OP_Action2", "action2Struct", SP_Zone, DIR_Client|DIR_Server, SZC_Match,
		this, SLOT(action2Message(const uint8_t*)));
	m_dataSource->connectReceiver("OP_Death", "newCorpseStruct", SP_Zone, DIR_Server, SZC_Match,
		this, SLOT(combatKillSpawn(const uint8_t*)));

	// TODO: Attach these signals (these aren't packet signals, but rather signals related to the packet manager)
	//connect(m_packet, SIGNAL(toggle_session_tracking(bool)), this, SLOT(toggle_net_session_tracking(bool)));
	//connect(m_packet, SIGNAL(stsMessage(const QString &, int)), this, SLOT(stsMessage(const QString &, int)));
	//connect(m_packet, SIGNAL(numPacket(int, int)), this, SLOT(numPacket(int, int)));
	//connect(m_packet, SIGNAL(resetPacket(int, int)), this, SLOT(resetPacket(int, int)));

	/* Start the packet capturing */
	//m_packet->start(10);
}


void Session::EnumerateDataSourceRegistration(DataSourceRegistrationHandler handler, void* param)
{
	// TODO: Set this up so that services register for messages they want to receive, not the 
	// other way around, like it is right now?
	DataSourceRegistrationInfo registrations[] =
	{
		// ZoneManager Opcode Handlers
		{"OP_ZoneEntry",		SP_Zone, DIR_Client, "ClientZoneEntryStruct", SZC_Match,
		(ObjectMember)&Session::m_zoneMgr, SLOT(zoneEntryClient(const uint8_t*, size_t, uint8_t))},
		{"OP_PlayerProfile",	SP_Zone, DIR_Server, "charProfileStruct", SZC_Match,
		(ObjectMember)&Session::m_zoneMgr, SLOT(zonePlayer(const uint8_t*))},
		{"OP_ZoneChange",		SP_Zone, DIR_Client | DIR_Server, "zoneChangeStruct", SZC_Match,
		(ObjectMember)&Session::m_zoneMgr, SLOT(zoneChange(const uint8_t*, size_t, uint8_t))},
		{"OP_NewZone",			SP_Zone, DIR_Server, "newZoneStruct", SZC_Match,
		(ObjectMember)&Session::m_zoneMgr, SLOT(zoneNew(const uint8_t*, size_t, uint8_t))},
		{"OP_SendZonePoints",	SP_Zone, DIR_Server, "zonePointsStruct", SZC_None,
		(ObjectMember)&Session::m_zoneMgr, SLOT(zonePoints(const uint8_t*, size_t, uint8_t))},
		{"OP_DzSwitchInfo",		SP_Zone, DIR_Server, "dzSwitchInfo", SZC_None,
		(ObjectMember)&Session::m_zoneMgr, SLOT(dynamicZonePoints(const uint8_t*, size_t, uint8_t))},
		{"OP_DzInfo",			SP_Zone, DIR_Server, "dzInfo", SZC_Match, 
		(ObjectMember)&Session::m_zoneMgr, SLOT(dynamicZoneInfo(const uint8_t*, size_t, uint8_t))},

		// GroupManager Opcode Handlers
		{"OP_GroupUpdate",		SP_Zone, DIR_Server, "uint8_t", SZC_None,
		(ObjectMember)&Session::m_groupMgr, SLOT(groupUpdate(const uint8_t*, size_t))},
		{"OP_GroupFollow",		SP_Zone, DIR_Server, "groupFollowStruct", SZC_Match,
		(ObjectMember)&Session::m_groupMgr, SLOT(addGroupMember(const uint8_t*))},
		{"OP_GroupDisband",		SP_Zone, DIR_Server, "groupDisbandStruct", SZC_Match,
		(ObjectMember)&Session::m_groupMgr, SLOT(removeGroupMember(const uint8_t*))},
		{"OP_GroupDisband2",	SP_Zone, DIR_Server, "groupDisbandStruct", SZC_Match,
		(ObjectMember)&Session::m_groupMgr, SLOT(removeGroupMember(const uint8_t*))},

		// GuildShell Opcode Handlers
		{"OP_GuildMemberList",	SP_Zone, DIR_Server, "uint8_t", SZC_None,
		(ObjectMember)&Session::m_guildShell, SLOT(guildMemberList(const uint8_t*, size_t))},
		{"OP_GuildMemberUpdate",	SP_Zone, DIR_Server, "GuildMemberUpdate", SZC_Match,
		(ObjectMember)&Session::m_guildShell, SLOT(guildMemberUpdate(const uint8_t*, size_t))},

		// MessageShell Opcode Handlers
		{"OP_CommonMessage",	SP_Zone, DIR_Client | DIR_Server, "channelMessageStruct", SZC_None,
		(ObjectMember)&Session::m_messageShell, SLOT(channelMessage(const uint8_t*, size_t, uint8_t))},
		{"OP_FormattedMessage",	SP_Zone, DIR_Server, "formattedMessageStruct", SZC_None,
		(ObjectMember)&Session::m_messageShell, SLOT(formattedMessage(const uint8_t*, size_t, uint8_t))},
		{"OP_SimpleMessage",	SP_Zone, DIR_Server, "simpleMessageStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(simpleMessage(const uint8_t*, size_t, uint8_t))},
		{"OP_SpecialMesg",		SP_Zone, DIR_Server, "specialMessageStruct", SZC_None,
		(ObjectMember)&Session::m_messageShell, SLOT(specialMessage(const uint8_t*, size_t, uint8_t))},
		{"OP_GuildMOTD",		SP_Zone, DIR_Server, "guildMOTDStruct", SZC_None,
		(ObjectMember)&Session::m_messageShell, SLOT(guildMOTD(const uint8_t*, size_t, uint8_t))},
		{"OP_RandomReq",		SP_Zone, DIR_Client, "randomReqStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(randomRequest(const uint8_t*))},
		{"OP_RandomReply",		SP_Zone, DIR_Server, "randomStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(random(const uint8_t*))},
		{"OP_ConsentResponse",	SP_Zone, DIR_Server, "consentResponseStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(consent(const uint8_t*, size_t, uint8_t))},
		{"OP_DenyResponse",		SP_Zone, DIR_Server, "consentResponseStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(consent(const uint8_t*, size_t, uint8_t))},
		{"OP_Emote",			SP_Zone, DIR_Server | DIR_Client, "emoteTextStruct", SZC_None,
		(ObjectMember)&Session::m_messageShell, SLOT(emoteText(const uint8_t*))},
		{"OP_InspectAnswer",	SP_Zone, DIR_Server, "inspectDataStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(inspectData(const uint8_t*))},
		{"OP_MoneyOnCorpse",	SP_Zone, DIR_Server, "moneyOnCorpseStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(moneyOnCorpse(const uint8_t*))},
		{"OP_Logout",			SP_Zone, DIR_Server, "none", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(logOut(const uint8_t*, size_t, uint8_t))},
		{"OP_NewZone",			SP_Zone, DIR_Server, "newZoneStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(zoneNew(const uint8_t*, size_t, uint8_t))},
		{"OP_MOTD",				SP_World, DIR_Server, "worldMOTDStruct", SZC_None,
		(ObjectMember)&Session::m_messageShell, SLOT(worldMOTD(const uint8_t*))},
		{"OP_MemorizeSpell",	SP_Zone, DIR_Server|DIR_Client, "memSpellStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(handleSpell(const uint8_t*, size_t, uint8_t))},
		{"OP_BeginCast",		SP_Zone, DIR_Server|DIR_Client, "beginCastStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(beginCast(const uint8_t*))},
		{"OP_BuffFadeMsg",		SP_Zone, DIR_Server|DIR_Client, "spellFadedStruct", SZC_None,
		(ObjectMember)&Session::m_messageShell, SLOT(spellFaded(const uint8_t*))},	
		{"OP_CastSpell",		SP_Zone, DIR_Server|DIR_Client, "startCastStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(startCast(const uint8_t*))},	
		{"OP_SkillUpdate",		SP_Zone, DIR_Server, "skillIncStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(increaseSkill(const uint8_t*))},		
		{"OP_LevelUpdate",		SP_Zone, DIR_Server, "levelUpUpdateStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(updateLevel(const uint8_t*))},		
		{"OP_Consider",			SP_Zone, DIR_Server, "considerStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(consMessage(const uint8_t*, size_t, uint8_t))},
		{"OP_GroupInvite",		SP_Zone, DIR_Client | DIR_Server, "groupInviteStruct", SZC_None,
		(ObjectMember)&Session::m_messageShell, SLOT(groupInvite(const uint8_t*, size_t, uint8_t))},		
		{"OP_GroupInvite2",		SP_Zone, DIR_Client, "groupInviteStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(groupInvite(const uint8_t*, size_t, uint8_t))},		
		{"OP_GroupFollow",		SP_Zone, DIR_Server, "groupFollowStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(groupFollow(const uint8_t*))},		
		{"OP_GroupFollow2",		SP_Zone, DIR_Server, "groupFollowStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(groupFollow(const uint8_t*))},		
		{"OP_GroupDisband",		SP_Zone, DIR_Server, "groupDisbandStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(groupDisband(const uint8_t*))},		
		{"OP_GroupDisband2",	SP_Zone, DIR_Server, "groupDisbandStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(groupDisband(const uint8_t*))},	
		{"OP_GroupCancelInvite",	SP_Zone, DIR_Server|DIR_Client, "groupDeclineStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(groupDecline(const uint8_t*))},		
		{"OP_GroupLeader",		SP_Zone, DIR_Server, "groupLeaderChangeStruct", SZC_Match,
		(ObjectMember)&Session::m_messageShell, SLOT(groupLeaderChange(const uint8_t*))},

		// SpawnShell Opcode Handlers
		{"OP_GroundSpawn",		SP_Zone, DIR_Server, "makeDropStruct", SZC_None,
		(ObjectMember)&Session::m_spawnShell, SLOT(newGroundItem(const uint8_t*, size_t, uint8_t))},
		{"OP_ClickObject",		SP_Zone, DIR_Server, "remDropStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(removeGroundItem(const uint8_t*, size_t, uint8_t))},
		{"OP_SpawnDoor",		SP_Zone, DIR_Server, "doorStruct", SZC_Modulus,
		(ObjectMember)&Session::m_spawnShell, SLOT(newDoorSpawns(const uint8_t*, size_t, uint8_t))},
		{"OP_ZoneEntry",		SP_Zone, DIR_Server, "uint8_t", SZC_None,
		(ObjectMember)&Session::m_spawnShell, SLOT(zoneEntry(const uint8_t*, size_t))},
		{"OP_MobUpdate",		SP_Zone, DIR_Server|DIR_Client, "spawnPositionUpdate", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(updateSpawns(const uint8_t*))},
		{"OP_WearChange",		SP_Zone, DIR_Server|DIR_Client, "SpawnUpdateStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(updateSpawnInfo(const uint8_t*))},
		{"OP_HPUpdate",			SP_Zone, DIR_Server|DIR_Client, "hpNpcUpdateStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(updateNpcHP(const uint8_t*))},
		{"OP_DeleteSpawn",		SP_Zone, DIR_Server|DIR_Client, "deleteSpawnStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(deleteSpawn(const uint8_t*))},
		{"OP_SpawnRename",		SP_Zone, DIR_Server, "spawnRenameStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(renameSpawn(const uint8_t*))},
		{"OP_Illusion",			SP_Zone, DIR_Server|DIR_Client, "spawnIllusionStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(illusionSpawn(const uint8_t*))},
		{"OP_SpawnAppearance",	SP_Zone, DIR_Server|DIR_Client, "spawnAppearanceStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(updateSpawnAppearance(const uint8_t*))},
		{"OP_Death",			SP_Zone, DIR_Server, "newCorpseStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(killSpawn(const uint8_t*))},
		{"OP_Shroud",			SP_Zone, DIR_Server, "spawnShroudSelf", SZC_None,
		(ObjectMember)&Session::m_spawnShell, SLOT(shroudSpawn(const uint8_t*, size_t, uint8_t))},
		{"OP_RemoveSpawn",		SP_Zone, DIR_Server|DIR_Client, "removeSpawnStruct", SZC_None,
		(ObjectMember)&Session::m_spawnShell, SLOT(removeSpawn(const uint8_t*, size_t, uint8_t))},
		{"OP_Consider",			SP_Zone, DIR_Server|DIR_Client, "considerStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(consMessage(const uint8_t*, size_t, uint8_t))},
		{"OP_NpcMoveUpdate",	SP_Zone, DIR_Server, "uint8_t", SZC_None,
		(ObjectMember)&Session::m_spawnShell, SLOT(npcMoveUpdate(const uint8_t*, size_t, uint8_t))},
		{"OP_ClientUpdate",		SP_Zone, DIR_Server, "playerSpawnPosStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(playerUpdate(const uint8_t*, size_t, uint8_t))},
		{"OP_CorpseLocResponse",	SP_Zone, DIR_Server, "corpseLocStruct", SZC_Match,
		(ObjectMember)&Session::m_spawnShell, SLOT(corpseLoc(const uint8_t*))},

		// SpellShell Opcode Handlers
		{"OP_CastSpell",		SP_Zone, DIR_Server|DIR_Client, "startCastStruct", SZC_Match,
		(ObjectMember)&Session::m_spellShell, SLOT(selfStartSpellCast(const uint8_t*))},
		{"OP_Buff",				SP_Zone, DIR_Server|DIR_Client, "buffStruct", SZC_Match,
		(ObjectMember)&Session::m_spellShell, SLOT(buff(const uint8_t*, size_t, uint8_t))},
		{"OP_Action",			SP_Zone, DIR_Server|DIR_Client, "actionStruct", SZC_Match,
		(ObjectMember)&Session::m_spellShell, SLOT(action(const uint8_t*, size_t, uint8_t))},
		{"OP_Action",			SP_Zone, DIR_Server|DIR_Client, "actionAltStruct", SZC_Match,
		(ObjectMember)&Session::m_spellShell, SLOT(action(const uint8_t*, size_t, uint8_t))},
		{"OP_SimpleMessage",	SP_Zone, DIR_Server, "simpleMessageStruct", SZC_Match,
		(ObjectMember)&Session::m_spellShell, SLOT(simpleMessage(const uint8_t*, size_t, uint8_t))},

		// Player Opcode Handlers
		{"OP_SkillUpdate",		SP_Zone, DIR_Server, "skillIncStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(increaseSkill(const uint8_t*))},
		{"OP_ManaChange",		SP_Zone, DIR_Server, "manaDecrementStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(manaChange(const uint8_t*))},
		{"OP_ClientUpdate",		SP_Zone, DIR_Server|DIR_Client, "playerSelfPosStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(playerUpdateSelf(const uint8_t*, size_t, uint8_t))},
		{"OP_ExpUpdate",		SP_Zone, DIR_Server, "expUpdateStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(updateExp(const uint8_t*))},
		{"OP_AAExpUpdate",		SP_Zone, DIR_Server, "altExpUpdateStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(updateAltExp(const uint8_t*))},
		{"OP_LevelUpdate",		SP_Zone, DIR_Server, "levelUpUpdateStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(updateLevel(const uint8_t*))},
		{"OP_HPUpdate",			SP_Zone, DIR_Server|DIR_Client, "hpNpcUpdateStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(updateNpcHP(const uint8_t*))},
		{"OP_WearChange",		SP_Zone, DIR_Server|DIR_Client, "SpawnUpdateStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(updateSpawnInfo(const uint8_t*))},
		{"OP_Stamina",			SP_Zone, DIR_Server, "staminaStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(updateStamina(const uint8_t*))},
		{"OP_Consider",			SP_Zone, DIR_Server|DIR_Client, "considerStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(consMessage(const uint8_t*, size_t, uint8_t))},
		{"OP_SwapSpell",		SP_Zone, DIR_Server, "tradeSpellBookSlotsStruct", SZC_Match,
		(ObjectMember)&Session::m_player, SLOT(tradeSpellBookSlots(const uint8_t*, size_t, uint8_t))},

		// TODO: This needs to be handled elsewhere? (or not at all?)
		// GuildManager Opcode Handlers
		//{"OP_GuildList",		SP_World, DIR_Server, "worldGuildListStruct", SZC_None,
		//(ObjectMember)&Session::m_guildmgr, SLOT(worldGuildList(const uint8_t*, size_t))},

		// Terminator
		{NULL}
	};


	DataSourceRegistrationInfo* dsri = registrations;
	while (dsri->opcodeName != NULL)
	{
		handler(dsri, param);
		dsri++;
	}
}

Session::~Session()
{
	emit DestroySession();
	
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
	
	if (m_dataSource != NULL)
		delete m_dataSource;
}


// -----------------------------------------------------------------------------
// Session Manager Class
// -----------------------------------------------------------------------------

SessionManager::SessionManager(QString configFile)
: QObject(NULL, "SessionManager")
{
	// Begin by creating the data location manager
	QString userDir = getUserDirectory();
	m_dataLocationMgr = new DataLocationMgr(userDir);
	m_dataLocationMgr->setupUserDirectory();
	
	// Set up preferences
#ifdef _WINDOWS
	QFileInfo defaultConfigFile = m_dataLocationMgr->findExistingFile(".", "seqdef.xml", true, false);
#else
	QFileInfo defaultConfigFile = m_dataLocationMgr->findExistingFile(".", "seqdef.xml", true, false);
#endif

	/* [brainiac] I'm not sure why this has to exist. I think it should be
	 * permissible to load without a default config file. Defaults should be
	 * hardcoded somewhere IMO. */
	if (!defaultConfigFile.exists()) {
		seqWarn("Fatal: Couldn't find seqdef.xml!\n"
				"\tDid you remember to do 'make install'?\n");

		/* maybe throw an exception to make this better handleable? */
		exit(-1);
	}
	QString defaultConfigFilePath = defaultConfigFile.absFilePath();
	
	// Get user config file, or use the specified file
	QString userConfigFilePath;
	if (configFile.isEmpty()) {
		QFileInfo userConfigFile = m_dataLocationMgr->findExistingFile(".", "showeq.xml", true, true);

		/* deal with funky border case since we may be running setuid */
// 		if (userConfigFile.dir() != QDir::root())
// 			userConfigFilePath = userConfigFile.absFilePath();
// 		else
			userConfigFilePath = QFileInfo(m_dataLocationMgr->userDataDir(".").absPath(), "showeq.xml").absFilePath();
	} else {
		/* TODO: Validate the path provided by the user */
		userConfigFilePath = configFile;
	}	
	seqInfo("Using config file '%s'\n", qPrintable(userConfigFilePath));
	
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

	
	/* get world opcodes file */
	QString fileName = pSEQPrefs->getPrefString("WorldOPCodes", "Network", "worldopcodes.xml");
	QFileInfo worldOpcodes = m_dataLocationMgr->findExistingFile(".", fileName);

	/* get zone opcodes file */
	QString fileName2 = pSEQPrefs->getPrefString("ZoneOPCodes", "Network", "zoneopcodes.xml");
	QFileInfo zoneOpcodes = m_dataLocationMgr->findExistingFile(".", fileName2);

	m_packetTypeDB		= new EQPacketTypeDB();	
	m_worldOPCodeDB		= new EQPacketOPCodeDB(29);
	m_zoneOPCodeDB		= new EQPacketOPCodeDB(211);

	// load the world opcode db
	if (!m_worldOPCodeDB->load(*m_packetTypeDB, worldOpcodes.absFilePath()))
		seqFatal("Error loading '%s'!", (const char*)worldOpcodes.absFilePath());

	// load the zone opcode db
	if (!m_zoneOPCodeDB->load(*m_packetTypeDB, zoneOpcodes.absFilePath()))
		seqFatal("Error loading '%s'!", (const char*)zoneOpcodes.absFilePath());
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

Session* SessionManager::newSession()
{
	// TODO: implement data source selection code
	Session* session		= new Session(this);
	DataSource* dataSource	= new RemotePacketServer(*m_zoneOPCodeDB, *m_worldOPCodeDB, session);

	// This might want to go elsewhere
	session->assignSource(dataSource);
	session->initializeDataSource();

	return session;
}

QString SessionManager::getUserDirectory()
{
#ifdef _WINDOWS
	/* userDirectory should be the working directory. In windows, just get the current working directory
	 * and use that as the user directory. I don't see why this needs to be user specific. at this time. */
	return ".";
#else
	return ".showeq";
#endif
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
	//m_spells->loadSpells(fileInfo.absFilePath());
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
