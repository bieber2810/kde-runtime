/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __kgzipdev_h
#define __kgzipdev_h

#include <qiodevice.h>

/**
 * A class for reading and writing gzipped data onto a device
 * (e.g. file, but other usages are possible, like a buffer or a socket)
 *
 * @author David Faure <faure@kde.org>
 */
class KGzipDev : public QIODevice
{
public:
    /**
     * Create a KGzipDev.
     * @param dev the device where the compressed data should
     * be read and written.
     */
    KGzipDev( QIODevice * dev );
    virtual ~KGzipDev();

    virtual bool open( int mode );
    virtual void close();
    virtual void flush();

    // Not implemented
    virtual uint size() const;

    virtual int  at() const;
    /**
     * That one can be quite slow, when going back. Use with care.
     */
    virtual bool at( int );

    virtual bool atEnd() const;

    virtual int readBlock( char *data, uint maxlen );
    virtual int writeBlock( const char *data, uint len );
    //int readLine( char *data, uint maxlen );

    virtual int getch();
    virtual int putch( int );
    virtual int ungetch( int );
private:
    class KGzipDevPrivate;
    KGzipDevPrivate * d;
};

#endif
