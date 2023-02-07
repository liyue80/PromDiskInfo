#pragma once

class CDiskInfo
{
public:
    void SaveText(std::wstring fileName);

protected:
    CAtaSmart m_Ata;
};
