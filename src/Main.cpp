#include <clang-c/Index.h>
#include <clang-c/CXString.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <unordered_set>

#include "CommonDefines.h"
#include "Common.h"
#include "AST.h"

#define OS_WINDOWS

void Logger(void* data, const char* fmt, va_list* args)
{
    vprintf(fmt, *args);
}

void AssertHandler(void* data, const char* file, const char* func, u32 line, const char* assertStr, const char* fmt, va_list* args)
{
    LogPrint("[Assertion failed] Expression (%s) result is false\nFile: %s, function: %s, line: %d.\n", assertStr, file, func, (int)line);
    if (args)
    {
        GlobalLogger(GlobalLoggerData, fmt, args);
    }
    BreakDebug();
}

LoggerFn* GlobalLogger = Logger;
void* GlobalLoggerData = nullptr;

AssertHandlerFn* GlobalAssertHandler = AssertHandler;
void* GlobalAssertHandlerData = nullptr;

struct VisitorData
{
    std::vector<AstNode*> stack;
    std::unordered_set<std::string> processedTypes;
    AstNode rootNode;
};

struct ASTAttribEntry
{
    CXCursor parent;
    CXCursor attrib;
};

struct ASTAttribCollection
{
    std::vector<ASTAttribEntry> enums;
    std::vector<ASTAttribEntry> structs;
    std::vector<ASTAttribEntry> fields;
};

BuiltInType ClangTypeToBuiltIn(CXTypeKind clangType)
{
    switch (clangType)
    {
    case CXType_Short: return BuiltInType_SignedShort;
    case CXType_UShort: return BuiltInType_UnsignedShort;
    case CXType_Int: return BuiltInType_SignedInt;
    case CXType_UInt: return BuiltInType_UnsignedInt;
    case CXType_Long: return BuiltInType_SignedLong;
    case CXType_ULong: return BuiltInType_UnsignedLong;
    case CXType_LongLong: return BuiltInType_SignedLongLong;
    case CXType_ULongLong: return BuiltInType_UnsignedLongLong;
    // nocheckin Check these
    case CXType_SChar: return BuiltInType_SignedChar;
    case CXType_Char_S: return BuiltInType_SignedChar;
    case CXType_UChar: return BuiltInType_UnsignedChar;
    case CXType_Char_U: return BuiltInType_UnsignedChar;
    //case CXType_SChar: return BuiltInType_Char8;

    case CXType_Bool: return BuiltInType_Bool;
    case CXType_Char16: return BuiltInType_Char16;
    case CXType_Char32: return BuiltInType_Char32;
    case CXType_WChar: return BuiltInType_Wchar;
    case CXType_Float: return BuiltInType_Float;
    case CXType_Double: return BuiltInType_Double;
    case CXType_LongDouble: return BuiltInType_LongDouble;
    default: return BuiltInType_Unknown;
    }
}

AstNode* PushNewChild(VisitorData* data)
{
    AstNode *children = data->stack.back()->children;
    auto childrenVector = (std::vector<AstNode> *)children;

    childrenVector->push_back(AstNode());

    AstNode* node = &childrenVector->back();

    data->stack.push_back(node);
    return node;
}

void PopNode(VisitorData* data)
{
    data->stack.pop_back();
}

CXChildVisitResult EnumVisitor(CXCursor cursor, CXCursor parent, CXClientData _data)
{
    auto data = (VisitorData*)_data;
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_EnumConstantDecl)
    {
        AstNode* node = PushNewChild(data);
        node->type = AstNodeType_EnumConstant;
        auto nodeData = (EnumConstantData*)malloc(sizeof(EnumConstantData));
        node->data = nodeData;

        CXString name = clang_getCursorSpelling(cursor);
        const char* nameStr = clang_getCString(name);

        unsigned long long uValue = clang_getEnumConstantDeclUnsignedValue(cursor);
        long long sValue = clang_getEnumConstantDeclValue(cursor);

        nodeData->name = nameStr;
        nodeData->signedValue = sValue;
        nodeData->unsignedValue = uValue;

        PopNode(data);
    }
    else if (kind != CXCursor_AnnotateAttr)
    {
        LogPrint("Unexpected syntax in enum\n.");
    }

    return CXChildVisit_Continue;
}

