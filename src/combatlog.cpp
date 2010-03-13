/*
 *  combatlog.cpp
 *
 *  ShowEQ Distributed under GPL
 *  http://seq.sourceforge.net
 *
 *  Copyright 2003-2007 by the respective ShowEQ Developers
 */

#include "pch.h"

#include "combatlog.h"
#include "player.h"
#include "util.h"
#include "diagnosticmessages.h"

#include <Q3Grid>
#include <Q3HBox>
#include <Q3VGroupBox>
#include <Q3GridLayout>
#include <Q3PopupMenu>
#include <Q3VBoxLayout>

#include <QVBoxLayout>

//#define DEBUGCOMBAT


////////////////////////////////////////////
//  CombatOffenseRecord implementation
////////////////////////////////////////////
CombatOffenseRecord::CombatOffenseRecord(int iType, Player* p, int iSpell)
  : m_iType(iType),
	m_iSpell(iSpell),
	m_player(p),
	m_iHits(0),
	m_iMisses(0),
	m_iMinDamage(65536),
	m_iMaxDamage(0),
	m_iTotalDamage(0)
{
}

void CombatOffenseRecord::addHit(int iDamage)
{
	if (iDamage <= 0)
		return;

	m_iHits++;
	m_iTotalDamage += iDamage;

	if (iDamage > 0 && iDamage < m_iMinDamage)
		m_iMinDamage = iDamage;

	if (iDamage > m_iMaxDamage)
		m_iMaxDamage = iDamage;
}


////////////////////////////////////////////
//  CombatDefenseRecord implementation
////////////////////////////////////////////
CombatDefenseRecord::CombatDefenseRecord(Player* p)
  : m_player(p)
{
	clear();
}

void CombatDefenseRecord::clear()
{
	m_iHits = 0;
	m_iMisses = 0;
	m_iBlocks = 0;
	m_iParries = 0;
	m_iRipostes = 0;
	m_iDodges = 0;
	m_iMinDamage = -1;
	m_iMaxDamage = 0;
	m_iTotalDamage = 0;
	m_iTotalAttacks = 0;
}

void CombatDefenseRecord::addHit(int iDamage)
{
	if (iDamage <= 0)
		return;

	m_iTotalAttacks++;
	m_iHits++;
	m_iTotalDamage += iDamage;

	if (iDamage > 0 && (iDamage < m_iMinDamage || m_iMinDamage == -1))
		m_iMinDamage = iDamage;

	if (iDamage > m_iMaxDamage)
		m_iMaxDamage = iDamage;
}

void CombatDefenseRecord::addMiss(int iMissReason)
{
	m_iTotalAttacks++;

	switch (iMissReason)
	{
		case COMBAT_MISS:
			m_iMisses++;
			break;

		case COMBAT_BLOCK:
			m_iBlocks++;
			break;

		case COMBAT_PARRY:
			m_iParries++;
			break;

		case COMBAT_RIPOSTE:
			m_iRipostes++;
			break;

		case COMBAT_DODGE:
			m_iDodges++;
			break;

		default:
#ifdef DEBUGCOMBAT
			seqDebug("CombatDefenseRecord::addMiss:WARNING: invalid miss reason");
#endif
			break;
	}
}


////////////////////////////////////////////
//	CombatMobRecord implementation
////////////////////////////////////////////
CombatMobRecord::CombatMobRecord(int iID, int iStartTime, Player* p)
  : m_iID(iID),
	m_player(p),
	m_iStartTime(iStartTime),
	m_iLastTime(iStartTime),
	m_iDamageGiven(0),
	m_dDPS(0.0),
	m_iDamageTaken(0),
	m_dMobDPS(0.0)
{
}

double CombatMobRecord::getDPS()
{
	int iTimeElapsed = (m_iLastTime - m_iStartTime) / 1000;

	if (iTimeElapsed > 0)
	{
		m_dDPS = (double)m_iDamageGiven / (double)iTimeElapsed;
	}

	return m_dDPS;
}

double CombatMobRecord::getMobDPS()
{
	int iTimeElapsed = (m_iLastTime - m_iStartTime) / 1000;

	if (iTimeElapsed > 0)
	{
		m_dMobDPS = (double)m_iDamageTaken / (double)iTimeElapsed;
	}

	return m_dMobDPS;
}

