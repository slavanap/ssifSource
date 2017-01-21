/*
 * (C) 2009-2013 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "stdafx.h"
#include "MultiFiles.h"


IMPLEMENT_DYNAMIC(CMultiFiles, CObject)

CMultiFiles::CMultiFiles()
    : m_hFile(nullptr)
    , m_llTotalLength(0)
    , m_nCurPart(-1)
    , m_pCurrentPTSOffset(nullptr)
{
}

void CMultiFiles::Reset()
{
    m_strFiles.RemoveAll();
    m_FilesSize.RemoveAll();
    m_rtPtsOffsets.RemoveAll();
    m_llTotalLength = 0;
}

BOOL CMultiFiles::Open(LPCTSTR lpszFileName, UINT nOpenFlags)
{
    Reset();
    m_strFiles.Add(lpszFileName);

    return OpenPart(0);
}

BOOL CMultiFiles::OpenFiles(CAtlList<CHdmvClipInfo::PlaylistItem>& files, UINT nOpenFlags)
{
    POSITION pos = files.GetHeadPosition();
    LARGE_INTEGER llSize;
    int nPos = 0;
    REFERENCE_TIME rtDur = 0;

    Reset();
    while (pos) {
        CHdmvClipInfo::PlaylistItem& s = files.GetNext(pos);
        m_strFiles.Add(s.m_strFileName);
        if (!OpenPart(nPos)) {
            return false;
        }

        llSize.QuadPart = m_hFile->size();
        m_llTotalLength += llSize.QuadPart;
        m_FilesSize.Add(llSize.QuadPart);
        m_rtPtsOffsets.Add(rtDur);
        rtDur += s.Duration();
        nPos++;
    }

    if (files.GetCount() > 1) {
        ClosePart();
    }

    return TRUE;
}

LONGLONG SeekStream(UniversalFileStream& stream, LONGLONG distanceToMove, UINT moveMethod) {
    std::ios_base::seekdir way;
    switch (moveMethod) {
        case FILE_BEGIN: way = std::ios_base::beg; break;
        case FILE_CURRENT: way = std::ios_base::cur; break;
        case FILE_END: way = std::ios_base::end; break;
        default:
            return -1;
    }
    return stream.rdbuf()->pubseekoff(distanceToMove, way);
}

ULONGLONG CMultiFiles::Seek(LONGLONG lOff, UINT nFrom)
{
    if (m_strFiles.GetCount() == 1) {
        return SeekStream(*m_hFile, lOff, nFrom);
    } else {
        ULONGLONG lAbsolutePos = GetAbsolutePosition(lOff, nFrom);
        int nNewPart = 0;
        ULONGLONG llSum = 0;

        while (m_FilesSize[nNewPart] + llSum <= lAbsolutePos) {
            llSum += m_FilesSize[nNewPart];
            nNewPart++;
        }

        OpenPart(nNewPart);
        LONGLONG ret = SeekStream(*m_hFile, lAbsolutePos - llSum, FILE_BEGIN);
        if (ret < 0)
            return ret;
        return llSum + ret;
    }
}

ULONGLONG CMultiFiles::GetAbsolutePosition(LONGLONG lOff, UINT nFrom)
{
    switch (nFrom) {
        case begin:
            return lOff;
        case current: {
            LONGLONG pos = m_hFile->pos();
            if (pos < 0)
                return pos;
            return lOff + pos;
        }
        case end:
            return m_llTotalLength - lOff;
        default:
            return 0;   // just used to quash "not all control paths return a value" warning
    }
}

ULONGLONG CMultiFiles::GetLength() const
{
    if (m_strFiles.GetCount() == 1) {
        return m_hFile->size();
    } else {
        return m_llTotalLength;
    }
}

UINT CMultiFiles::Read(void* lpBuf, UINT nCount)
{
    DWORD dwRead;
    do {
        std::streamsize ret = m_hFile->rdbuf()->sgetn((char*)lpBuf, nCount);
        if (ret == -1)
            break;
        dwRead = (DWORD)ret;

        if (dwRead != nCount && (m_nCurPart < 0 || (size_t)m_nCurPart < m_strFiles.GetCount() - 1)) {
            OpenPart(m_nCurPart + 1);
            lpBuf = (void*)((BYTE*)lpBuf + dwRead);
            nCount -= dwRead;
        }
    } while (nCount != dwRead && (m_nCurPart < 0 || (size_t)m_nCurPart < m_strFiles.GetCount() - 1));
    return dwRead;
}

void CMultiFiles::Close()
{
    ClosePart();
    Reset();
}

CMultiFiles::~CMultiFiles()
{
    Close();
}

BOOL CMultiFiles::OpenPart(int nPart)
{
    if (m_nCurPart == nPart) {
        return TRUE;
    } else {
        CString fn;

        ClosePart();

        fn = m_strFiles.GetAt(nPart);
        CT2A fn_ascii(fn, CP_ACP);
        try {
            m_hFile.reset(new UniversalFileStream(fn_ascii.m_psz));
            m_nCurPart = nPart;
            if (m_pCurrentPTSOffset != nullptr) {
                *m_pCurrentPTSOffset = m_rtPtsOffsets[nPart];
            }
            return TRUE;
        }
        catch (...) {
            return FALSE;
        }
    }
}

void CMultiFiles::ClosePart()
{
    if (m_hFile != nullptr) {
        m_hFile.reset();
        m_nCurPart = -1;
    }
}
