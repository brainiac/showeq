/*
 * interface.h
 *
 *  ShowEQ Distributed under GPL
 *  http://seq.sourceforge.net/
 */

#ifndef EQINT_H
#define EQINT_H

#include <QMainWindow>
#include <QToolBar>

#include "everquest.h"
#include "spawnlist.h"
#include "spawnshell.h"
#include "packetlog.h"

#include "session.h"

//--------------------------------------------------
// forward declarations
class Player;
class MapMgr;
class SpawnListWindow;
class SpawnListWindow2;
class SpellListWindow;
class SkillListWindow;
class StatListWindow;
class SpawnPointWindow;
class EQPacket;
class ZoneMgr;
class FilterMgr;
class CategoryMgr;
class SpawnShell;
class SpellShell;
class GroupMgr;
class SpawnMonitor;
class SpawnLog;
class FilteredSpawnLog;
class FilterNotifications;
class Item;
class CompassFrame;
class MapFrame;
class ExperienceWindow;
class CombatWindow;
class NetDiag;
class GuildMgr;
class Spells;
class DateTimeMgr;
class PacketLog;
class PacketStreamLog;
class UnknownPacketLog;
class OPCodeMonitorPacketLog;
class DataLocationMgr;
class EQStr;
class MessageFilters;
class Messages;
class MessageShell;
class MessageWindow;
class Terminal;
class MessageFilterDialog;
class GuildShell;
class GuildListWindow;
class BazaarLog;

//--------------------------------------------------
// typedefs
typedef QList<int> MenuIDList;

//--------------------------------------------------
// constants

// Number of strings, pass as a size to EQStr
const int maxNumEQStr = 8009;

// maximum number of maps
const int maxNumMaps = 5;

// maximum number of message windows
const int maxNumMessageWindows = 10;

//--------------------------------------------------
// EQInterface
// QMainWindow from Hell!  Also known as ShowEQ's main window.
class EQInterface : public QMainWindow
{
	Q_OBJECT
	
	MapMgr* m_mapMgr;
	EQPacket* m_packet;
	ZoneMgr* m_zoneMgr;
	FilterMgr* m_filterMgr;
	SpawnShell* m_spawnShell;
	SpellShell* m_spellShell;
	GroupMgr* m_groupMgr;
	SpawnMonitor* m_spawnMonitor;
	GuildMgr* m_guildmgr;
	GuildShell* m_guildShell;
	MessageFilters* m_messageFilters;
	Messages* m_messages;
	MessageShell* m_messageShell;
	Terminal* m_terminal;
	FilteredSpawnLog* m_filteredSpawnLog;
	FilterNotifications* m_filterNotifications;
	SpawnLog *m_spawnLogger;
	Player* m_player;
	
	SessionManager* m_sm;

public:
	EQInterface(SessionManager* sm);
	~EQInterface();

	QFont appFont;

public slots:
	void stsMessage(const QString &, int timeout = 0);
	void numSpawns(int);
	void setExp(uint32_t totalExp, uint32_t totalTick, uint32_t minExpLevel, uint32_t maxExpLevel, uint32_t tickExpLevel);
	void newExp(uint32_t newExp, uint32_t totalExp, uint32_t totalTick, uint32_t minExpLevel, uint32_t maxExpLevel, uint32_t tickExpLevel);
	void setAltExp(uint32_t totalExp, uint32_t maxExp, uint32_t tickExp, uint32_t aapoints);
	void newAltExp(uint32_t newExp, uint32_t totalExp, uint32_t totalTick, uint32_t maxExp, uint32_t tickExp, uint32_t aapoints);
	void levelChanged(uint8_t level);
	void newSpeed(double);
	void numPacket(int, int);
	void resetPacket(int, int);
	void attack2Hand1(const uint8_t*);
	void action2Message(const uint8_t *);
	void combatKillSpawn(const uint8_t*);
	void updatedDateTime(const QDateTime&);
	void syncDateTime(const QDateTime&);
	void clientTarget(const uint8_t* cts);

	void zoneBegin(const QString& shortZoneName);
	void zoneEnd(const QString& shortZoneName, const QString& longZoneName);
	void zoneChanged(const QString& shortZoneName);