void CombatMobRecord::addHit(int iTarget, int iSource, int iDamage)
{
	int iPlayerID = m_player->id();

	if (iSource == iPlayerID && iTarget == m_iID)
	{
		//	update m_iLastTime
		m_iLastTime = mTime();

		if (iDamage > 0)
		{
			m_iDamageGiven += iDamage;
		}
	}
	else if (iSource == m_iID && iTarget == iPlayerID)
	{
		//	update m_iLastTime
		m_iLastTime = mTime();

		if (iDamage > 0)
		{
			m_iDamageTaken += iDamage;
		}
	}
}

////////////////////////////////////////////
//	CombatWindow implementation
////////////////////////////////////////////

CombatWindow::~CombatWindow()
{
	if (m_combat_defense_record != 0)
	{
		delete m_combat_defense_record;
		m_combat_defense_record = 0;
	}
}

CombatWindow::CombatWindow(Player* player, QWidget* parent, const char* name)
  : SEQWindow("Combat", "ShowEQ - Combat", parent, name),
	m_player(player),
	m_iCurrentDPSTotal(0),
	m_iDPSStartTime(0),
	m_iDPSTimeLast(0),
	m_dDPS(0.0),
	m_dDPSLast(0.0)
{

	/* Hopefully this is only called once to set up the window,
     so this is a good place to initialize some things which
     otherwise won't be. */

	m_combat_offense_list.setAutoDelete(true);
	m_combat_defense_record = new CombatDefenseRecord(player);
	m_combat_mob_list.setAutoDelete(true);

	initUI();
}

void CombatWindow::initUI()
{
#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::initUI: starting...");
#endif

	m_tab = new QTabWidget();

	m_offenseTab = initOffenseWidget();
	m_tab->addTab(m_offenseTab, "&Offense");

	m_defenseTab = initDefenseWidget();
	m_tab->addTab(m_defenseTab, "&Defense");

	m_mobTab = initMobWidget();
	m_tab->addTab(m_mobTab, "&Mobs");

	//m_menuBar = new QMenuBar(this);
	//m_clearMenu = new Q3PopupMenu(this);
	//m_clearMenu->insertItem("Clear Offense Stats", this, SLOT(clearOffense()));
	//m_clearMenu->insertItem("Clear Mob Stats", this, SLOT(clearMob()));
	//m_menuBar->insertItem("&Clear", m_clearMenu);

	//QPushButton* m_clearButton = new QPushButton("Clear");
	//connect(m_clearButton, SIGNAL(clicked()), this, SLOT(clearOffense()));
	//connect(m_clearButton, SIGNAL(clicked()), this, SLOT(clearMob()));

	//QVBoxLayout* mainLayout = new QVBoxLayout();
	//mainLayout->addWidget(m_tab);
	//mainLayout->addWidget(m_clearButton);

	updateOffense();
	updateDefense();
	updateMob();

	//QWidget* layoutWidget = new QWidget();
	//layoutWidget->setLayout(mainLayout);
	//setWidget(layoutWidget);
	setWidget(m_tab);

#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::initUI: finished...");
#endif
}

