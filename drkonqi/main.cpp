/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************/

#include <config-runtime.h>

#include <stdlib.h>
#include <unistd.h>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kdefakes.h>

#include "drkonqi.h"
#include "drkonqidialog.h"

static const char version[] = "1.9";
static const char description[] = I18N_NOOP( "KDE crash handler gives the user feedback if a program crashed" );

int main( int argc, char* argv[] )
{
#ifndef Q_OS_WIN //krazy:exclude=cpp
// Drop privs.
setgid(getgid());
if (setuid(getuid()) < 0 && geteuid() != getuid())
    exit (255);
#endif

    // Make sure that DrKonqi doesn't start DrKonqi when it crashes :-]
    setenv("KDE_DEBUG", "true", 1);

    KAboutData aboutData( "drkonqi", 0, ki18n("The KDE Crash Handler"),
                            version, ki18n(description),
                            KAboutData::License_GPL,
                            ki18n("(C) 2000-2009, The DrKonqi Authors"));
    aboutData.addAuthor(ki18n("Hans Petter Bieker"), KLocalizedString(), "bieker@kde.org");
    aboutData.addAuthor(ki18n("Dario Andres Rodriguez"), KLocalizedString(), "andresbajotierra@gmail.com");
    aboutData.addAuthor(ki18n("George Kiagiadakis"), KLocalizedString(), "gkiagia@users.sourceforge.net");
    aboutData.setProgramIconName("tools-report-bug");

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("signal <number>", ki18n("The signal number that was caught"));
    options.add("appname <name>", ki18n("Name of the program"));
    options.add("apppath <path>", ki18n("Path to the executable"));
    options.add("appversion <version>", ki18n("The version of the program"));
    options.add("bugaddress <address>", ki18n("The bug address to use"));
    options.add("programname <name>", ki18n("Translated name of the program"));
    options.add("pid <pid>", ki18n("The PID of the program"));
    options.add("startupid <id>", ki18n("Startup ID of the program"));
    options.add("kdeinit", ki18n("The program was started by kdeinit"));
    options.add("safer", ki18n("Disable arbitrary disk access"));
    KCmdLineArgs::addCmdLineOptions( options );

    KComponentData inst(KCmdLineArgs::aboutData());
    QApplication *qa =
        KCmdLineArgs::parsedArgs()->isSet("safer") ?
        new QApplication(KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv()) :
        new KApplication;
    qa->setApplicationName(inst.componentName());

    if ( !DrKonqi::instance()->init() )
        return 1;

    int ret;
    {
        DrKonqiDialog w;
        w.show();
        ret = qa->exec();
    }

    DrKonqi::instance()->cleanup();
    delete qa;
    return ret;
}
