// JToolKits.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "hook_manager.h"
#include <iostream>
#include "core.h"

void* old_messagebox = nullptr;
void* old_messagebox_handler = nullptr;
int WINAPI MyMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    // 你可以在这里自定义行为
    return 0; // 或调用原始 MessageBoxW
}

void HookAPIs(HookManager& hook_manager)
{
    hook_manager.Begin();
    HMODULE dll = GetModuleHandleA("user32.dll");
    old_messagebox_handler = hook_manager.HookAPI(dll, "MessageBoxW", (void*)MyMessageBoxW, true, &old_messagebox);
    hook_manager.End();
}

void UnhookAPIs(HookManager& hook_manager)
{
    hook_manager.Begin();
    hook_manager.UnhookAPI(old_messagebox_handler);
    hook_manager.End();
}

int main()
{
    auto id = Core::Instance()->hardware_id();



    HookManager mgr;
    HookAPIs(mgr);
    MessageBox(NULL, L"Hello, World!", L"Hooked MessageBox<0>", MB_OK);
    UnhookAPIs(mgr);
    MessageBox(NULL, L"Hello, World!", L"Hooked MessageBox<1>", MB_OK);

}