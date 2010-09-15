/*
 * spawnlist3.h
 *
 * ShowEQ Distributed under GPL
 * http://www.hackersquest.gomp.ch/
 */

#ifndef SPAWNLIST3_H
#define SPAWNLIST3_H

#include <SEQWindow.h>
#include "ui_spawnlist3.h"

// Forward Declarations
class Player;
class SpawnShell;
class SpawnModel;
class CategoryMgr;
class Category;
class Item;

class SpawnListWindow3 : public SEQWindow
{
	Q_OBJECT

public:
	SpawnListWindow3(Player* player, SpawnShell* spawnShell, CategoryMgr* categoryMgr, QWidget *parent = 0);
	~SpawnListWindow3();


private:
	// Options - these might not be necessary to keep
	bool	m_immedateUpdate;
	bool	m_keepSorted;
	bool	m_keepSelectedVisible;

	// Data Sources
	Player*			m_player;
	CategoryMgr*	m_categoryMgr;
	SpawnShell*		m_spawnShell;
	SpawnModel*		m_spawnModel;

	// This may be migrated to another model
	Category*		m_currentCategory;

	// Currently selected item
	const Item* m_selectedItem;

	// UI Implementation
	Ui::SpawnList3Class ui;

public slots:
	void updateCount();

};

#endif // SPAWNLIST3_H
