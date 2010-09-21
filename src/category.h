/*
 * category.h
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 */

// Author: Zaphod (dohpaz@users.sourceforge.net)
//   Many parts derived from existing ShowEQ/SINS map code

//
// NOTE: Trying to keep this file ShowEQ/Everquest independent to allow it
// to be reused for other Show{} style projects.  Any existing ShowEQ/EQ
// dependencies will be migrated out.
//

#ifndef _CATEGORY_H_
#define _CATEGORY_H_

//----------------------------------------------------------------------
// forward declarations
class FilterItem;

class Category;
class CategoryMgr;
class CategoryDialog;
class CategoryModel;

enum tCategoryColumn
{
	tCatColName,
	tCatColFilter,
	tCatColFilterOut,
	tCatColColor,
	tCatColMax
};

// ------------------------------------------------------
// Category
class Category
{
public:
	Category(const QString& name, const QString& filter, const QString& filterout, QColor color);
	~Category();
	const QString& name() const { return m_name; }
	const QString& filter() const { return m_filter; }
	const QString& filterout() const { return m_filterout; }
	const QColor& color() const { return m_color; }

	bool isFilteredFilter() const { return m_filteredFilter; };
	bool isFiltered(const QString& filterString, int level = 0) const;

	void set(const Category& other);

private:
	QString m_name;
	QString m_filter;
	QString m_filterout;
	FilterItem* m_filterItem;
	FilterItem* m_filterOutItem;
	QColor m_color;
	bool m_filteredFilter;
};

typedef QList<Category*> CategoryList;
typedef QList<Category*>::iterator CategoryListIterator;
typedef QList<Category*>::const_iterator CategoryListConstIterator;

// ------------------------------------------------------
// CategoryMgr
class CategoryMgr : public QAbstractTableModel
{
	Q_OBJECT

public:
	// TODO: Remove category limit
	enum { tMaxNumCategories = 32 };

	CategoryMgr(QObject* parent = 0);
	virtual ~CategoryMgr();

	const CategoryList findCategories(const QString& filterString, int level) const;
	const CategoryList& getCategories() const { return m_categories; }
	uint count() { return m_categories.count(); }

	const Category* addCategory(const QString& name, const QString& filter, const QString& filterout, QColor color = Qt::black);
	void remCategory(const Category* cat);

	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

public slots:
	void clearCategories();
	void addCategory(QWidget* parent = 0);
	void editCategories(Category* cat, QWidget* parent = 0);

	void loadCategories();
	void savePrefs();

signals:
	void addCategory(const Category* cat);
	void delCategory(const Category* cat);
	void updateCategory(const Category* cat);

	void clearedCategories();
	void loadedCategories();

private:
	CategoryList	m_categories;
	bool m_changed;
};

#endif // _CATEGORY_H_
