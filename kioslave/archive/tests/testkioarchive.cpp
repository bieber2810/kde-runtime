/*  This file is part of the KDE project
    Copyright (C) 2007 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "testkioarchive.h"
#include "../kp7zip.h"
#include <qtest_kde.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <kio/netaccess.h>
#include <ktar.h>
#include <kstandarddirs.h>
#include <kdebug.h>

QTEST_KDEMAIN(TestKioArchive, NoGUI)
static const char s_tarFileName[] = "karchivetest.tar";
static const char s_p7zipFileName[] = "karchivetest.7z";

static void writeTestFilesToArchive( KArchive* archive )
{
    bool ok;
    ok = archive->writeFile( "empty", "weis", "users", "", 0 );
    QVERIFY( ok );
    ok = archive->writeFile( "test1", "weis", "users", "Hallo", 5 );
    QVERIFY( ok );
    ok = archive->writeFile( "mydir/subfile", "dfaure", "users", "Bonjour", 7 );
    QVERIFY( ok );
    ok = archive->writeSymLink( "mydir/symlink", "subfile", "dfaure", "users" );
    QVERIFY( ok );
}

static void writeTestFilesToP7zipArchive( KArchive* archive )
{
    bool ok;
    ok = archive->prepareWriting( "testFile", archive->directory()->user(), archive->directory()->group(), 0 /*size*/,
                                  0100644, 0 /*atime*/, 0 /*mtime*/, 0 /*ctime*/ );
    QVERIFY( ok );
    QByteArray buffer1( "First buffer" );
    ok = archive->writeData( buffer1.data(), buffer1.size() );
    QVERIFY( ok );
    QByteArray buffer2( "Second buffer" );
    ok = archive->writeData( buffer2.data(), buffer2.size() );
    QVERIFY( ok );
    QByteArray buffer3( "Third buffer" );
    ok = archive->writeData( buffer3.data(), buffer3.size() );
    QVERIFY( ok );
    ok = archive->finishWriting( buffer1.size() + buffer2.size() + buffer3.size() );
    QVERIFY( ok );

    writeTestFilesToArchive(archive);
}

void TestKioArchive::initTestCase()
{
    // Make sure we start clean
    cleanupTestCase();

    // Taken from KArchiveTest::testCreateTar
    KTar tar( s_tarFileName );
    bool ok = tar.open( QIODevice::WriteOnly );
    QVERIFY( ok );
    writeTestFilesToArchive( &tar );
    ok = tar.close();
    QVERIFY( ok );
    QFileInfo fileInfo( QFile::encodeName( s_tarFileName ) );
    QVERIFY( fileInfo.exists() );

    // need to pass absolute path or the archive will be created in a temporary directory.
    KP7zip p7zip( QDir::currentPath() + '/' + s_p7zipFileName );
    ok = p7zip.open( QIODevice::WriteOnly );
    QVERIFY( ok );
    writeTestFilesToP7zipArchive( &p7zip );
    ok = p7zip.close();
    QVERIFY( ok );
    fileInfo.setFile( QFile::encodeName( s_p7zipFileName ) );
    QVERIFY( fileInfo.exists() );
}

void TestKioArchive::testListTar()
{
    m_listResult.clear();
    KIO::ListJob* job = KIO::listDir(tarUrl(), KIO::HideProgressInfo);
    connect( job, SIGNAL(entries(KIO::Job*,KIO::UDSEntryList)),
             SLOT(slotEntries(KIO::Job*,KIO::UDSEntryList)) );
    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    kDebug() << "listDir done - entry count=" << m_listResult.count();
    QVERIFY( m_listResult.count() > 1 );

    kDebug() << m_listResult;
    QCOMPARE(m_listResult.count( "." ), 1); // found it, and only once
    QCOMPARE(m_listResult.count("empty"), 1);
    QCOMPARE(m_listResult.count("test1"), 1);
    QCOMPARE(m_listResult.count("mydir"), 1);
    QCOMPARE(m_listResult.count("mydir/subfile"), 0); // not a recursive listing
    QCOMPARE(m_listResult.count("mydir/symlink"), 0);
}

void TestKioArchive::testListRecursive()
{
    m_listResult.clear();
    KIO::ListJob* job = KIO::listRecursive(tarUrl(), KIO::HideProgressInfo);
    connect( job, SIGNAL(entries(KIO::Job*,KIO::UDSEntryList)),
             SLOT(slotEntries(KIO::Job*,KIO::UDSEntryList)) );
    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    kDebug() << "listDir done - entry count=" << m_listResult.count();
    QVERIFY( m_listResult.count() > 1 );

    kDebug() << m_listResult;
    QCOMPARE(m_listResult.count( "." ), 1); // found it, and only once
    QCOMPARE(m_listResult.count("empty"), 1);
    QCOMPARE(m_listResult.count("test1"), 1);
    QCOMPARE(m_listResult.count("mydir"), 1);
    QCOMPARE(m_listResult.count("mydir/subfile"), 1);
    QCOMPARE(m_listResult.count("mydir/symlink"), 1);
}

KUrl TestKioArchive::tarUrl() const
{
    KUrl url;
    url.setProtocol("tar");
    url.setPath(QDir::currentPath());
    url.addPath(s_tarFileName);
    return url;
}

void TestKioArchive::slotEntries( KIO::Job*, const KIO::UDSEntryList& lst )
{
    for( KIO::UDSEntryList::ConstIterator it = lst.begin(); it != lst.end(); ++it ) {
        const KIO::UDSEntry& entry (*it);
        QString displayName = entry.stringValue( KIO::UDSEntry::UDS_NAME );
        m_listResult << displayName;
    }
}

