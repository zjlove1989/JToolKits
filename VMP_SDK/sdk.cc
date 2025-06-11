#include "sdk.h"
#include <debugapi.h>
#include <stdint.h>
#include <sysinfoapi.h>
#include <libloaderapi.h>
#include <stringapiset.h>
#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>

bool  VMProtectIsProtected()
{
	return false;
}

void  VMProtectBegin(const char *) 
{

}

void  VMProtectBeginMutation(const char *)
{

}

void  VMProtectBeginVirtualization(const char *) 
{

}

void  VMProtectBeginUltra(const char *) 
{ 

}

void  VMProtectBeginVirtualizationLockByKey(const char *) 
{ 

}

void  VMProtectBeginUltraLockByKey(const char *) 
{ 

}

void  VMProtectEnd() 
{

}

bool  VMProtectIsDebuggerPresent(bool) 
{
#ifdef VMP_GNU
	return false;
#elif defined(WIN_DRIVER)
	return false;
#else
	return IsDebuggerPresent() != FALSE; 
#endif
}

bool  VMProtectIsVirtualMachinePresent() 
{ 
	return false; 
}

bool  VMProtectIsValidImageCRC()
{ 
	return true; 
}

const char *  VMProtectDecryptStringA(const char *value) 
{ 
	return value; 
}

const wchar_t *  VMProtectDecryptStringW(const wchar_t *value) 
{ 
	return value; 
}

bool  VMProtectFreeString(void *) 
{ 
	return true; 
}

int  VMProtectGetOfflineActivationString(const char *, char *, int) 
{
	return ACTIVATION_OK;
}

int  VMProtectGetOfflineDeactivationString(const char *, char *, int) 
{
	return ACTIVATION_OK;
}


bool g_serial_is_correct = false;
bool g_serial_is_blacklisted = false;
uint32_t g_time_of_start = GetTickCount();


static bool GetIniValue(const char *value_name, wchar_t *buffer, size_t size)
{
	wchar_t file_name[MAX_PATH];
	file_name[0] = 0;
	GetModuleFileNameW(NULL, file_name, _countof(file_name));
	wchar_t *p = wcsrchr(file_name, L'\\');
	if (p)
		*(p + 1) = 0;
	wcsncat_s(file_name, L"VMProtectLicense.ini", _countof(file_name));

	wchar_t key_name[1024] = {0};
	MultiByteToWideChar(CP_ACP, 0, value_name, -1, key_name, _countof(key_name));
	return GetPrivateProfileStringW(L"TestLicense", key_name, L"", buffer, static_cast<DWORD>(size), file_name) != 0;
}

static bool GetIniValue(const char *value_name, char *buffer, size_t size)
{
	wchar_t value[2048];
	if (GetIniValue(value_name, value, sizeof(value))) {
		WideCharToMultiByte(CP_ACP, 0, value, -1, buffer, static_cast<int>(size), NULL, NULL);
		return true;
	}
	if (buffer && size)
		buffer[0] = 0;
	return false;
}

#define MAKEDATE(y, m, d) (DWORD)((y << 16) + (m << 8) + d)

int  VMProtectGetSerialNumberState()
{
    if (!g_serial_is_correct)
        return SERIAL_STATE_FLAG_INVALID;
    if (g_serial_is_blacklisted)
        return SERIAL_STATE_FLAG_BLACKLISTED;

    int res = 0;

    char buf[256];
    if (GetIniValue("TimeLimit", buf, sizeof(buf))) {
        int running_time = atoi(buf);
        if (running_time >= 0 && running_time <= 255) {
            uint32_t dw = GetTickCount();
            int d = (dw - g_time_of_start) / 1000 / 60; // minutes
            if (running_time <= d)
                res |= SERIAL_STATE_FLAG_RUNNING_TIME_OVER;
        }
    }

    if (GetIniValue("ExpDate", buf, sizeof(buf))) {
        int y, m, d;
        if (sscanf_s(buf, "%04d%02d%02d", &y, &m, &d) == 3) {
            uint32_t ini_date = (y << 16) + (static_cast<uint8_t>(m) << 8) + static_cast<uint8_t>(d);
            uint32_t cur_date;
            SYSTEMTIME st;
            GetLocalTime(&st);
            cur_date = (st.wYear << 16) + (static_cast<uint8_t>(st.wMonth) << 8) + static_cast<uint8_t>(st.wDay);
            if (cur_date > ini_date)
                res |= SERIAL_STATE_FLAG_DATE_EXPIRED;
        }
    }

    if (GetIniValue("MaxBuildDate", buf, sizeof(buf))) {
        int y, m, d;
        if (sscanf_s(buf, "%04d%02d%02d", &y, &m, &d) == 3) {
            uint32_t ini_date = (y << 16) + (static_cast<uint8_t>(m) << 8) + static_cast<uint8_t>(d);
            uint32_t cur_date;
            SYSTEMTIME st;
            GetLocalTime(&st);
            cur_date = (st.wYear << 16) + (static_cast<uint8_t>(st.wMonth) << 8) + static_cast<uint8_t>(st.wDay);
            if (cur_date > ini_date)
                res |= SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED;
        }
    }

    if (GetIniValue("KeyHWID", buf, sizeof(buf))) {
        char buf2[256];
        GetIniValue("MyHWID", buf2, sizeof(buf2));
        if (strcmp(buf, buf2) != 0)
            res |= SERIAL_STATE_FLAG_BAD_HWID;
    }

    return res;
}

