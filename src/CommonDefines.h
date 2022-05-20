#include <stdint.h>

#if defined(_MSC_VER)
#define COMPILER_MSVC

#define BreakDebug() __debugbreak()
#define force_inline __forceinline

#elif defined(__clang__)
#define COMPILER_CLANG

#define BreakDebug() __builtin_debugtrap()
#define force_inline __attribute__((always_inline))

#else
#error Unsupported compiler
#endif

#define ArrayCount(arr) ((uint)(sizeof(arr) / sizeof(arr[0])))
#define TypeDecl(type, member) (((type*)0)->member)
#define OffsetOf(type, member) ((uptr)(&(((type*)0)->member)))
#define InvalidDefault() default: { BreakDebug(); } break
#define Unreachable() BreakDebug()

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

typedef float f32;
typedef double f64;

typedef u32 u32x;
typedef i32 i32x;
typedef u32 uint;

typedef u32 usize;