QWidget* CombatWindow::initOffenseWidget()
{
	// Create the list view widget
	m_offenseListView = new SEQListView(preferenceName());
	m_offenseListView->addColumn("Type");
	m_offenseListView->setColumnAlignment(0, Qt::AlignRight);
	m_offenseListView->addColumn("Hit");
	m_offenseListView->setColumnAlignment(1, Qt::AlignRight);
	m_offenseListView->addColumn("Miss");
	m_offenseListView->setColumnAlignment(2, Qt::AlignRight);
	m_offenseListView->addColumn("Ratio");
	m_offenseListView->setColumnAlignment(3, Qt::AlignRight);
	m_offenseListView->addColumn("Avg");
	m_offenseListView->setColumnAlignment(4, Qt::AlignRight);
	m_offenseListView->addColumn("Min");
	m_offenseListView->setColumnAlignment(5, Qt::AlignRight);
	m_offenseListView->addColumn("Max");
	m_offenseListView->setColumnAlignment(6, Qt::AlignRight);
	m_offenseListView->addColumn("Total");
	m_offenseListView->setColumnAlignment(7, Qt::AlignRight);
	m_offenseListView->restoreColumns();
	m_offenseListView->setMinimumSize(m_offenseListView->sizeHint().width(), 200);

	// lay out the widgets in a grid
	QGridLayout *summaryGrid = new QGridLayout();

	// Total damage
	summaryGrid->addWidget(new QLabel("Total Damage:"), 0, 0);
	m_offenseTotalDamage = new QLabel();
	summaryGrid->addWidget(m_offenseTotalDamage, 0, 1);

	// Average melee
	summaryGrid->addWidget(new QLabel("Avg Melee:"), 0, 2);
	m_offenseAvgMelee = new QLabel();
	summaryGrid->addWidget(m_offenseAvgMelee, 0, 3);

	// % from special
	summaryGrid->addWidget(new QLabel("% from Special:"), 1, 0);
	m_offensePercentSpecial = new QLabel();
	summaryGrid->addWidget(m_offensePercentSpecial, 1, 1);

	// Avg Special
	summaryGrid->addWidget(new QLabel("Avg Special:"), 1, 2);
	m_offenseAvgSpecial = new QLabel();
	summaryGrid->addWidget(m_offenseAvgSpecial, 1, 3);

	// % from non melee
	summaryGrid->addWidget(new QLabel("% from NonMelee:"), 2, 0);
	m_offensePercentNonMelee = new QLabel();
	summaryGrid->addWidget(m_offensePercentNonMelee, 2, 1);

	// Avg non melee
	summaryGrid->addWidget(new QLabel("Avg NonMelee:"), 2, 2);
	m_offenseAvgNonMelee = new QLabel();
	summaryGrid->addWidget(m_offenseAvgNonMelee, 2, 3);

	summaryGrid->setColStretch(1, 1);
	summaryGrid->setColStretch(3, 1);
	summaryGrid->setSpacing(5);

	QGroupBox* summaryWidget = new QGroupBox("Summary");
	summaryWidget->setLayout(summaryGrid);

	m_offenseLayout = new QVBoxLayout();
	m_offenseLayout->addWidget(m_offenseListView);
	m_offenseLayout->addWidget(summaryWidget);

	QWidget* layoutWidget = new QWidget();
	layoutWidget->setLayout(m_offenseLayout);

	return layoutWidget;
}

QWidget* CombatWindow::initDefenseWidget()
{
	QGridLayout* avoidanceLayout = new QGridLayout();

	// Misses
	avoidanceLayout->addWidget(new QLabel("Misses:"), 0, 0);
	m_defenseAvoidMisses = new QLabel();
	avoidanceLayout->addWidget(m_defenseAvoidMisses, 0, 1);

	// Blocks
	avoidanceLayout->addWidget(new QLabel("Blocks:"), 0, 2);
	m_defenseAvoidBlock = new QLabel();
	avoidanceLayout->addWidget(m_defenseAvoidBlock, 0, 3);

	// Parries
	avoidanceLayout->addWidget(new QLabel("Parries:"), 0, 4);
	m_defenseAvoidParry = new QLabel();
	avoidanceLayout->addWidget(m_defenseAvoidParry, 0, 5);

	// Ripostes
	avoidanceLayout->addWidget(new QLabel("Ripostes:"), 1, 0);
	m_defenseAvoidRiposte = new QLabel();
	avoidanceLayout->addWidget(m_defenseAvoidRiposte, 1, 1);

	// Dodges
	avoidanceLayout->addWidget(new QLabel("Dodges"), 1, 2);
	m_defenseAvoidDodge = new QLabel();
	avoidanceLayout->addWidget(m_defenseAvoidDodge, 1, 3);

	// Total
	avoidanceLayout->addWidget(new QLabel("Total:"), 1, 4);
	m_defenseAvoidTotal = new QLabel();
	avoidanceLayout->addWidget(m_defenseAvoidTotal, 1, 5);

	avoidanceLayout->setColStretch(1, 1);
	avoidanceLayout->setColStretch(3, 1);
	avoidanceLayout->setColStretch(5, 1);
	avoidanceLayout->setSpacing(5);

	QGroupBox* avoidanceBox = new QGroupBox("Avoidance");
	avoidanceBox->setLayout(avoidanceLayout);


	QGridLayout* mitigationLayout = new QGridLayout();

	// Avg Hit
	mitigationLayout->addWidget(new QLabel("Avg. Hit:"), 0, 0);
	m_defenseMitigateAvgHit = new QLabel();
	mitigationLayout->addWidget(m_defenseMitigateAvgHit, 0, 1);

	// Min Hit:
	mitigationLayout->addWidget(new QLabel("Min Hit:"), 0, 2);
	m_defenseMitigateMinHit = new QLabel();
	mitigationLayout->addWidget(m_defenseMitigateMinHit, 0, 3);

	// Max Hit:
	mitigationLayout->addWidget(new QLabel("Max Hit:"), 0, 4);
	m_defenseMitigateMaxHit = new QLabel();
	mitigationLayout->addWidget(m_defenseMitigateMaxHit, 0, 5);

	mitigationLayout->setColStretch(1, 1);
	mitigationLayout->setColStretch(3, 1);
	mitigationLayout->setColStretch(5, 1);
	mitigationLayout->setSpacing(5);

	QGroupBox* mitigationBox = new QGroupBox("Mitigation");
	mitigationBox->setLayout(mitigationLayout);

	QGridLayout* summaryLayout = new QGridLayout();

	// Mob Attacks
	summaryLayout->addWidget(new QLabel("Mob Attacks:"), 0, 0);
	m_defenseMobAttacks = new QLabel();
	summaryLayout->addWidget(m_defenseMobAttacks, 0, 1);

	// Percent Avoided
	summaryLayout->addWidget(new QLabel("% Avoided:"), 0, 2);
	m_defensePercentAvoided = new QLabel();
	summaryLayout->addWidget(m_defensePercentAvoided, 0, 3);

	// Total damage
	summaryLayout->addWidget(new QLabel("Total Damage:"), 0, 4);
	m_defenseTotalDamage = new QLabel();
	summaryLayout->addWidget(m_defenseTotalDamage, 0, 5);

	summaryLayout->setColStretch(1, 1);
	summaryLayout->setColStretch(3, 1);
	summaryLayout->setColStretch(5, 1);
	summaryLayout->setSpacing(5);

	QGroupBox* summaryBox = new QGroupBox("Summary");
	summaryBox->setLayout(summaryLayout);

	m_defenseLayout = new QVBoxLayout();
	m_defenseLayout->addWidget(avoidanceBox);
	m_defenseLayout->addWidget(mitigationBox);
	m_defenseLayout->addWidget(summaryBox);

	QWidget* layoutWidget = new QWidget();
	layoutWidget->setLayout(m_defenseLayout);
	return layoutWidget;
}

