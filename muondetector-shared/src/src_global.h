#ifndef SRC_GLOBAL_H
#define SRC_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(SRC_LIBRARY)
#  define SRCSHARED_EXPORT Q_DECL_EXPORT
#else
#  define SRCSHARED_EXPORT Q_DECL_IMPORT
#endif

#include <ublox_messages.h>
#include <tcpmessage_keys.h>
#include <tcpmessage.h>
#include <ubx_msg_key_name_map.h>
#include <gpio_pin_definitions.h>
#include <geodeticpos.h>
#include <tcpconnection.h>

#endif // SRC_GLOBAL_H
