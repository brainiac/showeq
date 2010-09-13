/*
 * experiencelog.h
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net
 */

#ifndef COMBATLOG_H
#define COMBATLOG_H

#include <Q3ListView>
#include <Q3PtrList>

#include "seqwindow.h"
#include "seqlistview.h"
#include "player.h"
#include "group.h"

//----------------------------------------------------------------------
// forward declarations
class Player;

////////////////////////////////////////////
//  CombatOffenseRecord definition
//////////////////////////////////////////`//
class CombatOffenseRecord
{
public:
	CombatOffenseRecord(int iType, Player* p, int iSpell);

	int	getType() { return m_iType; };
	int	getSpell() { return m_iSpell; };
	int	getHits() { return m_iHits; };
	int	getMisses() { return m_iMisses; };
	int	getMinDamage() { return m_iMinDamage; };
	int	getMaxDamage() { return m_iMaxDamage; };
	int	getTotalDamage() { return m_iTotalDamage; };

	void addMiss(int iMissReason) { m_iMisses++; };
	void addHit(int iDamage);

private:
	int m_iType;
	int m_iSpell;
	Player* m_player;

	int m_iHits;
	int m_iMisses;
	int m_iMinDamage;
	int m_iMaxDamage;
	int m_iTotalDamage;
};


////////////////////////////////////////////
//  CombatDefenseRecord definition
////////////////////////////////////////////
class CombatDefenseRecord
{
public:

	CombatDefenseRecord(Player* p);

	int getHits() { return m_iHits; };
	int getMisses() { return m_iMisses; };
	int getBlocks() { return m_iBlocks; };
	int getParries() { return m_iParries; };
	int getRipostes() { return m_iRipostes; };
	int getDodges() { return m_iDodges; };
	int getMinDamage() { return m_iMinDamage == -1 ? 0 : m_iMinDamage; };
	int getMaxDamage() { return m_iMaxDamage; };
	int getTotalDamage() { return m_iTotalDamage; };
	int getTotalAttacks() { return m_iTotalAttacks; };

	void clear();
	void addMiss(int iMissReason);
	void addHit(int iDamage);

private:
	Player* m_player;

	int m_iHits;
	int m_iMisses;
	int m_iBlocks;
	int m_iParries;
	int m_iRipostes;
	int m_iDodges;
	int m_iMinDamage;
	int m_iMaxDamage;
	int m_iTotalDamage;
	int m_iTotalAttacks;
};


////////////////////////////////////////////
//  CombatMobRecord definition
////////////////////////////////////////////
class CombatMobRecord
{
public:
	CombatMobRecord(int iID, int iStartTime, Player* p);

	int getID() { return m_iID; };
	int getDuration() { return (m_iLastTime - m_iStartTime); };
	int getDamageGiven() { return m_iDamageGiven; };
	int getDamageTaken() { return m_iDamageTaken; };

	QString	getName() { return m_iName; };
	void setName(QString iName) { m_iName = iName; };

	time_t getTime() { return m_time; };
	void setTime(time_t iTime) { m_time = iTime; };

	double getDPS();
	double getMobDPS();

	void addHit(int iTarget, int iSource, int iDamage);

private:
	int m_iID;
	Player* m_player;
	int m_iStartTime;
	int m_iLastTime;
	int m_iDamageGiven;
	double m_dDPS;
	int m_iDamageTaken;
	double m_dMobDPS;
	QString m_iName;
	time_t m_time;
};

////////////////////////////////////////////
//  CombatWindow definition
////////////////////////////////////////////
class CombatWindow : public SEQWindow
{
	Q_OBJECT

public:
	CombatWindow(Player* player, QWidget* parent = 0, const char* name = 0);
	~CombatWindow();

public slots:
	virtual void savePrefs();
	void addCombatRecord(int iTargetID, int iSourceID, int iType, int iSpell, int iDamage, QString tName, QString sName);
	void resetDPS();
	void clearMob();
	void clearOffense();
	void clear();

private:
	void initUI();
	QWidget* initOffenseWidget();
	QWidget* initDefenseWidget();
	QWidget* initMobWidget();

	void addOffenseRecord(int iType, int iDamage, int iSpell);
	void addDefenseRecord(int iDamage);
	void addMobRecord(int iTargetID, int iSourceID, int iDamage, QString tName, QString sName);

	void updateOffense();
	void updateDefense();
	void updateMob();
	void updateDPS(int iDamage);

private:

	Player*		m_player;
	QTabWidget*     m_tab;

	// Offense tab
	QWidget* m_offenseTab;
	QVBoxLayout* m_offenseLayout;
	SEQListView* m_offenseListView;
	QLabel* m_offenseTotalDamage;
	QLabel* m_offensePercentSpecial;
	QLabel* m_offensePercentNonMelee;
	QLabel* m_offenseAvgMelee;
	QLabel* m_offenseAvgSpecial;
	QLabel* m_offenseAvgNonMelee;

	// Defense tab
	QWidget* m_defenseTab;
	QVBoxLayout* m_defenseLayout;
	QLabel* m_defenseAvoidMisses;
	QLabel* m_defenseAvoidBlock;
	QLabel* m_defenseAvoidParry;
	QLabel* m_defenseAvoidRiposte;
	QLabel* m_defenseAvoidDodge;
	QLabel* m_defenseAvoidTotal;
	QLabel* m_defenseMitigateAvgHit;
	QLabel* m_defenseMitigateMinHit;
	QLabel* m_defenseMitigateMaxHit;
	QLabel* m_defenseMobAttacks;
	QLabel* m_defensePercentAvoided;
	QLabel* m_defenseTotalDamage;

	// Mob tab
	QWidget* m_mobTab;
	QVBoxLayout* m_mobLayout;
	SEQListView* m_mobListView;
	QLabel* m_mobTotalMobs;
	QLabel* m_mobAvgDps;
	QLabel* m_mobCurrentDps;
	QLabel* m_mobLastDps;

	Q3PtrList<CombatOffenseRecord> m_combat_offense_list;
	CombatDefenseRecord *m_combat_defense_record;
	Q3PtrList<CombatMobRecord> m_combat_mob_list;

	//QMenuBar *m_menuBar;
	//Q3PopupMenu *m_clearMenu;

	int m_iCurrentDPSTotal;
	int m_iDPSStartTime;
	int m_iDPSTimeLast;
	double m_dDPS;
	double m_dDPSLast;
};

#endif // COMBATLOG_H
