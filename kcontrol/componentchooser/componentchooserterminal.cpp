/***************************************************************************
                          componentchooser.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License verstion 2 as    *
 *   published by the Free Software Foundation                             *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>

#include "componentchooserterminal.h"
#include "componentchooserterminal.moc"

#include <ktoolinvocation.h>
#include <klauncher_iface.h>
#include <QtDBus/QtDBus>
#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kopenwithdialog.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kmimetypetrader.h>
#include <kurlrequester.h>
#include <kconfiggroup.h>


CfgTerminalEmulator::CfgTerminalEmulator(QWidget *parent):TerminalEmulatorConfig_UI(parent),CfgPlugin(){
	connect(terminalLE,SIGNAL(textChanged(const QString &)), this, SLOT(configChanged()));
	connect(terminalCB,SIGNAL(toggled(bool)),this,SLOT(configChanged()));
	connect(otherCB,SIGNAL(toggled(bool)),this,SLOT(configChanged()));
	connect(btnSelectTerminal,SIGNAL(clicked()),this,SLOT(selectTerminalApp()));

}

CfgTerminalEmulator::~CfgTerminalEmulator() {
}

void CfgTerminalEmulator::configChanged()
{
	emit changed(true);
}

void CfgTerminalEmulator::defaults()
{
    load(0L);
}


void CfgTerminalEmulator::load(KConfig *) {
        KConfigGroup config(KSharedConfig::openConfig("kdeglobals"), "General");
	QString terminal = config.readPathEntry("TerminalApplication","konsole");
	if (terminal == "konsole")
	{
	   terminalLE->setText("xterm");
	   terminalCB->setChecked(true);
	}
	else
	{
	  terminalLE->setText(terminal);
	  otherCB->setChecked(true);
	}

	emit changed(false);
}

void CfgTerminalEmulator::save(KConfig *)
{
        KConfigGroup config(KSharedConfig::openConfig("kdeglobals"), "General");
	config.writePathEntry("TerminalApplication", terminalCB->isChecked()?"konsole":terminalLE->text(), KConfig::Normal|KConfig::Global);
	config.sync();

	KGlobalSettings::self()->emitChange(KGlobalSettings::SettingsChanged);
        KToolInvocation::klauncher()->reparseConfiguration();

	emit changed(false);
}

void CfgTerminalEmulator::selectTerminalApp()
{
	KUrl::List urlList;
	KOpenWithDialog dlg(urlList, i18n("Select preferred terminal application:"), QString(), this);
	// hide "Run in &terminal" here, we don't need it for a Terminal Application
	dlg.hideRunInTerminal();
	if (dlg.exec() != QDialog::Accepted) return;
	QString client = dlg.text();

	if (!client.isEmpty())
	{
		terminalLE->setText(client);
	}
}
// vim: sw=4 ts=4 noet
