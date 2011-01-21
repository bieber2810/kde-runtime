/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>

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

#include "datamanagementservice.h"
#include "datamanagementmodel.h"
#include "dbusoperators.h"

#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusConnection>


Q_DECLARE_METATYPE(QList<QUrl>)
Q_DECLARE_METATYPE(Nepomuk::PropertyHash)
Q_DECLARE_METATYPE(Nepomuk::SimpleResource)

class Nepomuk::DataManagementService::Private
{
public:
    DataManagementModel* m_model;
};


Nepomuk::DataManagementService::DataManagementService(QObject *parent, const QVariantList& )
    : Service(parent),
      d(new Private())
{
    qDBusRegisterMetaType<QUrl>();
    qDBusRegisterMetaType<QList<QUrl> >();
    qDBusRegisterMetaType<QVariant>();
    qDBusRegisterMetaType<Nepomuk::PropertyHash>();
    qDBusRegisterMetaType<Nepomuk::SimpleResource>();

    d->m_model = new DataManagementModel(mainModel(), this);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/datamanagementmodel"), d->m_model, QDBusConnection::ExportAllSlots);
}


Nepomuk::DataManagementService::~DataManagementService()
{
    delete d;
}


#include <kpluginfactory.h>
#include <kpluginloader.h>

NEPOMUK_EXPORT_SERVICE( Nepomuk::DataManagementService, "nepomukdatamanagementservice" )
