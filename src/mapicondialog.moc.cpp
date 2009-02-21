/****************************************************************************
** Meta object code from reading C++ file 'mapicondialog.h'
**
** Created: Thu Feb 19 01:03:17 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "mapicondialog.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mapicondialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MapIconDialog[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x0a,
      23,   14,   14,   14, 0x0a,
      32,   14,   14,   14, 0x0a,
      39,   14,   14,   14, 0x0a,
      58,   49,   14,   14, 0x0a,
      84,   81,   14,   14, 0x0a,
     112,   14,   14,   14, 0x0a,
     136,   14,   14,   14, 0x0a,
     162,   14,   14,   14, 0x0a,
     190,   14,   14,   14, 0x0a,
     220,   14,   14,   14, 0x0a,
     244,   14,   14,   14, 0x0a,
     268,   14,   14,   14, 0x0a,
     292,   14,   14,   14, 0x0a,
     319,   14,   14,   14, 0x0a,
     341,   14,   14,   14, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_MapIconDialog[] = {
    "MapIconDialog\0\0apply()\0revert()\0init()\0"
    "destroy()\0mapIcons\0setMapIcons(MapIcons*)\0"
    "id\0mapIconCombo_activated(int)\0"
    "imagePenColor_clicked()\0"
    "imageBrushColor_clicked()\0"
    "highlightPenColor_clicked()\0"
    "highlightBrushColor_clicked()\0"
    "line0PenColor_clicked()\0line1PenColor_clicked()\0"
    "line2PenColor_clicked()\0"
    "walkPathPenColor_clicked()\0"
    "setupMapIconDisplay()\0languageChange()\0"
};

const QMetaObject MapIconDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_MapIconDialog,
      qt_meta_data_MapIconDialog, 0 }
};

const QMetaObject *MapIconDialog::metaObject() const
{
    return &staticMetaObject;
}

void *MapIconDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MapIconDialog))
        return static_cast<void*>(const_cast< MapIconDialog*>(this));
    if (!strcmp(_clname, "Ui::MapIconDialog"))
        return static_cast< Ui::MapIconDialog*>(const_cast< MapIconDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int MapIconDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: apply(); break;
        case 1: revert(); break;
        case 2: init(); break;
        case 3: destroy(); break;
        case 4: setMapIcons((*reinterpret_cast< MapIcons*(*)>(_a[1]))); break;
        case 5: mapIconCombo_activated((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: imagePenColor_clicked(); break;
        case 7: imageBrushColor_clicked(); break;
        case 8: highlightPenColor_clicked(); break;
        case 9: highlightBrushColor_clicked(); break;
        case 10: line0PenColor_clicked(); break;
        case 11: line1PenColor_clicked(); break;
        case 12: line2PenColor_clicked(); break;
        case 13: walkPathPenColor_clicked(); break;
        case 14: setupMapIconDisplay(); break;
        case 15: languageChange(); break;
        }
        _id -= 16;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