QWidget* CombatWindow::initMobWidget()
{
	m_mobListView = new SEQListView(preferenceName());
	m_mobListView->addColumn("Time");
	m_mobListView->setColumnAlignment(0, Qt::AlignRight);
	m_mobListView->addColumn("Name");
	m_mobListView->setColumnAlignment(1, Qt::AlignRight);
	m_mobListView->addColumn("ID");
	m_mobListView->setColumnAlignment(2, Qt::AlignRight);
	m_mobListView->addColumn("Duration");
	m_mobListView->setColumnAlignment(3, Qt::AlignRight);
	m_mobListView->addColumn("Damage Given");
	m_mobListView->setColumnAlignment(4, Qt::AlignRight);
	m_mobListView->addColumn("DPS");
	m_mobListView->setColumnAlignment(5, Qt::AlignRight);
	m_mobListView->addColumn("Damage Taken");
	m_mobListView->setColumnAlignment(6, Qt::AlignRight);
	m_mobListView->addColumn("MOB DPS");
	m_mobListView->setColumnAlignment(7, Qt::AlignRight);
	m_mobListView->restoreColumns();
	m_mobListView->setMinimumSize(m_mobListView->sizeHint().width(), 200);

	QGridLayout* summaryLayout = new QGridLayout();

	// Total mobs
	summaryLayout->addWidget(new QLabel("Total Mobs"), 0, 0);
	m_mobTotalMobs = new QLabel();
	summaryLayout->addWidget(m_mobTotalMobs, 0, 1);

	// Avg dps
	summaryLayout->addWidget(new QLabel("Avg DPS:"), 0, 2);
	m_mobAvgDps = new QLabel();
	summaryLayout->addWidget(m_mobAvgDps, 0, 3);

	// current dps
	summaryLayout->addWidget(new QLabel("Current DPS:"), 1, 0);
	m_mobCurrentDps = new QLabel();
	summaryLayout->addWidget(m_mobCurrentDps, 1, 1);

	// last dps
	summaryLayout->addWidget(new QLabel("Last DPS:"), 1, 2);
	m_mobLastDps = new QLabel();
	summaryLayout->addWidget(m_mobLastDps, 1, 3);

	summaryLayout->setColStretch(1, 1);
	summaryLayout->setColStretch(3, 1);
	summaryLayout->setSpacing(5);

	QGroupBox* summaryBox = new QGroupBox("Summary");
	summaryBox->setLayout(summaryLayout);

	m_mobLayout = new QVBoxLayout();
	m_mobLayout->addWidget(m_mobListView);
	m_mobLayout->addWidget(summaryBox);

	QWidget* layoutWidget = new QWidget();
	layoutWidget->setLayout(m_mobLayout);
	return layoutWidget;
}

