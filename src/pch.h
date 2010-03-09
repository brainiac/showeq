
#ifndef __PCH_H__
#define __PCH_H__

#pragma message("Using pre-compiled headers")

#include <QObject>

#include <QApplication>
#include <QBrush>
#include <QByteArray>
#include <QColor>
#include <QColorDialog>
#include <QCloseEvent>
#include <QDateTime>
#include <QCheckBox>
#include <QComboBox>
#include <QDataStream>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QFont>
#include <QHash>
#include <QHBoxLayout>
#include <QImage>
#include <QImageWriter>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QLayout>
#include <QLCDNumber>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QPushButton>
#include <QRegExp>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QToolTip>
#include <QVariant>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <QWidgetAction>

#include "compat.h"

#include <vector>
#include <map>
#include <deque>

#include <limits.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

#endif // __PCH_H__