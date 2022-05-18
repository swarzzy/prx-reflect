#include <clang-c/Index.h>
#include <clang-c/CXString.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <unordered_set>
#include <string>
#include <iostream>

#include "Common.h"
//#include "Array.h"

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

enum struct PrimitiveType
{
    Unknown = 0,
    SignedShort,
    UnsignedShort,
    SignedInt,
    UnsignedInt,
    SignedLong,
    UnsignedLong,
    SignedLongLong,
    UnsignedLongLong,
    SignedChar,
    UnsignedChar,
    Bool,
    Char8,
    Char16,
    Char32,
    Wchar,
    Float,
    Double,
    LongDouble,
    Pointer,
};

const char* ToString(PrimitiveType t)
{
    static const char* names[] = {
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
        "Pointer"
    };
    Assert((int)t < ArrayCount(names));
    return names[(int)t];
}

PrimitiveType ClangTypeToPrimitive(CXTypeKind clangType)
{
    switch (clangType)
    {
    case CXType_Short: return PrimitiveType::SignedShort;
    case CXType_UShort: return PrimitiveType::UnsignedShort;
    case CXType_Int: return PrimitiveType::SignedInt;
    case CXType_UInt: return PrimitiveType::UnsignedInt;
    case CXType_Long: return PrimitiveType::SignedLong;
    case CXType_ULong: return PrimitiveType::UnsignedLong;
    case CXType_LongLong: return PrimitiveType::SignedLongLong;
    case CXType_ULongLong: return PrimitiveType::UnsignedLongLong;
    // nocheckin Check these
    case CXType_SChar: return PrimitiveType::SignedChar;
    case CXType_Char_S: return PrimitiveType::SignedChar;
    case CXType_UChar: return PrimitiveType::UnsignedChar;
    case CXType_Char_U: return PrimitiveType::UnsignedChar;
    //case CXType_SChar: return PrimitiveType::Char8;

    case CXType_Bool: return PrimitiveType::Bool;
    case CXType_Char16: return PrimitiveType::Char16;
    case CXType_Char32: return PrimitiveType::Char32;
    case CXType_WChar: return PrimitiveType::Wchar;
    case CXType_Float: return PrimitiveType::Float;
    case CXType_Double: return PrimitiveType::Double;
    case CXType_LongDouble: return PrimitiveType::LongDouble;
    case CXType_Pointer: return PrimitiveType::Pointer;
    default: return PrimitiveType::Unknown;
    }
}


enum class AstNodeType
{
    Root,
    Enum,
    EnumConstant,
    Struct,
    Field,
};

const char* ToString(AstNodeType type)
{
    switch (type)
    {
        case AstNodeType::Root: return "Root";
        case AstNodeType::Enum: return "Enum";
        case AstNodeType::EnumConstant: return "EnumConstant";
        case AstNodeType::Struct: return "Struct";
        case AstNodeType::Field: return "Field";
        InvalidDefault();
    }
    return nullptr;
}

struct AstNode
{
    AstNodeType type;
    void* data;
    std::vector<AstNode> children;
};

struct EnumData
{
    const char* name;
    PrimitiveType underlyingType;
};

struct EnumConstantData
{
    const char* name;
    signed long long signedValue;
    unsigned long long unsignedValue;
};

struct StructData
{
    const char* name;
    u32 size;
    u32 align;
};

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

AstNode* PushNewChild(VisitorData* data)
{
    auto *current = data->stack.back();
    current->children.push_back(AstNode());

    AstNode* node = &current->children.back();

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
        AstNode node {};
        node.type = AstNodeType::EnumConstant;
        auto nodeData = (EnumConstantData*)malloc(sizeof(EnumConstantData));
        node.data = nodeData;

        CXString name = clang_getCursorSpelling(cursor);
        const char* nameStr = clang_getCString(name);

        unsigned long long uValue = clang_getEnumConstantDeclUnsignedValue(cursor);
        long long sValue = clang_getEnumConstantDeclValue(cursor);

        nodeData->name = nameStr;
        nodeData->signedValue = sValue;
        nodeData->unsignedValue = uValue;

        data->stack.back()->children.push_back(node);
    }
    else if (kind != CXCursor_AnnotateAttr)
    {
        LogPrint("Unexpected syntax in enum\n.");
    }

    return CXChildVisit_Continue;
}

CXChildVisitResult StructVisitor(CXCursor cursor, CXCursor parent, CXClientData _data)
{
    auto data = (VisitorData*)_data;
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_FieldDecl)
    {
        AstNode node;
        node.type = AstNodeType::Field;
        data->stack.back()->children.push_back(node);
    }
    else if (kind != CXCursor_AnnotateAttr)
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
                node->type = AstNodeType::Enum;

                CXType clangUnderlyingType = clang_getEnumDeclIntegerType(parent);
                PrimitiveType type = ClangTypeToPrimitive(clangUnderlyingType.kind);

                auto nodeData = (EnumData*)malloc(sizeof(EnumData));
                nodeData->name = spellingStr;
                nodeData->underlyingType = type;
                node->data = nodeData;

                clang_visitChildren(parent, EnumVisitor, data);

                PopNode(data);
            } break;

            case CXCursor_StructDecl:
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
                node->type = AstNodeType::Struct;

                auto nodeData = (StructData *)malloc(sizeof(StructData));
                nodeData->name = spellingStr;
                //nodeData->size = (u32)clang_Type_getSizeOf();
                //nodeData->align = (u32)clang_Type_getAlignOf();

                node->data = nodeData;

                clang_visitChildren(parent, StructVisitor, data);

                PopNode(data);
            } break;

            default: { printf("Error: Found annotation at invalid location.\n"); } break;
        }
    }

    return CXChildVisit_Recurse;
}

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

void VisitAst(AstNode* node)
{
    LogPrint("\nNode: %s\n", ToString(node->type));
    if (node->type == AstNodeType::Enum)
    {
        auto data = (EnumData*)node->data;
        LogPrint("Name: %s, underlying type: %s\n", data->name, ToString(data->underlyingType));
    }

    if (node->type == AstNodeType::EnumConstant)
    {
        auto data = (EnumConstantData*)node->data;
        LogPrint("Name: %s, sValue: %lld, uValue: %llu\n", data->name, data->signedValue, data->unsignedValue);
    }


    if (node->children.size())
    {
        LogPrint("Children:\n");
    }

    for (auto& it : node->children)
    {
        VisitAst(&it);
    }

    if (node->children.size())
    {
        LogPrint("End:\n");
    }
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

int main(int argc, char** argv)
{
    CXIndex index = clang_createIndex(0, 1);
    Assert(index);
    CXTranslationUnit unit = nullptr;
    CXErrorCode resultTr = clang_parseTranslationUnit2FullArgv(index, nullptr, argv, argc, 0, 0, CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_DetailedPreprocessingRecord, &unit);

    printf("Result %d\n", (int)resultTr);

    CXCursor rootCursor = clang_getTranslationUnitCursor(unit);

    VisitorData data;
    data.rootNode.type = AstNodeType::Root;
    data.stack.push_back(&data.rootNode);

    unsigned int level = 0;
    clang_visitChildren(rootCursor, DumpAst, &level);


    clang_visitChildren(rootCursor, AttributesVisitor, &data);

    VisitAst(&data.rootNode);

    return 0;
}