	void spawnSelected(const Item* item);
	void spawnConsidered(const Item* item);
	void addItem(const Item* item);
	void delItem(const Item* item);
	void killSpawn(const Item* item);
	void changeItem(const Item* item);

	void updateSelectedSpawnStatus(const Item* item);

	void savePrefs();
	void rebuildSpawnList();
	void selectNext();
	void selectPrev();
	void saveSelectedSpawnPath();
	void saveSpawnPaths();
	void saveSpawnPath(QTextStream& out, const Item* item);
	void toggleLogAllPackets();
	void toggleLogWorldData();
	void toggleLogZoneData();
	void toggle_log_UnknownData();
	void toggle_log_RawData();
	void listSpawns();
	void listDrops();
	void listMapInfo();
	void listInterfaceInfo();
	void listGroup();
	void listGuild();
	void dumpSpawns();
	void dumpDrops();
	void dumpMapInfo();
	void dumpGuildInfo();
	void dumpSpellBook();
	void dumpGroup();
	void dumpGuild();
	void launchFilterEditor();
	void launchZoneFilterEditor();
	void toggleAutoDetectPlayerSettings(int id);
	void SetDefaultCharacterClass(int id);
	void SetDefaultCharacterRace(int id);
	void SetDefaultCharacterLevel(int id);

	void togglePlayerStat(QAction*);
	void togglePlayerSkill(QAction*);
	void toggleSpawnListCol(QAction*);

	void toggleWindowDocked(QAction*);
	void toggleWindowDockable(QAction*);

	void toggle_log_Filter_ZoneData_Client();
	void toggle_log_Filter_ZoneData_Server();

	void toggleOpcodeMonitoring(bool);
	void set_opcode_monitored_list();
	void toggle_opcode_view(bool);
	void toggle_opcode_log(bool);
	void select_opcode_file();
	void toggle_net_session_tracking(bool);
	void toggle_net_real_time_thread();
	void set_net_monitor_next_client();
	void set_net_client_IP_address();
	void set_net_client_MAC_address();
	void set_net_device();
	void set_net_arq_giveup(int giveup);
	virtual void setCaption(const QString&);
	void restoreStatusFont();
	void showMessageFilterDialog();

signals:
	void guildList2text(QString);
	void loadFileMap();
	void selectSpawn(const Item* item);
	void saveAllPrefs();
	void newZoneName (const QString &);
	void spellMessage(QString&);
	void restoreFonts();

	// Decoder signals
	void theKey(uint64_t);
	void backfillPlayer(charProfileStruct *);
	void combatSignal(int, int, int, int, int, QString, QString);

private slots:

	// Window toggle slots
	void toggleChannelMsgs(QAction*);
	void toggleMap(QAction*);
	void toggleExpWindow();
	void toggleCombatWindow();
	void toggleSpawnList();
	void toggleSpawnList2();
	void toggleSpawnPointList();
	void toggleSpellList();
	void togglePlayerStats();
	void toggleCompass();
	void togglePlayerSkills();
	void toggleNetDiag();
	void toggleGuildList();

	// Options menu slots
	void toggleFastMachine(bool);
	void toggleSelectOnConsider(bool);
	void toggleSelectOnTarget(bool);
	void toggleKeepSelectedVisible(bool);
	void toggleLogSpawns(bool);
	void togglePvPTeams(bool);
	void togglePvPDeity(bool);
	void toggleCreateUnknownSpawns(bool);
	void toggleRecordWalkPaths(bool);
	void toggleRetardedCoords(bool);
	void toggleSystemSpawnTime(bool);
	void selectConColorBase(QAction*);
	void setWalkPathLength(int len);
	void resetMaxMana();
	void toggleSavePlayerState(bool);
	void toggleSaveZoneState(bool);
	void toggleSaveSpawnState(bool);
	void setSpawnSaveFrequency(int frequency);
	void setSaveBaseFilename();
	void clearChannelMessages();
	void toggleUseUpdateRadius(bool);
	void toggleLogBazaarData(bool);

	void toggle_view_UnknownData();
	void select_filter_file();
	void toggle_filter_Case(bool);
	void toggle_filter_AlertInfo(bool);
	void toggle_filter_UseSystemBeep(bool);
	void toggle_filter_UseCommands(bool);
	void toggle_filter_Log(QAction*);
	void set_filter_AudioCommand(QAction*);

