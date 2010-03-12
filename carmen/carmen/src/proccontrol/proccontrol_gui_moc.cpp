/****************************************************************************
** QDisplay meta object code from reading C++ file 'proccontrol_gui.h'
**
** Created: Mon Feb 20 22:09:23 2006
**      by: The Qt MOC ($Id: proccontrol_gui_moc.cpp 1600 2006-02-27 22:53:24Z dhaehnel $)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "proccontrol_gui.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *QDisplay::className() const
{
    return "QDisplay";
}

QMetaObject *QDisplay::metaObj = 0;
static QMetaObjectCleanUp cleanUp_QDisplay( "QDisplay", &QDisplay::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString QDisplay::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QDisplay", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString QDisplay::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QDisplay", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* QDisplay::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QWidget::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ 0, &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"startClicked", 1, param_slot_0 };
    static const QUParameter param_slot_1[] = {
	{ 0, &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_1 = {"stopClicked", 1, param_slot_1 };
    static const QUParameter param_slot_2[] = {
	{ 0, &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_2 = {"showClicked", 1, param_slot_2 };
    static const QUParameter param_slot_3[] = {
	{ 0, &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_3 = {"noClicked", 1, param_slot_3 };
    static const QMetaData slot_tbl[] = {
	{ "startClicked(int)", &slot_0, QMetaData::Private },
	{ "stopClicked(int)", &slot_1, QMetaData::Private },
	{ "showClicked(int)", &slot_2, QMetaData::Private },
	{ "noClicked(int)", &slot_3, QMetaData::Private }
    };
    metaObj = QMetaObject::new_metaobject(
	"QDisplay", parentObject,
	slot_tbl, 4,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_QDisplay.setMetaObject( metaObj );
    return metaObj;
}

void* QDisplay::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "QDisplay" ) )
	return this;
    return QWidget::qt_cast( clname );
}

bool QDisplay::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: startClicked((int)static_QUType_int.get(_o+1)); break;
    case 1: stopClicked((int)static_QUType_int.get(_o+1)); break;
    case 2: showClicked((int)static_QUType_int.get(_o+1)); break;
    case 3: noClicked((int)static_QUType_int.get(_o+1)); break;
    default:
	return QWidget::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool QDisplay::qt_emit( int _id, QUObject* _o )
{
    return QWidget::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool QDisplay::qt_property( int id, int f, QVariant* v)
{
    return QWidget::qt_property( id, f, v);
}

bool QDisplay::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
