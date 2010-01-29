/*
 * spells.h
 *
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 *
 * Copyright 2003 Zaphod (dohpaz@users.sourceforge.net). All Rights Reserved.
 *
 * Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net)
 * for use under the terms of the GNU General Public License,
 * incorporated herein by reference.
 *
 */

#ifndef SPELLS_H_
#define SPELLS_H_

#include "compat.h"

#include <QStringList>
#include <QString>
#include <QHash>

const size_t playerClasses = 15;

class Spell
{
public:
	Spell(const QString& spells_enLine);
	~Spell();

	// accessors
	uint16_t spell() const { return m_spell; }
	const QString& name() const { return m_name; }
	uint8_t level(uint8_t class_) const;
	uint8_t targetType() const { return m_targetType; }

	QString spellField(uint8_t field) const;
	int16_t calcDuration(uint8_t level) const;

private:
	QString m_name;
	uint16_t m_spell;
	int16_t m_buffDurationFormula;
	int16_t m_buffDurationArgument;
	uint8_t m_targetType;
	uint8_t m_classLevels[playerClasses];
};

class Spells
{
public:
	Spells();
	~Spells();
	void loadSpells(const QString& spellsFileName);
	void unloadSpells();

	const Spell* spell(uint32_t spell) const;
	uint32_t maxSpell() const { return m_maxSpell; }

private:
	QHash<uint32_t, Spell*> m_spells;
	uint32_t m_maxSpell;

};

#endif // SPELLS_H_
