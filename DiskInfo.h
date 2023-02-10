#pragma once

#include "AtaSmart.h"

class CDiskInfo
{
public:
    CDiskInfo();

    void InitAta(BOOL useWmi, BOOL advancedDiskSearch, PBOOL flagChangeDisk, BOOL workaroundHD204UI, BOOL workaroundAdataSsd);
    void SaveSmartInfo(DWORD i);
    void SaveText(const CString &fileName);

protected:
    CAtaSmart m_Ata;

private:
    BOOL m_bHideSerialNumber;
    BOOL m_bHideNoSmartDisk;
    BOOL m_bDumpIdentifyDevice;
    BOOL m_bDumpSmartReadData;
    BOOL m_bDumpSmartReadThreshold;
    BOOL m_bAsciiView;
    BOOL m_bSmartEnglish;
    CString m_Ini;
    CString m_SmartDir;
    BOOL m_NowDetectingUnitPowerOnHours;
    CString m_DefaultLangPath;

private:
    CString i18n(CString section, CString key);

    CHAR AsciiFilter(BYTE ch)
    {
        if ((0x00 <= ch && ch < 0x20) || ch >= 0x7f)
        {
            return '.';
        }
        return ch;
    }

    BOOL AppendLog(CString dir, CString disk, CString file, CTime time, int value, BOOL firstTime = FALSE, int threshold = 0);
    CString GetDiskStatus(DWORD statusCode);
    CString __Number(DWORD value);
};
