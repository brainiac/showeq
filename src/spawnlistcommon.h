/*
 * spawnlist.h
 *
 * ShowEQ Distributed under GPL
 * http://www.hackersquest.gomp.ch/
 */

/*
 * Orig Author - Maerlyn (MaerlynTheWiz@yahoo.com)
 * Date   - 3/16/00
 */

#ifndef SPAWNLISTCOMMON_H
#define SPAWNLISTCOMMON_H

#include "compat.h"

#include <Q3ListView>
#include <QString>
#include <Q3PopupMenu>

#include "spawn.h"

//--------------------------------------------------
// forward declarations
class Category;
class CategoryMgr;
class SpawnShell;
class FilterMgr;
class SEQListView;
class SEQWindow;
class Player;

class SpawnListItem;
class SpawnListMenu;

//--------------------------------------------------
// constants
enum SpawnColumns
{
	tSpawnColName,
	tSpawnColLevel,
	tSpawnColHP,
	tSpawnColMaxHP,
	tSpawnColXPos,
	tSpawnColYPos,
	tSpawnColZPos,
	tSpawnColID,
	tSpawnColDist,
	tSpawnColRace,
	tSpawnColClass,
	tSpawnColInfo,
	tSpawnColSpawnTime,
	tSpawnColDeity,
	tSpawnColBodyType,
	tSpawnColGuildID,
	tSpawnColMaxCols
};

//--------------------------------------------------
// SpawnListItem
class SpawnListItem : public Q3ListViewItem
{
public:
	SpawnListItem(Q3ListViewItem *parent);
	SpawnListItem(Q3ListView *parent);

	virtual ~SpawnListItem();
	virtual void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);

	const QColor textColor()  { return m_textColor; }
	void setTextColor(const QColor &color) { m_textColor = color; }
	void pickTextColor(const Item* item, Player* player, QColor def = Qt::black);
	const Item* item() { return m_item; }

	void update(Player* player, uint32_t changeType);
	void updateTitle(const QString& name);
	void setShellItem(const Item *);
	spawnItemType type();
	virtual int compare(Q3ListViewItem *i, int col, bool ascending) const;

	//--------------------------------------------------
	int m_npc;

private:
	QColor m_textColor;
	const Item *m_item;
};

//--------------------------------------------------
// SpawnListMenu
class SpawnListMenu : public Q3PopupMenu
{
	Q_OBJECT

public:
	SpawnListMenu(SEQListView* spawnlist, SEQWindow* spawnlistWindow, FilterMgr* filterMgr,
				  CategoryMgr* categoryMgr, QWidget* parent = 0, const char* name = 0);
	virtual ~SpawnListMenu();
	void setCurrentCategory(const Category* cat);
	void setCurrentItem(const Item* item);

protected slots:
	void init_Menu();
	void toggle_spawnListCol(int id);
	void add_filter(int id);
	void add_zoneFilter(int id);
	void add_category(int id);
	void edit_category(int id);
	void delete_category(int id);
	void reload_categories(int id);
	void set_font(int id);
	void set_caption(int id);

protected:
	SEQListView* m_spawnlist;
	SEQWindow* m_spawnlistWindow;
	FilterMgr* m_filterMgr;
	CategoryMgr* m_categoryMgr;
	const Category* m_currentCategory;
	const Item* m_currentItem;
	int m_id_filterMenu;
	int m_id_zoneFilterMenu;
	int m_id_spawnList_Cols[tSpawnColMaxCols];
	int m_id_edit_category;
	int m_id_delete_category;
};

#endif // SPAWNLISTCOMMON_H
