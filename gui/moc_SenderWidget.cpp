#include "src/SenderWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SenderWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SenderWidget_t {
    QByteArrayData data[20];
    char stringdata0[250];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SenderWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SenderWidget_t qt_meta_stringdata_SenderWidget = {
    {
QT_MOC_LITERAL(0, 0, 12), // "SenderWidget"
QT_MOC_LITERAL(1, 13, 12), // "browseConfig"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 10), // "loadConfig"
QT_MOC_LITERAL(4, 38, 10), // "saveConfig"
QT_MOC_LITERAL(5, 49, 12), // "saveConfigAs"
QT_MOC_LITERAL(6, 62, 8), // "addFrame"
QT_MOC_LITERAL(7, 71, 20), // "removeSelectedFrames"
QT_MOC_LITERAL(8, 92, 11), // "startSender"
QT_MOC_LITERAL(9, 104, 16), // "startSenderMulti"
QT_MOC_LITERAL(10, 121, 10), // "stopSender"
QT_MOC_LITERAL(11, 132, 11), // "clearOutput"
QT_MOC_LITERAL(12, 144, 15), // "exportOutputTxt"
QT_MOC_LITERAL(13, 160, 16), // "reloadInterfaces"
QT_MOC_LITERAL(14, 177, 17), // "updateVlanDerived"
QT_MOC_LITERAL(15, 195, 6), // "onTick"
QT_MOC_LITERAL(16, 202, 10), // "onFinished"
QT_MOC_LITERAL(17, 213, 8), // "exitCode"
QT_MOC_LITERAL(18, 222, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(19, 243, 6) // "status"

    },
    "SenderWidget\0browseConfig\0\0loadConfig\0"
    "saveConfig\0saveConfigAs\0addFrame\0"
    "removeSelectedFrames\0startSender\0"
    "startSenderMulti\0stopSender\0clearOutput\0"
    "exportOutputTxt\0reloadInterfaces\0"
    "updateVlanDerived\0onTick\0onFinished\0"
    "exitCode\0QProcess::ExitStatus\0status"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SenderWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   89,    2, 0x08 /* Private */,
       3,    0,   90,    2, 0x08 /* Private */,
       4,    0,   91,    2, 0x08 /* Private */,
       5,    0,   92,    2, 0x08 /* Private */,
       6,    0,   93,    2, 0x08 /* Private */,
       7,    0,   94,    2, 0x08 /* Private */,
       8,    0,   95,    2, 0x08 /* Private */,
       9,    0,   96,    2, 0x08 /* Private */,
      10,    0,   97,    2, 0x08 /* Private */,
      11,    0,   98,    2, 0x08 /* Private */,
      12,    0,   99,    2, 0x08 /* Private */,
      13,    0,  100,    2, 0x08 /* Private */,
      14,    0,  101,    2, 0x08 /* Private */,
      15,    0,  102,    2, 0x08 /* Private */,
      16,    2,  103,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 18,   17,   19,

       0        // eod
};

void SenderWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SenderWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->browseConfig(); break;
        case 1: _t->loadConfig(); break;
        case 2: _t->saveConfig(); break;
        case 3: _t->saveConfigAs(); break;
        case 4: _t->addFrame(); break;
        case 5: _t->removeSelectedFrames(); break;
        case 6: _t->startSender(); break;
        case 7: _t->startSenderMulti(); break;
        case 8: _t->stopSender(); break;
        case 9: _t->clearOutput(); break;
        case 10: _t->exportOutputTxt(); break;
        case 11: _t->reloadInterfaces(); break;
        case 12: _t->updateVlanDerived(); break;
        case 13: _t->onTick(); break;
        case 14: _t->onFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SenderWidget::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_SenderWidget.data,
    qt_meta_data_SenderWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SenderWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SenderWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SenderWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SenderWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 15;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
