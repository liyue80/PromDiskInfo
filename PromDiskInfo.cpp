// PromDiskInfo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include <iostream>
#include <shellapi.h>
#include "Priscilla/UtilityFx.h"
#include "DiskInfo.h"

void printUsage(BOOL bVerbose = FALSE)
{

}

constexpr UINT_PTR str2int(const TCHAR* str, int h = 0)
{
    return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
}

int main(
#ifndef UNICODE
int argc, char **argv
#endif // !UNICODE
)
{
#ifdef UNICODE
    int argc;
    LPTSTR cmdLine = ::GetCommandLine();
    LPTSTR *argv = ::CommandLineToArgvW(cmdLine, &argc);
#endif // UNICODE

    CString strOutFile;
    for (int i = 1; i < argc; i++)
    {
        CString v = argv[i];

        if (_tcschr(_T("-/"), v.GetAt(0)) != NULL) // begin with '-' or '/'
        {
            v = v.TrimLeft(_T("-/"));

            switch (str2int((LPCTSTR)v))
            {
            case str2int(_T("?")):
            case str2int(_T("h")):
            case str2int(_T("help")):
                printUsage(TRUE);
                return 0;

            case str2int(_T("o")):
            case str2int(_T("out")):
                if (++i < argc)
                {
                    strOutFile = argv[i];
                }
                else
                {
                    _tprintf_s(_T("Missing argument of %s\n\n"), argv[i]);
                    printUsage();
                    return -1;
                }
                break;

            default:
                _tprintf_s(_T("Unknown option %s\n\n"), argv[i]);
                printUsage();
                return -1;
            }
        }
        else
        {
            _tprintf_s(_T("Invalid argument '%s'\n\n"), (LPCTSTR)v);
            printUsage();
            return -1;
        }
    }

    SetDebugMode(0);
    CDiskInfo diskInfo;
    diskInfo.SaveText(strOutFile);

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