TypeInfo* ResolveFieldType(CXType type)
{
    BuiltInType builtInType = ClangTypeToBuiltIn(type.kind);

    if (builtInType != BuiltInType_Unknown)
    {
        auto typeInfo = (TypeInfo*)malloc(sizeof(TypeInfo));
        memset(typeInfo, 0, sizeof(TypeInfo));

        typeInfo->kind = TypeKind_BuiltIn;
        typeInfo->builtInType = builtInType;
        return typeInfo;
    }

    if (type.kind == CXType_Pointer)
    {
        auto typeInfo = (TypeInfo *)malloc(sizeof(TypeInfo));
        memset(typeInfo, 0, sizeof(TypeInfo));

        typeInfo->kind = TypeKind_Pointer;

        CXType pointeeType = clang_getPointeeType(type);

        typeInfo->underlyingType = ResolveFieldType(pointeeType);
        return typeInfo;
    }

    if (type.kind == CXType_ConstantArray)
    {
        auto typeInfo = (TypeInfo *)malloc(sizeof(TypeInfo));
        memset(typeInfo, 0, sizeof(TypeInfo));

        long long arraySize = clang_getArraySize(type);
        Assert(arraySize >= 0);

        CXType elementType = clang_getArrayElementType(type);

        typeInfo->kind = TypeKind_Array;
        typeInfo->arrayCount = (u32)arraySize;
        typeInfo->underlyingType = ResolveFieldType(elementType);
        return typeInfo;
    }

    if (type.kind == CXType_Typedef)
    {
        CXType actualType = clang_getCanonicalType(type);
        return ResolveFieldType(actualType);
    }

    if (type.kind == CXType_Record)
    {
        auto typeInfo = (TypeInfo *)malloc(sizeof(TypeInfo));
        memset(typeInfo, 0, sizeof(TypeInfo));

        CXString typeSpelling = clang_getTypeSpelling(type);
        const char* typeSpellingStr = clang_getCString(typeSpelling);

        typeInfo->kind = TypeKind_Struct;
        typeInfo->structName = typeSpellingStr;
        return typeInfo;
    }

    if (type.kind == CXType_Elaborated)
    {
        CXType actualType = clang_Type_getNamedType(type);
        return ResolveFieldType(actualType);
    }

    if (type.kind == CXType_Enum)
    {
        auto typeInfo = (TypeInfo *)malloc(sizeof(TypeInfo));
        memset(typeInfo, 0, sizeof(TypeInfo));

        CXString typeSpelling = clang_getTypeSpelling(type);
        const char *typeSpellingStr = clang_getCString(typeSpelling);

        typeInfo->kind = TypeKind_Enum;
        typeInfo->enumName = typeSpellingStr;
        return typeInfo;
    }

    return nullptr;
}

CXChildVisitResult StructVisitor(CXCursor cursor, CXCursor parent, CXClientData _data)
{
    auto data = (VisitorData*)_data;
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_FieldDecl)
    {
        AstNode* node = PushNewChild(data);
        node->type = AstNodeType_Field;
        auto nodeData = (FieldData *)malloc(sizeof(FieldData));
        node->data = nodeData;

        CXString name = clang_getCursorSpelling(cursor);
        const char* nameStr = clang_getCString(name);

        long long offset = clang_Cursor_getOffsetOfField(cursor);
        // TODO: Proper error handling.
        Assert(offset >= 0);

        CXType type = clang_getCursorType(cursor);
        TypeInfo* typeInfo = ResolveFieldType(type);

        nodeData->name = nameStr;
        nodeData->offset = (u32)offset;
        nodeData->typeInfo = typeInfo;

        PopNode(data);
    }
    else if (kind != CXCursor_AnnotateAttr &&
             kind != CXCursor_StructDecl &&
             kind != CXCursor_EnumDecl)
    {
        LogPrint("Unexpected syntax in struct\n.");
    }


    return CXChildVisit_Continue;
}

