/*
 *  Copyright (C) 2002, 2003 David Faure   <faure@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation;
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "qtest_kde.h"

#include <kurifilter.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <ksharedconfig.h>
#include <klocale.h>
#include <kio/netaccess.h>

#include <QtCore/QDir>
#include <QtCore/QRegExp>

#include <config.h>
#include <iostream>
#include "kurifiltertest.h"

QTEST_KDEMAIN( KUriFilterTest, NoGUI )

static const char * const s_uritypes[] = { "NET_PROTOCOL", "LOCAL_FILE", "LOCAL_DIR", "EXECUTABLE", "HELP", "SHELL", "BLOCKED", "ERROR", "UNKNOWN" };
#define NO_FILTERING -2

static void filter( const char* u, const char * expectedResult = 0, int expectedUriType = -1, QStringList list = QStringList(), const char * abs_path = 0, bool checkForExecutables = true )
{
    QString a = QString::fromUtf8( u );
    KUriFilterData * filterData = new KUriFilterData;
    filterData->setData( a );
    filterData->setCheckForExecutables( checkForExecutables );

    if( abs_path )
    {
        filterData->setAbsolutePath( QLatin1String( abs_path ) );
        kDebug() << "Filtering: " << a << " with abs_path=" << abs_path << endl;
    }
    else
        kDebug() << "Filtering: " << a << endl;

    if (KUriFilter::self()->filterUri(*filterData, list))
    {
        // Copied from minicli...
        QString cmd;
        KUrl uri = filterData->uri();

        if ( uri.isLocalFile() && !uri.hasRef() && uri.query().isEmpty() )
            cmd = uri.path();
        else
            cmd = uri.url();

        switch( filterData->uriType() )
        {
            case KUriFilterData::LOCAL_FILE:
            case KUriFilterData::LOCAL_DIR:
            case KUriFilterData::HELP:
                kDebug() << "*** Result: Local Resource =>  '"
                          << filterData->uri().url() << "'" << endl;
                break;
            case KUriFilterData::NET_PROTOCOL:
                kDebug() << "*** Result: Network Resource => '"
                          << filterData->uri().url() << "'" << endl;
                break;
            case KUriFilterData::SHELL:
            case KUriFilterData::EXECUTABLE:
                if( filterData->hasArgsAndOptions() )
                    cmd += filterData->argsAndOptions();
                kDebug() << "*** Result: Executable/Shell => '" << cmd << "'"<< endl;
                break;
            case KUriFilterData::ERROR:
                kDebug() << "*** Result: Encountered error. See reason below." << endl;
                break;
            default:
                kDebug() << "*** Result: Unknown or invalid resource." << endl;
        }

        if ( expectedUriType != -1 && expectedUriType != filterData->uriType() )
        {
            QCOMPARE( s_uritypes[filterData->uriType()],
                      s_uritypes[expectedUriType] );
            kError() << " Got URI type " << s_uritypes[filterData->uriType()]
                      << " expected " << s_uritypes[expectedUriType] << endl;
        }

        if ( expectedResult )
        {
            // Hack for other locales than english, normalize google hosts to google.com
            cmd = cmd.replace( QRegExp( "www\\.google\\.[^/]*/" ), "www.google.com/" );
            QString expected = QString::fromUtf8( expectedResult );
            QCOMPARE( cmd, expected );
        }
    }
    else
    {
        if ( expectedUriType == NO_FILTERING )
            kDebug() << "*** No filtering required." << endl;
        else
        {
            kDebug() << "*** Could not be filtered." << endl;
            if( expectedUriType != filterData->uriType() )
            {
                QCOMPARE( s_uritypes[filterData->uriType()],
                          s_uritypes[expectedUriType] );
            }
        }
    }

    delete filterData;
    kDebug() << "-----" << endl;
}

static void testLocalFile( const QString& filename )
{
    QFile tmpFile( filename ); // Yeah, I know, security risk blah blah. This is a test prog!

    if ( tmpFile.open( QIODevice::ReadWrite ) )
    {
        QByteArray fname = QFile::encodeName( tmpFile.fileName() );
        filter(fname, fname, KUriFilterData::LOCAL_FILE);
        tmpFile.close();
        tmpFile.remove();
    }
    else
        kDebug() << "Couldn't create " << tmpFile.fileName() << ", skipping test" << endl;
}

static char s_delimiter = ':'; // the alternative is ' '

KUriFilterTest::KUriFilterTest()
{
    minicliFilters << "kshorturifilter" << "kurisearchfilter" << "localdomainurifilter";
    qtdir = getenv("QTDIR");
    home = getenv("HOME");
    kdehome = getenv("KDEHOME");
}

