/*
 * spawnlist3.cpp
 *
 *  ShowEQ Distributed under GPL
 *  http://www.sourceforge.net/projects/seq
 *
 *  Copyright 2000-2010 by the respective ShowEQ Developers
 */

#include "pch.h"

#include "spawnlist3.h"

#include "category.h"
#include "filteredspawnmodel.h"
#include "spawnshell.h"

#include "diagnosticmessages.h"


SpawnListWindow3::SpawnListWindow3(Player* player, SpawnShell* spawnShell, CategoryMgr* categoryMgr, QWidget *parent)
	: SEQWindow("SpawnList3", "ShowEQ - Spawns", parent),
	m_player(player),
	m_categoryMgr(categoryMgr),
	m_spawnShell(spawnShell),
	m_currentCategory(NULL),
	m_selectedItem(NULL)
{
	ui.setupUi(this);

	QWidget* pWidget = new QWidget();
	pWidget->setLayout(ui.m_layout);
	setWidget(pWidget);

	ui.m_label->setText("");

	m_spawnModel = new FilteredSpawnModel(m_spawnShell, this);
	m_spawnModel->setDynamicSortFilter(true);

	ui.m_treeView->setModel(m_spawnModel);
	ui.m_treeView->setSortingEnabled(true);
	ui.m_treeView->setRootIsDecorated(false);
	ui.m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);

	ui.m_treeView->setPreferenceName("SpawnList3");
	ui.m_treeView->saveColumnPreferences();
	connect(ui.m_treeView, SIGNAL(clicked(const QModelIndex&)), ui.m_treeView, SLOT(saveColumnPreferences()));

	delete ui.m_filterBox->model();
	ui.m_filterBox->setModel(categoryMgr);
	ui.m_filterBox->setModelColumn(tCatColName);

	connect(ui.m_filterBox, SIGNAL(currentIndexChanged(int)), this, SLOT(categorySelected(int)));
	connect(categoryMgr, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(updateFilterBox()));


	connect(m_spawnModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(updateCount()));
	connect(m_spawnModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),  this, SLOT(updateCount()));

	connect(ui.m_treeView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(doubleClicked(const QModelIndex&)));
}

SpawnListWindow3::~SpawnListWindow3()
{
}

void SpawnListWindow3::updateFilterBox()
{
	int row = ui.m_filterBox->currentIndex();

	ui.m_filterBox->setModel(m_categoryMgr);
	ui.m_filterBox->setCurrentIndex(row);
}

void SpawnListWindow3::updateCount()
{
	int count = m_spawnModel->rowCount();
	ui.m_label->setText(QString::number(count));
}


void SpawnListWindow3::categorySelected(int index)
{
	const CategoryList& list = m_categoryMgr->getCategories();
	Category* category = NULL;

	//seqDebug("category selected: %i", index);
	if (index >= 0 && index < list.count())
		category = list[index];

	m_spawnModel->setCategory(category);
}

void SpawnListWindow3::doubleClicked(const QModelIndex& index)
{
	const Item* item = m_spawnModel->item(index);
	if (item != NULL)
	{
		seqInfo("%s", (const char*)m_spawnModel->filterString(item));
	}
}