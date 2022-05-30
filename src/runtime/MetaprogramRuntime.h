#pragma once
#if defined(META_PASS)
#define attribute(...) __attribute__((annotate("__metaprogram " #__VA_ARGS__)))
#define metaprogram_visible attribute("metaprogram_visible")
#else
#define attribute(...)
#define metaprogram_visible
#endif
