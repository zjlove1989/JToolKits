#include "core.h"
#include <intrin.h>
#include <windows.h>
#include <winternl.h>

bool core::IsVirtualMachinePresent()
{
    // hardware detection
    int cpu_info[4];
    __cpuid(cpu_info, 1);
    if ((cpu_info[2] >> 31) & 1) {
        // check Hyper-V root partition
        cpu_info[1] = 0;
        cpu_info[2] = 0;
        cpu_info[3] = 0;
        __cpuid(cpu_info, 0x40000000);
        if (cpu_info[1] == 0x7263694d && cpu_info[2] == 0x666f736f && cpu_info[3] == 0x76482074) { // "Microsoft Hv"
            cpu_info[1] = 0;
            __cpuid(cpu_info, 0x40000003);
            if (cpu_info[1] & 1)
                return false;
        }
        return true;
    }

    uint64_t val;
    uint8_t mem_val;
    __try {
        // set T flag
        __writeeflags(__readeflags() | 0x100);
        val = __rdtsc();
        __nop();
    } __except (mem_val = *static_cast<uint8_t*>((GetExceptionInformation())->ExceptionRecord->ExceptionAddress), EXCEPTION_EXECUTE_HANDLER) {
        if (mem_val != 0x90)
            return true;
    }

    __try {
        // set T flag
        __writeeflags(__readeflags() | 0x100);
        __cpuid(cpu_info, 1);
        __nop();
    } __except (mem_val = *static_cast<uint8_t*>((GetExceptionInformation())->ExceptionRecord->ExceptionAddress), EXCEPTION_EXECUTE_HANDLER) {
        if (mem_val != 0x90)
            return true;
    }

    HMODULE dll = GetModuleHandleA("kernel32.dll");
    bool is_found = false;
    typedef UINT(WINAPI tEnumSystemFirmwareTables)(DWORD FirmwareTableProviderSignature, PVOID pFirmwareTableEnumBuffer, DWORD BufferSize);
    typedef UINT(WINAPI tGetSystemFirmwareTable)(DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID, PVOID pFirmwareTableBuffer, DWORD BufferSize);
    tEnumSystemFirmwareTables* enum_system_firmware_tables = reinterpret_cast<tEnumSystemFirmwareTables*>(GetProcAddress(dll, "EnumSystemFirmwareTables"));
    tGetSystemFirmwareTable* get_system_firmware_table = reinterpret_cast<tGetSystemFirmwareTable*>(GetProcAddress(dll, "GetSystemFirmwareTable"));

    if (enum_system_firmware_tables && get_system_firmware_table) {
        UINT tables_size = enum_system_firmware_tables('FIRM', NULL, 0);
        if (tables_size) {
            DWORD* tables = new DWORD[tables_size / sizeof(DWORD)];
            enum_system_firmware_tables('FIRM', tables, tables_size);
            for (size_t i = 0; i < tables_size / sizeof(DWORD); i++) {
                UINT data_size = get_system_firmware_table('FIRM', tables[i], NULL, 0);
                if (data_size) {
                    uint8_t* data = new uint8_t[data_size];
                    get_system_firmware_table('FIRM', tables[i], data, data_size);
                    if (FindFirmwareVendor(data, data_size))
                        is_found = true;
                    delete[] data;
                }
            }
            delete[] tables;
        }
    } else {
        dll = LoadLibraryA("ntdll.dll");
        typedef NTSTATUS(tNtOpenSection)(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes);
        typedef NTSTATUS(tNtMapViewOfSection)(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID * BaseAddress, ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, ULONG Win32Protect);
        typedef NTSTATUS(tNtUnmapViewOfSection)(HANDLE ProcessHandle, PVOID BaseAddress);
        typedef NTSTATUS(tNtClose)(HANDLE Handle);
        
        tNtOpenSection* open_section = reinterpret_cast<tNtOpenSection*>(GetProcAddress(dll, "NtOpenSection"));
        tNtMapViewOfSection* map_view_of_section = reinterpret_cast<tNtMapViewOfSection*>(GetProcAddress(dll, "NtMapViewOfSection"));
        tNtUnmapViewOfSection* unmap_view_of_section = reinterpret_cast<tNtUnmapViewOfSection*>(GetProcAddress(dll, "NtUnmapViewOfSection"));
        tNtClose* close = reinterpret_cast<tNtClose*>(GetProcAddress(dll, "NtClose"));

        if (open_section && map_view_of_section && unmap_view_of_section && close) {
            HANDLE process = GetCurrentProcess();
            HANDLE physical_memory = NULL;
            UNICODE_STRING str;
            OBJECT_ATTRIBUTES attrs;

            wchar_t buf[] = { '\\', 'd', 'e', 'v', 'i', 'c', 'e', '\\', 'p', 'h', 'y', 's', 'i', 'c', 'a', 'l', 'm', 'e', 'm', 'o', 'r', 'y', 0 };
            str.Buffer = buf;
            str.Length = sizeof(buf) - sizeof(wchar_t);
            str.MaximumLength = sizeof(buf);

            InitializeObjectAttributes(&attrs, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);
            NTSTATUS status = open_section(&physical_memory, SECTION_MAP_READ, &attrs);
            if (NT_SUCCESS(status)) {
                void* data = NULL;
                SIZE_T data_size = 0x10000;
                LARGE_INTEGER offset;
                offset.QuadPart = 0xc0000;

                status = map_view_of_section(physical_memory, process, &data, NULL, data_size, &offset, &data_size, ViewShare, 0, PAGE_READONLY);
                if (NT_SUCCESS(status)) {
                    if (FindFirmwareVendor(static_cast<uint8_t*>(data), data_size))
                        is_found = true;
                    unmap_view_of_section(process, data);
                }
                close(physical_memory);
            }
        }
    }
    if (is_found)
        return true;

    if (GetModuleHandleA("sbiedll.dll"))
        return true;

    return false;
}

bool core::FindFirmwareVendor(const uint8_t* data, size_t data_size)
{
    for (size_t i = 0; i < data_size; i++) {
        if (i + 9 < data_size && data[i + 0] == 'V' && data[i + 1] == 'i' && data[i + 2] == 'r' && data[i + 3] == 't' && data[i + 4] == 'u' && data[i + 5] == 'a' && data[i + 6] == 'l' && data[i + 7] == 'B' && data[i + 8] == 'o' && data[i + 9] == 'x')
            return true;
        if (i + 5 < data_size && data[i + 0] == 'V' && data[i + 1] == 'M' && data[i + 2] == 'w' && data[i + 3] == 'a' && data[i + 4] == 'r' && data[i + 5] == 'e')
            return true;
        if (i + 8 < data_size && data[i + 0] == 'P' && data[i + 1] == 'a' && data[i + 2] == 'r' && data[i + 3] == 'a' && data[i + 4] == 'l' && data[i + 5] == 'l' && data[i + 6] == 'e' && data[i + 7] == 'l' && data[i + 8] == 's')
            return true;
    }
    return false;
}
