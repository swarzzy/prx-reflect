#pragma once
#include "CommonDefines.h"

typedef enum
{
    BuiltInType_Unknown = 0,
    BuiltInType_SignedShort,
    BuiltInType_UnsignedShort,
    BuiltInType_SignedInt,
    BuiltInType_UnsignedInt,
    BuiltInType_SignedLong,
    BuiltInType_UnsignedLong,
    BuiltInType_SignedLongLong,
    BuiltInType_UnsignedLongLong,
    BuiltInType_SignedChar,
    BuiltInType_UnsignedChar,
    BuiltInType_Bool,
    BuiltInType_Char8,
    BuiltInType_Char16,
    BuiltInType_Char32,
    BuiltInType_Wchar,
    BuiltInType_Float,
    BuiltInType_Double,
    BuiltInType_LongDouble,
} BuiltInType;

typedef enum
{
    AstNodeType_Root = 0,
    AstNodeType_Enum,
    AstNodeType_EnumConstant,
    AstNodeType_Struct,
    AstNodeType_Field,
} AstNodeType;

typedef enum
{
    TypeKind_Unknown = 0,
    TypeKind_BuiltIn,
    TypeKind_Pointer,
    TypeKind_Array,
    TypeKind_Struct,
    TypeKind_Enum
} TypeKind;

typedef struct _AstNode
{
    AstNodeType type;
    void* data;
    u32 childrenCount;
    struct _AstNode* children;
} AstNode;

typedef struct
{
    const char* name;
    BuiltInType underlyingType;
} EnumData;

typedef struct
{
    const char* name;
    signed long long signedValue;
    unsigned long long unsignedValue;
} EnumConstantData;

typedef struct
{
    const char* name;
    u32 size;
    u32 align;
} StructData;

typedef struct _TypeInfo
{
    TypeKind kind;
    BuiltInType builtInType;
    u32 arrayCount;
    // TODO: Further resolve
    const char* structName;
    const char* enumName;
    //BuiltInType enumUnderlyingType;
    struct _TypeInfo* underlyingType;
} TypeInfo;

typedef struct
{
    const char* name;
    u32 offset;
    TypeInfo* typeInfo;
} FieldData;

typedef void(MetaprogramProc)(AstNode* astRoot);

static const char* BuiltInType_Strings[] =
{
    "Unknown",
    "SignedShort",
    "UnsignedShort",
    "SignedInt",
    "UnsignedInt",
    "SignedLong",
    "UnsignedLong",
    "SignedLongLong",
    "UnsignedLongLong",
    "SignedChar",
    "UnsignedChar",
    "Bool",
    "Char8",
    "Char16",
    "Char32",
    "Wchar",
    "Float",
    "Double",
    "LongDouble",
};

static const char* AstNodeType_Strings[] =
{
    "Root",
    "Enum",
    "EnumConstant",
    "Struct",
    "Field"
};

static  const char* TypeKind_Strings[] =
{
    "Unknown",
    "BuiltIn",
    "Pointer",
    "Array",
    "Struct",
    "Enum"
};