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

#ifndef PHONON_COMPOINTER_H
#define PHONON_COMPOINTER_H

#include <dshow.h>

namespace Phonon
{
    namespace DS9
    {
        template<class T> class ComPointer
        {
        public:
            explicit ComPointer(T *t = 0) : m_t(t)
            {
            }

            explicit ComPointer(IUnknown *_unk, const GUID &guid = __uuidof(T)) : m_t(0)
            {
                if (_unk)
                    _unk->QueryInterface(guid, pdata());
            }

            ComPointer(const ComPointer<T> &other) : m_t(other.m_t)
            {
                if (m_t) m_t->AddRef();
            }

            ComPointer<T> &operator=(const ComPointer<T> &other)
            {
                if (other.m_t) other.m_t->AddRef();
                if (m_t) m_t->Release();
                m_t = other.m_t;
                return *this;
            }

            T *operator->() const { return m_t; }

            operator T*() const { return m_t; }

            //the 2 following methods first reinitialize their value to avoid mem leaks
            void **pdata() { if (m_t) m_t->Release(); m_t = 0; return reinterpret_cast<void**>(&m_t);}
            T **pobject() { if (m_t) m_t->Release(); m_t = 0; return &m_t;}

            bool operator==(const ComPointer<T> &other) const
            {
                return m_t == other.m_t;
            }

            bool operator!=(const ComPointer<T> &other) const
            {
                return m_t != other.m_t;
            }

            ~ComPointer()
            {
                if (m_t) m_t->Release();
            }

        private:
            T *m_t;
        };
    }
}

#endif
