/*
 * spawnlist2.h
 *
 * ShowEQ Distributed under GPL
 * http://www.hackersquest.gomp.ch/
 */

#ifndef SPAWNLIST2_H
#define SPAWNLIST2_H

#include <QHash>
#include <QTableView>
#include <QTreeView>

#include "seqwindow.h"
#include "seqlistview.h"
#include "spawnlistcommon.h"

//#define NEW_SPAWNLIST

//--------------------------------------------------
// forward declarations
class Category;
class CategoryMgr;
class Player;
class SpawnShell;
class FilterMgr;

class QComboBox;
class QTimer;
class QSpinBox;
class QLabel;

class Item;

class SpawnListWindow2 : public SEQWindow
{
	Q_OBJECT

public:
	SpawnListWindow2(Player* player, SpawnShell* spawnShell, CategoryMgr* categoryMgr,
					 QWidget* parent = 0, const char* name = 0);
	~SpawnListWindow2();

	virtual QMenu* menu();

	SpawnListItem* selected();
	SpawnListItem* find(const Item* item);

	QString filterString(const Item* item);

signals:
	void listUpdated();   // flags in spawns have changed
	void listChanged();   // categories have changed
	void spawnSelected(const Item* item);
	void keepUpdated(bool on);

public slots:
	// SpawnShell signals
	void addItem(const Item *);
	void delItem(const Item *);
	void changeItem(const Item *, uint32_t changeType);
	void killSpawn(const Item *);
	void selectSpawn(const Item *);
	void clear();

	// CategoryMgr signals
	void addCategory(const Category* cat);
	void delCategory(const Category* cat);
	void updateCategory(const Category* cat);
	void clearedCategories();
	void loadedCategories();

	// Player signals
	void playerLevelChanged(uint8_t);
	void setPlayer(int16_t x, int16_t y, int16_t z, int16_t deltaX, int16_t deltaY, int16_t deltaZ, int32_t degrees);

	void rebuildSpawnList();
	void refresh();
	virtual void savePrefs();

private slots:
	// category combo box signals
	void categorySelected(int index);

    // listview signals
	void selChanged(Q3ListViewItem*);

	void mousePressEvent (int button, Q3ListViewItem *litem, const QPoint &point, int col);
	void mouseDoubleClickEvent(Q3ListViewItem *litem);

	// fpm spinbox signals
	void setFPM(int rate);

	// additional menu items
	void toggle_immediateUpdate(int id);
	void toggle_keepSorted(int id);
	void toggle_keepSelectedVisible(int id);

private:
	void setSelectedQuiet(Q3ListViewItem* item, bool selected);
	void populateSpawns();
	void populateCategory(const Category* cat);
	void updateCount();

	// data sources
	Player *m_player;
	CategoryMgr* m_categoryMgr;
	SpawnShell* m_spawnShell;

	// category currently being viewed
	Category* m_currentCategory;

	// currently selected item
	const Item* m_selectedItem;

	// GUI Items
	QComboBox*		m_categoryCombo;
	QSpinBox*		m_fpmSpinBox;
#ifndef NEW_SPAWNLIST
	SEQListView*	m_spawnList;
#else
	QTreeView*		m_tableView;
#endif
	SpawnListMenu*	m_menu;
	QLineEdit*		m_totalSpawns;


	// index dictionary for retrieving SpawnListItems by Item
	QHash<const Item*, SpawnListItem*> m_spawnListItemDict;

	// timer used
	QTimer* m_timer;

	// frames per minute (how many times spawnlist is updated per minute)
	int m_delay;
	time_t m_lastUpdate;

	// whether to immediately update the spawn list as changes occur, or
	// use timer.
	bool m_immediateUpdate;
	bool m_keepSorted;
	bool m_keepSelectedVisible;
};

#endif // SPAWNLIST2_H