QString TestKioArchive::tmpDir() const
{
    // Note that this goes into ~/.kde-unit-test (see qtest_kde.h)
    // Use saveLocation if locateLocal doesn't work
    return KStandardDirs::locateLocal("tmp", "test_kio_archive/");
}

void TestKioArchive::cleanupTestCase()
{
    KIO::NetAccess::synchronousRun(KIO::del(tmpDir(), KIO::HideProgressInfo), 0);
}

void TestKioArchive::copyFromArchive(const KUrl& src, const QString& destPath)
{
    KUrl dest(destPath);
    // Check that src exists
    KIO::StatJob* statJob = KIO::stat(src, KIO::StatJob::SourceSide, 0, KIO::HideProgressInfo);
    QVERIFY(KIO::NetAccess::synchronousRun(statJob, 0));

    KIO::Job* job = KIO::copyAs( src, dest, KIO::HideProgressInfo );
    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    QVERIFY( QFile::exists( destPath ) );
}

void TestKioArchive::testExtractFileFromTar()
{
    const QString destPath = tmpDir() + "fileFromTar_copied";
    KUrl u = tarUrl();
    u.addPath("mydir/subfile");
    copyFromArchive(u, destPath);
    QVERIFY(QFileInfo(destPath).isFile());
    QVERIFY(QFileInfo(destPath).size() == 7);
}

void TestKioArchive::testExtractSymlinkFromTar()
{
    const QString destPath = tmpDir() + "symlinkFromTar_copied";
    KUrl u = tarUrl();
    u.addPath("mydir/symlink");
    copyFromArchive(u, destPath);
    QVERIFY(QFileInfo(destPath).isFile());
    QEXPECT_FAIL("", "See #5601 -- on FTP we want to download the real file, not the symlink...", Continue);
    // See comment in 149903
    // Maybe this is something we can do depending on Class=:local and Class=:internet
    // (we already know if a protocol is local or remote).
    // So local->local should copy symlinks, while internet->local and internet->internet should
    // copy the actual file, I guess?
    // -> ### TODO
    QVERIFY(QFileInfo(destPath).isSymLink());
}

KUrl TestKioArchive::p7zipUrl() const
{
    KUrl url;
    url.setProtocol("p7zip");
    url.setPath(QDir::currentPath());
    url.addPath(s_p7zipFileName);
    return url;
}


void TestKioArchive::testListP7zip()
{
    m_listResult.clear();
    KIO::ListJob* job = KIO::listDir(p7zipUrl(), KIO::HideProgressInfo);
    connect( job, SIGNAL(entries(KIO::Job*,KIO::UDSEntryList)),
             SLOT(slotEntries(KIO::Job*,KIO::UDSEntryList)) );
    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    kDebug() << "listDir done - entry count=" << m_listResult.count();
    QVERIFY( m_listResult.count() > 1 );

    kDebug() << m_listResult;
    QCOMPARE(m_listResult.count( "." ), 1); // found it, and only once
    QCOMPARE(m_listResult.count("testFile"), 1);
    QCOMPARE(m_listResult.count("empty"), 1);
    QCOMPARE(m_listResult.count("test1"), 1);
    QCOMPARE(m_listResult.count("mydir"), 1);
    QCOMPARE(m_listResult.count("mydir/subfile"), 0); // not a recursive listing
    QCOMPARE(m_listResult.count("mydir/symlink"), 0);
}

void TestKioArchive::testListP7zipRecursive()
{
    m_listResult.clear();
    KIO::ListJob* job = KIO::listRecursive(p7zipUrl(), KIO::HideProgressInfo);
    connect( job, SIGNAL(entries(KIO::Job*,KIO::UDSEntryList)),
             SLOT(slotEntries(KIO::Job*,KIO::UDSEntryList)) );
    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    kDebug() << "listDir done - entry count=" << m_listResult.count();
    QVERIFY( m_listResult.count() > 1 );

    kDebug() << m_listResult;
    QCOMPARE(m_listResult.count( "." ), 1); // found it, and only once
    QCOMPARE(m_listResult.count("testFile"), 1);
    QCOMPARE(m_listResult.count("empty"), 1);
    QCOMPARE(m_listResult.count("test1"), 1);
    QCOMPARE(m_listResult.count("mydir"), 1);
    QCOMPARE(m_listResult.count("mydir/subfile"), 1);
    QCOMPARE(m_listResult.count("mydir/symlink"), 1);
}

void TestKioArchive::testExtractFileFromP7zip()
{
    const QString destPath = tmpDir() + "fileFromP7zip_copied";
    KUrl u = p7zipUrl();
    u.addPath("mydir/subfile");
    copyFromArchive(u, destPath);
    QVERIFY(QFileInfo(destPath).isFile());
    QVERIFY(QFileInfo(destPath).size() == 7);
}

void TestKioArchive::testExtractSymlinkFromP7zip()
{
    const QString destPath = tmpDir() + "symlinkFromP7zip_copied";
    KUrl u = p7zipUrl();
    u.addPath("mydir/symlink");
    copyFromArchive(u, destPath);
    QVERIFY(QFileInfo(destPath).isFile());
    QEXPECT_FAIL("", "See #5601 -- on FTP we want to download the real file, not the symlink...", Continue);
    // See comment in 149903
    // Maybe this is something we can do depending on Class=:local and Class=:internet
    // (we already know if a protocol is local or remote).
    // So local->local should copy symlinks, while internet->local and internet->internet should
    // copy the actual file, I guess?
    // -> ### TODO
    QVERIFY(QFileInfo(destPath).isSymLink());
}
