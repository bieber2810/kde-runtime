/*
 *   Copyright 2009 by Alan Alpert <alan.alpert@nokia.com>
 *   Copyright 2010 by Ménard Alexis <menard@kde.org>
 *   Copyright 2010 by Marco Martin <mart@kde.org>

 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GRAPHICSWIDGETSBINDINGSPLUGIN_H
#define GRAPHICSWIDGETSBINDINGSPLUGIN_H

#include <QDeclarativeExtensionPlugin>

#include <Plasma/SignalPlotter>

class SignalPlotter : public Plasma::SignalPlotter
{
    Q_OBJECT

public:
    SignalPlotter(QGraphicsItem *parent = 0);
    ~SignalPlotter();
    Q_INVOKABLE void addSample(const QVariantList &samples);
};

class GraphicsWidgetsBindingsPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char *uri);
};

Q_EXPORT_PLUGIN2(graphicswidgetsbindingsplugin, GraphicsWidgetsBindingsPlugin)

#endif