bool CheckMetaprogramVisibility(CXCursor attribCursor)
{
    bool visible = false;
    static const char *metaprogramKeyword = "__metaprogram";
    static const int metaprogramKeywordSize = sizeof("__metaprogram");

    CXString attribSpelling = clang_getCursorSpelling(attribCursor);
    const char *attribSpellingStr = clang_getCString(attribSpelling);

    const char *keyword = strstr(attribSpellingStr, metaprogramKeyword);
    if (keyword)
    {
        keyword += metaprogramKeywordSize;
        if (strcmp(keyword, "\"metaprogram_visible\"") == 0)
        {
            visible = true;
        }
    }

    clang_disposeString(attribSpelling);

    return visible;
}

bool CheckTypeIsAlreadyProcessed(VisitorData* data, const char* name)
{
    auto it = data->processedTypes.find(std::string(name));
    return it != data->processedTypes.end();
}

void MarkTypeAsProcessed(VisitorData* data, const char* name)
{
    data->processedTypes.insert(std::string(name));
}

void MakeChildrenVector(AstNode* node)
{
    // Put vector in children array pointer. We will collect children here
    node->children = (AstNode *)(new std::vector<AstNode>());
}

void ConvertChildrenVectorToArray(AstNode* node)
{
    // Convert children vector to plain array
    auto childrenVector = (std::vector<AstNode> *)node->children;
    u32 childrenCount = (u32)childrenVector->size();
    u32 arraySize = sizeof(AstNode) * childrenCount;

    node->children = (AstNode *)malloc(arraySize);
    node->childrenCount = (u32)childrenCount;
    memcpy(node->children, childrenVector->data(), arraySize);
    delete childrenVector;
}

CXChildVisitResult AttributesVisitor(CXCursor cursor, CXCursor parent, CXClientData _data)
{
    auto data = (VisitorData*)_data;

    CXCursorKind kind = clang_getCursorKind(cursor);
    CXCursorKind parentKind = clang_getCursorKind(parent);

    if (kind == CXCursor_AnnotateAttr && CheckMetaprogramVisibility(cursor))
    {
        ASTAttribEntry entry;
        entry.parent = parent;
        entry.attrib = cursor;

        switch (parentKind)
        {
            case CXCursor_EnumDecl:
            {
                Assert(data->stack.back() = &(data->rootNode));

                if (!clang_isCursorDefinition(parent))
                {
                    break;
                }

                CXString spelling = clang_getCursorSpelling(parent);
                const char *spellingStr = clang_getCString(spelling);

                if (CheckTypeIsAlreadyProcessed(data, spellingStr))
                {
                    clang_disposeString(spelling);
                    break;
                }

                MarkTypeAsProcessed(data, spellingStr);

                auto node = PushNewChild(data);
                node->type = AstNodeType_Enum;

                MakeChildrenVector(node);

                CXType clangUnderlyingType = clang_getEnumDeclIntegerType(parent);
                BuiltInType type = ClangTypeToBuiltIn(clangUnderlyingType.kind);

                auto nodeData = (EnumData*)malloc(sizeof(EnumData));
                nodeData->name = spellingStr;
                nodeData->underlyingType = type;
                node->data = nodeData;

                clang_visitChildren(parent, EnumVisitor, data);

                ConvertChildrenVectorToArray(node);

                PopNode(data);
            } break;

            case CXCursor_StructDecl:
            {
                Assert(data->stack.back() = &(data->rootNode));

                if (!clang_isCursorDefinition(parent))
                {
                    break;
                }

                CXType type = clang_getCursorType(parent);
                CXString spelling = clang_getTypeSpelling(type);
                const char *spellingStr = clang_getCString(spelling);

                if (CheckTypeIsAlreadyProcessed(data, spellingStr))
                {
                    clang_disposeString(spelling);
                    break;
                }

                MarkTypeAsProcessed(data, spellingStr);

                auto node = PushNewChild(data);
                node->type = AstNodeType_Struct;

                MakeChildrenVector(node);

                auto nodeData = (StructData *)malloc(sizeof(StructData));
                nodeData->name = spellingStr;
                nodeData->size = (u32)clang_Type_getSizeOf(type);
                nodeData->align = (u32)clang_Type_getAlignOf(type);

                node->data = nodeData;

                clang_visitChildren(parent, StructVisitor, data);

                ConvertChildrenVectorToArray(node);

                PopNode(data);
            } break;

            default: { printf("Error: Found annotation at invalid location.\n"); } break;
        }
    }

    return CXChildVisit_Recurse;
}

