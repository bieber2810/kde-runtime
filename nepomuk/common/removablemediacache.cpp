/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>

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

#include "removablemediacache.h"

#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/Block>
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/NetworkShare>
#include <Solid/OpticalDisc>
#include <Solid/Predicate>

#include <KDebug>

#include <QtCore/QMutexLocker>


namespace {
    bool isUsableVolume( const Solid::Device& dev ) {
        if ( dev.is<Solid::StorageAccess>() ) {
            if( dev.is<Solid::StorageVolume>() &&
                    dev.parent().is<Solid::StorageDrive>() &&
                    ( dev.parent().as<Solid::StorageDrive>()->isRemovable() ||
                      dev.parent().as<Solid::StorageDrive>()->isHotpluggable() ) ) {
                const Solid::StorageVolume* volume = dev.as<Solid::StorageVolume>();
                if ( !volume->isIgnored() && volume->usage() == Solid::StorageVolume::FileSystem )
                    return true;
            }
            else if(dev.is<Solid::NetworkShare>()) {
                return !dev.as<Solid::NetworkShare>()->url().isEmpty();
            }
        }

        // fallback
        return false;
    }

    bool isUsableVolume( const QString& udi ) {
        Solid::Device dev( udi );
        return isUsableVolume( dev );
    }
}


Nepomuk::RemovableMediaCache::RemovableMediaCache(QObject *parent)
    : QObject(parent)
{
    initCacheEntries();

    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString& ) ),
             this, SLOT( slotSolidDeviceAdded( const QString& ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( const QString& ) ),
             this, SLOT( slotSolidDeviceRemoved( const QString& ) ) );
}


Nepomuk::RemovableMediaCache::~RemovableMediaCache()
{
}


void Nepomuk::RemovableMediaCache::initCacheEntries()
{
    QList<Solid::Device> devices
            = Solid::Device::listFromQuery(QLatin1String("StorageVolume.usage=='FileSystem'"))
            + Solid::Device::listFromType(Solid::DeviceInterface::NetworkShare);
    foreach( const Solid::Device& dev, devices ) {
        if ( isUsableVolume( dev ) ) {
            if(Entry* entry = createCacheEntry( dev )) {
                const Solid::StorageAccess* storage = entry->device().as<Solid::StorageAccess>();
                if ( storage && storage->isAccessible() )
                    slotAccessibilityChanged( true, dev.udi() );
            }
        }
    }
}

Nepomuk::RemovableMediaCache::Entry* Nepomuk::RemovableMediaCache::createCacheEntry( const Solid::Device& dev )
{
    QMutexLocker lock(&m_entryCacheMutex);

    Entry entry(dev);
    if(!entry.url().isEmpty()) {
        kDebug() << "Usable" << dev.udi();

        // we only add to this set and never remove. This is no problem as this is a small set
        m_usedSchemas.insert(KUrl(entry.url()).scheme());

        connect( dev.as<Solid::StorageAccess>(), SIGNAL(accessibilityChanged(bool, QString)),
                 this, SLOT(slotAccessibilityChanged(bool, QString)) );

        m_metadataCache.insert( dev.udi(), entry );

        emit deviceAdded(&m_metadataCache[dev.udi()]);

        return &m_metadataCache[dev.udi()];
    }
    else {
        kDebug() << "Cannot use device due to empty identifier:" << dev.udi();
        return 0;
    }
}


const Nepomuk::RemovableMediaCache::Entry* Nepomuk::RemovableMediaCache::findEntryByFilePath( const QString& path ) const
{
    QMutexLocker lock(&m_entryCacheMutex);

    for( QHash<QString, Entry>::const_iterator it = m_metadataCache.begin();
         it != m_metadataCache.end(); ++it ) {
        const Entry& entry = *it;
        if ( entry.device().as<Solid::StorageAccess>()->isAccessible() &&
             path.startsWith( entry.device().as<Solid::StorageAccess>()->filePath() ) )
            return &entry;
    }

    return 0;
}