	void toggle_view_menubar();
	void toggle_view_statusbar();
	void set_main_WindowCaption(QAction*);
	void set_main_WindowFont(QAction*);
	void set_main_Font();
	void select_main_FormatFile();
	void select_main_SpellsFile();
	void toggle_main_statusbar_Window(QAction*);
	void set_main_statusbar_Font();


	void updateViewMenu();

	void toggle_main_SavePosition(bool);
	void toggle_main_UseWindowPos(bool);

	// Filter Menu Actions
	void toggleTypeFilter(QAction*);
	void disableAllTypeFilters();
	void enableAllTypeFilters();
	void toggleShowUserFilter(QAction*);
	void disableAllShowUserFilters();
	void enableAllShowUserFilters();
	void toggleHideUserFilter(QAction*);
	void disableAllHideUserFilters();
	void enableAllHideUserFilters();
	void toggleDisplayType(bool);
	void toggleDisplayTime(bool);
	void toggleEQDisplayTime(bool);
	void toggleUseColor(bool);

protected:
	bool getMonitorOpCodeList(const QString& title, QString& opcodeList);
	void showMap(int mapNum);
	void showMessageWindow(int winNum);
	void showSpawnList();
	void showSpawnList2();
	void showSpawnPointList();
	void showStatList();
	void showSkillList();
	void showSpellList();
	void showCompass();
	void showNetDiag();
	void showGuildList();
	void createFilteredSpawnLog();
	void createSpawnLog();
	void createGlobalLog();
	void createWorldLog();
	void createZoneLog();
	void createBazaarLog();
	void createUnknownZoneLog();
	void createOPCodeMonitorLog(const QString&);
	void insertWindowMenu(SEQWindow* window);
	void removeWindowMenu(SEQWindow* window);
	void setDockEnabled(SEQWindow* dw, bool enable);

protected:

	// window Id enumeration
	enum WindowId {
		menuSpawnList = 0,
		menuPlayerStats = 1,
		menuPlayerSkills = 2,
		menuSpellList = 3,
		menuCompass = 4,
		menuSpawnPointList = 5,
		menuSpawnList2 = 6,
		menuExperienceWindow = 7,
		menuCombatWindow = 8,
		menuGuildList = 9,
		menuNetDiag = 10,
		mapDockBase = 11,
		mapCaptionBase = 11,
		messageWindowDockBase = 16,
	};

private:

	
	SpawnPointWindow* m_spawnPointList;
	BazaarLog* m_bazaarLog;
#ifndef _WINDOWS
	PacketLog* m_globalLog;
	PacketStreamLog* m_worldLog;
	PacketStreamLog* m_zoneLog;
	UnknownPacketLog* m_unknownZoneLog;
	OPCodeMonitorPacketLog* m_opcodeMonitorLog;
#endif

	const Item* m_selectedSpawn;

	// Windows
	SpawnListWindow* m_spawnList;
	SpawnListWindow2* m_spawnList2;
	SpellListWindow* m_spellList;
	SkillListWindow* m_skillList;
	StatListWindow* m_statList;
	CompassFrame* m_compass;
	MessageWindow* m_messageWindow[maxNumMessageWindows];
	MapFrame*  m_map[maxNumMaps];
	ExperienceWindow* m_expWindow;
	CombatWindow* m_combatWindow;
	NetDiag* m_netDiag;
	MessageFilterDialog* m_messageFilterDialog;
	GuildListWindow* m_guildListWindow;

	QString m_ipstr[5];
	QString m_macstr[5];

	// Menu Pieces
	QMenu* m_netMenu;
	QMenu* m_decoderMenu;
	QMenu* m_playerStatsMenu;
	QMenu* m_playerSkillsMenu;
	QMenu* m_spawnListMenu;
	QMenu* m_dockedWinMenu;
	QMenu* m_dockableWinMenu;
	QMenu* m_windowCaptionMenu;
	QMenu* m_charMenu;
	QMenu* m_charLevelMenu;
	QSpinBox* m_levelSpinBox;
	QMenu* m_charClassMenu;
	QMenu* m_charRaceMenu;
	QMenu* m_terminalMenu;
	QMenu* m_terminalTypeFilterMenu;
	QMenu* m_terminalShowUserFilterMenu;
	QMenu* m_terminalHideUserFilterMenu;
	QMenu* m_windowMenu;
	QHash<SEQWindow*, QMenu*> m_windowsMenus;
	QMenu* m_filterZoneDataMenu;

