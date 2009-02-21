/*
 * xmlconv.cpp
 * 
 * ShowEQ Distributed under GPL
 * http://seq.sourceforge.net/
 *
 * Copyright 2002-2003,2007 Zaphod (dohpaz@users.sourceforge.net). 
 *     All Rights Reserved.
 *
 * Contributed to ShowEQ by Zaphod (dohpaz@users.sourceforge.net) 
 * for use under the terms of the GNU General Public License, 
 * incorporated herein by reference.
 *
 */


#include "xmlpreferences.h"
#include "xmlconv.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegExp>batt

const float seqPrefVersion = 1.0;
const char* seqPrefName = "seqpreferences";
const char* seqPrefSysId = "seqpref.dtd";

XMLPreferences::XMLPreferences(const QString& defaultsFileName, const QString& inFileName)
  : m_defaultsFilename(defaultsFileName),
    m_filename(inFileName), 
    m_modified(0)
{
	m_templateDoc.sprintf(
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<!DOCTYPE %s SYSTEM \"%s\">\n"
			"<seqpreferences version=\"%1.1f\">\n"
			"<!-- ============================================================= -->"
			"</seqpreferences>\n",
			seqPrefName, seqPrefSysId, seqPrefVersion);

	// load the preferences
	load();
}

XMLPreferences::~XMLPreferences()
{
	m_userSections.clear();
	m_defaultsSections.clear();
	m_commentSections.clear();
}

void XMLPreferences::load()
{
	// load the default preferences
	loadPreferences(m_defaultsFilename, m_defaultsSections);

	// load the user preferences
	loadPreferences(m_filename, m_userSections);
}

void XMLPreferences::save()
{
	// save the user preferences if they've changed
	if (m_modified & User)
		savePreferences(m_filename, m_userSections);

	// save the user preferences if they've changed
	if (m_modified & Defaults)
		savePreferences(m_defaultsFilename, m_defaultsSections);
}

void XMLPreferences::revert()
{
	// clear out all default preferecnes
	m_defaultsSections.clear();

	// clear out all user preferences
	m_userSections.clear();

	// load the default preferences
	loadPreferences(m_defaultsFilename, m_defaultsSections);

	// load the user preferences back in from the file
	loadPreferences(m_filename, m_userSections);
}

void XMLPreferences::loadPreferences(const QString& filename, PrefSectionDict& dict)
{
	QDomDocument doc(seqPrefName);
	QFile f(filename);
	
	if (!f.open(QIODevice::ReadOnly))
	{
		qWarning("Unable to open file: %s!", (const char*)filename);
		return;
	}

	QString errorMsg;
	int errorLine = 0;
	int errorColumn = 0;	
	if (!doc.setContent(&f, false, &errorMsg, &errorLine, &errorColumn))
	{
		qWarning("Error processing file: %s!\n\t %s on line %d in column %d!", (const char*)filename, (const char*)errorMsg, errorLine, errorColumn);
		f.close();
		return;
	}
	
	DomConvenience conv(doc);
	
	// do more processing here	
	QDomNodeList sectionList = doc.elementsByTagName("section");
	for (uint i = 0; i < sectionList.length(); i++)
	{
		QDomElement section = sectionList.item(i).toElement();
		if (!section.hasAttribute("name"))
		{
			qWarning("section without name!");
			continue;
		}

		QString sectionName = section.attribute("name");
		PreferenceDict* sectionDict = NULL;

		// see if the section exists in the dictionary
		if (dict.contains(sectionName))
		{
			sectionDict = dict[sectionName];
		}
		else
		{
			// create the new preference dictionary
			sectionDict = new PreferenceDict();

			// insert the preference dictionary into the section
			dict[sectionName] = sectionDict;
		}

		// see if comment section exists in the dictionary
		CommentDict* commentSectionDict = NULL;

		// if not, then create it
		if (m_commentSections.contains(sectionName))
		{
			commentSectionDict = m_commentSections[sectionName];
		}
		else
		{
			// create the new preference dictionary
			commentSectionDict = new CommentDict();
			
			// insert the preference dictionary into the section
			m_commentSections[sectionName] = commentSectionDict;
		}

		QDomNodeList propertyList = section.elementsByTagName("property");
		for (uint j = 0; j < propertyList.length(); j++)
		{
			QDomElement property = propertyList.item(j).toElement();
			if (!property.hasAttribute("name"))
			{
				qWarning("property in section '%s' without name! Ignoring!", (const char*)sectionName);
				continue;
			}
		
			QString propertyName = property.attribute("name");
			bool foundValue = false;
     
			// iterate over the nodes under the property
			for (QDomNode n = property.firstChild(); !n.isNull(); n = n.nextSibling())
			{
				if (!n.isElement())
					continue;

				QDomElement valueElement = n.toElement();
				if (valueElement.tagName() == "comment")
				{
					// get comment if any
					QString comment = valueElement.text();

					// if there is a comment, cache it
					if (!comment.isEmpty())
					{
						if (commentSectionDict->contains(propertyName))
						{
							commentSectionDict->insert(propertyName, comment);
						}
					}
					continue;
				}

				QVariant value;
				if (!conv.elementToVariant(valueElement, value))
				{
					qWarning("property '%s' in section '%s' with bogus value in tag '%s'! Ignoring!",
							 (const char*)propertyName, (const char*)sectionName, (const char*)valueElement.tagName());
					continue;
				}

				// found the value
				foundValue = true;
       
				// insert value into the section dictionary
				sectionDict->insert(propertyName, value);
				break;
			}

#if 0 // ZBTEMP : Support properties without values to get comments?
			if (!foundValue)
			{
				qWarning("property '%s' in section '%s' without value! Ignoring!",
						 (const char*)propertyName, (const char*)sectionName);
				continue;
			}
#endif
		}
	}

	// close the file
	f.close();

#if 1 // ZBTEMP
	printf("Loaded preferences file: %s!\n", (const char*)filename);
#endif
}