void KUriFilterTest::init()
{
    kDebug() << k_funcinfo << endl;
    setenv( "KDE_FORK_SLAVES", "yes", true ); // simpler, for the final cleanup

    // Allow testing of the search engine using both delimiters...
    const char* envDelimiter = ::getenv( "KURIFILTERTEST_DELIMITER" );
    if ( envDelimiter )
        s_delimiter = envDelimiter[0];

    // Many tests check the "default search engine" feature.
    // There is no default search engine by default (since it was annoying when making typos),
    // so the user has to set it up, which we do here.
    {
      KConfigGroup cfg( KSharedConfig::openConfig( "kuriikwsfilterrc", KConfig::OnlyLocal ), "General" );
      cfg.writeEntry( "DefaultSearchEngine", "google" );
      cfg.writeEntry( "Verbose", true );
      cfg.writeEntry( "KeywordDelimiter", QString(s_delimiter) );
      cfg.sync();
    }

    // Enable verbosity for debugging
    {
      KSharedConfig::openConfig("kshorturifilterrc", KConfig::OnlyLocal )->group(QString()).writeEntry( "Verbose", true );
    }

    KStandardDirs::makeDir( kdehome+"/urifilter" );
}

void KUriFilterTest::tests()
{
    // URI that should require no filtering
    filter( "http://www.kde.org", "http://www.kde.org", KUriFilterData::NET_PROTOCOL );
    filter( "http://www.kde.org/developer//index.html", "http://www.kde.org/developer//index.html", KUriFilterData::NET_PROTOCOL );
    // URL with reference
    filter( "http://www.kde.org/index.html#q8", "http://www.kde.org/index.html#q8", KUriFilterData::NET_PROTOCOL );
    // local file with reference
    filter( "file:/etc/passwd#q8", "file:///etc/passwd#q8", KUriFilterData::LOCAL_FILE );
    filter( "file:///etc/passwd#q8", "file:///etc/passwd#q8", KUriFilterData::LOCAL_FILE );
    filter( "/etc/passwd#q8", "file:///etc/passwd#q8", KUriFilterData::LOCAL_FILE );
    // local file with query (can be used by javascript)
    filter( "file:/etc/passwd?foo=bar", "file:///etc/passwd?foo=bar", KUriFilterData::LOCAL_FILE );
    testLocalFile( "/tmp/kurifiltertest?foo" ); // local file with ? in the name (#58990)
    testLocalFile( "/tmp/kurlfiltertest#foo" ); // local file with '#' in the name
    testLocalFile( "/tmp/kurlfiltertest#foo?bar" ); // local file with both
    testLocalFile( "/tmp/kurlfiltertest?foo#bar" ); // local file with both, the other way round

    // hostnames are lowercased by KUrl
    filter( "http://www.myDomain.commyPort/ViewObjectRes//Default:name=hello",
            "http://www.mydomain.commyport/ViewObjectRes//Default:name=hello", KUriFilterData::NET_PROTOCOL);
    filter( "ftp://ftp.kde.org", "ftp://ftp.kde.org", KUriFilterData::NET_PROTOCOL );
    filter( "ftp://username@ftp.kde.org:500", "ftp://username@ftp.kde.org:500", KUriFilterData::NET_PROTOCOL );

    // ShortURI/LocalDomain filter tests. NOTE: any of these tests can fail
    // if you have specified your own patterns in kshorturifilterrc. For
    // examples, see $KDEDIR/share/config/kshorturifilterrc .
    filter( "linuxtoday.com", "http://linuxtoday.com", KUriFilterData::NET_PROTOCOL );
    filter( "LINUXTODAY.COM", "http://linuxtoday.com", KUriFilterData::NET_PROTOCOL );
    filter( "kde.org", "http://kde.org", KUriFilterData::NET_PROTOCOL );
    filter( "ftp.kde.org", "ftp://ftp.kde.org", KUriFilterData::NET_PROTOCOL );
    filter( "ftp.kde.org:21", "ftp://ftp.kde.org:21", KUriFilterData::NET_PROTOCOL );
    filter( "cr.yp.to", "http://cr.yp.to", KUriFilterData::NET_PROTOCOL );
    filter( "user@192.168.1.0:3128", "http://user@192.168.1.0:3128", KUriFilterData::NET_PROTOCOL );
    filter( "127.0.0.1", "http://127.0.0.1", KUriFilterData::NET_PROTOCOL );
    filter( "127.0.0.1:3128", "http://127.0.0.1:3128", KUriFilterData::NET_PROTOCOL );
    filter( "foo@bar.com", "mailto:foo@bar.com", KUriFilterData::NET_PROTOCOL );
    filter( "firstname.lastname@x.foo.bar", "mailto:firstname.lastname@x.foo.bar", KUriFilterData::NET_PROTOCOL );
    filter( "www.123.foo", "http://www.123.foo", KUriFilterData::NET_PROTOCOL );
    filter( "user@www.123.foo:3128", "http://user@www.123.foo:3128", KUriFilterData::NET_PROTOCOL );

    // Exotic IPv4 address formats...
    filter( "127.1", "http://127.1", KUriFilterData::NET_PROTOCOL );
    filter( "127.0.1", "http://127.0.1", KUriFilterData::NET_PROTOCOL );

    // Local domain filter - If you uncomment these test, make sure you
    // you adjust it based on the localhost entry in your /etc/hosts file.
    // filter( "localhost:3128", "http://localhost.localdomain:3128", KUriFilterData::NET_PROTOCOL );
    // filter( "localhost", "http://localhost.localdomain", KUriFilterData::NET_PROTOCOL );
    // filter( "localhost/~blah", "http://localhost.localdomain/~blah", KUriFilterData::NET_PROTOCOL );

    filter( "/", "/", KUriFilterData::LOCAL_DIR );
    filter( "/", "/", KUriFilterData::LOCAL_DIR, QStringList( "kshorturifilter" ) );
    filter( "~/.bashrc", QDir::homePath().toLocal8Bit()+"/.bashrc", KUriFilterData::LOCAL_FILE, QStringList( "kshorturifilter" ) );
    filter( "~", QDir::homePath().toLocal8Bit(), KUriFilterData::LOCAL_DIR, QStringList( "kshorturifilter" ), "/tmp" );
    filter( "~foobar", 0, KUriFilterData::ERROR, QStringList( "kshorturifilter" ) );
    filter( "user@host.domain", "mailto:user@host.domain", KUriFilterData::NET_PROTOCOL ); // new in KDE-3.2

    // Windows style SMB (UNC) URL. Should be converted into the valid smb format...
    filter( "\\\\mainserver\\share\\file", "smb://mainserver/share/file" , KUriFilterData::NET_PROTOCOL );

    // KDE3: was not be filtered at all. All valid protocols of this form were be ignored.
    // KDE4: parsed as "network protocol", seems fine to me (DF)
    filter( "ftp:" , "ftp:", KUriFilterData::NET_PROTOCOL );
    filter( "http:" , "http:", KUriFilterData::NET_PROTOCOL );

    // The default search engine is set to 'Google'
    filter( "gg:", "http://www.google.com/search?q=gg%3A&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL );
    filter( "KDE", "http://www.google.com/search?q=KDE&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL );
    filter( "FTP", "http://www.google.com/search?q=FTP&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL );

    // Typing 'cp' or any other valid unix command in konq's location bar should result in
    // a search using the default search engine
    // 'ls' is a bit of a special case though, due to the toplevel domain called 'ls'
    filter( "cp", "http://www.google.com/search?q=cp&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL,
            QStringList(), 0, false /* don't check for executables, see konq_misc.cc */ );

    // Executable tests - No IKWS in minicli
    filter( "cp", "cp", KUriFilterData::EXECUTABLE, minicliFilters );
    filter( "kfmclient", "kfmclient", KUriFilterData::EXECUTABLE, minicliFilters );
    filter( "xwininfo", "xwininfo", KUriFilterData::EXECUTABLE, minicliFilters );
    filter( "KDE", "KDE", NO_FILTERING, minicliFilters );
    filter( "I/dont/exist", "I/dont/exist", NO_FILTERING, minicliFilters );
    filter( "/I/dont/exist", 0, KUriFilterData::ERROR, minicliFilters );
    filter( "/I/dont/exist#a", 0, KUriFilterData::ERROR, minicliFilters );
    filter( "kfmclient --help", "kfmclient --help", KUriFilterData::EXECUTABLE, minicliFilters ); // the args are in argsAndOptions()
    filter( "/usr/bin/gs", "/usr/bin/gs", KUriFilterData::EXECUTABLE, minicliFilters );
    filter( "/usr/bin/gs -q -option arg1", "/usr/bin/gs -q -option arg1", KUriFilterData::EXECUTABLE, minicliFilters ); // the args are in argsAndOptions()

    // ENVIRONMENT variable
    setenv( "SOMEVAR", "/somevar", 0 );
    setenv( "ETC", "/etc", 0 );

    filter( "$SOMEVAR/kdelibs/kio", 0, KUriFilterData::ERROR ); // note: this dir doesn't exist...
    filter( "$ETC/passwd", "/etc/passwd", KUriFilterData::LOCAL_FILE );
    filter( "$QTDIR/doc/html/functions.html#s", QByteArray("file://")+qtdir+"/doc/html/functions.html#s", KUriFilterData::LOCAL_FILE );
    filter( "http://www.kde.org/$USER", "http://www.kde.org/$USER", KUriFilterData::NET_PROTOCOL ); // no expansion

    filter( "$KDEHOME/share", kdehome+"/share", KUriFilterData::LOCAL_DIR );
    KStandardDirs::makeDir( kdehome+"/urifilter/a+plus" );
    filter( "$KDEHOME/urifilter/a+plus", kdehome+"/urifilter/a+plus", KUriFilterData::LOCAL_DIR );

    // BR 27788
    KStandardDirs::makeDir( kdehome+"/share/Dir With Space" );
    filter( "$KDEHOME/share/Dir With Space", kdehome+"/share/Dir With Space", KUriFilterData::LOCAL_DIR );

    // support for name filters (BR 93825)
    filter( "$KDEHOME/*.txt", kdehome+"/*.txt", KUriFilterData::LOCAL_DIR );
    filter( "$KDEHOME/[a-b]*.txt", kdehome+"/[a-b]*.txt", KUriFilterData::LOCAL_DIR );
    filter( "$KDEHOME/a?c.txt", kdehome+"/a?c.txt", KUriFilterData::LOCAL_DIR );
    filter( "$KDEHOME/?c.txt", kdehome+"/?c.txt", KUriFilterData::LOCAL_DIR );
    // but let's check that a directory with * in the name still works
    KStandardDirs::makeDir( kdehome+"/share/Dir*With*Stars" );
    filter( "$KDEHOME/share/Dir*With*Stars", kdehome+"/share/Dir*With*Stars", KUriFilterData::LOCAL_DIR );
    KStandardDirs::makeDir( kdehome+"/share/Dir?QuestionMark" );
    filter( "$KDEHOME/share/Dir?QuestionMark", kdehome+"/share/Dir?QuestionMark", KUriFilterData::LOCAL_DIR );
    KStandardDirs::makeDir( kdehome+"/share/Dir[Bracket" );
    filter( "$KDEHOME/share/Dir[Bracket", kdehome+"/share/Dir[Bracket", KUriFilterData::LOCAL_DIR );

    filter( "$HOME/$KDEDIR/kdebase/kcontrol/ebrowsing", 0, KUriFilterData::ERROR );
    filter( "$1/$2/$3", "http://www.google.com/search?q=%241%2F%242%2F%243&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL );  // can be used as bogus or valid test. Currently triggers default search, i.e. google
    filter( "$$$$", "http://www.google.com/search?q=%24%24%24%24&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL ); // worst case scenarios.

    // Replaced the match testing with a 0 since
    // the shortURI filter will return the string
    // itself if the requested environment variable
    // is not already set.
    filter( "$QTDIR", 0, KUriFilterData::LOCAL_DIR, QStringList( "kshorturifilter" ) ); //use specific filter.
    filter( "$HOME", home, KUriFilterData::LOCAL_DIR, QStringList( "kshorturifilter" ) ); //use specific filter.


    QString sc;
    filter( sc.sprintf("gg%cfoo bar",s_delimiter).toUtf8(), "http://www.google.com/search?q=foo+bar&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL );
    filter( sc.sprintf("bug%c55798", s_delimiter).toUtf8(), "http://bugs.kde.org/show_bug.cgi?id=55798", KUriFilterData::NET_PROTOCOL );

    filter( sc.sprintf("gg%cC++", s_delimiter).toUtf8(), "http://www.google.com/search?q=C%2B%2B&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL );
    filter( sc.sprintf("ya%cfoo bar was here", s_delimiter).toUtf8(), 0, -1 ); // this triggers default search, i.e. google
    filter( sc.sprintf("gg%cwww.kde.org", s_delimiter).toUtf8(), "http://www.google.com/search?q=www.kde.org&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL );
    filter( sc.sprintf("av%c+rock +sample", s_delimiter).toUtf8(), "http://www.altavista.com/cgi-bin/query?pg=q&kl=XX&stype=stext&q=%2Brock+%2Bsample", KUriFilterData::NET_PROTOCOL );
    filter( sc.sprintf("gg%cé", s_delimiter).toUtf8() /*eaccent in utf8*/, "http://www.google.com/search?q=%C3%A9&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL );
    filter( sc.sprintf("gg%cпрйвет", s_delimiter).toUtf8() /* greetings in russian utf-8*/, "http://www.google.com/search?q=%D0%BF%D1%80%D0%B9%D0%B2%D0%B5%D1%82&ie=UTF-8&oe=UTF-8", KUriFilterData::NET_PROTOCOL );

    // Absolute Path tests for kshorturifilter
    filter( "./", kdehome+"/share", KUriFilterData::LOCAL_DIR, QStringList( "kshorturifilter" ), kdehome+"/share/" ); // cleanPath removes the trailing slash
    filter( "../", kdehome, KUriFilterData::LOCAL_DIR, QStringList( "kshorturifilter" ), kdehome+"/share" );
    filter( "config", kdehome+"/share/config", KUriFilterData::LOCAL_DIR, QStringList( "kshorturifilter" ), kdehome+"/share" );
}

#include "kurifiltertest.moc"
