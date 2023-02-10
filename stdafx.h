#pragma once

#ifndef _WIN32_WINNT              
#define _WIN32_WINNT 0x0501
#endif						

#include <afx.h>

#include <string>
#include <regex>
#include <vector>

#include "MFCAlternative/C_Array.h"

#define PRODUCT_NAME L"DiskInfo"
#define PRODUCT_VERSION L"22.02.01.0000"
#define PRODUCT_COPY_YEAR L"2008-2023"


#define HIDE_SERIAL_NUMBER 0
#define DUMP_IDENTIFY_DEVICE 1
#define DUMP_SMART_READ_DATA 1
#define DUMP_SMART_READ_THRESHOLD 1
#define ASCII_VIEW 1
#define HIDE_NO_SMART_DISK 0
