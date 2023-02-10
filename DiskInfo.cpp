#include "stdafx.h"

#include <sysinfoapi.h>
#include "DiskInfo.h"
#include "AtaSmart.h"
#include "Priscilla/OsInfoFx.h"
#include "Priscilla/UtilityFx.h"
#include <io.h>
#include <fcntl.h>
#include <fileapi.h>

#define SMART_INI					_T("Smart.ini")

CDiskInfo::CDiskInfo()
    : m_bHideSerialNumber(HIDE_SERIAL_NUMBER)
    , m_bHideNoSmartDisk(HIDE_NO_SMART_DISK)
    , m_bDumpIdentifyDevice(DUMP_IDENTIFY_DEVICE)
    , m_bDumpSmartReadData(DUMP_SMART_READ_DATA)
    , m_bDumpSmartReadThreshold(DUMP_SMART_READ_THRESHOLD)
    , m_bAsciiView(ASCII_VIEW)
    , m_NowDetectingUnitPowerOnHours(FALSE)
{
#pragma region Folders and files for reading
    TCHAR *ptrEnd;
    TCHAR ini[MAX_PATH];
    ::GetModuleFileName(NULL, ini, MAX_PATH);
    if ((ptrEnd = _tcsrchr(ini, '\\')) != NULL)
    {
        *ptrEnd = '\0';
        m_Ini = ini;
        m_DefaultLangPath = ini;
        // _tcscat_s(ini, MAX_PATH, _T("\\DiskInfo.ini"));
    }
    m_Ini += _T("\\DiskInfo.ini");
    m_DefaultLangPath += _T("\\language\\English.lang");
#pragma endregion

#pragma region Folder for writing
    TCHAR tmpdir[MAX_PATH];
    ::GetTempPath(MAX_PATH, tmpdir);
    m_SmartDir = tmpdir;
    m_SmartDir += _T("CC8030C4870F\\");
#pragma endregion

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return;

    BOOL bFlag = FALSE;
    m_Ata.SetAtaPassThroughSmart(TRUE);
    InitAta(1, 0, NULL, 0, 1);

    CoUninitialize();
}

void CDiskInfo::InitAta(BOOL useWmi, BOOL advancedDiskSearch, PBOOL flagChangeDisk, BOOL workaroundHD204UI, BOOL workaroundAdataSsd)
{
    m_Ata.Init(useWmi, advancedDiskSearch, flagChangeDisk, workaroundHD204UI, workaroundAdataSsd, m_bHideNoSmartDisk);

    DWORD errorCount = 0;
    for (int i = 0; i < m_Ata.vars.GetCount(); i++)
    {
        int unitType = GetPrivateProfileIntFx(_T("PowerOnUnit"), m_Ata.vars[i].ModelSerial, -1, m_Ini);
        if (unitType >= 0)
        {
            if (m_Ata.vars[i].DetectedTimeUnitType == m_Ata.POWER_ON_MILLI_SECONDS)
            {
                unitType = m_Ata.POWER_ON_MILLI_SECONDS;
            }
            else if (m_Ata.vars[i].DetectedTimeUnitType == m_Ata.POWER_ON_10_MINUTES)
            {
                unitType = m_Ata.POWER_ON_10_MINUTES;
            }
            m_Ata.vars[i].MeasuredTimeUnitType = unitType;
            m_Ata.vars[i].MeasuredPowerOnHours = m_Ata.GetPowerOnHoursEx(i, unitType);
        }
        else if (m_Ata.vars[i].PowerOnRawValue > 0)
        {
            errorCount++;
        }

        //m_bAutoRefreshTarget[i] = GetPrivateProfileIntFx(_T("AutoRefreshTarget"), m_Ata.vars[i].ModelSerial, 1, m_Ini);
        if (m_Ata.vars[i].IsSsd)
        {
            m_Ata.vars[i].AlarmTemperature = GetPrivateProfileIntFx(_T("AlarmTemperature"), m_Ata.vars[i].ModelSerial, 60, m_Ini);
        }
        else
        {
            m_Ata.vars[i].AlarmTemperature = GetPrivateProfileIntFx(_T("AlarmTemperature"), m_Ata.vars[i].ModelSerial, 50, m_Ini);
        }
        m_Ata.vars[i].AlarmHealthStatus = GetPrivateProfileIntFx(_T("AlarmHealthStatus"), m_Ata.vars[i].ModelSerial, 1, m_Ini);

        m_Ata.vars[i].Threshold05 = (WORD)GetPrivateProfileIntFx(_T("ThreasholdOfCaution05"), m_Ata.vars[i].ModelSerial, 1, m_Ini);
        m_Ata.vars[i].ThresholdC5 = (WORD)GetPrivateProfileIntFx(_T("ThreasholdOfCautionC5"), m_Ata.vars[i].ModelSerial, 1, m_Ini);
        m_Ata.vars[i].ThresholdC6 = (WORD)GetPrivateProfileIntFx(_T("ThreasholdOfCautionC6"), m_Ata.vars[i].ModelSerial, 1, m_Ini);
        m_Ata.vars[i].ThresholdFF = (WORD)GetPrivateProfileIntFx(_T("ThreasholdOfCautionFF"), m_Ata.vars[i].ModelSerial, 10, m_Ini);

        m_Ata.vars[i].DiskStatus = m_Ata.CheckDiskStatus(i);
        //DebugPrint(_T("SaveSmartInfo(i)"));
        SaveSmartInfo(i);
    }

    if (errorCount)
    {
        m_NowDetectingUnitPowerOnHours = TRUE;
    }

#if 0
    DebugPrint(_T("AutoAamApmAdaption()"));
    AutoAamApmAdaption();

    DebugPrint(_T("UpdateShareInfo()"));
    UpdateShareInfo();
#endif
}

