/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * toplevel.cpp
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

#ifndef TOPLEVEL_H
#define TOPLEVEL_H

class KrashConfig;
class DrKBugReport;
class KrashDebugger;
class QLabel;

#include <KDialog>

class Toplevel : public KDialog
{
  Q_OBJECT

public:
  explicit Toplevel(KrashConfig *krash, QWidget *parent = 0);
  ~Toplevel();

private:
  // helper methods
  QString generateText() const;

protected Q_SLOTS:
  void slotUser1();
  void slotUser2();
  void slotNewDebuggingApp(const QString& launchName);
  void slotUser3();

protected Q_SLOTS:
  void slotBacktraceSomeError();
  void slotBacktraceDone(const QString &);
  void expandDetails(bool expand);

private:
  KrashConfig *m_krashconf;
  DrKBugReport *m_bugreport;
  KrashDebugger *m_debugger;
  QLabel *m_detailDescriptionLabel;
};

#endif