void CombatWindow::savePrefs()
{
	// save the SEQWindow's prefs
	SEQWindow::savePrefs();

	// save the SEQListViews' prefs
	m_mobListView->savePrefs();
	m_offenseListView->savePrefs();
}

void CombatWindow::updateOffense()
{
#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::updateOffense starting...");
#endif

	QString s_totaldamage;
	QString s_percentspecial;
	QString s_percentnonmelee;
	QString s_avgmelee;
	QString s_avgspecial;
	QString s_avgnonmelee;

	int iTotalDamage = 0;
	int iTotalHits = 0;
	double dPercentSpecial = 0.0;
	double dPercentNonmelee = 0.0;
	double dAvgMelee = 0.0;
	double dAvgSpecial = 0.0;
	double dAvgNonmelee = 0.0;

	int iMeleeDamage = 0;
	int iMeleeHits = 0;
	int iSpecialDamage = 0;
	int iSpecialHits = 0;
	int iNonmeleeDamage = 0;
	int iNonmeleeHits = 0;

	//	empty the list so we can repopulate
	m_offenseListView->clear();

	CombatOffenseRecord *pRecord;

	for (pRecord = m_combat_offense_list.first(); pRecord != 0; pRecord = m_combat_offense_list.next())
	{
		int iType = pRecord->getType();
		int iSpell = pRecord->getSpell();
		int iHits = pRecord->getHits();
		int iMisses = pRecord->getMisses();
		int iMinDamage = pRecord->getMinDamage();
		int iMaxDamage = pRecord->getMaxDamage();
		int iDamage = pRecord->getTotalDamage();

		double dAvgDamage, dRatio;

		if (iHits != 0)
			dAvgDamage = (double)iDamage / (double)iHits;
		else
			dAvgDamage = 0.0;

		if (iMisses != 0)
			dRatio = (double)iHits / (double)iMisses;
		else
			dRatio = 0.0;

		QString s_type;
		// Belith -- Damage shields are strange!
		switch (iType)
		{
			case 0:		// 1H Blunt
			case 1:		// 1H Slashing
			case 2:		// 2H Blunt
			case 3:		// 2H Slashing
			case 28:	// Hand To Hand
			case 36:	// Piercing
			case 7:		// Archery
			case 8:		// Backstab
			case 10:	// Bash
			case 21:	// Dragon Punch
			case 23:	// Eagle Strike
			case 26:	// Flying Kick
			case 30:	// Kick
			case 38:	// Round Kick
			case 51:	// Throwing
			case 52:	// Tiger Claw
				// this is a normal skill
				s_type.sprintf("%s(%d)", (const char*)skill_name(iType), iType);
				break;

			case 231:       // Non Melee Damage
				s_type.sprintf("Spell: %s(%d)", (const char*)spell_name(iSpell), iSpell);
				break;

			default:    // Damage Shield?
				// 245 Mark of Retribution
				// 248 Flameshield of Ro? (45pt) (mage)
				// -11 Killing Blow with MoR
				// -8  Killing Blow with Ro? (45pt) (mage)
				s_type.sprintf("Damage Shield: (%d)", iType);
				break;
		}

		QString s_hits;
		s_hits.setNum(iHits);
		QString s_misses;
		s_misses.setNum(iMisses);
		QString s_ratio;
		s_ratio = QString("%1 to 1").arg(dRatio);
		QString s_avgdamage;
		s_avgdamage.setNum(dAvgDamage);
		QString s_mindamage;
		s_mindamage.setNum(iMinDamage);
		QString s_maxdamage;
		s_maxdamage.setNum(iMaxDamage);
		QString s_damage;
		s_damage.setNum(iDamage);

		Q3ListViewItem *pItem = new Q3ListViewItem(m_offenseListView,
				s_type, s_hits, s_misses, s_ratio, s_avgdamage, s_mindamage,
				s_maxdamage, s_damage);
		m_offenseListView->insertItem(pItem);

		switch (iType)
		{
			case 0:		// 1H Blunt
			case 1:		// 1H Slashing
			case 2:		// 2H Blunt
			case 3:		// 2H Slashing
			case 28:	// Hand To Hand
			case 36:	// Piercing
				iMeleeDamage += iDamage;
				iMeleeHits += iHits;
				break;

			case 7:		// Archery
			case 8:		// Backstab
			case 10:	// Bash
			case 21:	// Dragon Punch
			case 23:	// Eagle Strike
			case 26:	// Flying Kick
			case 30:	// Kick
			case 38:	// Round Kick
			case 51:	// Throwing
			case 52:	// Tiger Claw
				iSpecialDamage += iDamage;
				iSpecialHits += iHits;
				break;

			default:
				iNonmeleeDamage += iDamage;
				iNonmeleeHits += iHits;
				break;
		}
	}

	iTotalDamage = iMeleeDamage + iSpecialDamage + iNonmeleeDamage;
	iTotalHits = iMeleeHits + iSpecialHits + iNonmeleeHits;

	if (iTotalDamage != 0)
	{
		dPercentSpecial = ((double)iSpecialDamage / (double)iTotalDamage) * 100.0;
		dPercentNonmelee = ((double)iNonmeleeDamage / (double)iTotalDamage) * 100.0;
	}
	else
	{
		dPercentSpecial = 0.0;
		dPercentNonmelee = 0.0;
	}

	if (iMeleeHits != 0)
		dAvgMelee = (double)iMeleeDamage / (double)iMeleeHits;
	else
		dAvgMelee = 0.0;

	if (iSpecialHits != 0)
		dAvgSpecial = (double)iSpecialDamage / (double)iSpecialHits;
	else
		dAvgSpecial = 0.0;

	if (iNonmeleeHits != 0)
		dAvgNonmelee = (double)iNonmeleeDamage / (double)iNonmeleeHits;
	else
		dAvgNonmelee = 0.0;

	s_totaldamage.setNum(iTotalDamage);
	s_percentspecial.setNum(dPercentSpecial);
	s_percentnonmelee.setNum(dPercentNonmelee);
	s_avgmelee.setNum(dAvgMelee);
	s_avgspecial.setNum(dAvgSpecial);
	s_avgnonmelee.setNum(dAvgNonmelee);

	m_offenseTotalDamage->setText(s_totaldamage);
	m_offensePercentSpecial->setText(s_percentspecial);
	m_offensePercentNonMelee->setText(s_percentnonmelee);
	m_offenseAvgMelee->setText(s_avgmelee);
	m_offenseAvgSpecial->setText(s_avgspecial);
	m_offenseAvgNonMelee->setText(s_avgnonmelee);


#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::updateOffense finished...");
#endif

}