void CDiskInfo::SaveSmartInfo(DWORD i)
{
    if (!m_Ata.vars[i].IsSmartCorrect)
    {
        return;
    }
    static CTime preTime[CAtaSmart::MAX_DISK] = { 0 };
    CTime time = CTime::GetTickCount();

#pragma region ∑¿÷π∏ﬂ∆µ≤È—Ø
#if 0
    if (time - preTime[i] < SAVE_SMART_PERIOD)
    {
        return;
    }
    else
    {
        preTime[i] = time;
    }
#endif
#pragma endregion

    CString line;
    CString cstr;
    CStdioFile outFile;
    CString dir;
    CString disk;
    CString path;
    BOOL flagFirst = FALSE;
    TCHAR str[256];

    dir = m_SmartDir;
    CreateDirectory(dir, NULL);

    disk = m_Ata.vars[i].ModelSerial;
    dir += disk;
    CreateDirectory(dir, NULL);

    // AlarmHealthStatus(i, dir, disk);

    // Computer Name
    // 
    DWORD length = 256;
    GetComputerNameW(str, &length);
    WritePrivateProfileStringFx(L"PC", L"ComputerName", str, dir + _T("\\") + SMART_INI);

    GetPrivateProfileStringFx(disk, _T("Date"), _T(""), str, 256, dir + _T("\\") + SMART_INI);
    cstr = str;
    if (cstr.IsEmpty())
    {
        flagFirst = TRUE;
        _stprintf_s(str, 256, _T("%s"), time.Format(_T("%Y/%m/%d %H:%M:%S")).GetBuffer());
        WritePrivateProfileStringFx(disk + _T("FIRST"), _T("Date"), str, dir + _T("\\") + SMART_INI);

        _stprintf_s(str, 256, _T("%d"), m_Ata.vars[i].DiskStatus);
        WritePrivateProfileStringFx(disk + _T("FIRST"), _T("HealthStatus"), str, dir + _T("\\") + SMART_INI);
    }

    // Check Threshold of Reallocated Sectors Count
    GetPrivateProfileStringFx(disk + _T("THRESHOLD"), _T("05"), _T(""), str, 256, dir + _T("\\") + SMART_INI);
    cstr = str;
    if (cstr.IsEmpty())
    {
        flagFirst = TRUE;
    }

    _stprintf_s(str, 256, _T("%s"), time.Format(_T("%Y/%m/%d %H:%M:%S")).GetBuffer());
    WritePrivateProfileStringFx(disk, _T("Date"), str, dir + _T("\\") + SMART_INI);

    _stprintf_s(str, 256, _T("%d"), m_Ata.vars[i].DiskStatus);
    WritePrivateProfileStringFx(disk, _T("HealthStatus"), str, dir + _T("\\") + SMART_INI);

    if (m_Ata.vars[i].Temperature > -300)
    {
        AppendLog(dir, disk, _T("Temperature"), time, m_Ata.vars[i].Temperature, flagFirst);
    }

    if (m_Ata.vars[i].MeasuredPowerOnHours > 0 && m_NowDetectingUnitPowerOnHours == FALSE)
    {
        AppendLog(dir, disk, _T("PowerOnHours"), time, m_Ata.vars[i].MeasuredPowerOnHours, flagFirst);
    }

    if (m_Ata.vars[i].PowerOnCount >= 0)
    {
        AppendLog(dir, disk, _T("PowerOnCount"), time, m_Ata.vars[i].PowerOnCount, flagFirst);
    }

    if (m_Ata.vars[i].Life >= 0)
    {
        AppendLog(dir, disk, _T("Life"), time, m_Ata.vars[i].Life, flagFirst);
    }

    if (m_Ata.vars[i].HostWrites >= 0)
    {
        AppendLog(dir, disk, _T("HostWrites"), time, m_Ata.vars[i].HostWrites, flagFirst);
    }

    if (m_Ata.vars[i].HostReads >= 0)
    {
        AppendLog(dir, disk, _T("HostReads"), time, m_Ata.vars[i].HostReads, flagFirst);
    }

    if (m_Ata.vars[i].NandWrites >= 0)
    {
        AppendLog(dir, disk, _T("NandWrites"), time, m_Ata.vars[i].NandWrites, flagFirst);
    }

    if (m_Ata.vars[i].GBytesErased >= 0)
    {
        AppendLog(dir, disk, _T("GBytesErased"), time, m_Ata.vars[i].GBytesErased, flagFirst);
    }

    if (m_Ata.vars[i].WearLevelingCount >= 0)
    {
        AppendLog(dir, disk, _T("WearLevelingCount"), time, m_Ata.vars[i].WearLevelingCount, flagFirst);
    }

    for (DWORD j = 0; j < m_Ata.vars[i].AttributeCount; j++)
    {
        cstr.Format(_T("%02X"), m_Ata.vars[i].Attribute[j].Id);
        AppendLog(dir, disk, cstr, time, m_Ata.vars[i].Attribute[j].CurrentValue,
            flagFirst, m_Ata.vars[i].Threshold[j].ThresholdValue);

        switch (m_Ata.vars[i].Attribute[j].Id)
        {
        case 0x05: // Reallocated Sectors Count
            AppendLog(dir, disk, _T("ReallocatedSectorsCount"), time,
                MAKEWORD(m_Ata.vars[i].Attribute[j].RawValue[0], m_Ata.vars[i].Attribute[j].RawValue[1]), flagFirst);
            break;
        case 0xC4: // Reallocation Event Count
            AppendLog(dir, disk, _T("ReallocationEventCount"), time,
                MAKEWORD(m_Ata.vars[i].Attribute[j].RawValue[0], m_Ata.vars[i].Attribute[j].RawValue[1]), flagFirst);
            break;
        case 0xC5: // Current Pending Sector Count
            AppendLog(dir, disk, _T("CurrentPendingSectorCount"), time,
                MAKEWORD(m_Ata.vars[i].Attribute[j].RawValue[0], m_Ata.vars[i].Attribute[j].RawValue[1]), flagFirst);
            break;
        case 0xC6: // Off-Line Scan Uncorrectable Sector Count
            AppendLog(dir, disk, _T("UncorrectableSectorCount"), time,
                MAKEWORD(m_Ata.vars[i].Attribute[j].RawValue[0], m_Ata.vars[i].Attribute[j].RawValue[1]), flagFirst);
            break;
        default:
            break;
        }
    }
}

BOOL CDiskInfo::AppendLog(CString dir, CString disk, CString file, CTime time, int value, BOOL flagFirst, int threshold)
{
    TCHAR str[256];

    // First Time
    if (flagFirst)
    {
        wsprintf(str, _T("%d"), value);
        WritePrivateProfileStringFx(disk + _T("FIRST"), file, str, dir + _T("\\") + SMART_INI);

        if (file.GetLength() == 2)
        {
            wsprintf(str, _T("%d"), threshold);
            WritePrivateProfileStringFx(disk + _T("THRESHOLD"), file, str, dir + _T("\\") + SMART_INI);
        }
    }

    GetPrivateProfileStringFx(disk, file, _T("-1"), str, 256, dir + _T("\\") + SMART_INI);
    int pre = _tstoi(str);

    if (pre != value)
    {
        // Update
        wsprintf(str, _T("%d"), value);
        WritePrivateProfileStringFx(disk, file, str, dir + _T("\\") + SMART_INI);

        CString line;
        line.Format(_T("%s,%d\n"), time.Format(_T("%Y/%m/%d %H:%M:%S")).GetString(), value);

        CStdioFile outFile;
        if (outFile.Open(dir + _T("\\") + file + _T(".csv"),
            CFile::modeCreate | CFile::modeNoTruncate | CFile::modeReadWrite | CFile::typeText))
        {
            ULONGLONG fileLength = outFile.GetLength();
            try
            {
                if (outFile.SeekToEnd() == fileLength)
                {
                    outFile.WriteString(line);
                }
            }
            catch (CFileException * e)
            {
                DebugPrint(L"CFileException");
                e->Delete();
            }
            outFile.Close();
            return TRUE;
        }
    }
    return FALSE;
}

