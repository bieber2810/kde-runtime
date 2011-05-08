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

#include "nepomukindexer.h"
#include "indexer/nepomukindexfeeder.h"

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Resource>
#include <Nepomuk/Variant>
#include <Nepomuk/Vocabulary/NIE>

#include <KUrl>
#include <KDebug>
#include <KProcess>
#include <KStandardDirs>

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <strigi/strigiconfig.h>
#include <strigi/indexwriter.h>
#include <strigi/analysisresult.h>
#include <strigi/fileinputstream.h>
#include <strigi/analyzerconfiguration.h>


class Nepomuk::Indexer::Private
{
public:
};


Nepomuk::Indexer::Indexer( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
}


Nepomuk::Indexer::~Indexer()
{
}


void Nepomuk::Indexer::indexFile( const KUrl& url )
{
    const QString exe = KStandardDirs::findExe(QLatin1String("nepomukstrigiindexer"));
    KProcess process;
    process.setOutputChannelMode( KProcess::MergedChannels );
    kDebug() << exe;
    kDebug() << "Executing the process with args " << url.toLocalFile();
    process.setProgram( exe, QStringList() << url.toLocalFile() );
    kDebug() << process.execute();
    kDebug() << "Done executing";
}


void Nepomuk::Indexer::indexFile( const QFileInfo& info )
{
    indexFile( KUrl(info.absoluteFilePath()) );
}

/*
namespace {
    class QDataStreamStrigiBufferedStream : public Strigi::BufferedStream<char>
    {
    public:
        QDataStreamStrigiBufferedStream( QDataStream& stream )
            : m_stream( stream ) {
        }

        int32_t fillBuffer( char* start, int32_t space ) {
            int r = m_stream.readRawData( start, space );
            if ( r == 0 ) {
                // Strigi's API is so weird!
                return -1;
            }
            else if ( r < 0 ) {
                // Again: weird API. m_status is a protected member of StreamBaseBase (yes, 2x Base)
                m_status = Strigi::Error;
                return -1;
            }
            else {
                return r;
            }
        }

    private:
        QDataStream& m_stream;
    };
}
*/

void Nepomuk::Indexer::indexResource( const KUrl& uri, const QDateTime& modificationTime, QDataStream& data )
{
    /*d->m_analyzerConfig.setStop( false );

    Resource dirRes( uri );
    if ( !dirRes.exists() ||
         dirRes.property( Nepomuk::Vocabulary::NIE::lastModified() ).toDateTime() != modificationTime ) {
        Strigi::AnalysisResult analysisresult( uri.toEncoded().data(),
                                               modificationTime.toTime_t(),
                                               *d->m_indexWriter,
                                               *d->m_streamAnalyzer );
        QDataStreamStrigiBufferedStream stream( data );
        analysisresult.index( &stream );
    }
    else {
        kDebug() << uri << "up to date";
    }*/
}


void Nepomuk::Indexer::clearIndexedData( const KUrl& url )
{
    Nepomuk::IndexFeeder::clearIndexedDataForUrl( url );
}


void Nepomuk::Indexer::clearIndexedData( const QFileInfo& info )
{
    Nepomuk::IndexFeeder::clearIndexedDataForUrl( KUrl(info.filePath()) );
}


void Nepomuk::Indexer::clearIndexedData( const Nepomuk::Resource& res )
{
    Nepomuk::IndexFeeder::clearIndexedDataForResourceUri( res.resourceUri() );
}


void Nepomuk::Indexer::stop()
{
    /*kDebug();
    d->m_analyzerConfig.setStop( true );
    d->m_indexFeeder->stop();
    d->m_indexFeeder->wait();
    kDebug() << "done";*/
}


void Nepomuk::Indexer::start()
{
    /*
    kDebug();
    d->m_analyzerConfig.setStop( false );
    d->m_indexFeeder->start();
    kDebug() << "Started";*/
}

#include "nepomukindexer.moc"