void CombatWindow::updateDefense()
{
	int iMisses = m_combat_defense_record->getMisses();
	int iBlocks = m_combat_defense_record->getBlocks();
	int iParries = m_combat_defense_record->getParries();
	int iRipostes = m_combat_defense_record->getRipostes();
	int iDodges = m_combat_defense_record->getDodges();
	int iTotalAvoid = iMisses+iBlocks+iParries+iRipostes+iDodges;

	double dAvgHit, dAvoided;
	int hits = m_combat_defense_record->getHits();
	if (hits != 0)
		dAvgHit = (double)m_combat_defense_record->getTotalDamage() / (double)hits;
	else
		dAvgHit = 0.0;

	int iMinHit = m_combat_defense_record->getMinDamage();
	int iMaxHit = m_combat_defense_record->getMaxDamage();

	int iMobAttacks = m_combat_defense_record->getTotalAttacks();
	if (iMobAttacks != 0)
		dAvoided = ((double)iTotalAvoid / (double)iMobAttacks) * 100.0;
	else
		dAvoided = 0.0;
	int iTotalDamage = m_combat_defense_record->getTotalDamage();

	m_defenseAvoidMisses->setText(QString::number(iMisses));
	m_defenseAvoidBlock->setText(QString::number(iBlocks));
	m_defenseAvoidParry->setText(QString::number(iParries));
	m_defenseAvoidRiposte->setText(QString::number(iRipostes));
	m_defenseAvoidDodge->setText(QString::number(iDodges));
	m_defenseAvoidTotal->setText(QString::number(iTotalAvoid));
	m_defenseMitigateAvgHit->setText(QString::number(dAvgHit));
	m_defenseMitigateMinHit->setText(QString::number(iMinHit));
	m_defenseMitigateMaxHit->setText(QString::number(iMaxHit));
	m_defenseMobAttacks->setText(QString::number(iMobAttacks));
	m_defensePercentAvoided->setText(QString::number(dAvoided));
	m_defenseTotalDamage->setText(QString::number(iTotalDamage));
}

