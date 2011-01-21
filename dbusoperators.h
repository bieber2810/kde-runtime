/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DBUSOPERATORS_H
#define DBUSOPERATORS_H

#include <QtDBus/QDBusArgument>
#include <QtCore/QUrl>

#include "simpleresource.h"

QDBusArgument& operator<<( QDBusArgument& arg, const QUrl& url )
{
    return arg << QString::fromAscii( url.toEncoded() );
}

const QDBusArgument& operator>>( const QDBusArgument& arg, QUrl& url )
{
    QString s;
    arg >> s;
    url = QUrl::fromEncoded( s.toAscii() );
    return arg;
}

QDBusArgument& operator<<( QDBusArgument& arg, const QVariant& v )
{
    return arg << QDBusVariant(v);
}

const QDBusArgument& operator>>( const QDBusArgument& arg, QVariant& v )
{
    QDBusVariant dbusV;
    arg >> dbusV;
    v = dbusV.variant();
    return arg;
}

QDBusArgument& operator<<( QDBusArgument& arg, const Nepomuk::SimpleResource& res )
{
    arg.beginStructure();

    arg << res.m_uri << res.m_properties;

    arg.endStructure();

    return arg;
}

const QDBusArgument& operator>>( const QDBusArgument& arg, Nepomuk::SimpleResource& res )
{
    arg.beginStructure();
    arg >> res.m_uri;
    arg >> res.m_properties;
    arg.endStructure();
    return arg;
}

#endif // DBUSOPERATORS_H
