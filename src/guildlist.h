/*
 * guildlist.h
 *
 *  ShowEQ Distributed under GPL
 *  http://www.sourceforge.net/projects/seq
 *
 *  Copyright 2004 Zaphod (dohpaz@users.sourceforge.net).
 *
 */

#ifndef _GUILDLIST_H_
#define _GUILDLIST_H_

#ifdef __FreeBSD__
# include <sys/types.h>
#else
# include <stdint.h>
#endif

#include "seqwindow.h"
#include "seqlistview.h"

#include <Q3ListView>
#include <Q3PtrDict>
#include <QString>
#include <QLabel>
#include <QMenu>

//----------------------------------------------------------------------
// forward declarations
class Player;
class ZoneMgr;
class GuildMember;
class GuildShell;

class QLabel;
class QLineEdit;
class QMenu;

//--------------------------------------------------
// constants
const int tGuildListColName = 0;
const int tGuildListColLevel = 1;
const int tGuildListColClass = 2;
const int tGuildListColRank = 3;
const int tGuildListColBank = 4;
const int tGuildListColAlt = 5;
const int tGuildListColLastOn = 6;
const int tGuildListColZone = 7;
const int tGuildListColPublicNote = 8;
const int tGuildListColMaxCols = 9;

//----------------------------------------------------------------------
// GuildListItem
class GuildListItem : public Q3ListViewItem
{
public:
	GuildListItem(Q3ListView* parent, const GuildMember* member, const GuildShell* guildShell);
	virtual ~GuildListItem();

	virtual void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);
	virtual int compare(Q3ListViewItem *i, int col, bool ascending) const;
	void update(const GuildShell* guildShell);

	const GuildMember* guildMember() { return m_member; }
	void setGuildMember(const GuildMember* member);
	virtual int rtti() const;

protected:
	const GuildMember* m_member;
};

//----------------------------------------------------------------------
// GuildListWindow
class GuildListWindow : public SEQWindow
{
	Q_OBJECT

public:
	GuildListWindow(Player* player, GuildShell* guildShell, QWidget* parent = 0, const char* name = 0);
	~GuildListWindow();

	virtual QMenu* menu();

public slots:
	void cleared();
	void loaded();
	void updated(const GuildMember* gm);
	void guildChanged();

protected slots:
	void init_Menu();
	void toggle_showOffline(int);
	void toggle_keepSorted(int);
	void toggle_showAlts(int);
	void toggle_guildListCol(int);
	void set_font(int);
	void set_caption(int);

protected:
	void clear();
	void populate();
	void updateCount();

	Player* m_player;
	GuildShell* m_guildShell;

	QLabel* m_guildName;
	QLabel* m_guildTotals;
	SEQListView* m_guildList;
	Q3PtrDict<GuildListItem> m_guildListItemDict;
	QMenu* m_menu;
	int m_id_guildList_Cols[tGuildListColMaxCols];

	uint32_t m_membersOn;
	bool m_showOffline;
	bool m_showAlts;
	bool m_keepSorted;
};

#endif // _GUILDLIST_H_
