/*
 *  kwelcome.h - part of the KDE Help Center
 *
 *  Copyright (C) 1999 Matthias Elter (me@kde.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __kwelcome_h__
#define __kwelcome_h__

#include <qwidget.h>

class QCheckBox;
class QLabel;
class QPushbutton;

class KWelcome : public QWidget
{
  Q_OBJECT

public:
  KWelcome(QWidget *parent = 0, const char *name = 0);
  virtual ~KWelcome();

public slots:
  void slotAboutKDE();
  void slotWizardStart();
  void slotHelpCenterStart();

protected:
  void saveSettings();
  void readSettings();
  virtual void keyPressEvent( QKeyEvent *e );

private:
  QWidget *topView, *bottomView;
  QCheckBox *autostart_kwelcome;
  QPushButton *aboutButton, *quitButton, *wizardButton, *helpcenterButton;
  QLabel *welcome;
};

#endif