#if defined(OS_WINDOWS)

#include "Windows.h"

bool CallMetaprogram(AstNode* root, const char* moduleName, const char* procName)
{
    HMODULE modulePtr = LoadLibraryA(moduleName);
    if (modulePtr == nullptr)
    {
        LogPrint("Failed to load metaprogram module %s\n", moduleName);
        return false;
    }

    auto userProc = (MetaprogramProc *)GetProcAddress(modulePtr, procName);
    if (userProc == nullptr)
    {
        LogPrint("Unable to find metaprogram procedure %s in module %s\n", procName, moduleName);
        return false;
    }

    LogPrint("Running metaprogram procedure %s from module %s ...\n", procName, moduleName);

    userProc(root);

    return true;
}
#endif

int main(int argc, char** argv)
{
    int compilerArgsCount = argc;
    char** compilerArgsPtr = nullptr;

    char* moduleName = nullptr;
    char* procName = nullptr;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-m") == 0 && i < argc - 1)
        {
            moduleName = argv[i + 1];
            i++;
        }

        if (strcmp(argv[i], "-p") == 0 && i < argc - 1)
        {
            procName = argv[i + 1];
            i++;
        }

        if (strcmp(argv[i], "-a") == 0)
        {
            compilerArgsCount = argc - (i + 1);
            compilerArgsPtr = argv + (i + 1);
            break;
        }
    }

    bool failed = false;

    if (moduleName == nullptr)
    {
        LogPrint("Metaprogram module is not specified! Use -m <module> to specify metaprogram module.\n");
        failed = true;
    }

    if (procName == nullptr)
    {
        LogPrint("Metaprogram procedure name is not specified! Use -p <name> to specify procedure name.\n");
        failed = true;
    }

    if (compilerArgsPtr == nullptr)
    {
        failed = true;
        LogPrint("Compiler flags are not specified! Use -a to specify compiler flags.\n");
    }

    if (failed)
    {
        return -1;
    }

    CXIndex index = clang_createIndex(0, 1);
    Assert(index);
    CXTranslationUnit unit = nullptr;
    CXErrorCode resultTr = clang_parseTranslationUnit2FullArgv(index, nullptr, argv, argc, 0, 0, CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_DetailedPreprocessingRecord, &unit);

    CXCursor rootCursor = clang_getTranslationUnitCursor(unit);

    VisitorData data;
    data.rootNode.type = AstNodeType_Root;
    MakeChildrenVector(&data.rootNode);

    data.stack.push_back(&data.rootNode);

    clang_visitChildren(rootCursor, AttributesVisitor, &data);

    ConvertChildrenVectorToArray(&data.rootNode);

    CallMetaprogram(&data.rootNode, moduleName, procName);

    //VisitAst(&data.rootNode);

    return 0;
}

#if false

#include <string>
#include <iostream>

std::string getCursorKindName( CXCursorKind cursorKind )
{
  CXString kindName  = clang_getCursorKindSpelling( cursorKind );
  std::string result = clang_getCString( kindName );

  clang_disposeString( kindName );
  return result;
}

std::string getCursorSpelling( CXCursor cursor )
{
  CXString cursorSpelling = clang_getCursorSpelling( cursor );
  std::string result      = clang_getCString( cursorSpelling );

  clang_disposeString( cursorSpelling );
  return result;
}

CXChildVisitResult DumpAst(CXCursor cursor, CXCursor parent, CXClientData clientData)
{
    CXSourceLocation location = clang_getCursorLocation(cursor);
    if (clang_Location_isFromMainFile(location) == 0)
        return CXChildVisit_Continue;

    CXCursorKind cursorKind = clang_getCursorKind(cursor);

    unsigned int curLevel = *(reinterpret_cast<unsigned int *>(clientData));
    unsigned int nextLevel = curLevel + 1;

    std::cout << std::string(curLevel, '-') << " " << getCursorKindName(cursorKind) << " (" << getCursorSpelling(cursor) << ")\n";

    clang_visitChildren(cursor,
                        DumpAst,
                        &nextLevel);

    return CXChildVisit_Continue;
}
#endif