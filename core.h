#pragma once
#include <windows.h>

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef __int64 int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT,
    *PSECTION_INHERIT;

namespace core {
bool IsVirtualMachinePresent();
bool FindFirmwareVendor(const uint8_t* data, size_t data_size);
};
