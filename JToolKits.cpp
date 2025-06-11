// JToolKits.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "VMP_SDK/hook_manager.h"
#include "VMP_SDK/hwid.h"
#include "VMP_SDK/third-party/lzma/LzmaDecode.h"
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

void TestDecodeLzma(Byte* compressedData, SizeT compressedSize, Byte* propsEncoded)
{
    // 假设这是之前压缩的数据和属性

    // 预估解压后的数据大小（如果知道原始大小，可以直接用）
    SizeT uncompressedSize = compressedSize * 10 + 1; // 预估
    Byte* uncompressedData = (Byte*)malloc(uncompressedSize);
    if (!uncompressedData) {
        printf("Memory allocation failed for uncompressed buffer.\n");
        return;
    }

    memset(uncompressedData, 0, uncompressedSize); // 初始化缓冲区

    // 初始化 LZMA 解码器状态
    CLzmaDecoderState state;
    memset(&state, 0, sizeof(state));

    // 解析压缩属性（propsEncoded）
    CLzmaProperties props;
    int res = LzmaDecodeProperties(&props, propsEncoded, LZMA_PROPERTIES_SIZE);
    if (res != LZMA_RESULT_OK) {
        printf("Failed to decode LZMA properties: %d\n", res);
        free(uncompressedData);
        return;
    }
    state.Properties = props;

    // 分配概率模型所需的内存
    SizeT numProbs = LzmaGetNumProbs(&props);
    state.Probs = (CProb*)g_Alloc.Alloc(NULL, numProbs * sizeof(CProb));
    if (!state.Probs) {
        printf("Memory allocation failed for probability model.\n");
        free(uncompressedData);
        return;
    }

// 初始化解码器
#ifdef _LZMA_OUT_READ
    LzmaDecoderInit(&state);
#endif

    // 解压数据
    SizeT inProcessed = 0, outProcessed = 0;
    res = LzmaDecode(
        &state,
#ifdef _LZMA_IN_CB
        NULL, // 不使用回调
#else
        compressedData, compressedSize, &inProcessed,
#endif
        uncompressedData, uncompressedSize, &outProcessed);

    if (res == LZMA_RESULT_OK) {
        printf("Decompression successful!\n");
        printf("Decompressed data: %s\n", uncompressedData);

        printf("Compressed size: %zu, Decompressed size: %zu\n", inProcessed, outProcessed);

        // 现在 uncompressedData 包含解压后的数据
        // 可以保存到文件或进一步处理
    } else {
        printf("Decompression failed with error: %d\n", res);
    }

    // 释放资源
    g_Alloc.Free(NULL, state.Probs);
    free(uncompressedData);
}

void TestEncodeLzma()
{
    // 示例输入数据
    const Byte src[] = "This is a test string to be compressed using LZMA.-|||-This is a test string to be compressed using LZMA.";
    SizeT srcLen = sizeof(src) + 1; // 不包括末尾的 '\0'

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
        TestDecodeLzma(dest, destLen, propsEncoded);
    } else {
        printf("Compression failed with error: %d\n", res);
    }

    free(dest);
}

int main()
{
    TestHwid();
    TestHookMessageBox();
    TestEncodeLzma();
}