void CombatWindow::updateMob()
{

	int iTotalMobs = 0;
	double dAvgDPS = 0.0;
	double dDPSSum = 0.0;

	//	empty the list so we can repopulate
	m_mobListView->clear();

	CombatMobRecord *pRecord;

	for (pRecord = m_combat_mob_list.first(); pRecord != 0; pRecord = m_combat_mob_list.next())
	{
		int iID = pRecord->getID();
		int iDuration = pRecord->getDuration() / 1000;
		int iDamageGiven = pRecord->getDamageGiven();
		double dDPS = pRecord->getDPS();
		int iDamageTaken = pRecord->getDamageTaken();
		double dMobDPS = pRecord->getMobDPS();

		char s_time[64];
		time_t timev = pRecord->getTime();
		strftime(s_time, 64, "%m/%d %H:%M:%S", localtime(&timev));
		QString s_name = pRecord->getName();
		QString s_id = QString::number(iID);
		QString s_duration = QString::number(iDuration);
		QString s_damagegiven = QString::number(iDamageGiven);
		QString s_dps = QString::number(dDPS);
		QString s_iDamageTaken = QString::number(iDamageTaken);
		QString s_mobdps = QString::number(dMobDPS);


		Q3ListViewItem *pItem = new Q3ListViewItem(m_mobListView,
				s_time, s_name, s_id, s_duration, s_damagegiven,
				s_dps, s_iDamageTaken, s_mobdps);
		m_mobListView->insertItem(pItem);

		iTotalMobs++;
		dDPSSum += dDPS;
	}

	if (iTotalMobs)
		dAvgDPS = dDPSSum / (double)iTotalMobs;
	else
		dAvgDPS = 0;

	m_mobTotalMobs->setText(QString::number(iTotalMobs));
	m_mobAvgDps->setText(QString::number(dAvgDPS));
	m_mobCurrentDps->setText(QString::number(m_dDPS));
	m_mobLastDps->setText(QString::number(m_dDPSLast));
}

void CombatWindow::addCombatRecord(int iTargetID, int iSourceID, int iType, int iSpell, int iDamage, QString tName, QString sName)
{
#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addCombatRecord starting...");
	seqDebug("target=%d, source=%d, type=%d, spell=%d, damage=%d",
			 iTargetID, iSourceID, iType, iSpell, iDamage);
#endif

	int iPlayerID = m_player->id();

	//	The one case we won't handle (for now) is where the Target
	//	and Source are the same.

	if (iTargetID == iPlayerID && iSourceID != iPlayerID)
	{
		addDefenseRecord(iDamage);
		updateDefense();
		addMobRecord(iTargetID, iSourceID, iDamage, tName, sName);
		updateMob();
	}
	else if (iSourceID == iPlayerID && iTargetID != iPlayerID)
	{
		addOffenseRecord(iType, iDamage, iSpell);
		updateOffense();
		// Belith -- Lets not add buffs, etc
		if ((iType == 231 && iDamage > 0) || iType != 231) {
			addMobRecord(iTargetID, iSourceID, iDamage, tName, sName);
			updateMob();
		}
		if (iDamage > 0)
			updateDPS(iDamage);
	}

#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addCombatRecord finished...");
#endif
}

