/*
 * spawnmodel.h
 *
 * ShowEQ Distributed under GPL
 * http://sourceforge.net/projects/seq/
 */

#ifndef SPAWNMODEL_H
#define SPAWNMODEL_H

#include "player.h"
#include "spawn.h"
#include "spawnlistcommon.h"

#include <QAbstractTableModel>
#include <QColor>
#include <QList>

class SpawnModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	explicit SpawnModel(Player* player, QObject *parent);
	~SpawnModel();

	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

public slots:
	void addItem(const Item* item);
	void delItem(const Item* item);
	void changeItem(const Item* item, uint32_t changeType);
	void reset();

protected:
	int findItemIndex(const Item* item);
	QColor pickTextColor(const Item* item) const;

	Player*					m_player;
	QList<const Item*>		m_items;
	QColor					m_defaultColor;
};

#endif // SPAWNMODEL_H