void XMLPreferences::savePreferences(const QString& filename, PrefSectionDict& dict)
{
	// open the existing preference file
	QDomDocument doc;
	QFile f(filename);
	bool loaded = false;
	
	if (f.open(QIODevice::ReadOnly))
	{
		QString errorMsg;
		int errorLine = 0;
		int errorColumn = 0;
		if (doc.setContent(&f, false, &errorMsg, &errorLine, &errorColumn))
			loaded = true;
		else
		{
			qWarning("Error processing file: %s!\n\t %s on line %d in column %d!", 
					 (const char*)filename, (const char*)errorMsg, errorLine, errorColumn);
		}

		// close the file
		f.close();
	}

	// if no file was loaded, use the template document
	if (!loaded)
	{
		QString errorMsg;
		int errorLine = 0;
		int errorColumn = 0;
		if (doc.setContent(m_templateDoc, false, &errorMsg, &errorLine, &errorColumn))
			loaded = true;
	}

	// if there was an existing file, rename it
	QFileInfo fileInfo(filename);
	if (fileInfo.exists())
	{
		QDir dir(fileInfo.dirPath(true));
		dir.rename(filename, filename + QString(".bak"));
	}

	// do more processing here
	DomConvenience conv(doc);	
	QDomElement docElem = doc.documentElement();
	QDomNodeList sectionList = docElem.elementsByTagName("section");

	QHashIterator<QString, PreferenceDict*> sdit(dict);
	while (sdit.hasNext())
	{
		sdit.next();
		
		QString sectionName = sdit.key();
		PreferenceDict* sectionDict = sdit.value();
		QDomElement section;
		
		// iterate over all the sections in the document
		for (uint i = 0; i < sectionList.length(); i++)
		{
			QDomElement e = sectionList.item(i).toElement();
			if (!e.hasAttribute("name"))
			{
				qWarning("section without name!");
				continue;
			}

			// printf("found section: %s\n", (const char*)section.attribute("name"));

			// is this the section?
			if (sectionName == e.attribute("name"))
			{
				// yes, done
				section = e;
				break;
			}
		}

		// if no section was found, create a new one
		if (section.isNull())
		{
			// create the section element
			section = doc.createElement("section");

			// set the name attribute of the section element
			section.setAttribute("name", sectionName);

			// append the new section to the document
			docElem.appendChild(section);
		}

		// iterate over all the properties in the section
		QHashIterator<QString, QVariant> pdit(*sectionDict);
		while (pdit.hasNext())
		{
			pdit.next();
			
			QString propertyName = pdit.key();
			QVariant propertyValue = pdit.value();
			QDomElement property;
			
			// get all the property elements in the section
			QDomNodeList propertyList = section.elementsByTagName("property");

			// iterate over all the property elements until a match is found
			for (uint j = 0; j < propertyList.length(); j++)
			{
				QDomElement e = propertyList.item(j).toElement();
				if (!e.hasAttribute("name"))
				{
					qWarning("property in section '%s' without name! Ignoring!",
							 (const char*)sectionName);
					continue;
				}

				// is this the property being searched for?
				if (propertyName == e.attribute("name"))
				{
					// yes, done
					property = e;
					break;
				}
			}
			
			// if no property was found, create a new one
			if (property.isNull())
			{
				// create the property element
				property = doc.createElement("property");

				// set the name attribute of the property element
				property.setAttribute("name", propertyName);

				// append the new property to the section
				section.appendChild(property);
			}

			QDomElement value;

			// iterate over the children
			for (QDomNode n = property.firstChild(); !n.isNull(); n = n.nextSibling())
			{
				if (!n.isElement())
					continue;

				// don't replace comments
				if (n.toElement().tagName() == "comment")
					continue;
				
				// convert it to an element
				value = n.toElement();
				break;
			}

			// if no value element was found, create a new one
			if (value.isNull())
			{
				// create the value element, bogus type, will be filled in later
				value = doc.createElement("bogus");
	
				// append the new value to the property
				property.appendChild(value);
			}

			if (!conv.variantToElement(propertyValue, value))
			{
				qWarning("Unable to set value element in section '%s' property '%s'!",
						 (const char*)propertyName, (const char*)propertyName);
			}
		}
	}

	// write the modified DOM to disk
	if (!f.open(QIODevice::WriteOnly))
	{
		qWarning("Unable to open file for writing: %s!", 
				 (const char*)filename);
	}

	// open a Text Stream on the file
	QTextStream out(&f);

	// make sure stream is UTF8 encoded
	out.setEncoding(QTextStream::UnicodeUTF8);

	// save the document to the text stream
	QString docText;
	QTextStream docTextStream(&docText, QIODevice::WriteOnly);
	doc.save(docTextStream, 4);

	// put newlines after comments (which unfortunately Qt's DOM doesn't track)
	QRegExp commentEnd("-->");
	docText.replace(commentEnd, "-->\n");

	// write the fixed document out to the file
	out << docText;

	// close the file
	f.close();

	printf("Finished saving preferences to file: %s\n", (const char*)filename);
}

