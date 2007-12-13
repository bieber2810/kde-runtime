/*  This file is part of the KDE project.

    Copyright (C) 2007 Trolltech ASA. All rights reserved.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 or 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PHONON_QPIN_H
#define PHONON_QPIN_H

#include <QtCore/QString>
#include <QtCore/QMutex>
#include <QtCore/QVector>

#include <dshow.h>

namespace Phonon
{
    namespace DS9
    {
        class QBaseFilter;
        //this is the base class for our self-implemented Pins
        class QPin : public IPin
        {
        public:
            QPin(QBaseFilter *parent, PIN_DIRECTION dir, const QVector<AM_MEDIA_TYPE> &mt);
            virtual ~QPin();

            //reimplementation from IUnknown
            STDMETHODIMP QueryInterface(REFIID iid, void** ppvObject);
            STDMETHODIMP_(ULONG) AddRef();
            STDMETHODIMP_(ULONG) Release();

            //reimplementation from IPin
            STDMETHODIMP Connect(IPin *,const AM_MEDIA_TYPE *);
            STDMETHODIMP ReceiveConnection(IPin *,const AM_MEDIA_TYPE *);
            STDMETHODIMP Disconnect();
            STDMETHODIMP ConnectedTo(IPin **);
            STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE *);
            STDMETHODIMP QueryPinInfo(PIN_INFO *);
            STDMETHODIMP QueryDirection(PIN_DIRECTION *);
            STDMETHODIMP QueryId(LPWSTR*);
            STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE*);
            STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **);
            STDMETHODIMP QueryInternalConnections(IPin **, ULONG*);
            STDMETHODIMP EndOfStream();
            STDMETHODIMP BeginFlush();
            STDMETHODIMP EndFlush();
            STDMETHODIMP NewSegment(REFERENCE_TIME, REFERENCE_TIME, double);

            const QVector<AM_MEDIA_TYPE> &mediaTypes() const { return m_mediaTypes;}


        protected:
            bool isFlushing() const;

            FILTER_STATE filterState() const;

            //this can be used by sub-classes
            mutable QMutex m_mutex;
            IMemAllocator *m_memAlloc;

        private:
            HRESULT checkOutputMediaTypesConnection(IPin *pin);
            HRESULT checkOwnMediaTypesConnection(IPin *pin);
            void freeMediaType(AM_MEDIA_TYPE *type);

            QBaseFilter *m_parent;
            LONG m_refCount;
            IPin *m_connected;
            PIN_DIRECTION m_direction;
            QVector<AM_MEDIA_TYPE> m_mediaTypes;
            AM_MEDIA_TYPE m_connectedType;
            QString m_name;
            bool m_flushing;



        };
    }
}

#endif //PHONON_QPIN_H
