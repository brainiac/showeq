/*
 * spawnlistview.cpp
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 *
 * Copyright 2001,2007 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *
 * Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net)
 * for use under the terms of the GNU General Public License,
 * incorporated herein by reference.
 *
 */

#include "pch.h"
#include "spawnlistview.h"
#include "diagnosticmessages.h"

#include <QHeaderView>

SpawnListView::SpawnListView(QWidget *parent)
	: QTreeView(parent)
{
	setupUi(this);


}

SpawnListView::~SpawnListView()
{
}


void SpawnListView::saveColumnPreferences()
{
	if (m_preferenceName.isEmpty())
		return;

	QHeaderView* headerView = this->header();

	int sortIndex		= headerView->sortIndicatorSection();
	Qt::SortOrder order = headerView->sortIndicatorOrder();

	for (int i = 0; i < headerView->count(); i++)
	{
		int visualIndex = headerView->visualIndex(i);
		int size		= headerView->sectionSize(i);
		bool hidden		= headerView->isSectionHidden(i);

		QVariant headerData = this->model()->headerData(i, Qt::Horizontal);

		seqInfo("Index: %i: %s, %i (size=%i) hidden=%i", i, (const char*)headerData.asString(), visualIndex, size, hidden);

		if (i == sortIndex)
		{
			seqInfo("\tSorted: %s", order == Qt::AscendingOrder ? "ascending" : "descending");
		}
	}
}

void SpawnListView::loadColumnPreferences()
{

}