CString CDiskInfo::__Number(DWORD value)
{
    CString cstr;

    if (value >= 1000)
    {
        cstr = _T("");
    }
    else if (value >= 100)
    {
        cstr.Format(_T("%3d"), value);
    }
    else if (value >= 10)
    {
        cstr.Format(_T("_%2d"), value);
    }
    else
    {
        cstr.Format(_T("__%1d"), value);
    }

    return cstr;
}

CString CDiskInfo::GetDiskStatus(DWORD statusCode)
{
    switch (statusCode)
    {
    case CAtaSmart::DISK_STATUS_GOOD:
        return i18n(_T("DiskStatus"), _T("GOOD"));
        break;
    case CAtaSmart::DISK_STATUS_CAUTION:
        return i18n(_T("DiskStatus"), _T("CAUTION"));
        break;
    case CAtaSmart::DISK_STATUS_BAD:
        return i18n(_T("DiskStatus"), _T("BAD"));
        break;
    case CAtaSmart::DISK_STATUS_UNKNOWN:
    default:
        return i18n(_T("DiskStatus"), _T("UNKNOWN"));
        break;
    }
}

void CDiskInfo::SaveText(const CString &fileName)
{
    CString cstr, clip, driveTemplate, drive, feature, temp, line, csd;

    clip = _T("\
----------------------------------------------------------------------------\r\n\
%PRODUCT% %VERSION% (C) %COPY_YEAR% www.promise.com\r\n\
----------------------------------------------------------------------------\r\n\
\r\n\
    OS : %OS%\r\n\
  Date : %DATE%\r\n\
\r\n\
-- Controller Map ----------------------------------------------------------\r\n\
%CONTROLLER_MAP%\
\r\n\
-- Disk List ---------------------------------------------------------------\r\n\
%DISK_LIST%\
\r\n");
    clip.Replace(_T("%PRODUCT%"), PRODUCT_NAME);
    clip.Replace(_T("%VERSION%"), PRODUCT_VERSION);
    clip.Replace(_T("%COPY_YEAR%"), PRODUCT_COPY_YEAR);

    SYSTEMTIME st;
    GetLocalTime(&st);
    cstr.Format(_T("%04d/%02d/%02d %d:%02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    clip.Replace(_T("%DATE%"), cstr);

    clip.Replace(_T("%CONTROLLER_MAP%"), m_Ata.m_ControllerMap);

    GetOsName(cstr);
    clip.Replace(_T("%OS%"), cstr);

    cstr = _T("");
    temp = _T("");
    for (int i = 0; i < m_Ata.vars.GetCount(); i++)
    {
        if (m_Ata.vars[i].TotalDiskSize >= 1000)
        {
            temp.Format(_T(" (%02d) %s : %.1f GB"), i + 1,
                m_Ata.vars[i].Model.GetString(), m_Ata.vars[i].TotalDiskSize / 1000.0);
        }
        else
        {
            temp.Format(_T(" (%02d) %s : %d MB"), i + 1,
                m_Ata.vars[i].Model.GetString(), m_Ata.vars[i].TotalDiskSize);
        }

        cstr += temp;
        if (m_Ata.vars[i].CommandType == m_Ata.CMD_TYPE_CSMI)
        {
            if (m_Ata.vars[i].PhysicalDriveId < 0)
            {
                temp.Format(_T(" [X/%d/%d, %s]"),
                    m_Ata.vars[i].ScsiPort, m_Ata.vars[i].sasPhyEntity.bPortIdentifier, m_Ata.vars[i].CommandTypeString.GetString());
            }
            else
            {
                temp.Format(_T(" [%d/%d/%d, %s]"),
                    m_Ata.vars[i].PhysicalDriveId, m_Ata.vars[i].ScsiPort, m_Ata.vars[i].sasPhyEntity.bPortIdentifier, m_Ata.vars[i].CommandTypeString.GetString());
            }
        }
        else if (m_Ata.vars[i].PhysicalDriveId < 0)
        {
            temp.Format(_T(" [X/%d/%d, %s]"),
                m_Ata.vars[i].ScsiPort, m_Ata.vars[i].ScsiTargetId, m_Ata.vars[i].CommandTypeString.GetString());
        }
        else if (m_Ata.vars[i].ScsiPort < 0)
        {
            if (m_Ata.vars[i].UsbVendorId > 0 && m_Ata.vars[i].UsbProductId > 0)
            {
                temp.Format(_T(" [%d/X/X, %s] (V=%04X, P=%04X)"),
                    m_Ata.vars[i].PhysicalDriveId, m_Ata.vars[i].CommandTypeString.GetString(), m_Ata.vars[i].UsbVendorId, m_Ata.vars[i].UsbProductId);
            }
            else
            {
                temp.Format(_T(" [%d/X/X, %s]"),
                    m_Ata.vars[i].PhysicalDriveId, m_Ata.vars[i].CommandTypeString.GetString());
            }
        }
        else
        {
            temp.Format(_T(" [%d/%d/%d, %s]"),
                m_Ata.vars[i].PhysicalDriveId, m_Ata.vars[i].ScsiPort, m_Ata.vars[i].ScsiTargetId, m_Ata.vars[i].CommandTypeString.GetString());
        }

        cstr += temp;

        if (!m_Ata.vars[i].SsdVendorString.IsEmpty())
        {
            cstr += _T(" - ") + m_Ata.vars[i].SsdVendorString;
        }

        cstr += _T("\r\n");
    }
    clip.Replace(_T("%DISK_LIST%"), cstr);

    for (int i = 0; i < m_Ata.vars.GetCount(); i++)
    {
        CAtaSmart::ATA_SMART_INFO *pCurSI = &(m_Ata.vars[i]);

        if (m_Ata.vars[i].DiskVendorId == m_Ata.SSD_VENDOR_NVME)
        {
            driveTemplate = _T("\
----------------------------------------------------------------------------\r\n\
 (%I%) %MODEL%\r\n\
----------------------------------------------------------------------------\r\n\
%ENCLOSURE%\
           Model : %MODEL%\r\n\
        Firmware : %FIRMWARE%\r\n\
   Serial Number : %SERIAL_NUMBER%\r\n\
       Disk Size : %TOTAL_DISK_SIZE%\r\n\
       Interface : %INTERFACE%\r\n\
        Standard : %MAJOR_VERSION%\r\n\
   Transfer Mode : %TRANSFER_MODE%\r\n\
  Power On Hours : %POWER_ON_HOURS%\r\n\
  Power On Count : %POWER_ON_COUNT%\r\n\
%HOST_READS%\
%HOST_WRITES%\
     Temperature : %TEMPERATURE%\r\n\
   Health Status : %DISK_STATUS%\r\n\
        Features : %SUPPORTED_FEATURE%\r\n\
    Drive Letter : %DRIVE_LETTER%\r\n\
");
        }
        else
        {
            driveTemplate = _T("\
----------------------------------------------------------------------------\r\n\
 (%I%) %MODEL%\r\n\
----------------------------------------------------------------------------\r\n\
%ENCLOSURE%\
           Model : %MODEL%\r\n\
        Firmware : %FIRMWARE%\r\n\
   Serial Number : %SERIAL_NUMBER%\r\n\
       Disk Size : %TOTAL_DISK_SIZE%\r\n\
     Buffer Size : %BUFFER_SIZE%\r\n\
%NV_CACHE_SIZE%\
%QUEUE_DEPTH%\
    # of Sectors : %NUMBER_OF_SECTORS%\r\n\
   Rotation Rate : %ROTATION_RATE%\r\n\
       Interface : %INTERFACE%\r\n\
   Major Version : %MAJOR_VERSION%\r\n\
   Minor Version : %MINOR_VERSION%\r\n\
   Transfer Mode : %TRANSFER_MODE%\r\n\
  Power On Hours : %POWER_ON_HOURS%\r\n\
  Power On Count : %POWER_ON_COUNT%\r\n\
%HOST_READS%\
%HOST_WRITES%\
%NAND_WRITES%\
%GBYTES_ERASED%\
%WEAR_LEVELING_COUNT%\
     Temperature : %TEMPERATURE%\r\n\
   Health Status : %DISK_STATUS%\r\n\
        Features : %SUPPORTED_FEATURE%\r\n\
       APM Level : %APM_LEVEL%\r\n\
       AAM Level : %AAM_LEVEL%\r\n\
    Drive Letter : %DRIVE_LETTER%\r\n\
");
        }

        driveTemplate += _T("\r\n");

        drive = driveTemplate;
        cstr.Format(_T("%02d"), i + 1);
        drive.Replace(_T("%I%"), cstr);
        if (m_Ata.vars[i].Enclosure.IsEmpty())
        {
            drive.Replace(_T("%ENCLOSURE%"), _T(""));
        }
        else
        {
            cstr.Format(_T("       Enclosure : %s (V=%04X, P=%04X, %s)"),
                m_Ata.vars[i].Enclosure.GetString(), m_Ata.vars[i].UsbVendorId, m_Ata.vars[i].UsbProductId, m_Ata.vars[i].CommandTypeString.GetString());
            if (!m_Ata.vars[i].SsdVendorString.IsEmpty())
            {
                cstr += _T(" - ") + m_Ata.vars[i].SsdVendorString;
            }
            cstr += _T("\r\n");
            drive.Replace(_T("%ENCLOSURE%"), cstr);
        }
        drive.Replace(_T("%MODEL%"), m_Ata.vars[i].Model);
        //		drive.Replace(_T("%MODEL_WMI%"), m_Ata.vars[i].ModelWmi);
        drive.Replace(_T("%FIRMWARE%"), m_Ata.vars[i].FirmwareRev);
        if (m_bHideSerialNumber)
        {
            CString temp = _T("");
            for (int j = 0; j < m_Ata.vars[i].SerialNumber.GetLength(); j++)
            {
                temp += _T("*");
            }
            drive.Replace(_T("%SERIAL_NUMBER%"), temp);
        }
        else
        {
            drive.Replace(_T("%SERIAL_NUMBER%"), m_Ata.vars[i].SerialNumber);
        }
        drive.Replace(_T("%INTERFACE%"), m_Ata.vars[i].Interface);
        if (m_Ata.vars[i].MajorVersion.IsEmpty())
        {
            drive.Replace(_T("%MAJOR_VERSION%"), i18n(_T("Dialog"), _T("UNKNOWN")));
        }
        else
        {
            drive.Replace(_T("%MAJOR_VERSION%"), m_Ata.vars[i].MajorVersion);
        }
        if (m_Ata.vars[i].MajorVersion.IsEmpty())
        {
            drive.Replace(_T("%MINOR_VERSION%"), i18n(_T("Dialog"), _T("UNKNOWN")));
        }
        else
        {
            drive.Replace(_T("%MINOR_VERSION%"), m_Ata.vars[i].MinorVersion);
        }

        temp.Format(_T("%s | %s"), m_Ata.vars[i].CurrentTransferMode.GetString(), m_Ata.vars[i].MaxTransferMode.GetString());
        drive.Replace(_T("%TRANSFER_MODE%"), temp);

        if (m_Ata.vars[i].CommandType == m_Ata.CMD_TYPE_AMD_RC2)
        {
            temp = i18n(_T("Dialog"), _T("UNKNOWN"));
        }
        else
        {
            temp.Format(_T("%I64d"), m_Ata.vars[i].NumberOfSectors);
        }
        drive.Replace(_T("%NUMBER_OF_SECTORS%"), temp);
        // Temporary ///////////////////////


        CString diskStatus;
        diskStatus = GetDiskStatus(m_Ata.vars[i].DiskStatus);
        if (m_Ata.vars[i].Life >= 0)
        {
            cstr.Format(_T(" (%d %%)"), m_Ata.vars[i].Life);
            diskStatus += cstr;
        }
        drive.Replace(_T("%DISK_STATUS%"), diskStatus);

        CString IsMinutes;

        if (m_Ata.vars[i].MeasuredPowerOnHours > 0)
        {
            if (m_Ata.vars[i].MeasuredTimeUnitType == CAtaSmart::POWER_ON_MINUTES)
            {
                if (m_Ata.vars[i].IsMaxtorMinute)
                {
                    IsMinutes = _T(" (?)");
                }
            }
            else
            {
                IsMinutes = _T("");
            }
            cstr.Format(_T("%d %s%s"), m_Ata.vars[i].MeasuredPowerOnHours, i18n(_T("Dialog"), _T("POWER_ON_HOURS_UNIT")).GetString(), IsMinutes.GetString());
        }
        else if (m_Ata.vars[i].DetectedPowerOnHours >= 0)
        {
            if (m_Ata.vars[i].DetectedTimeUnitType == CAtaSmart::POWER_ON_MINUTES)
            {
                if (m_Ata.vars[i].IsMaxtorMinute)
                {
                    IsMinutes = _T(" (?)");
                }
            }
            else
            {
                IsMinutes = _T("");
            }
            cstr.Format(_T("%d %s%s"), m_Ata.vars[i].DetectedPowerOnHours, i18n(_T("Dialog"), _T("POWER_ON_HOURS_UNIT")).GetString(), IsMinutes.GetString());
        }
        else
        {
            cstr = i18n(_T("Dialog"), _T("UNKNOWN"));
        }
        drive.Replace(_T("%POWER_ON_HOURS%"), cstr);

        if (m_Ata.vars[i].PowerOnCount > 0)
        {
            cstr.Format(_T("%d %s"), m_Ata.vars[i].PowerOnCount, i18n(_T("Dialog"), _T("POWER_ON_COUNT_UNIT")).GetString());
        }
        else
        {
            cstr = i18n(_T("Dialog"), _T("UNKNOWN"));
        }
        drive.Replace(_T("%POWER_ON_COUNT%"), cstr);

        if (m_Ata.vars[i].NvCacheSize > 0)
        {
            cstr.Format(_T("%d MB"), (int)(m_Ata.vars[i].NvCacheSize / 1024 / 1024));
        }
        else
        {
            cstr = _T("----");
        }

        if (m_Ata.vars[i].NvCacheSize == 0)
        {
            drive.Replace(_T("%NV_CACHE_SIZE%"), _T(""));
        }
        else
        {
            cstr.Format(_T("   NV Cache Size : %d MB\r\n"), (int)(m_Ata.vars[i].NvCacheSize / 1024 / 1024));
            drive.Replace(_T("%NV_CACHE_SIZE%"), cstr);
        }

        if (m_Ata.vars[i].HostWrites == -1)
        {
            drive.Replace(_T("%HOST_WRITES%"), _T(""));
        }
        else
        {
            cstr.Format(_T("     Host Writes : %d GB\r\n"), m_Ata.vars[i].HostWrites);
            drive.Replace(_T("%HOST_WRITES%"), cstr);
        }

        if (m_Ata.vars[i].HostReads == -1)
        {
            drive.Replace(_T("%HOST_READS%"), _T(""));
        }
        else
        {
            cstr.Format(_T("      Host Reads : %d GB\r\n"), m_Ata.vars[i].HostReads);
            drive.Replace(_T("%HOST_READS%"), cstr);
        }

        if (m_Ata.vars[i].NandWrites == -1)
        {
            drive.Replace(_T("%NAND_WRITES%"), _T(""));
        }
        else
        {
            /*
            if (m_Ata.vars[i].PlextorNandWritesUnit)
            {
                cstr.Format(_T("     NAND Writes : %d GB (?)\r\n"), m_Ata.vars[i].NandWrites);
            }
            else
            {
                cstr.Format(_T("     NAND Writes : %d GB\r\n"), m_Ata.vars[i].NandWrites);
            }
            */
            cstr.Format(_T("     NAND Writes : %d GB\r\n"), m_Ata.vars[i].NandWrites);
            drive.Replace(_T("%NAND_WRITES%"), cstr);
        }

        if (m_Ata.vars[i].GBytesErased == -1)
        {
            drive.Replace(_T("%GBYTES_ERASED%"), _T(""));
        }
        else
        {
            cstr.Format(_T("   GBytes Erased : %d GB\r\n"), m_Ata.vars[i].GBytesErased);
            drive.Replace(_T("%GBYTES_ERASED%"), cstr);
        }

        if (m_Ata.vars[i].WearLevelingCount == -1)
        {
            drive.Replace(_T("%WEAR_LEVELING_COUNT%"), _T(""));
        }
        else
        {
            cstr.Format(_T("Wear Level Count : %d\r\n"), m_Ata.vars[i].WearLevelingCount);
            drive.Replace(_T("%WEAR_LEVELING_COUNT%"), cstr);
        }

        if (m_Ata.vars[i].Temperature > -300)
        {
            cstr.Format(_T("%d C (%d F)"), m_Ata.vars[i].Temperature, m_Ata.vars[i].Temperature * 9 / 5 + 32);
        }
        else
        {
            cstr = i18n(_T("Dialog"), _T("UNKNOWN"));
        }
        drive.Replace(_T("%TEMPERATURE%"), cstr);

        if (m_Ata.vars[i].DiskVendorId == m_Ata.SSD_VENDOR_NVME || m_Ata.vars[i].CommandType == m_Ata.CMD_TYPE_AMD_RC2)
        {
            cstr.Format(_T("%.1f GB"), m_Ata.vars[i].TotalDiskSize / 1000.0);
        }
        else if (m_Ata.vars[i].DiskSizeWmi > 0)
        {
            if (m_Ata.vars[i].TotalDiskSize < 1000)
            {
                cstr.Format(_T("%d MB (%.2f/----/----/%.2f)"),
                    m_Ata.vars[i].TotalDiskSize,
                    m_Ata.vars[i].DiskSizeChs / 1000.0,
                    m_Ata.vars[i].DiskSizeWmi / 1000.0);
            }
            else if (m_Ata.vars[i].IsLba48Supported)
            {
                cstr.Format(_T("%.1f GB (%.1f/%.1f/%.1f/%.1f)"),
                    m_Ata.vars[i].TotalDiskSize / 1000.0,
                    m_Ata.vars[i].DiskSizeChs / 1000.0,
                    m_Ata.vars[i].DiskSizeLba28 / 1000.0,
                    m_Ata.vars[i].DiskSizeLba48 / 1000.0,
                    m_Ata.vars[i].DiskSizeWmi / 1000.0);
            }
            else
            {
                cstr.Format(_T("%.1f GB (%.1f/%.1f/----/%.1f)"),
                    m_Ata.vars[i].TotalDiskSize / 1000.0,
                    m_Ata.vars[i].DiskSizeChs / 1000.0,
                    m_Ata.vars[i].DiskSizeLba28 / 1000.0,
                    m_Ata.vars[i].DiskSizeWmi / 1000.0);
            }
        }
        else
        {
            if (m_Ata.vars[i].TotalDiskSize < 1000)
            {
                cstr.Format(_T("%d MB (%.2f/----/----/----)"),
                    m_Ata.vars[i].TotalDiskSize,
                    m_Ata.vars[i].DiskSizeChs / 1000.0);
            }
            else if (m_Ata.vars[i].IsLba48Supported)
            {
                cstr.Format(_T("%.1f GB (%.1f/%.1f/%.1f/----)"),
                    m_Ata.vars[i].TotalDiskSize / 1000.0,
                    m_Ata.vars[i].DiskSizeChs / 1000.0,
                    m_Ata.vars[i].DiskSizeLba28 / 1000.0,
                    m_Ata.vars[i].DiskSizeLba48 / 1000.0);
            }
            else
            {
                cstr.Format(_T("%.1f GB (%.1f/%.1f/----/----)"),
                    m_Ata.vars[i].TotalDiskSize / 1000.0,
                    m_Ata.vars[i].DiskSizeChs / 1000.0,
                    m_Ata.vars[i].DiskSizeLba28 / 1000.0);
            }
        }
        drive.Replace(_T("%TOTAL_DISK_SIZE%"), cstr);


        if (m_Ata.vars[i].IsSsd && m_Ata.vars[i].BufferSize == 0xFFFF * 512)
        {
            cstr.Format(_T(">= %d KB"), m_Ata.vars[i].BufferSize / 1024);
        }
        else if (m_Ata.vars[i].BufferSize > 0)
        {
            cstr.Format(_T("%d KB"), m_Ata.vars[i].BufferSize / 1024);
        }
        else
        {
            cstr = i18n(_T("Dialog"), _T("UNKNOWN"));
        }
        drive.Replace(_T("%BUFFER_SIZE%"), cstr);


        if (m_Ata.vars[i].CommandType == m_Ata.CMD_TYPE_AMD_RC2)
        {
            cstr.Format(_T("     Queue Depth : %s\r\n"), i18n(_T("Dialog"), _T("UNKNOWN")).GetString());
        }
        else if (0 <= m_Ata.vars[i].IdentifyDevice.A.QueueDepth && m_Ata.vars[i].IdentifyDevice.A.QueueDepth < 32)
        {
            cstr.Format(_T("     Queue Depth : %d\r\n"), m_Ata.vars[i].IdentifyDevice.A.QueueDepth + 1);
        }
        else
        {
            cstr.Format(_T("     Queue Depth : 1\n"));
        }
        drive.Replace(_T("%QUEUE_DEPTH%"), cstr);

        if (m_Ata.vars[i].CommandType == m_Ata.CMD_TYPE_AMD_RC2)
        {
            cstr = i18n(_T("Dialog"), _T("UNKNOWN"));
        }
        else if (m_Ata.vars[i].NominalMediaRotationRate == 1)
        {
            cstr = _T("---- (SSD)");
        }
        else if (m_Ata.vars[i].NominalMediaRotationRate > 0)
        {
            cstr.Format(_T("%d RPM"), m_Ata.vars[i].NominalMediaRotationRate);
        }
        else
        {
            cstr = i18n(_T("Dialog"), _T("UNKNOWN"));
        }
        drive.Replace(_T("%ROTATION_RATE%"), cstr);

        feature = _T("");
        if (m_Ata.vars[i].IsSmartSupported)
        {
            feature += _T("S.M.A.R.T., ");
        }
        if (m_Ata.vars[i].IsApmSupported)
        {
            feature += _T("APM, ");
        }
        if (m_Ata.vars[i].IsAamSupported)
        {
            feature += _T("AAM, ");
        }
        if (m_Ata.vars[i].IsNcqSupported)
        {
            feature += _T("NCQ, ");
        }
        if (m_Ata.vars[i].IsTrimSupported)
        {
            feature += _T("TRIM, ");
        }
        if (m_Ata.vars[i].IsDeviceSleepSupported)
        {
            feature += _T("DevSleep, ");
        }
        if (m_Ata.vars[i].IsStreamingSupported)
        {
            feature += _T("Streaming, ");
        }
        if (m_Ata.vars[i].IsGplSupported)
        {
            feature += _T("GPL, ");
        }
        if (m_Ata.vars[i].IsVolatileWriteCachePresent)
        {
            feature += _T("VolatileWriteCache, ");
        }

        /*
        if(m_Ata.vars[i].IsNvCacheSupported)
        {
            feature += _T("NV Cache, ");
        }

        if(m_Ata.vars[i].IsSsd)
        {
            feature += _T("SSD, ");
        }
        */

        if (!feature.IsEmpty())
        {
            feature.Delete(feature.GetLength() - 2, 2);
        }

        if (m_Ata.vars[i].CommandType == m_Ata.CMD_TYPE_AMD_RC2)
        {
            feature = i18n(_T("Dialog"), _T("UNKNOWN"));
        }

        drive.Replace(_T("%SUPPORTED_FEATURE%"), feature);

        if (m_Ata.vars[i].IsAamSupported)
        {
            cstr.Format(_T("%04Xh"), m_Ata.vars[i].IdentifyDevice.A.AcoustricManagement);
            if (m_Ata.vars[i].IsAamEnabled)
            {
                cstr += _T(" [ON]");
            }
            else
            {
                cstr += _T(" [OFF]");
            }
        }
        else
        {
            cstr = _T("----");
        }
        drive.Replace(_T("%AAM_LEVEL%"), cstr);

        if (m_Ata.vars[i].IsApmSupported)
        {
            cstr.Format(_T("%04Xh"), m_Ata.vars[i].IdentifyDevice.A.CurrentPowerManagement);
            if (m_Ata.vars[i].IsApmEnabled)
            {
                cstr += _T(" [ON]");
            }
            else
            {
                cstr += _T(" [OFF]");
            }
        }
        else
        {
            cstr = _T("----");
        }
        drive.Replace(_T("%APM_LEVEL%"), cstr);


        drive.Replace(_T("%DRIVE_LETTER%"), m_Ata.vars[i].DriveMap);


        clip += drive;

        CString vendorSpecific;
        vendorSpecific = i18n(_T("Smart"), _T("VENDOR_SPECIFIC"));

        if (m_Ata.vars[i].AttributeCount > 0)
        {
            cstr.Format(_T("-- S.M.A.R.T. --------------------------------------------------------------\r\n"));
            line = cstr;

            if (m_Ata.vars[i].DiskVendorId == m_Ata.SSD_VENDOR_NVME)
            {
                line += _T("ID RawValues(6) Attribute Name\r\n");
            }
            else if (m_Ata.vars[i].IsRawValues8)
            {
                line += _T("ID Raw Values (8)   Attribute Name\r\n");
            }
            else if (m_Ata.vars[i].IsRawValues7)
            {
                line += _T("ID Cur Wor Thr Raw Values (7) Attribute Name\r\n");
            }
            else
            {
                line += _T("ID Cur Wor Thr RawValues(6) Attribute Name\r\n");
            }

            for (DWORD j = 0; j < m_Ata.vars[i].AttributeCount; j++)
            {
                TCHAR str[256];
                if (m_Ata.vars[i].Attribute[j].Id == 0)
                {
                    continue;
                }

                cstr.Format(_T("%02X"), m_Ata.vars[i].Attribute[j].Id);

                GetPrivateProfileStringFx(m_Ata.vars[i].SmartKeyName, cstr, vendorSpecific, str, 256, m_DefaultLangPath);

                if (m_Ata.vars[i].DiskVendorId == m_Ata.SSD_VENDOR_NVME)
                {
                    //SMART_ATTRIBUTE smartAttr = pSmartInfo->Attribute[j];

                    cstr.Format(_T("%02X %02X%02X%02X%02X%02X%02X %s\r\n"),
                        m_Ata.vars[i].Attribute[j].Id,
                        m_Ata.vars[i].Attribute[j].RawValue[5],
                        m_Ata.vars[i].Attribute[j].RawValue[4],
                        m_Ata.vars[i].Attribute[j].RawValue[3],
                        m_Ata.vars[i].Attribute[j].RawValue[2],
                        m_Ata.vars[i].Attribute[j].RawValue[1],
                        m_Ata.vars[i].Attribute[j].RawValue[0],
                        str
                    );
                }
                else if (m_Ata.vars[i].DiskVendorId == m_Ata.SSD_VENDOR_SANDFORCE)
                {
                    cstr.Format(_T("%02X %s %s %s %02X%02X%02X%02X%02X%02X%02X %s\r\n"),
                        m_Ata.vars[i].Attribute[j].Id,
                        __Number(m_Ata.vars[i].Attribute[j].CurrentValue).GetString(),
                        __Number(m_Ata.vars[i].Attribute[j].WorstValue).GetString(),
                        __Number(m_Ata.vars[i].Threshold[j].ThresholdValue).GetString(),
                        m_Ata.vars[i].Attribute[j].Reserved,
                        m_Ata.vars[i].Attribute[j].RawValue[5],
                        m_Ata.vars[i].Attribute[j].RawValue[4],
                        m_Ata.vars[i].Attribute[j].RawValue[3],
                        m_Ata.vars[i].Attribute[j].RawValue[2],
                        m_Ata.vars[i].Attribute[j].RawValue[1],
                        m_Ata.vars[i].Attribute[j].RawValue[0],
                        str
                    );
                }
                // For JMicron 60x
                else if (m_Ata.vars[i].DiskVendorId == m_Ata.SSD_VENDOR_JMICRON && m_Ata.vars[i].IsRawValues8)
                {
                    cstr.Format(_T("%02X %s %02X%02X%02X%02X%02X%02X%02X%02X %s\r\n"),
                        m_Ata.vars[i].Attribute[j].Id,
                        __Number(m_Ata.vars[i].Attribute[j].CurrentValue).GetString(),
                        m_Ata.vars[i].Attribute[j].Reserved,
                        m_Ata.vars[i].Attribute[j].RawValue[5],
                        m_Ata.vars[i].Attribute[j].RawValue[4],
                        m_Ata.vars[i].Attribute[j].RawValue[3],
                        m_Ata.vars[i].Attribute[j].RawValue[2],
                        m_Ata.vars[i].Attribute[j].RawValue[1],
                        m_Ata.vars[i].Attribute[j].RawValue[0],
                        m_Ata.vars[i].Attribute[j].WorstValue,
                        str
                    );
                }
                else if (m_Ata.vars[i].DiskVendorId == m_Ata.SSD_VENDOR_INDILINX)
                {
                    cstr.Format(_T("%02X %02X%02X%02X%02X%02X%02X%02X%02X %s\r\n"),
                        m_Ata.vars[i].Attribute[j].Id,
                        m_Ata.vars[i].Attribute[j].RawValue[5],
                        m_Ata.vars[i].Attribute[j].RawValue[4],
                        m_Ata.vars[i].Attribute[j].RawValue[3],
                        m_Ata.vars[i].Attribute[j].RawValue[2],
                        m_Ata.vars[i].Attribute[j].RawValue[1],
                        m_Ata.vars[i].Attribute[j].RawValue[0],
                        m_Ata.vars[i].Attribute[j].WorstValue,
                        m_Ata.vars[i].Attribute[j].CurrentValue,
                        str
                    );
                }
                else
                {
                    if (m_Ata.vars[i].IsThresholdCorrect)
                    {
                        cstr.Format(_T("%02X %s %s %s %02X%02X%02X%02X%02X%02X %s\r\n"),
                            m_Ata.vars[i].Attribute[j].Id,
                            __Number(m_Ata.vars[i].Attribute[j].CurrentValue).GetString(),
                            __Number(m_Ata.vars[i].Attribute[j].WorstValue).GetString(),
                            __Number(m_Ata.vars[i].Threshold[j].ThresholdValue).GetString(),
                            m_Ata.vars[i].Attribute[j].RawValue[5],
                            m_Ata.vars[i].Attribute[j].RawValue[4],
                            m_Ata.vars[i].Attribute[j].RawValue[3],
                            m_Ata.vars[i].Attribute[j].RawValue[2],
                            m_Ata.vars[i].Attribute[j].RawValue[1],
                            m_Ata.vars[i].Attribute[j].RawValue[0],
                            str
                        );
                    }
                    else
                    {
                        cstr.Format(_T("%02X %s %s --- %02X%02X%02X%02X%02X%02X %s\r\n"),
                            m_Ata.vars[i].Attribute[j].Id,
                            __Number(m_Ata.vars[i].Attribute[j].CurrentValue).GetString(),
                            __Number(m_Ata.vars[i].Attribute[j].WorstValue).GetString(),
                            m_Ata.vars[i].Attribute[j].RawValue[5],
                            m_Ata.vars[i].Attribute[j].RawValue[4],
                            m_Ata.vars[i].Attribute[j].RawValue[3],
                            m_Ata.vars[i].Attribute[j].RawValue[2],
                            m_Ata.vars[i].Attribute[j].RawValue[1],
                            m_Ata.vars[i].Attribute[j].RawValue[0],
                            str
                        );
                    }
                }
                line += cstr;
            }
            clip += line;
            clip += _T("\r\n");
        }

        WORD data[256];
        if (m_bDumpIdentifyDevice)
        {
            memcpy(data, &(m_Ata.vars[i].IdentifyDevice), 512);
            if (m_bHideSerialNumber)
            {
                if (m_Ata.vars[i].DiskVendorId == m_Ata.SSD_VENDOR_NVME)
                {
                    for (int j = 2; j <= 11; j++) // Serial Number : WORD[2-11]
                    {
                        data[j] = 0xFFFF;
                    }
                }
                else
                {
                    for (int j = 10; j <= 19; j++) // Serial Number : WORD[10-19]
                    {
                        data[j] = 0xFFFF;
                    }
                }

            }
            cstr.Format(_T("-- IDENTIFY_DEVICE ---------------------------------------------------------\r\n"));
            line = cstr;

            line += _T("        0    1    2    3    4    5    6    7    8    9\r\n");
            for (int k = 0; k < 25; k++)
            {
                cstr.Format(_T("%03d: %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X"),
                    k * 10,
                    data[10 * k + 0], data[10 * k + 1], data[10 * k + 2], data[10 * k + 3], data[10 * k + 4],
                    data[10 * k + 5], data[10 * k + 6], data[10 * k + 7], data[10 * k + 8], data[10 * k + 9]);
                line += cstr;
                if (m_bAsciiView)
                {
                    cstr.Format(_T("  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"),
                        AsciiFilter(HIBYTE(data[10 * k + 0])), AsciiFilter(LOBYTE(data[10 * k + 0])),
                        AsciiFilter(HIBYTE(data[10 * k + 1])), AsciiFilter(LOBYTE(data[10 * k + 1])),
                        AsciiFilter(HIBYTE(data[10 * k + 2])), AsciiFilter(LOBYTE(data[10 * k + 2])),
                        AsciiFilter(HIBYTE(data[10 * k + 3])), AsciiFilter(LOBYTE(data[10 * k + 3])),
                        AsciiFilter(HIBYTE(data[10 * k + 4])), AsciiFilter(LOBYTE(data[10 * k + 4])),
                        AsciiFilter(HIBYTE(data[10 * k + 5])), AsciiFilter(LOBYTE(data[10 * k + 5])),
                        AsciiFilter(HIBYTE(data[10 * k + 6])), AsciiFilter(LOBYTE(data[10 * k + 6])),
                        AsciiFilter(HIBYTE(data[10 * k + 7])), AsciiFilter(LOBYTE(data[10 * k + 7])),
                        AsciiFilter(HIBYTE(data[10 * k + 8])), AsciiFilter(LOBYTE(data[10 * k + 8])),
                        AsciiFilter(HIBYTE(data[10 * k + 9])), AsciiFilter(LOBYTE(data[10 * k + 9])));
                    line += cstr;
                }
                line += _T("\r\n");
            }

            cstr.Format(_T("%03d: %04X %04X %04X %04X %04X %04X"),
                250,
                data[250], data[251], data[252], data[253], data[254], data[255]);
            line += cstr;
            if (m_bAsciiView)
            {
                cstr.Format(_T("                      %c%c%c%c%c%c%c%c%c%c%c%c"),
                    AsciiFilter(HIBYTE(data[250])), AsciiFilter(LOBYTE(data[250])),
                    AsciiFilter(HIBYTE(data[251])), AsciiFilter(LOBYTE(data[251])),
                    AsciiFilter(HIBYTE(data[252])), AsciiFilter(LOBYTE(data[252])),
                    AsciiFilter(HIBYTE(data[253])), AsciiFilter(LOBYTE(data[253])),
                    AsciiFilter(HIBYTE(data[254])), AsciiFilter(LOBYTE(data[254])),
                    AsciiFilter(HIBYTE(data[255])), AsciiFilter(LOBYTE(data[255])));
                line += cstr;
            }
            line += _T("\r\n");


            clip += line;
            clip += _T("\r\n");
        }

        if (m_bDumpSmartReadData)
        {
            memcpy(data, &(m_Ata.vars[i].SmartReadData), 512);
            if (m_Ata.vars[i].DiskVendorId == m_Ata.SSD_VENDOR_NVME)
            {
                cstr.Format(_T("-- SMART_NVME --------------------------------------------------------------\r\n"));
            }
            else
            {
                cstr.Format(_T("-- SMART_READ_DATA ---------------------------------------------------------\r\n"));
            }

            line = cstr;
            line += _T("     +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F\r\n");
            for (int k = 0; k < 32; k++)
            {
                cstr.Format(_T("%03X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X"),
                    k * 16,
                    LOBYTE(data[8 * k + 0]), HIBYTE(data[8 * k + 0]),
                    LOBYTE(data[8 * k + 1]), HIBYTE(data[8 * k + 1]),
                    LOBYTE(data[8 * k + 2]), HIBYTE(data[8 * k + 2]),
                    LOBYTE(data[8 * k + 3]), HIBYTE(data[8 * k + 3]),
                    LOBYTE(data[8 * k + 4]), HIBYTE(data[8 * k + 4]),
                    LOBYTE(data[8 * k + 5]), HIBYTE(data[8 * k + 5]),
                    LOBYTE(data[8 * k + 6]), HIBYTE(data[8 * k + 6]),
                    LOBYTE(data[8 * k + 7]), HIBYTE(data[8 * k + 7]));
                line += cstr;
                if (m_bAsciiView)
                {
                    cstr.Format(_T("  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\r\n"),
                        AsciiFilter(LOBYTE(data[8 * k + 0])), AsciiFilter(HIBYTE(data[8 * k + 0])),
                        AsciiFilter(LOBYTE(data[8 * k + 1])), AsciiFilter(HIBYTE(data[8 * k + 1])),
                        AsciiFilter(LOBYTE(data[8 * k + 2])), AsciiFilter(HIBYTE(data[8 * k + 2])),
                        AsciiFilter(LOBYTE(data[8 * k + 3])), AsciiFilter(HIBYTE(data[8 * k + 3])),
                        AsciiFilter(LOBYTE(data[8 * k + 4])), AsciiFilter(HIBYTE(data[8 * k + 4])),
                        AsciiFilter(LOBYTE(data[8 * k + 5])), AsciiFilter(HIBYTE(data[8 * k + 5])),
                        AsciiFilter(LOBYTE(data[8 * k + 6])), AsciiFilter(HIBYTE(data[8 * k + 6])),
                        AsciiFilter(LOBYTE(data[8 * k + 7])), AsciiFilter(HIBYTE(data[8 * k + 7])));
                }
                else
                {
                    cstr = _T("\r\n");
                }
                line += cstr;
            }
            clip += line;
            clip += _T("\r\n");
        }

        if (m_bDumpSmartReadThreshold && (m_Ata.vars[i].DiskVendorId != m_Ata.SSD_VENDOR_NVME))
        {
            memcpy(data, &(m_Ata.vars[i].SmartReadThreshold), 512);
            cstr.Format(_T("-- SMART_READ_THRESHOLD ----------------------------------------------------\r\n"));
            line = cstr;
            line += _T("     +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F\r\n");
            for (int k = 0; k < 32; k++)
            {
                cstr.Format(_T("%03X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X"),
                    k * 16,
                    LOBYTE(data[8 * k + 0]), HIBYTE(data[8 * k + 0]),
                    LOBYTE(data[8 * k + 1]), HIBYTE(data[8 * k + 1]),
                    LOBYTE(data[8 * k + 2]), HIBYTE(data[8 * k + 2]),
                    LOBYTE(data[8 * k + 3]), HIBYTE(data[8 * k + 3]),
                    LOBYTE(data[8 * k + 4]), HIBYTE(data[8 * k + 4]),
                    LOBYTE(data[8 * k + 5]), HIBYTE(data[8 * k + 5]),
                    LOBYTE(data[8 * k + 6]), HIBYTE(data[8 * k + 6]),
                    LOBYTE(data[8 * k + 7]), HIBYTE(data[8 * k + 7]));
                line += cstr;
                if (m_bAsciiView)
                {
                    cstr.Format(_T("  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\r\n"),
                        AsciiFilter(LOBYTE(data[8 * k + 0])), AsciiFilter(HIBYTE(data[8 * k + 0])),
                        AsciiFilter(LOBYTE(data[8 * k + 1])), AsciiFilter(HIBYTE(data[8 * k + 1])),
                        AsciiFilter(LOBYTE(data[8 * k + 2])), AsciiFilter(HIBYTE(data[8 * k + 2])),
                        AsciiFilter(LOBYTE(data[8 * k + 3])), AsciiFilter(HIBYTE(data[8 * k + 3])),
                        AsciiFilter(LOBYTE(data[8 * k + 4])), AsciiFilter(HIBYTE(data[8 * k + 4])),
                        AsciiFilter(LOBYTE(data[8 * k + 5])), AsciiFilter(HIBYTE(data[8 * k + 5])),
                        AsciiFilter(LOBYTE(data[8 * k + 6])), AsciiFilter(HIBYTE(data[8 * k + 6])),
                        AsciiFilter(LOBYTE(data[8 * k + 7])), AsciiFilter(HIBYTE(data[8 * k + 7])));
                }
                else
                {
                    cstr = _T("\r\n");
                }
                line += cstr;
            }
            clip += line;
            clip += _T("\r\n");
        }
    }

    if (fileName.IsEmpty())
    {
        _setmode(_fileno(stdout), _O_U16TEXT);
        _tprintf_s(_T("%s\n"), (LPCTSTR)clip);
    }
    else
    {
        CT2A utf8(clip, CP_UTF8);

        CFile file;
        CFileException e;
        if (file.Open(fileName, CFile::modeCreate | CFile::modeWrite, &e))
        {
            file.Write((char*)utf8, (UINT)strlen(utf8));
            file.Close();
        }
        else
        {
            TCHAR szError[1024];
            e.GetErrorMessage(szError, 1024);
            _tprintf_s(_T("File could not be created %s\n"), szError);
        }
    }
}

CString CDiskInfo::i18n(CString section, CString key)
{
    TCHAR str[256];
    CString cstr;

    GetPrivateProfileStringFx(section, key, _T(""), str, 256, m_DefaultLangPath);
    cstr = str;

    return cstr;
}