bool XMLPreferences::getPref(const QString& inName, const QString& inSection, QVariant& outValue, Persistence pers)
{
	PreferenceDict* sectionDict = NULL;

	if (pers & Runtime)
	{
		// see if the section exists in the dictionary
		if (m_runtimeSections.contains(inSection))
		{
			sectionDict = m_runtimeSections[inSection];
		
			// if so, then see if the preference exists
			if (sectionDict->contains(inName))
			{
				outValue = sectionDict->value(inName);
				return true;
			}
		}
	}

	if (pers & User)
	{
		// see if the section exists in the dictionary
		if (m_userSections.contains(inSection))
		{
			sectionDict = m_userSections[inSection];
			
			// if so, then see if the preference exists
			if (sectionDict->contains(inName))
			{
				outValue = sectionDict->value(inName);
				return true;
			}
		}
	}

	if (pers & Defaults)
	{
		// see if the section exists in the defaults dictionary
		if (m_defaultsSections.contains(inSection))
		{
			sectionDict = m_defaultsSections[inSection];
			
			// if so, then see if the preference exists
			if (sectionDict->contains(inName))
			{
				outValue = sectionDict->value(inName);
				return true;
			}
		}
	}
	return false;
}

void XMLPreferences::setPref(const QString& inName, const QString& inSection, const QVariant& inValue, Persistence pers)
{
	// set the preference in the appropriate section
	if (pers & Runtime)
		setPref(m_runtimeSections, inName, inSection, inValue);
	if (pers & User)
		setPref(m_userSections, inName, inSection, inValue);
	if (pers & Defaults)
		setPref(m_defaultsSections, inName, inSection, inValue);

	m_modified |= pers;
}

void XMLPreferences::setPref(PrefSectionDict& dict, const QString& inName, const QString& inSection, const QVariant& inValue)
{
	PreferenceDict* sectionDict;
	QVariant preference;
	
	if (dict.contains(inSection))
	{
		sectionDict = dict[inSection];
	}
	else
	{
		sectionDict = new PreferenceDict();
		dict[inSection] = sectionDict;
	}
	
	sectionDict->insert(inName, inValue);
}

