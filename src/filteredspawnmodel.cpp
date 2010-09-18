/*
 * filteredspawnmodel.cpp
 *
 *  ShowEQ Distributed under GPL
 *  http://www.sourceforge.net/projects/seq
 *
 *  Copyright 2000-2010 by the respective ShowEQ Developers
 */

#include "pch.h"

#include "filteredspawnmodel.h"
#include "spawnmodel.h"
#include "spawnshell.h"
#include "category.h"
#include "filtermgr.h"

#include "diagnosticmessages.h"


FilteredSpawnModel::FilteredSpawnModel(SpawnShell* spawnShell, QObject *parent)
	: QSortFilterProxyModel(parent)
{
	m_currentCategory = NULL;

	m_spawnShell = spawnShell;
	m_spawnModel = spawnShell->spawnModel();
	this->setSourceModel(m_spawnModel);
}

FilteredSpawnModel::~FilteredSpawnModel()
{

}

void FilteredSpawnModel::setCategory(const Category* category)
{
	m_currentCategory = category;

	if (category != NULL)
		seqDebug("[fsm] changed category: %s", (const char*)category->name());
	else
		seqDebug("[fsm] changed category: <none>");
	
	invalidateFilter();
}

void FilteredSpawnModel::clearFilters()
{
	seqDebug("[fsm] cleared categories");

	m_currentCategory = NULL;
	invalidateFilter();
}

bool FilteredSpawnModel::filterAcceptsRow(int sourceRow, const QModelIndex& parent) const
{
	if (m_currentCategory == NULL)
		return true;

	const Item* item = m_spawnModel->item(sourceRow);
	if (item == NULL)
		return false;

	uint8_t level = 0;

	// skip filtered spawns
	if ((item->filterFlags() & FILTER_FLAG_FILTERED)
			&& !m_currentCategory->isFilteredFilter())
		return false;

	if ((item->type() == tSpawn) || (item->type() == tPlayer))
		level = ((Spawn*)item)->level();

	// does this spawn match the category?
	return m_currentCategory->isFiltered(filterString(item), level);
}

QString FilteredSpawnModel::filterString(const Item* item) const
{
	if (item == NULL)
		return "";

	QString text = ":";

	// get the filter flags
	text += m_spawnShell->filterMgr()->filterString(item->filterFlags());

	// get runtime filter flags
	text += m_spawnShell->filterMgr()->runtimeFilterString(item->runtimeFilterFlags());
	text += item->filterString();

	return text;
}

const Item* FilteredSpawnModel::item(const QModelIndex& index)
{
	if (index.isValid())
	{
		QModelIndex sourceIndex = mapToSource(index);

		if (sourceIndex.isValid())
			return m_spawnModel->item(sourceIndex);
	}
	return NULL;
}