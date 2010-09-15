/*
 * category.cpp
 *
 * ShowEQ Distributed under GPL
 * http://seq.sf.net/
 *
 *  Copyright 2003-2007 by the respective ShowEQ Developers
 */

// Author: Zaphod (dohpaz@users.sourceforge.net)
//   Many parts derived from existing ShowEQ/SINS map code

//
// NOTE: Trying to keep this file ShowEQ/Everquest independent to allow it
// to be reused for other Show{} style projects.  Any existing ShowEQ/EQ
// dependencies will be migrated out.
//

#include "pch.h"

#include "category.h"
#include "filter.h"
#include "diagnosticmessages.h"
#include "main.h"

#include "categorydialog.h"

#include <Q3HBoxLayout>
#include <Q3BoxLayout>
#include <Q3VBoxLayout>

using namespace Qt;

// ------------------------------------------------------
// Category
Category::Category(const QString& name, const QString& filter, const QString& filterout, QColor color)
{
	m_name = name;
	m_filter = filter;
	if (!filterout.isEmpty())
		m_filterout = filterout;
	m_color = color;

	// allocate the filter item
	m_filterItem = new FilterItem(filter, true);
	m_filteredFilter = (filter.find(":Filtered:", 0, false) != -1);

	// allocate the filter out item
	if (m_filterout.isEmpty())
		m_filterOutItem = NULL;
	else
		m_filterOutItem = new FilterItem(filterout, true);
}

Category::~Category()
{
	delete m_filterItem;
	delete m_filterOutItem;
}

bool Category::isFiltered(const QString& filterString, int level) const
{
	if ((m_filterItem != NULL) && m_filterItem->isFiltered(filterString, level))
	{
		if ((m_filterOutItem != NULL) &&  m_filterOutItem->isFiltered(filterString, level))
			return false;
		else
			return true;
	}
	return false;
}

// ------------------------------------------------------
// CategoryMgr
CategoryMgr::CategoryMgr(QObject* parent, const char* name)
  : QObject(parent, name)
{
	m_categories.setAutoDelete(false);
	reloadCategories();
}

CategoryMgr::~CategoryMgr()
{
	// Clear the categories list. Since AutoDelete is off. This is manual.
	if (m_categories.first())
	{
		Category* deleteMe;

		while ((deleteMe = m_categories.current()))
		{
			m_categories.remove();

			delete deleteMe;
		}
	}
}

const CategoryList CategoryMgr::findCategories(const QString& filterString, int level) const
{
	CategoryList tmpList;

	// iterate over all the categories looking for a match
	CategoryListIterator it(m_categories);
	for (Category* curCategory = it.toFirst(); curCategory != NULL; curCategory = ++it)
	{
		// if it matches the category add it to the dictionary
		if (curCategory->isFiltered(filterString, level))
			tmpList.append(curCategory);
	}

	return tmpList;
}

const Category* CategoryMgr::addCategory(const QString& name, const QString& filter, const QString& filterout, QColor color)
{
	//seqDebug("addCategory() '%s' - '%s':'%s'", name, filter, filterout?filterout:"null");
	// TODO, need to add check for duplicate category name
	m_changed = true;
	if (!name.isEmpty() && !filter.isEmpty())
	{
		Category* newcat = new Category(name, filter, filterout, color);

		m_categories.append(newcat);

		emit addCategory(newcat);

		//seqDebug("Added '%s'-'%s' '%s' %d", newcat->name, newcat->filter, newcat->listitem->text(0).ascii(), newcat->listitem);
		return newcat;
	}
	return NULL;
}

void CategoryMgr::remCategory(const Category* cat)
{
	m_changed = true;

	if (cat != NULL)
	{
		// signal that the category is being deleted
		emit delCategory(cat);

		// remove the category from the list
		m_categories.remove(cat);

		// delete the category
		delete cat;
	}
}

void CategoryMgr::clearCategories()
{
	//seqDebug("clearCategories()");
	emit clearedCategories();

	m_categories.clear();
	m_changed = true;
}