QString XMLPreferences::getPrefComment(const QString& inName, const QString& inSection)
{
	QString comment;
	
	if (m_commentSections.contains(inSection))
	{
		CommentDict* commentSectionDict = m_commentSections[inSection];
		return commentSectionDict->value(inName);
	}
			
	return QString("");
}

bool XMLPreferences::isSection(const QString& inSection, Persistence pers)
{	
	if (pers & Runtime)
	{
		// see if the section exists in the dictionary
		if (m_runtimeSections.contains(inSection))
			return true;
	}
	
	if (pers & User)
	{
		// see if the section exists in user settings.
		if (m_userSections.contains(inSection))
			return true;
	}
	
	if (pers & Defaults)
	{
		if (m_defaultsSections.contains(inSection))
			return true;
	}
	
	return false;
}

bool XMLPreferences::isPreference(const QString& inName, const QString& inSection, Persistence pers)
{
	QVariant dummy;
	
	// try to retrieve the preference
	return getPref(inName, inSection, dummy, pers);
}

// easy/sleezy way to create common get methods
#define getPrefMethod(retType, Type, Def) \
retType XMLPreferences::getPref##Type(const QString& inName, const QString& inSection, Def def, Persistence pers) \
{ \
	QVariant value; \
	if (getPref(inName, inSection, value, pers)) \
		return value.value<retType>(); \
	return def; \
} 

// implement common get methods
getPrefMethod(QString, String, const QString&);
getPrefMethod(int, Int, int);
getPrefMethod(uint, UInt, uint);
getPrefMethod(double, Double, double);
getPrefMethod(bool, Bool, bool);
getPrefMethod(QColor, Color, const QColor&);
getPrefMethod(QPen, Pen, const QPen&);
getPrefMethod(QBrush, Brush, const QBrush&);
getPrefMethod(QPoint, Point, const QPoint&);
getPrefMethod(QRect, Rect, const QRect&);
getPrefMethod(QSize, Size, const QSize&);
getPrefMethod(QFont, Font, const QFont&);
getPrefMethod(QSizePolicy, SizePolicy, const QSizePolicy&);
getPrefMethod(QCursor, Cursor, const QCursor&);
getPrefMethod(QStringList, StringList, const QStringList&);

// implement get methods that require special behavior
QKeySequence XMLPreferences::getPrefKey(const QString& inName, const QString& inSection, const QString& def, Persistence pers)
{
	return getPrefKey(inName, inSection, QKeySequence(def) & ~Qt::UNICODE_ACCEL, pers);
}

QKeySequence XMLPreferences::getPrefKey(const QString& inName, const QString& inSection, const QKeySequence& def, Persistence pers)
{
	// try to retrieve the preference
	QVariant preference;
	if (getPref(inName, inSection, preference, pers))
	{
		QKeySequence key = def;
		
		switch (preference.type())
		{
			case QVariant::KeySequence:
				key = preference.value<QKeySequence>();
				break;
				
			case QVariant::String:
				// convert it to a key
				key = (QKeySequence)preference.toString();
				break;
				
			case QVariant::Int:
			case QVariant::UInt:
			case QVariant::Double:
				key = (QKeySequence)preference.toInt();
				break;	
				
			default:
				qWarning("XMLPreferences::getPrefKey(%s, %s): preference found,\n"
						 "\tbut type %s is not convertable to type key!",
						 (const char*)inName, (const char*)inSection, preference.typeName());
		}
		
		// fix the key code and return (deal with Qt brain death)
		return key & ~Qt::UNICODE_ACCEL;
	}
	
	// return the default value
	return def;
}