	// View Menu Actions
	QAction* m_viewExpWindow;
	QAction* m_viewCombatWindow;
	QAction* m_viewSpawnList;
	QAction* m_viewSpawnList2;
	QAction* m_viewSpawnPointList;
	QAction* m_viewSpellList;
	QAction* m_viewPlayerStats;
	QAction* m_viewPlayerSkills;
	QAction* m_viewCompass;
	QAction* m_viewMap[maxNumMaps];
	QAction* m_viewMessageWindow[maxNumMessageWindows];
	QAction* m_viewNetDiag;
	QAction* m_viewGuildList;

	QAction* m_playerStatsMenuAction;
	QAction* m_spawnListMenuAction;
	QAction* m_playerSkillsMenuAction;
	QAction* m_playerSkillsLanguages;

	// network menu actions
	QAction* m_netSessionTracking;
	QAction* m_netRealTimeThread;

	QAction* m_netLogAllPackets;
	QAction* m_netLogWorldData;
	QAction* m_netLogZoneData;
	QAction* m_netLogUnknownData;
	QAction* m_netViewUnknownData;
	QAction* m_netLogRawData;

	QAction* m_netLogItems;
	QAction* m_netLogItemPackets;

	QAction* m_netLogFilterZoneClient;
	QAction* m_netLogFilterZoneServer;

	QAction* m_netOpcodeLog;
	QAction* m_netOpcodeView;

	// Filter list
	QAction* m_filterTerminalEnableAll;
	QAction* m_filterTerminalDisableAll;
	QMap<int, QAction*> m_filterTerminalActionMap;

	QAction* m_filterTerminalShowUserEnableAll;
	QAction* m_filterTerminalShowUserDisableAll;
	QMap<int, QAction*> m_filterTerminalShowUserMap;

	QAction* m_filterTerminalHideUserEnableAll;
	QAction* m_filterTerminalHideUserDisableAll;
	QMap<int, QAction*> m_filterTerminalHideUserMap;

	int char_ClassID[PLAYER_CLASSES];
	int char_RaceID[PLAYER_RACES];
	int m_packetStartTime;
	int m_initialcount;

	MenuIDList IDList_StyleMenu;

	// Status bar pieces
	QLabel* m_stsbarSpawns;
	QLabel* m_stsbarStatus;
	QLabel* m_stsbarZone;
	QLabel* m_stsbarID;
	QLabel* m_stsbarExp;
	QLabel* m_stsbarExpAA;
	QLabel* m_stsbarPkt;
	QLabel* m_stsbarEQTime;
	QLabel* m_stsbarSpeed;
	// ZEM code
	QLabel* m_stsbarZEM;

	QAction* m_windowBottomAction;

	QToolBar* m_toolBar;


	// ===================================================
	QStringList m_StringList;
	QDialog *dialogbox;

	bool m_isSkillListDocked;
	bool m_isStatListDocked;
	bool m_isMapDocked[maxNumMaps];
	bool m_isMessageWindowDocked[maxNumMessageWindows];
	bool m_isSpawnListDocked;
	bool m_isSpawnList2Docked;
	bool m_isSpawnPointListDocked;
	bool m_isSpellListDocked;
	bool m_isCompassDocked;

	bool m_selectOnConsider;
	bool m_selectOnTarget;
	bool m_useUpdateRadius;

	int m_stateVersion;

	QWidget* m_filler;

private:
	// create interface widgets
	void setupExperienceWindow();
	void setupCombatWindow();
	void createInterfaceWidgets();

	// create logs
	void createLogs();

	// menu items
	void createFileMenu();
	void createViewMenu();
	void createOptionsMenu();
	void createNetworkMenu();
	void createCharacterMenu();
	void createFiltersMenu();
	void createInterfaceMenu();
	void createWindowMenu();
	void createDebugMenu();

	// status bar
	void createStatusBar();

	// connect packet signals
	void connectSignals();

	bool m_creating;	// true if we're creating the interface
};

#endif // EQINT_H
