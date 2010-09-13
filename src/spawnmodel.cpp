/*
 * spawnmodel.cpp
 *
 *  ShowEQ Distributed under GPL
 *  http://www.sourceforge.net/projects/seq
 *
 *  Copyright 2000-2010 by the respective ShowEQ Developers
 */


#include "pch.h"

#include "main.h"
#include "spawnmodel.h"

SpawnModel::SpawnModel(Player* player, QObject *parent)
	: QAbstractTableModel(parent), m_player(player)
{
	m_defaultColor = Qt::black;

}

SpawnModel::~SpawnModel()
{

}

QVariant SpawnModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	const Item* item = m_items.at(index.row());
	const Spawn* spawn = NULL;

	if (item->type() == tSpawn || item->type() == tPlayer)
		spawn = (const Spawn*)item;

	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case tSpawnColName:
			{
				QString name = showeq_params->showRealName ? item->name() : item->transformedName();
				
				if (spawn != NULL)
				{
					if (!spawn->lastName().isEmpty())
						name += QString(" (%2)").arg(spawn->lastName());
					if (spawn->gm())
						name += " *GM*";
				}

				return name;
			}

		case tSpawnColLevel:
			if (spawn != NULL)
				return spawn->level();
			return QVariant();

		case tSpawnColHP:
			if (spawn != NULL)
				return spawn->HP();
			return QVariant();

		case tSpawnColMaxHP:
			if (spawn != NULL)
				return spawn->maxHP();
			return QVariant();

		case tSpawnColXPos:
			return showeq_params->retarded_coords ? (int)item->y() : (int)item->x();

		case tSpawnColYPos:
			return showeq_params->retarded_coords ? (int)item->x() : (int)item->y();

		case tSpawnColZPos:
			return (int)item->z();

		case tSpawnColID:
			return (int)item->id();

		case tSpawnColDist:
			return showeq_params->fast_machine ? item->getFDistanceToPlayer() : item->getIDistanceToPlayer();

		case tSpawnColRace:
			return item->raceString();

		case tSpawnColClass:
			return item->classString();

		case tSpawnColInfo:
			return item->info();

		case tSpawnColSpawnTime:
			return item->spawnTimeStr();

		case tSpawnColDeity:
			return spawn != NULL ? spawn->deityName() : QVariant();

		case tSpawnColBodyType:
			return spawn != NULL ? spawn->typeString() : QVariant();

		case tSpawnColGuildID:
			if (spawn != NULL)
			{
				if (spawn->guildID() < MAX_GUILDS)
					return spawn->guildTag().isEmpty() ? QVariant(spawn->guildID()) : QVariant(spawn->guildTag());
			}
			return QVariant();

		default: Q_ASSERT(false);
		}
	}
	else if (role == Qt::ForegroundRole)
	{
		return QBrush(pickTextColor(item));
	}
	else if (role == Qt::FontRole)
	{
		// TODO: Change me
		QFont font;
		font.setPointSize(8);

		return font;
	}

	return QVariant();
}

QVariant SpawnModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
	{
		switch (section)
		{
		case tSpawnColName:
			return "Name";
		case tSpawnColLevel:
			return "Lvl";
		case tSpawnColHP:
			return "HP";
		case tSpawnColMaxHP:
			return "MaxHP";
		case tSpawnColXPos:
			return showeq_params->retarded_coords ? "N/S" : "X";

		case tSpawnColYPos:
			return showeq_params->retarded_coords ? "E/W" : "Y";

		case tSpawnColZPos:
			return "Z";
		case tSpawnColID:
			return "ID";
		case tSpawnColDist:
			return "Dist";
		case tSpawnColRace:
			return "Race";
		case tSpawnColClass:
			return "Class";
		case tSpawnColInfo:
			return "Info";
		case tSpawnColSpawnTime:
			return "SpawnTime";
		case tSpawnColDeity:
			return "Deity";
		case tSpawnColBodyType:
			return "Body Type";
		case tSpawnColGuildID:
			return "Guild Tag";
		default: Q_ASSERT(false);
		}
	}
	return section + 1;
}

int SpawnModel::rowCount(const QModelIndex& index) const
{
	return index.isValid() ? 0 : m_items.count();
}

int SpawnModel::columnCount(const QModelIndex& index) const
{
	return index.isValid() ? 0 : tSpawnColMaxCols;
}

void SpawnModel::addItem(const Item* item)
{
	int index = m_items.count();
	beginInsertRows(QModelIndex(), index, index);

	m_items.append(item);

	endInsertRows();
}

void SpawnModel::delItem(const Item* item)
{
	int index = findItemIndex(item);
	beginRemoveRows(QModelIndex(), index, index);

	m_items.removeAt(index);

	endRemoveRows();
}

void SpawnModel::changeItem(const Item* item, uint32_t changeType)
{
	int itemIndex = findItemIndex(item);
	emit dataChanged(index(itemIndex, 0),
					 index(itemIndex, tSpawnColMaxCols));
}

void SpawnModel::reset()
{
	beginResetModel();

	m_items.clear();

	endResetModel();
}

int SpawnModel::findItemIndex(const Item* item)
{
	for (int index = 0; index < m_items.count(); index++)
	{
		if (m_items[index] == item)
			return index;
	}
	return -1;
}

QColor SpawnModel::pickTextColor(const Item* item) const
{
	if (item == NULL)
		return m_defaultColor;

	const Spawn* spawn = NULL;
	if (item->type() == tSpawn || item->type() == tPlayer)
		spawn = (const Spawn*)item;

	if (spawn == NULL)
		return m_defaultColor;

	switch (spawn->typeflag())
	{
		case 65:
			return Qt::magenta;
		case 66:
		case 67:
		case 100:
			return Qt::darkMagenta;
	}

	if (showeq_params->pvp) // if pvp
	{
		switch (spawn->raceTeam())
		{
		case RTEAM_HUMAN:
			return Qt::blue;

		case RTEAM_ELF:
			return QColor(196,206,12);

		case RTEAM_DARK:
			return QColor(206,151,33);

		case RTEAM_SHORT:
			return Qt::magenta;
		}
	}
	else if (showeq_params->deitypvp) // if deitypvp
	{
		switch (spawn->deityTeam())
		{
		case DTEAM_GOOD:
			return Qt::blue;

		case DTEAM_NEUTRAL:
			return QColor(196,206,12);

		case DTEAM_EVIL:
			return Qt::magenta;
		}
	}

	QColor color = m_player->pickConColor(spawn->level());

	if (color == Qt::white)
		return Qt::black;

	if (color == Qt::yellow)
		return QColor(206, 151, 33);

	return color;
}