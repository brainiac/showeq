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
	
	// attach signals
	
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
	
	assignSource(source);
}

void Session::assignSource(DataSource* source)
{
	// connects signals from the source to the objects that consume packets
	m_source = source;
	
	if (m_source != NULL)
	{	
		// attach signals to zone manager
		m_source->connectReceiver("OP_ZoneEntry", "ClientZoneEntryStruct", SP_Zone, DIR_Client, SZC_Match,
								  m_zoneMgr, SLOT(zoneEntryClient(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_PlayerProfile", "charProfileStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_zoneMgr, SLOT(zonePlayer(const uint8_t*)));
		m_source->connectReceiver("OP_ZoneChange", "zoneChangeStruct", SP_Zone, DIR_Client | DIR_Server, SZC_Match,
								  m_zoneMgr, SLOT(zoneChange(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_NewZone", "newZoneStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_zoneMgr, SLOT(zoneNew(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_SendZonePoints", "zonePointsStruct", SP_Zone, DIR_Server, SZC_None,
								  m_zoneMgr, SLOT(zonePoints(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_DzSwitchInfo", "dzSwitchInfo", SP_Zone, DIR_Server, SZC_None,
								  m_zoneMgr, SLOT(dynamicZonePoints(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_DzInfo", "dzInfo", SP_Zone, DIR_Server, SZC_Match,
								  m_zoneMgr, SLOT(dynamicZoneInfo(const uint8_t*, size_t, uint8_t)));
		
		// attach signals to group manager
		m_source->connectReceiver("OP_GroupUpdate", "uint8_t", SP_Zone, DIR_Server, SZC_None,
								  m_groupMgr, SLOT(groupUpdate(const uint8_t*, size_t)));
		m_source->connectReceiver("OP_GroupFollow", "groupFollowStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_groupMgr, SLOT(addGroupMember(const uint8_t*)));
		m_source->connectReceiver("OP_GroupDisband", "groupDisbandStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_groupMgr, SLOT(removeGroupMember(const uint8_t*)));
		m_source->connectReceiver("OP_GroupDisband2", "groupDisbandStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_groupMgr, SLOT(removeGroupMember(const uint8_t*)));
		
		// attach signals to guild manager
		m_source->connectReceiver("OP_GuildList", "worldGuildListStruct", SP_World, DIR_Server, SZC_None,
								  m_guildMgr, SLOT(worldGuildList(const uint8_t*, size_t)));
		
		// attach signals to guild shell
		m_source->connectReceiver("OP_GuildMemberList", "uint8_t", SP_Zone, DIR_Server, SZC_None,
								  m_guildShell, SLOT(guildMemberList(const uint8_t*, size_t)));
		m_source->connectReceiver("OP_GuildMemberUpdate", "GuildMemberUpdate", SP_Zone, DIR_Server, SZC_Match,
								  m_guildShell, SLOT(guildMemberUpdate(const uint8_t*, size_t)));
		
		// attach signals to message shell
		m_source->connectReceiver("OP_CommonMessage", "channelMessageStruct", SP_Zone, DIR_Client | DIR_Server, SZC_None,
								  m_messageShell, SLOT(channelMessage(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_FormattedMessage", "formattedMessageStruct", SP_Zone, DIR_Server, SZC_None,
								  m_messageShell, SLOT(formattedMessage(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_SimpleMessage", "simpleMessageStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(simpleMessage(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_SpecialMesg", "specialMessageStruct", SP_Zone, DIR_Server, SZC_None,
								  m_messageShell, SLOT(specialMessage(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_GuildMOTD", "guildMOTDStruct", SP_Zone, DIR_Server, SZC_None,
								  m_messageShell, SLOT(guildMOTD(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_RandomReq", "randomReqStruct", SP_Zone, DIR_Client, SZC_Match,
								  m_messageShell, SLOT(randomRequest(const uint8_t*)));
		m_source->connectReceiver("OP_RandomReply", "randomStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(random(const uint8_t*)));
		m_source->connectReceiver("OP_ConsentResponse", "consentResponseStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(consent(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_DenyResponse", "consentResponseStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(consent(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_Emote", "emoteTextStruct", SP_Zone, DIR_Server | DIR_Client, SZC_None,
								  m_messageShell, SLOT(emoteText(const uint8_t*)));
		m_source->connectReceiver("OP_InspectAnswer", "inspectDataStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(inspectData(const uint8_t*)));
		m_source->connectReceiver("OP_MoneyOnCorpse", "moneyOnCorpseStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(moneyOnCorpse(const uint8_t*)));
		m_source->connectReceiver("OP_Logout", "none", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(logOut(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_NewZone", "newZoneStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(zoneNew(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_MOTD", "worldMOTDStruct", SP_World, DIR_Server, SZC_None,
								  m_messageShell, SLOT(worldMOTD(const uint8_t*)));
		m_source->connectReceiver("OP_MemorizeSpell", "memSpellStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_messageShell, SLOT(handleSpell(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_BeginCast", "beginCastStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_messageShell, SLOT(beginCast(const uint8_t*)));
		m_source->connectReceiver("OP_BuffFadeMsg", "spellFadedStruct", SP_Zone, DIR_Server | DIR_Client, SZC_None,
								  m_messageShell, SLOT(spellFaded(const uint8_t*)));
		m_source->connectReceiver("OP_CastSpell", "startCastStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_messageShell, SLOT(startCast(const uint8_t*)));
		m_source->connectReceiver("OP_SkillUpdate", "skillIncStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(increaseSkill(const uint8_t*)));
		m_source->connectReceiver("OP_LevelUpdate", "levelUpUpdateStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(updateLevel(const uint8_t*)));
		m_source->connectReceiver("OP_Consider", "considerStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(consMessage(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_GroupInvite", "groupInviteStruct", SP_Zone, DIR_Client, SZC_Match,
								  m_messageShell, SLOT(groupInvite(const uint8_t*)));
//		m_source->connectReceiver("OP_GroupInvite", "groupAltInviteStruct", SP_Zone, DIR_Server, SZC_Match,
//								  m_messageShell, SLOT(groupInvite(const uint8_t*)));
		m_source->connectReceiver("OP_GroupInvite2", "groupInviteStruct", SP_Zone, DIR_Client, SZC_Match,
								  m_messageShell, SLOT(groupInvite(const uint8_t*)));
		m_source->connectReceiver("OP_GroupFollow", "groupFollowStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(groupFollow(const uint8_t*)));
		m_source->connectReceiver("OP_GroupFollow2", "groupFollowStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(groupFollow(const uint8_t*)));
		m_source->connectReceiver("OP_GroupDisband", "groupDisbandStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(groupDisband(const uint8_t*)));
		m_source->connectReceiver("OP_GroupDisband2", "groupDisbandStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(groupDisband(const uint8_t*)));
		m_source->connectReceiver("OP_GroupCancelInvite", "groupDeclineStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_messageShell, SLOT(groupDecline(const uint8_t*)));
		m_source->connectReceiver("OP_GroupLeader", "groupLeaderChangeStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_messageShell, SLOT(groupLeaderChange(const uint8_t*)));
		
		// attach signals to spawnshell
		m_source->connectReceiver("OP_GroundSpawn", "makeDropStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_spawnShell, SLOT(newGroundItem(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_ClickObject", "remDropStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_spawnShell, SLOT(removeGroundItem(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_SpawnDoor", "doorStruct", SP_Zone, DIR_Server, SZC_Modulus,
								  m_spawnShell, SLOT(newDoorSpawns(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_ZoneEntry", "uint8_t", SP_Zone, DIR_Server, SZC_None,
								  m_spawnShell, SLOT(zoneEntry(const uint8_t*, size_t)));
		m_source->connectReceiver("OP_MobUpdate", "spawnPositionUpdate", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spawnShell, SLOT(updateSpawns(const uint8_t*)));
		m_source->connectReceiver("OP_WearChange", "SpawnUpdateStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spawnShell, SLOT(updateSpawnInfo(const uint8_t*)));
		m_source->connectReceiver("OP_HPUpdate", "hpNpcUpdateStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spawnShell, SLOT(updateNpcHP(const uint8_t*)));
		m_source->connectReceiver("OP_DeleteSpawn", "deleteSpawnStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spawnShell, SLOT(deleteSpawn(const uint8_t*)));
		m_source->connectReceiver("OP_SpawnRename", "spawnRenameStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_spawnShell, SLOT(renameSpawn(const uint8_t*)));
		m_source->connectReceiver("OP_Illusion", "spawnIllusionStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spawnShell, SLOT(illusionSpawn(const uint8_t*)));
		m_source->connectReceiver("OP_SpawnAppearance", "spawnAppearanceStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spawnShell, SLOT(updateSpawnAppearance(const uint8_t*)));
		m_source->connectReceiver("OP_Death", "newCorpseStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_spawnShell, SLOT(killSpawn(const uint8_t*)));
//		m_source->connectReceiver("OP_RespawnFromHover", "uint8_t", SP_Zone, DIR_Server | DIR_Client, SZC_None,
//								  m_spawnShell, SLOT(respawnFromHover(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_Shroud", "spawnShroudSelf", SP_Zone, DIR_Server, SZC_None,
								  m_spawnShell, SLOT(shroudSpawn(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_RemoveSpawn", "removeSpawnStruct", SP_Zone, DIR_Server | DIR_Client, SZC_None,
								  m_spawnShell, SLOT(removeSpawn(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_Consider", "considerStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spawnShell, SLOT(consMessage(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_NpcMoveUpdate", "uint8_t", SP_Zone, DIR_Server, SZC_None,
								  m_spawnShell, SLOT(npcMoveUpdate(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_ClientUpdate", "playerSpawnPosStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_spawnShell, SLOT(playerUpdate(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_CorpseLocResponse", "corpseLocStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_spawnShell, SLOT(corpseLoc(const uint8_t*)));
		
		// attach signals to spell shell
		m_source->connectReceiver("OP_CastSpell", "startCastStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spellShell, SLOT(selfStartSpellCast(const uint8_t*)));
		m_source->connectReceiver("OP_Buff", "buffStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spellShell, SLOT(buff(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_Action", "actionStruct", SP_Zone, DIR_Server|DIR_Client, SZC_Match,
								  m_spellShell, SLOT(action(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_Action", "actionAltStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_spellShell, SLOT(action(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_SimpleMessage", "simpleMessageStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_spellShell, SLOT(simpleMessage(const uint8_t*, size_t, uint8_t)));
		
		// attach signals to player object
		m_source->connectReceiver("OP_SkillUpdate", "skillIncStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_player, SLOT(increaseSkill(const uint8_t*)));
		m_source->connectReceiver("OP_ManaChange", "manaDecrementStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_player, SLOT(manaChange(const uint8_t*)));
		m_source->connectReceiver("OP_ClientUpdate", "playerSelfPosStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_player, SLOT(playerUpdateSelf(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_ExpUpdate", "expUpdateStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_player, SLOT(updateExp(const uint8_t*)));
		m_source->connectReceiver("OP_AAExpUpdate", "altExpUpdateStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_player, SLOT(updateAltExp(const uint8_t*)));
		m_source->connectReceiver("OP_LevelUpdate", "levelUpUpdateStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_player, SLOT(updateLevel(const uint8_t*)));
		m_source->connectReceiver("OP_HPUpdate", "hpNpcUpdateStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_player, SLOT(updateNpcHP(const uint8_t*)));
		m_source->connectReceiver("OP_WearChange", "SpawnUpdateStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_player, SLOT(updateSpawnInfo(const uint8_t*)));
		m_source->connectReceiver("OP_Stamina", "staminaStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_player, SLOT(updateStamina(const uint8_t*)));
		m_source->connectReceiver("OP_Consider", "considerStruct", SP_Zone, DIR_Server | DIR_Client, SZC_Match,
								  m_player, SLOT(consMessage(const uint8_t*, size_t, uint8_t)));
		m_source->connectReceiver("OP_SwapSpell", "tradeSpellBookSlotsStruct", SP_Zone, DIR_Server, SZC_Match,
								  m_player, SLOT(tradeSpellBookSlots(const uint8_t*, size_t, uint8_t)));
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
	
	if (m_source != NULL)
		delete m_source;
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
	return NULL;
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
