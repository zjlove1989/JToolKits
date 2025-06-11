// JToolKits.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "VMP_SDK/hook_manager.h"
#include "VMP_SDK/hwid.h"
#include "VMP_SDK/third-party/lzma/LzmaEncode.h"
#include <iostream>

void TestHwid()
{
    char buffer[256];
    HardwareID hardware_id;
    hardware_id.GetCurrent(buffer, 256);
    MessageBoxA(NULL, buffer, "Tip", MB_OK);
}

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

void TestHookMessageBox()
{
    HookManager hook_manager;
    HookAPIs(hook_manager);
    // 测试钩子
    MessageBoxW(NULL, L"0：Hello, World!", L"Test", MB_OK);
    // 结束钩子
    UnhookAPIs(hook_manager);
    // 测试钩子
    MessageBoxW(NULL, L"1：Hello, World!", L"Test", MB_OK);
}

// 自定义内存分配函数
static void* SzAlloc(void* p, size_t size)
{
    (void)p; // 避免未使用参数警告
    return malloc(size);
}
static void SzFree(void* p, void* address)
{
    (void)p; // 避免未使用参数警告
    free(address);
}
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

void TestLzma()
{
    // 示例输入数据
    const Byte src[] = "This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.This is a test string to be compressed using LZMA.";
    SizeT srcLen = sizeof(src) - 1; // 不包括末尾的 '\0'

    // 预估输出缓冲区大小（通常比输入稍大）
    SizeT destLen = srcLen * 2;
    Byte* dest = (Byte*)malloc(destLen);
    if (!dest) {
        printf("Memory allocation failed for dest buffer.\n");
        return;
    }

    // 设置 LZMA 编码属性
    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    props.level = 5; // 压缩级别 0-9，越高压缩率越高但速度越慢
    props.dictSize = 1 << 20; // 字典大小 1MB

    // 编码后的属性（用于解压）
    Byte propsEncoded[LZMA_PROPS_SIZE];
    SizeT propsSize = LZMA_PROPS_SIZE;

    // 执行压缩
    SRes res = LzmaEncode(
        dest, &destLen, // 输出缓冲区和大小
        src, srcLen, // 输入数据和大小
        &props, // 压缩属性
        propsEncoded, &propsSize, // 编码后的属性
        1, // 写入结束标记
        NULL, // 进度回调（可选）
        &g_Alloc, &g_Alloc // 内存分配器
    );

    if (res == SZ_OK) {
        printf("Compression successful!\n");
        printf("Original size: %zu, Compressed size: %zu\n", srcLen, destLen);

        // 这里可以将 dest 和 propsEncoded 保存到文件或传输
        // 解压时需要 propsEncoded 和 dest 数据

    } else {
        printf("Compression failed with error: %d\n", res);
    }

    free(dest);
}

int main()
{
    TestHwid();
    TestHookMessageBox();
    TestLzma();
}