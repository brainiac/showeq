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

	const Item* item(int index) { return m_items.at(index); }
	const Item* item(const QModelIndex& index) { return index.isValid() ? m_items.at(index.row()) : NULL; }

public slots:
	void addItem(const Item* item);
	void delItem(const Item* item);
	void changeItem(const Item* item, uint32_t changeType);
	void reset();

	void updatePosition(int16_t x, int16_t y, int16_t z, int16_t deltaX, int16_t deltaY, int16_t deltaZ, int32_t degrees);
	void updateLevel(uint8_t level);

protected:
	int findItemIndex(const Item* item);
	QColor pickTextColor(const Item* item) const;

	Player*					m_player;
	QList<const Item*>		m_items;
	QColor					m_defaultColor;
};

#endif // SPAWNMODEL_H
