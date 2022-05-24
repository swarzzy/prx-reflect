#define _CRT_SECURE_NO_WARNINGS
#include "AST.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

const char* GetTypeName(AstNode* node)
{
    switch (node->type)
    {
        case AstNodeType_Enum: return ((EnumData*)node->data)->name;
        case AstNodeType_Struct: return ((StructData*)node->data)->name;
        InvalidDefault();
    }

    return nullptr;
}

void SerializeType(TypeInfo* info, void(*outputProc)(void* data, const char* fmt, ...), void* userData)
{
    if (!info)
    {
        return;
    }

    switch (info->kind)
    {
        case TypeKind_Unknown:
        {
            outputProc(userData, " <unknown>");
        } break;

        case TypeKind_BuiltIn:
        {
            outputProc(userData, " %s", BuiltInType_Strings[info->builtInType]);
        } break;

        case TypeKind_Pointer:
        {
            outputProc(userData, " *");
            SerializeType(info->underlyingType, outputProc, userData);
        } break;

        case TypeKind_FunctionProto:
        {
            outputProc(userData, " FunctionProto");
        } break;

        case TypeKind_Array:
        {
            if (info->arrayHasSize)
            {
                outputProc(userData, " [%lu]", info->arrayCount);
            }
            else
            {
                outputProc(userData, " []");
            }

            SerializeType(info->underlyingType, outputProc, userData);
        } break;

        case TypeKind_Struct:
        {
            outputProc(userData, " %s", GetTypeName(info->resolvedTypeNode));
        } break;

        case TypeKind_Enum:
        {
            outputProc(userData, " %s", GetTypeName(info->resolvedTypeNode));
        } break;

        case TypeKind_Unresolved:
        {
            outputProc(userData, " %s", info->unresolvedTypeName);
        } break;
    }
}

void SerializeAst(AstNode* node, u32 level, void(*outputProc)(void* data, const char* fmt, ...), void* userData)
{
    if (node->type != AstNodeType_Root)
    {
        outputProc(userData, "\n");
    }

    for (u32 i = 0; i < level; i++)
    {
        outputProc(userData, "%c", '-');
    }

    if (level == 0)
    {
        outputProc(userData, "%s", AstNodeType_Strings[node->type]);
    }
    else
    {
        outputProc(userData, " %s", AstNodeType_Strings[node->type]);
    }

    switch (node->type)
    {
        case AstNodeType_Enum:
        {
            auto data = (EnumData*)node->data;
            outputProc(userData, " %s %s anonymous: %s", data->name, BuiltInType_Strings[data->underlyingType], data->anonymous ? "true" : "false");
        } break;

        case AstNodeType_EnumConstant:
        {
            auto data = (EnumConstantData*)node->data;
            outputProc(userData, " %s sValue: %ld uValue: %lu", data->name, data->signedValue, data->unsignedValue);
        } break;

        case AstNodeType_Struct:
        {
            auto data = (StructData*)node->data;
            outputProc(userData, " %s size: %lu align: %lu anonymous: %s", data->name, data->size, data->align, data->anonymous ? "true" : "false");
        } break;

        case AstNodeType_Field:
        {
            auto data = (FieldData*)node->data;
            outputProc(userData, " %s offset: %lu, type:", data->name, data->offset);
            SerializeType(data->typeInfo, outputProc, userData);
        } break;
    }

    for (u32 i = 0; i < node->childrenCount; i++)
    {
        SerializeAst(node->children[i], level + 1, outputProc, userData);
    }
}

void SerializeAst(AstNode* node, void(*outputProc)(void* data, const char* fmt, ...), void* userData)
{
    SerializeAst(node, 0, outputProc, userData);
}

void AstOutputProc(void*, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void AstOutputToFileProc(void* file, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf((FILE*)file, fmt, args);
    va_end(args);
}

bool Test(AstNode* astRoot)
{
    int fd[2];
    _pipe(fd, 200 * 1024, O_BINARY);
    FILE* writeFile = _fdopen(fd[1], "wb");
    FILE* readFile = _fdopen(fd[0], "r");
    
    SerializeAst(astRoot, AstOutputToFileProc, writeFile);

    fclose(writeFile);

    FILE* referenceFile = fopen("tests/RefData.txt", "r");
    assert(referenceFile);

    char testBuffer[1024];
    char refBuffer[1024];

    int lineCount = 0;

    int testFileCharCount = 0;
    int refFileCharCount = 0;

    while (!feof(readFile) && !feof(referenceFile))
    {        
        if (fgets(testBuffer, 1024, readFile) == NULL) break;
        if (fgets(refBuffer, 1024, referenceFile) == NULL) break;

        lineCount++;
        testFileCharCount += (int)strlen(testBuffer);
        refFileCharCount += (int)strlen(refBuffer);

        if (strcmp(testBuffer, refBuffer) != 0)
        {
            printf("Error at line %d\n", lineCount);
            printf("Expected: \"%s\"\n", refBuffer);
            printf("Got:      \"%s\"\n", testBuffer);
            return false;
        }
    }

    if (testFileCharCount != refFileCharCount)
    {
        printf("Character count mismatch. Test data:  %d. Reference data: %d.\n", testFileCharCount, refFileCharCount);
        return false;
    }

    return true;
}

extern "C" void Metaprogram(AstNode* astRoot)
{
    SerializeAst(astRoot, AstOutputProc, nullptr);

    printf("\nRunning tests...\n");
    if (Test(astRoot))
    {
        printf("All tests passed!\n");
    }
    else
    {
        printf("Test failed!\n");
    }
}