void CategoryMgr::addCategory(QWidget* parent)
{
	// not editing an existing, adding a new
	editCategories(NULL, parent);
}

void CategoryMgr::editCategories(const Category* cat, QWidget* parent)
{
	// Create the filter dialog
	CategoryDialog* dialog = new CategoryDialog(parent);

	// editing an existing category?
	if (cat != NULL)
	{
		// yes, use it's info for the defaults
		dialog->setName(cat->name());
		dialog->setFilterIn(cat->filter());
		dialog->setFilterOut(cat->filterout());
		dialog->setColor(cat->color());
	}
	else
	{
		dialog->setName("");
		dialog->setFilterIn(".");
		dialog->setFilterOut("");
	}

	// execute the dialog
	int res = dialog->exec();

	// if the dialog wasn't accepted, don't add/change a category
	if (res != QDialog::Accepted)
		return;

	// remove the old category
	if (cat != NULL)
		remCategory(cat);

	// Add Category
	QString name   = dialog->getName();
	QString filter = dialog->getFilterIn();

	//seqDebug("Got name: '%s', filter '%s', filterout '%s', color '%s'",
	//  name?name:"", color?color:"", filter?filter:"", filterout?filterout:"");

	if (!name.isEmpty() && !filter.isEmpty())
		addCategory(name, filter, dialog->getFilterOut(), dialog->getColor());

	delete dialog;
}

void CategoryMgr::reloadCategories()
{
	clearCategories();
	m_changed = false;

	QString section = "CategoryMgr";
	int i = 0;
	QString prefBaseName;
	QString tempStr;
	for (i = 1; i <= tMaxNumCategories; i++)
	{
		prefBaseName.sprintf("Category%d_", i);

		// attempt to pull a button title from the preferences
		tempStr = prefBaseName + "Name";
		if (pSEQPrefs->isPreference(tempStr, section))
		{
			QString name   = pSEQPrefs->getPrefString(tempStr, section);
			QString filter = pSEQPrefs->getPrefString(prefBaseName + "Filter", section);
			QColor color   = pSEQPrefs->getPrefColor(prefBaseName + "Color", section, QColor("black"));
			tempStr = prefBaseName + "FilterOut";
			QString filterout;
			if (pSEQPrefs->isPreference(tempStr, section))
				filterout = pSEQPrefs->getPrefString(tempStr, section);

			//seqDebug("%d: Got '%s' '%s' '%s'", i, name, filter, color);
			if (!name.isEmpty() && !filter.isEmpty())
			{
				Category* newcat = new Category(name, filter, filterout, color);
				m_categories.append(newcat);
			}
		}
	}

	// signal that the categories have been loaded
	emit loadedCategories();

	seqInfo("Categories Reloaded");
}

void CategoryMgr::savePrefs()
{
	if (!m_changed)
		return;

	int count = 1;
	QString section = "CategoryMgr";
	QString prefBaseName;

	CategoryListIterator it(m_categories);
	for (Category* curCategory = it.toFirst(); curCategory != NULL; curCategory = ++it)
	{
		prefBaseName.sprintf("Category%d_", count++);
		pSEQPrefs->setPrefString(prefBaseName + "Name", section, curCategory->name());
		pSEQPrefs->setPrefString(prefBaseName + "Filter", section, curCategory->filter());
		pSEQPrefs->setPrefString(prefBaseName + "FilterOut", section, curCategory->filterout());
		pSEQPrefs->setPrefColor(prefBaseName + "Color", section, curCategory->color());
	}

	QColor black("black");
	while (count <= tMaxNumCategories)
	{
		prefBaseName.sprintf("Category%d_", count++);
		pSEQPrefs->setPrefString(prefBaseName + "Name", section, "");
		pSEQPrefs->setPrefString(prefBaseName + "Filter", section, "");
		pSEQPrefs->setPrefString(prefBaseName + "FilterOut", section, "");
		pSEQPrefs->setPrefColor(prefBaseName + "Color", section, black);
	}
}
