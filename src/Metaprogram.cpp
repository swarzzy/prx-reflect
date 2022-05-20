#include "AST.h"

#include <stdio.h>
#include <stdarg.h>

void SerializeType(TypeInfo* info, void(*outputProc)(const char* fmt, ...))
{
    if (!info)
    {
        return;
    }

    switch (info->kind)
    {
        case TypeKind_Unknown:
        {
            outputProc(" <unknown>");
        } break;

        case TypeKind_BuiltIn:
        {
            outputProc(" %s", BuiltInType_Strings[info->builtInType]);
        } break;

        case TypeKind_Pointer:
        {
            outputProc(" *");
            SerializeType(info->underlyingType, outputProc);
        } break;

        case TypeKind_Array:
        {
            outputProc(" [%lu]", info->arrayCount);
            SerializeType(info->underlyingType, outputProc);
        } break;

        case TypeKind_Struct:
        {
            outputProc(" %s", info->structName);
        } break;

        case TypeKind_Enum:
        {
            outputProc(" %s", info->enumName);
        } break;
    }
}

void SerializeAst(AstNode* node, u32 level, void(*outputProc)(const char* fmt, ...))
{
    for (u32 i = 0; i < level; i++)
    {
        outputProc("%c", '-');
    }

    if (level == 0)
    {
        outputProc("%s", AstNodeType_Strings[node->type]);
    }
    else
    {
        outputProc(" %s", AstNodeType_Strings[node->type]);
    }

    switch (node->type)
    {
        case AstNodeType_Enum:
        {
            auto data = (EnumData*)node->data;
            outputProc(" %s %s", data->name, BuiltInType_Strings[data->underlyingType]);
        } break;

        case AstNodeType_EnumConstant:
        {
            auto data = (EnumConstantData*)node->data;
            outputProc(" %s sValue: %ld uValue: %lu", data->name, data->signedValue, data->unsignedValue);
        } break;

        case AstNodeType_Struct:
        {
            auto data = (StructData*)node->data;
            outputProc(" %s size: %lu align: %lu", data->name, data->size, data->align);
        } break;

        case AstNodeType_Field:
        {
            auto data = (FieldData*)node->data;
            outputProc(" %s offset: %lu, type:", data->name, data->offset);
            SerializeType(data->typeInfo, outputProc);
        } break;
    }

    outputProc("\n");

    for (u32 i = 0; i < node->childrenCount; i++)
    {
        SerializeAst(node->children + i, level + 1, outputProc);
    }
}

void SerializeAst(AstNode* node, void(*outputProc)(const char* fmt, ...))
{
    SerializeAst(node, 0, outputProc);
}

void AstOutputProc(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}


extern "C" void Metaprogram(AstNode* astRoot)
{
    SerializeAst(astRoot, AstOutputProc);
}