void CombatWindow::addOffenseRecord(int iType, int iDamage, int iSpell)
{
#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addOffenseRecord starting...");
#endif

	bool bFoundRecord = false;

	CombatOffenseRecord *pRecord;
	for (pRecord = m_combat_offense_list.first(); pRecord != 0; pRecord = m_combat_offense_list.next())
	{
		// Belith -- Lets match spells up as well
		if (pRecord->getType() == iType && pRecord->getType() != 231)
		{
			bFoundRecord = true;
			break;
		}

		if (pRecord->getType() == iType && pRecord->getType() == 231 && pRecord->getSpell() == iSpell)
		{
			bFoundRecord = true;
			break;
		}
	}

	if (!bFoundRecord)
	{
		// Belith -- Again lets skip buffs, etc
		if ((iDamage > 0 && iType == 231) || iType != 231) {
			pRecord = new CombatOffenseRecord(iType, m_player, iSpell);
			m_combat_offense_list.append(pRecord);
		}
	}

	if (iDamage > 0)
	{
		pRecord->addHit(iDamage);
	}
	else if (iType != 231)
	{
		pRecord->addMiss(iDamage);
	}

#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addOffenseRecord finished...");
#endif
}


void CombatWindow::addDefenseRecord(int iDamage)
{
	if (iDamage > 0)
		m_combat_defense_record->addHit(iDamage);
	else
		m_combat_defense_record->addMiss(iDamage);
}

void CombatWindow::addMobRecord(int iTargetID, int iSourceID, int iDamage, QString tName, QString sName)
{
#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addMobRecord starting...");
#endif

	int iTimeNow = mTime();
	int iPlayerID = m_player->id();
	int iMobID;
	QString mobName;

	if (iPlayerID == iTargetID)
	{
		iMobID = iSourceID;
		mobName = sName;
	}
	else if (iPlayerID == iSourceID)
	{
		iMobID = iTargetID;
		mobName = tName;
	}
	else
	{
		//invalid record
		return;
	}


	bool bFoundRecord = false;
	CombatMobRecord *pRecord;

	for (pRecord = m_combat_mob_list.first(); pRecord != 0; pRecord = m_combat_mob_list.next())
	{
		if (pRecord->getID() == iMobID)
		{
			bFoundRecord = true;
			break;
		}
	}

	if (!bFoundRecord)
	{
		pRecord = new CombatMobRecord(iMobID, iTimeNow, m_player);
		pRecord->setName(mobName);
		m_combat_mob_list.append(pRecord);
	}
	pRecord->setTime(time(0));
	pRecord->addHit(iTargetID, iSourceID, iDamage);

#ifdef DEBUGCOMBAT
	seqDebug("CombatWindow::addMobRecord finished...");
#endif
}


void CombatWindow::updateDPS(int iDamage)
{
	int iTimeNow = mTime();

	//	reset if it's been 10 seconds without an update
	if (iTimeNow > (m_iDPSTimeLast + 10000))
	{
		//	reset DPS
		m_dDPSLast = m_dDPS;
		m_dDPS = 0;
		m_iDPSStartTime = iTimeNow;
		m_iCurrentDPSTotal = 0;
	}

	m_iDPSTimeLast = mTime();
	m_iCurrentDPSTotal += iDamage;

	int iTimeElapsed = (iTimeNow - m_iDPSStartTime) / 1000;

	if (iTimeElapsed > 0)
	{
		m_dDPS = (double)m_iCurrentDPSTotal / (double)iTimeElapsed;
	}

	m_mobCurrentDps->setText(QString::number(m_dDPS));
	m_mobLastDps->setText(QString::number(m_dDPSLast));
}

void CombatWindow::resetDPS()
{
	//	we'll let updateDPS do all the work
	//	by simply setting m_iDPSTimeLast to 0
	m_iDPSTimeLast = 0;
	updateDPS(0);
}

void CombatWindow::clearMob()
{
	switch (QMessageBox::information(this, "ShowEQ",
		"This function will clear all data listed on the mob "
		"tab.  Do you want to continue?",
		"&OK", "&Cancel", QString::null, 1, 1))
	{
		case 0:
			m_combat_mob_list.clear();
			updateMob();
			break;

		default:
			break;
	}
}

void CombatWindow::clearOffense()
{
	switch (QMessageBox::information(this, "ShowEQ",
		"This function will clear all data listed on the offense "
		"tab.  Do you want to continue?",
		"&OK", "&Cancel", QString::null, 1, 1))
	{
		case 0:
			m_combat_offense_list.clear();
			updateOffense();
			break;

		default:
			break;
	}
}

void CombatWindow::clear()
{
	m_combat_mob_list.clear();
	updateMob();
	m_combat_offense_list.clear();
	updateOffense();
	m_combat_defense_record->clear();
	updateDefense();
	resetDPS();
}


//////////////////////////////////////////////////////////////////////////
