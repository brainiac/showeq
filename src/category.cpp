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

void Category::set(const Category& other)
{
	delete m_filterItem;
	delete m_filterOutItem;

	m_name		= other.m_name;
	m_filter	= other.m_filter;
	m_color		= other.m_color;
	m_filterout	= other.m_filterout;

	m_filterItem = new FilterItem(m_filter, true);
	m_filteredFilter = (m_filter.find(":Filtered:", 0, false) != -1);

	if (m_filterout.isEmpty())
		m_filterOutItem = NULL;
	else
		m_filterOutItem = new FilterItem(m_filterout, true);

}

// ------------------------------------------------------
// CategoryMgr
CategoryMgr::CategoryMgr(QObject* parent)
  : QAbstractTableModel(parent)
{
	loadCategories();
}

CategoryMgr::~CategoryMgr()
{
	// Clear the categories list. Since AutoDelete is off. This is manual.
	foreach (Category* category, m_categories)
	{
		delete category;
	}
}

const CategoryList CategoryMgr::findCategories(const QString& filterString, int level) const
{
	CategoryList tmpList;

	// iterate over all the categories looking for a match
	foreach (Category* category, m_categories)
	{
		// if it matches the category add it to the dictionary
		if (category->isFiltered(filterString, level))
			tmpList << category;
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

		int count = m_categories.size();
		beginInsertRows(QModelIndex(), count, count);

		m_categories.insert(count, newcat);

		endInsertRows();

		//seqDebug("Added '%s'-'%s' '%s' %d", newcat->name, newcat->filter, newcat->listitem->text(0).ascii(), newcat->listitem);
		emit addCategory(newcat);
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

		int position = m_categories.indexOf(const_cast<Category*>(cat));
		if (position != -1)
		{
			beginRemoveRows(QModelIndex(), position, position);

			m_categories.removeAt(position);

			endRemoveRows();
		}

		// delete the category
		delete cat;
	}
}

void CategoryMgr::clearCategories()
{
	//seqDebug("clearCategories()");
	emit clearedCategories();

	beginResetModel();

	foreach (Category* category, m_categories)
	{
		delete category;
	}
	m_categories.clear();

	endResetModel();
	 
	m_changed = true;
}

void CategoryMgr::addCategory(QWidget* parent)
{
	// not editing an existing, adding a new
	editCategories(NULL, parent);
}

void CategoryMgr::editCategories(Category* cat, QWidget* parent)
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

	// Add Category
	QString name   = dialog->getName();
	QString filter = dialog->getFilterIn();

	//seqDebug("Got name: '%s', filter '%s', filterout '%s', color '%s'",
	//  name?name:"", color?color:"", filter?filter:"", filterout?filterout:"");

	if (!name.isEmpty() && !filter.isEmpty())
	{
		if (cat != NULL)
		{
			Category newcat(name, filter, dialog->getFilterOut(), dialog->getColor());

			cat->set(newcat);

			int row = m_categories.indexOf(cat);
			seqDebug("[cat] row changed: %i", row);
			dataChanged(index(row, 0), index(row, tCatColMax));

			emit updateCategory(cat);
		}
		else
		{
			addCategory(name, filter, dialog->getFilterOut(), dialog->getColor());
		}
	}

	delete dialog;
}

void CategoryMgr::loadCategories()
{
	clearCategories();

	QList<Category*> tempList;
	QString section = "CategoryMgr";
	for (int i = 1; i <= tMaxNumCategories; i++)
	{
		QString prefBaseName;
		prefBaseName.sprintf("Category%d_", i);

		// attempt to pull a button title from the preferences
		QString tempStr = prefBaseName + "Name";
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
				int count = m_categories.count();

				tempList.append(newcat);
			}
		}
	}

	// Insert all the new rows in one go.
	beginInsertRows(QModelIndex(), 0, tempList.count());
	m_categories = tempList;
	endInsertRows();

	// Reset changed so that they aren't saved (we just loaded)
	m_changed = false;

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

	foreach (Category* category, m_categories)
	{
		prefBaseName.sprintf("Category%d_", count++);
		pSEQPrefs->setPrefString(prefBaseName + "Name", section, category->name());
		pSEQPrefs->setPrefString(prefBaseName + "Filter", section, category->filter());
		pSEQPrefs->setPrefString(prefBaseName + "FilterOut", section, category->filterout());
		pSEQPrefs->setPrefColor(prefBaseName + "Color", section, category->color());
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


QVariant CategoryMgr::data(const QModelIndex& index, int role /* = Qt::DisplayRole */) const
{
	if (!index.isValid())
		return QVariant();

	const Category* category = m_categories.at(index.row());

	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case tCatColName:
			return category->name();
		case tCatColFilter:
			return category->filter();
		case tCatColFilterOut:
			return category->filterout();
		case tCatColColor:
			{
				QColor color = category->color();
				return QString("(%1,%2,%3)")
					.arg(color.red())
					.arg(color.green())
					.arg(color.blue());
			}
		}
	}
	else if (role == Qt::ForegroundRole)
	{
		if (index.column() == tCatColColor)
		{
			return QBrush(category->color());
		}
	}
	return QVariant();
}

QVariant CategoryMgr::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole*/) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation != Qt::Horizontal)
		return QVariant();

	switch (section)
	{
	case tCatColName:
		return "Name";
	case tCatColFilter:
		return "Filter";
	case tCatColFilterOut:
		return "FilterOut";
	case tCatColColor:
		return "Color";
	}

	return QVariant();
}

int CategoryMgr::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : m_categories.count();
}

int CategoryMgr::columnCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : tCatColMax;
}