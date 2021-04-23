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

#if defined(_MSC_VER)
#define COMPILER_MSVC

// unknown attribute
#pragma warning(disable: 5030)
// gogo skips variable initialization
#pragma warning(disable: 4533)

#define BreakDebug() __debugbreak()
#define WriteFence() (_WriteBarrier(), _mm_sfence())
#define ReadFence() (_ReadBarrier(), _mm_lfence())

#define force_inline __forceinline

#elif defined(__clang__)
#define COMPILER_CLANG

#pragma clang diagnostic ignored "-Wparentheses-equality"

#define BreakDebug() __builtin_debugtrap()
// TODO: Fences
#define WriteFence() do {} while(false) //((__asm__("" ::: "memory")), _mm_sfence())
#define ReadFence() do {} while(false) //((__asm__("" ::: "memory")), _mm_lfence())

#define forceinline __attribute__((always_inline))

#else
#error Unsupported compiler
#endif

#define ArrayCount(arr) ((uint)(sizeof(arr) / sizeof(arr[0])))
#define TypeDecl(type, member) (((type*)0)->member)
#define OffsetOf(type, member) ((uptr)(&(((type*)0)->member)))
#define InvalidDefault() default: { BreakDebug(); } break
#define unreachable() BreakDebug()

// NOTE: Jonathan Blow defer implementation. Reference: https://pastebin.com/SX3mSC9n
#define __Concat(x,y) x##y
#define Concat(x,y) __Concat(x,y)

template<typename T>
struct ExitScope
{
    T lambda;
    ExitScope(T lambda) : lambda(lambda) {}
    ~ExitScope() { lambda(); }
    ExitScope(const ExitScope&);
  private:
    ExitScope& operator =(const ExitScope&);
};

struct ExitScopeHelp
{
    template<typename T>
    ExitScope<T> operator+(T t) { return t; }
};

#define defer const auto& Concat(defer__, __LINE__) = ExitScopeHelp() + [&]()

// Making tuples be a thing using suuuper crazy template nonsence
template <typename T1, typename T2 = void, typename T3 = void, typename T4 = void, typename T5 = void>
struct Tuple { T1 item1; T2 item2; T2 item3; T4 item4; T5 item5; };
template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline Tuple<T1, T2, T3, T4, T5> MakeTuple(T1 item1, T2 item2, T3 item3, T4 item4, T5 item5) { return Tuple<T1, T2, T3, T4, T5> { item1, item2, item3, item4, item5 }; }

template <typename T1, typename T2, typename T3, typename T4>
struct Tuple<T1, T2, T3, T4, void> { T1 item1; T2 item2; T3 item3; T4 item4; };
template <typename T1, typename T2, typename T3, typename T4>
inline Tuple<T1, T2, T3, T4> MakeTuple(T1 item1, T2 item2, T3 item3, T4 item4) { return Tuple<T1, T2, T3, T4> { item1, item2, item3, item4 }; }

template <typename T1, typename T2, typename T3>
struct Tuple<T1, T2, T3, void, void> { T1 item1; T2 item2; T3 item3; };
template <typename T1, typename T2, typename T3>
inline Tuple<T1, T2, T3> MakeTuple(T1 item1, T2 item2, T3 item3) { return Tuple<T1, T2, T3> { item1, item2, item3 }; }

template <typename T1, typename T2>
struct Tuple<T1, T2, void, void, void> { T1 item1; T2 item2; };
template <typename T1, typename T2>
inline Tuple<T1, T2> MakeTuple(T1 item1, T2 item2) { return Tuple<T1, T2> { item1, item2 }; }

typedef uint8_t byte;
typedef unsigned char uchar;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef char16_t char16;
typedef char32_t char32;

typedef uintptr_t uptr;

typedef u32 b32;
typedef byte b8;

typedef float f32;
typedef double f64;

typedef u32 u32x;
typedef i32 i32x;
typedef u32 uint;

typedef u32 usize;

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