const Nepomuk::RemovableMediaCache::Entry* Nepomuk::RemovableMediaCache::findEntryByUrl(const KUrl &url) const
{
    QMutexLocker lock(&m_entryCacheMutex);

    for( QHash<QString, Entry>::const_iterator it = m_metadataCache.constBegin();
        it != m_metadataCache.constEnd(); ++it ) {
        const Entry& entry = *it;
        kDebug() << url << entry.url();
        if(url.url().startsWith(entry.url())) {
            return &entry;
        }
    }

    return 0;
}


bool Nepomuk::RemovableMediaCache::hasRemovableSchema(const KUrl &url) const
{
    return m_usedSchemas.contains(url.scheme());
}


void Nepomuk::RemovableMediaCache::slotSolidDeviceAdded( const QString& udi )
{
    kDebug() << udi;

    if ( isUsableVolume( udi ) ) {
        createCacheEntry( Solid::Device( udi ) );
    }
}


void Nepomuk::RemovableMediaCache::slotSolidDeviceRemoved( const QString& udi )
{
    kDebug() << udi;
    if ( m_metadataCache.contains( udi ) ) {
        kDebug() << "Found removable storage volume for Nepomuk undocking:" << udi;
        m_metadataCache.remove( udi );
    }
}


void Nepomuk::RemovableMediaCache::slotAccessibilityChanged( bool accessible, const QString& udi )
{
    kDebug() << accessible << udi;

    //
    // cache new mount path
    //
    if ( accessible ) {
        QMutexLocker lock(&m_entryCacheMutex);
        Entry* entry = &m_metadataCache[udi];
        kDebug() << udi << "accessible at" << entry->device().as<Solid::StorageAccess>()->filePath() << "with identifier" << entry->url();
        emit deviceMounted(entry);
    }
}


Nepomuk::RemovableMediaCache::Entry::Entry()
{
}


Nepomuk::RemovableMediaCache::Entry::Entry(const Solid::Device& device)
    : m_device(device)
{
    if(device.is<Solid::StorageVolume>()) {
        const Solid::StorageVolume* volume = m_device.as<Solid::StorageVolume>();
        if(device.is<Solid::OpticalDisc>()) {
            // we use the label as is - it is not even close to unique but
            // so far we have nothing better
            m_urlPrefix = QLatin1String("optical://") + volume->label();
        }
        else if(!volume->uuid().isEmpty()) {
            // we always use lower-case uuids
            m_urlPrefix = QLatin1String("filex://") + volume->uuid().toLower();
        }
    }
    else if(device.is<Solid::NetworkShare>()) {
        m_urlPrefix = device.as<Solid::NetworkShare>()->url().toString();
    }
}

KUrl Nepomuk::RemovableMediaCache::Entry::constructRelativeUrl( const QString& path ) const
{
    if(const Solid::StorageAccess* sa = m_device.as<Solid::StorageAccess>()) {
        if(sa->isAccessible()) {
            const QString relativePath = path.mid( sa->filePath().count() );
            return KUrl( m_urlPrefix + relativePath );
        }
    }

    // fallback
    return KUrl();
}


QString Nepomuk::RemovableMediaCache::Entry::constructLocalPath( const KUrl& filexUrl ) const
{
    if(const Solid::StorageAccess* sa = m_device.as<Solid::StorageAccess>()) {
        if(sa->isAccessible()) {
            // the base of the path: the mount path
            QString path( sa->filePath() );
            if ( path.endsWith( QLatin1String( "/" ) ) )
                path.truncate( path.length()-1 );

            return path + filexUrl.url().mid(m_urlPrefix.count());
        }
    }

    // fallback
    return QString();
}

QString Nepomuk::RemovableMediaCache::Entry::mountPath() const
{
    if(const Solid::StorageAccess* sa = m_device.as<Solid::StorageAccess>()) {
        return sa->filePath();
    }
    else {
        return QString();
    }
}

#include "removablemediacache.moc"