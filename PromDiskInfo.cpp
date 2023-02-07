// PromDiskInfo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include <iostream>
#include "AtaSmart.h"
#include "Priscilla/UtilityFx.h"

int main()
{
    CAtaSmart ata;
    BOOL bFlag = FALSE;
    SetDebugMode(2);

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        //m_iStatus = ERROR_COINITIALIZE_EX;
        return 0;
    }

    ata.Init(TRUE, TRUE, &bFlag, FALSE, FALSE, FALSE);
    
    CoUninitialize();

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