int64_t XMLPreferences::getPrefInt64(const QString& inName, const QString& inSection, int64_t def, Persistence pers)
{
	QVariant pref;
	
	// if preference was retrieved, return it as a string
	if (getPref(inName, inSection, pref, pers))
	{
		int64_t value = def;
		
		switch (pref.type())
		{
			case QVariant::String:
				// convert it to a int64_t (in base 16)
				value = strtoll(pref.toString(), 0, 16);
				break;
				
			case QVariant::Int:
			case QVariant::UInt:
				value = pref.toInt();
				break;
				
			case QVariant::Double:
				value = (int64_t)pref.toDouble();
				break;
				
			case QVariant::ByteArray:
			{
				QByteArray& ba = pref.asByteArray();
				if (ba.size() == sizeof(int64_t))
					value = *(int64_t*)ba.data();
				break;
			}
				
			default:
				qWarning("XMLPreferences::getPrefInt64(%s, %s, %lld): preference found,\n"
						 "\tbut type %s is not convertable to type int64_t!",
						 (const char*)inName, (const char*)inSection, (long long)def,
						 pref.typeName());
		}
		
		// return the key
		return value;
	}
	
	// return the default value
	return def;
}

uint64_t XMLPreferences::getPrefUInt64(const QString& inName, const QString& inSection, uint64_t def, Persistence pers)
{
	QVariant pref;
		
	// if preference was retrieved, return it as a string
	if (getPref(inName, inSection, pref, pers))
	{
		uint64_t value = def;
		
		switch (pref.type())
		{
			case QVariant::String:
				// convert it to a uint64_t (in base 16)
				value = strtoull(pref.toString(), 0, 16);
				break;
				
			case QVariant::Int:
			case QVariant::UInt:
				value = pref.toInt();
				break;
				
			case QVariant::Double:
				value = (uint64_t)pref.toDouble();
				break;
				
			case QVariant::ByteArray:
			{
				QByteArray& ba = pref.asByteArray();
				if (ba.size() == sizeof(uint64_t))
					value = *(uint64_t*)ba.data();
				break;
			}
				
			default:
				qWarning("XMLPreferences::getPrefUInt64(%s, %s, %llu): preference found,\n"
						 "\tbut type %s is not convertable to type uint64_t!",
						 (const char*)inName, (const char*)inSection, (unsigned long long)def, pref.typeName());
		}
		
		// return the key
		return value;
	}
	
	// return the default value
	return def;
}


QVariant XMLPreferences::getPrefVariant(const QString& inName, const QString& inSection, const QVariant& outDefault, Persistence pers)
{
	// try to retrieve the preference
	QVariant value;
	if (getPref(inName, inSection, value, pers))
		return value;
	
	// return the default value
	return outDefault;
}

// generic common set preference method
#define setPrefMethod(Type, In) \
void XMLPreferences::setPref##Type(const QString& inName, const QString& inSection, In inValue, Persistence pers) \
{ \
	setPref(inName, inSection, QVariant(inValue), pers); \
} 

// create the methods for the types that can be handled the common way
setPrefMethod(String, const QString&);
setPrefMethod(Int, int);
setPrefMethod(UInt, uint);
setPrefMethod(Double, double);
setPrefMethod(Color, const QColor&);
setPrefMethod(Pen, const QPen&);
setPrefMethod(Brush, const QBrush&);
setPrefMethod(Point, const QPoint&);
setPrefMethod(Rect, const QRect&);
setPrefMethod(Size, const QSize&);
setPrefMethod(Font, const QFont&);
setPrefMethod(SizePolicy, const QSizePolicy&);
setPrefMethod(StringList, const QStringList&);

void XMLPreferences::setPrefBool(const QString& inName, const QString& inSection, bool inValue, Persistence pers)
{
	setPref(inName, inSection, QVariant(inValue, 0), pers);
}

void XMLPreferences::setPrefKey(const QString& inName, const QString& inSection, int inValue, Persistence pers)
{
	const QVariant cv = QKeySequence(inValue);
	setPref(inName, inSection, cv, pers);
}

void XMLPreferences::setPrefInt64(const QString& inName, const QString& inSection, int64_t inValue, Persistence pers)
{
	QByteArray ba;
	ba.duplicate((const char*)&inValue, sizeof(int64_t));
	setPref(inName, inSection, ba, pers);
}


void XMLPreferences::setPrefUInt64(const QString& inName, const QString& inSection, uint64_t inValue, Persistence pers)
{
	QByteArray ba;
	ba.duplicate((const char*)&inValue, sizeof(uint64_t));
	setPref(inName, inSection, ba, pers);
}

void XMLPreferences::setPrefVariant(const QString& inName, const QString& inSection, const QVariant& inValue, Persistence pers)
{
	setPref(inName, inSection, inValue, pers);
}

#ifndef QMAKEBUILD
#include "xmlpreferences.moc"
#endif
