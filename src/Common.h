#pragma once
#include <stdint.h>
#include <stdio.h>
#include <float.h>
// NOTE: offsetof
#include <stddef.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <intrin.h>

namespace Uptr
{
    constexpr uptr Max = UINTPTR_MAX;
}

namespace Usize
{
    constexpr usize Max = 0xffffffff;
}

namespace F32
{
    constexpr f32 Pi = 3.14159265358979323846f;
    constexpr f32 Eps = 0.000001f;
    constexpr f32 Nan = NAN;
    constexpr f32 Infinity = INFINITY;
    constexpr f32 Max = FLT_MAX;
    constexpr f32 Min = FLT_MIN;
};

namespace I32
{
    constexpr i32 Max = INT32_MAX;
    constexpr i32 Min = INT32_MIN;
}

namespace U32
{
    constexpr u32 Max = 0xffffffff;
}

namespace U16
{
    constexpr u16 Max = 0xffff;
}

namespace U64
{
    constexpr u64 Max = UINT64_MAX;
}

#define Kilobytes(kb) ((kb) * (u32)1024)
#define Megabytes(mb) ((mb) * (u32)1024 * (u32)1024)

// NOTE: Allocator API
typedef void*(AllocateFn)(uptr size, b32 clear, uptr alignment, void* allocatorData);
typedef void(DeallocateFn)(void* ptr, void* allocatorData);

struct Allocator
{
    AllocateFn* allocate;
    DeallocateFn* deallocate;
    void* data;

    template <typename T> inline T* Alloc(bool clear = 1) { return (T*)allocate(sizeof(T), clear, 0, data); }
    template <typename T> inline T* Alloc(u32 count, bool clear = 1) { return (T*)allocate(sizeof(T) * count, clear, 0, data); }
    inline void* Alloc(uptr size, b32 clear, uptr alignment = 0) { return allocate(size, clear, alignment, data); }
    inline void Dealloc(void* ptr) { deallocate(ptr, data); }
};

inline Allocator MakeAllocator(AllocateFn* allocate, DeallocateFn* deallocate, void* data)
{
    Allocator allocator;
    allocator.allocate = allocate;
    allocator.deallocate = deallocate;
    allocator.data = data;
    return allocator;
}

// NOTE: Logger API
typedef void(LoggerFn)(void* loggerData, const char* fmt, va_list* args);
typedef void(AssertHandlerFn)(void* userData, const char* file, const char* func, u32 line, const char* exprString, const char* fmt, va_list* args);

extern LoggerFn* GlobalLogger;
extern void* GlobalLoggerData;

extern AssertHandlerFn* GlobalAssertHandler;
extern void* GlobalAssertHandlerData;

#define LogPrint(fmt, ...) _GlobalLoggerWithArgs(GlobalLoggerData, fmt, ##__VA_ARGS__)
#define Assert(expr, ...) do { if (!(expr)) {_GlobalAssertHandler(GlobalAssertHandlerData, __FILE__, __func__, __LINE__, #expr, ##__VA_ARGS__);}} while(false)
// NOTE: Defined always
#define Panic(expr, ...) do { if (!(expr)) {_GlobalAssertHandler(GlobalAssertHandlerData, __FILE__, __func__, __LINE__, #expr, ##__VA_ARGS__);}} while(false)

inline void _GlobalLoggerWithArgs(void* data, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    GlobalLogger(data, fmt,  &args);
    va_end(args);
}

inline void _GlobalAssertHandler(void* userData, const char* file, const char* func, u32 line, const char* exprString)
{
    GlobalAssertHandler(userData, file, func, line, exprString, nullptr, nullptr);
}

inline void _GlobalAssertHandler(void* userData, const char* file, const char* func, u32 line, const char* exprString, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    GlobalAssertHandler(userData, file, func, line, exprString, fmt, &args);
    va_end(args);
}
