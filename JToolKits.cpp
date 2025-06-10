// JToolKits.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "hook_manager.h"
#include <iostream>

int WINAPI MyMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    // 你可以在这里自定义行为
    return 0; // 或调用原始 MessageBoxW
}

int main()
{
    HMODULE hUser32 = LoadLibraryA("user32.dll");
    void* oldMessageBoxW = nullptr;
    HookManager mgr;
    auto old_handler = mgr.HookAPI(hUser32, "MessageBoxW", (void*)MyMessageBoxW, true, &oldMessageBoxW);
    MessageBox(NULL, L"Hello, World!", L"Hooked MessageBox<0>", MB_OK);
    mgr.UnhookAPI(old_handler);
    MessageBox(NULL, L"Hello, World!", L"Hooked MessageBox<1>", MB_OK);
}