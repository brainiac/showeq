#ifndef FILTEREDSPAWNMODEL_H
#define FILTEREDSPAWNMODEL_H

#include <QSortFilterProxyModel>
#include <QString>

class Category;
class Item;
class SpawnShell;
class SpawnModel;

class FilteredSpawnModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	explicit FilteredSpawnModel(SpawnShell* spawnShell, QObject *parent = 0);
	~FilteredSpawnModel();

	const Category* currentCategory() { return m_currentCategory; }
	QString filterString(const Item* item) const;

	const Item* item(const QModelIndex& index);

	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

public slots:
	void setCategory(const Category* category);
	void clearFilters();

protected:
	virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& parent) const;

private:
	const Category* m_currentCategory;

	SpawnModel* m_spawnModel;
	SpawnShell* m_spawnShell;		// TODO: Get rid of this
};

#endif // FILTEREDSPAWNMODEL_H
