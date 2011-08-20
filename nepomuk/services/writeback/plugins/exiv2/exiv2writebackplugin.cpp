/*
Copyright (C) 2011  Smit Shah <Who828@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libkexiv2/kexiv2.h>

#include <QFile>
#include <QUrl>

#include <Nepomuk/Resource>
#include <Nepomuk/Vocabulary/NEXIF>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Variant>

#include "exiv2writebackplugin.h"

#define datetimeformat QLatin1String("yyyy:MM:dd hh:mm:ss")

using namespace Nepomuk::Vocabulary;

Nepomuk::Exiv2WritebackPlugin::Exiv2WritebackPlugin(QObject* parent,const QList<QVariant>&): WritebackPlugin(parent)
{

}

Nepomuk::Exiv2WritebackPlugin::~Exiv2WritebackPlugin()
{

}

void Nepomuk::Exiv2WritebackPlugin::doWriteback(const QUrl& url)
{
    Nepomuk::Resource resource(url);
    if (resource.isValid())
    {
        KExiv2Iface::KExiv2 image;
        image.load((resource.property(NIE::url())).toString());

        if ((resource.property(NEXIF::dateTime())).isValid()) {
            const QDateTime datetime = (resource.property(NEXIF::dateTime())).toDateTime();
            if (datetime != exifDateToDateTime(image.getExifTagString("Exif.Image.DateTime")))
                image.setExifTagString("Exif.Image.DateTime",exifDateFromDateTime(datetime));
        }
        if ((resource.property(NEXIF::dateTimeDigitized())).isValid()) {
            const QDateTime datetime = (resource.property(NEXIF::dateTimeDigitized())).toDateTime();
            if (datetime != exifDateToDateTime(image.getExifTagString("Exif.Photo.DateTimeDigitized")))
                image.setExifTagString("Exif.Photo.DateTimeDigitized",exifDateFromDateTime(datetime));
        }
        if ((resource.property(NEXIF::dateTimeOriginal())).isValid()) {
            const QDateTime datetime = (resource.property(NEXIF::dateTimeOriginal())).toDateTime();
            if (datetime !=exifDateToDateTime(image.getExifTagString("Exif.Image.DateTimeOriginal")))
                image.setExifTagString("Exif.Image.DateTimeOriginal",exifDateFromDateTime(datetime));
        }
        if ((resource.property(NEXIF::imageDescription())).isValid()) {
            const QString imagedes = (resource.property(NEXIF::imageDescription())).toString();
            if (imagedes != image.getExifTagString("Exif.Image.ImageDescription"))
                image.setExifTagString("Exif.Image.ImageDescription",imagedes);
        }
        image.applyChanges();
        emitFinished(true);
    }
}

QDateTime Nepomuk::Exiv2WritebackPlugin::exifDateToDateTime(const QString& string)
{
    return QDateTime::fromString(string,datetimeformat);
}

QString Nepomuk::Exiv2WritebackPlugin::exifDateFromDateTime(const QDateTime& datetime)
{
    return datetime.toString(datetimeformat);
}

NEPOMUK_EXPORT_WRITEBACK_PLUGIN(Nepomuk::Exiv2WritebackPlugin,"nepomuk_writeback_exiv2")

#include "exiv2writebackplugin.moc"
