#ifndef SDK_H
#define SDK_H

#include <iostream>
#include <vector>
#include <windows.h>

// protection
void VMProtectBegin(const char*);
void VMProtectBeginVirtualization(const char*);
void VMProtectBeginMutation(const char*);
void VMProtectBeginUltra(const char*);
void VMProtectBeginVirtualizationLockByKey(const char*);
void VMProtectBeginUltraLockByKey(const char*);
void VMProtectEnd(void);

// utils
bool VMProtectIsProtected();
bool VMProtectIsDebuggerPresent(bool);
bool VMProtectIsVirtualMachinePresent(void);
bool VMProtectIsValidImageCRC(void);
const char* VMProtectDecryptStringA(const char* value);
const wchar_t* VMProtectDecryptStringW(const wchar_t* value);
bool VMProtectFreeString(void* value);

// licensing
enum VMProtectSerialStateFlags {
    SERIAL_STATE_SUCCESS = 0,
    SERIAL_STATE_FLAG_CORRUPTED = 0x00000001,
    SERIAL_STATE_FLAG_INVALID = 0x00000002,
    SERIAL_STATE_FLAG_BLACKLISTED = 0x00000004,
    SERIAL_STATE_FLAG_DATE_EXPIRED = 0x00000008,
    SERIAL_STATE_FLAG_RUNNING_TIME_OVER = 0x00000010,
    SERIAL_STATE_FLAG_BAD_HWID = 0x00000020,
    SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED = 0x00000040,
};

#pragma pack(push, 1)
typedef struct
{
    unsigned short wYear;
    unsigned char bMonth;
    unsigned char bDay;
} VMProtectDate;

typedef struct
{
    int nState; // VMProtectSerialStateFlags
    wchar_t wUserName[256]; // user name
    wchar_t wEMail[256]; // email
    VMProtectDate dtExpire; // date of serial number expiration
    VMProtectDate dtMaxBuild; // max date of build, that will accept this key
    int bRunningTime; // running time in minutes
    unsigned char nUserDataLength; // length of user data in bUserData
    unsigned char bUserData[255]; // up to 255 bytes of user data
} VMProtectSerialNumberData;
#pragma pack(pop)

int VMProtectSetSerialNumber(const char* serial);
int VMProtectGetSerialNumberState();
bool VMProtectGetSerialNumberData(VMProtectSerialNumberData* data, int size);
int VMProtectGetCurrentHWID(char* hwid, int size);

// activation
enum VMProtectActivationFlags {
    ACTIVATION_OK = 0,
    ACTIVATION_SMALL_BUFFER,
    ACTIVATION_NO_CONNECTION,
    ACTIVATION_BAD_REPLY,
    ACTIVATION_BANNED,
    ACTIVATION_CORRUPTED,
    ACTIVATION_BAD_CODE,
    ACTIVATION_ALREADY_USED,
    ACTIVATION_SERIAL_UNKNOWN,
    ACTIVATION_EXPIRED,
    ACTIVATION_NOT_AVAILABLE
};

int VMProtectActivateLicense(const char* code, char* serial, int size);
int VMProtectDeactivateLicense(const char* serial);
int VMProtectGetOfflineActivationString(const char* code, char* buf, int size);
int VMProtectGetOfflineDeactivationString(const char* serial, char* buf, int size);

#endif