int  VMProtectSetSerialNumber(const char *serial)
{
    g_serial_is_correct = false;
    g_serial_is_blacklisted = false;
    if (!serial || !serial[0])
        return SERIAL_STATE_FLAG_INVALID;

    char buf_serial[2048];
    const char* src = serial;
    char* dst = buf_serial;
    while (*src) {
        char c = *src;
        // check agains base64 alphabet
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')
            *dst++ = c;
        src++;
    }
    *dst = 0;

    char ini_serial[2048];
    if (!GetIniValue("AcceptedSerialNumber", ini_serial, sizeof(ini_serial)))
        strcpy_s(ini_serial, "serialnumber");
    g_serial_is_correct = strcmp(buf_serial, ini_serial) == 0;

    if (GetIniValue("BlackListedSerialNumber", ini_serial, sizeof(ini_serial)))
        g_serial_is_blacklisted = strcmp(buf_serial, ini_serial) == 0;

    return VMProtectGetSerialNumberState();
}

bool  VMProtectGetSerialNumberData(VMProtectSerialNumberData *data, int size)
{
    if (size != sizeof(VMProtectSerialNumberData))
        return false;
    memset(data, 0, sizeof(VMProtectSerialNumberData));

    data->nState = VMProtectGetSerialNumberState();
    if (data->nState & (SERIAL_STATE_FLAG_INVALID | SERIAL_STATE_FLAG_BLACKLISTED))
        return true; // do not need to read the rest

    GetIniValue("UserName", data->wUserName, _countof(data->wUserName));
    GetIniValue("EMail", data->wEMail, _countof(data->wEMail));

    char buf[2048];
    if (GetIniValue("TimeLimit", buf, sizeof(buf))) {
        int running_time = atoi(buf);
        if (running_time < 0)
            running_time = 0;
        else if (running_time > 255)
            running_time = 255;
        data->bRunningTime = static_cast<unsigned char>(running_time);
    }

    if (GetIniValue("ExpDate", buf, sizeof(buf))) {
        int y, m, d;
        if (sscanf_s(buf, "%04d%02d%02d", &y, &m, &d) == 3) {
            data->dtExpire.wYear = static_cast<unsigned short>(y);
            data->dtExpire.bMonth = static_cast<unsigned char>(m);
            data->dtExpire.bDay = static_cast<unsigned char>(d);
        }
    }

    if (GetIniValue("MaxBuildDate", buf, sizeof(buf))) {
        int y, m, d;
        if (sscanf_s(buf, "%04d%02d%02d", &y, &m, &d) == 3) {
            data->dtMaxBuild.wYear = static_cast<unsigned short>(y);
            data->dtMaxBuild.bMonth = static_cast<unsigned char>(m);
            data->dtMaxBuild.bDay = static_cast<unsigned char>(d);
        }
    }

    if (GetIniValue("UserData", buf, sizeof(buf))) {
        size_t len = strlen(buf);
        if (len > 0 && len % 2 == 0 && len <= 255 * 2) // otherwise UserData is empty or has bad length
        {
            for (size_t src = 0, dst = 0; src < len; src++) {
                int v = 0;
                char c = buf[src];

                if (c >= '0' && c <= '9')
                    v = c - '0';
                else if (c >= 'a' && c <= 'f')
                    v = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F')
                    v = c - 'A' + 10;
                else {
                    data->nUserDataLength = 0;
                    memset(data->bUserData, 0, sizeof(data->bUserData));
                    break;
                }

                if (src % 2 == 0) {
                    data->bUserData[dst] = static_cast<unsigned char>(v << 4);
                } else {
                    data->bUserData[dst] |= v;
                    dst++;
                    data->nUserDataLength = static_cast<unsigned char>(dst);
                }
            }
        }
    }

    return true;
}

int  VMProtectGetCurrentHWID(char *hwid, int size)
{
    if (hwid && size == 0)
        return 0;

    char buf[1024];
    if (!GetIniValue("MyHWID", buf, sizeof(buf)))
        strcpy_s(buf, "myhwid");

    int res = static_cast<int>(strlen(buf));
    if (hwid) {
        if (size - 1 < res)
            res = size - 1;
        memcpy(hwid, buf, res);
        hwid[res] = 0;
    }
    return res + 1;
}

int  VMProtectActivateLicense(const char *code, char *serial, int size) 
{ 
if (!code)
        return ACTIVATION_BAD_CODE;

    char buf[2048];
    if (!GetIniValue("AcceptedActivationCode", buf, sizeof(buf)))
        strcpy_s(buf, "activationcode");
    if (strcmp(code, buf) != 0)
        return ACTIVATION_BAD_CODE;

    if (!GetIniValue("AcceptedSerialNumber", buf, sizeof(buf)))
        strcpy_s(buf, "serialnumber");

    int need = static_cast<int>(strlen(buf));
    if (need > size - 1)
        return ACTIVATION_SMALL_BUFFER;

    strncpy_s(serial, size, buf, _TRUNCATE);
    return ACTIVATION_OK;
}

int  VMProtectDeactivateLicense(const char *) 
{ 
	return ACTIVATION_OK;
}
