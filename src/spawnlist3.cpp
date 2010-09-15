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
#include "spawnshell.h"



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

	m_spawnModel = m_spawnShell->spawnModel();
	ui.m_treeView->setModel(m_spawnModel);
	ui.m_treeView->setSortingEnabled(true);
	ui.m_treeView->setRootIsDecorated(false);

	connect(m_spawnModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(updateCount()));
	connect(m_spawnModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),  this, SLOT(updateCount()));
}

SpawnListWindow3::~SpawnListWindow3()
{

}

void SpawnListWindow3::updateCount()
{
	int count = m_spawnModel->rowCount();
	ui.m_label->setText(QString("%1").arg(count));
}