#include <clang-c/Index.h>
#include <clang-c/CXString.h>
#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "Common.h"

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

struct ASTAttribEntry
{
    CXCursor parent;
    CXCursor attrib;

    ASTAttribEntry() = default;
    ASTAttribEntry(CXCursor p, CXCursor a) : parent(p), attrib(a) {}
};

CXChildVisitResult CollectTypeAttribs(CXCursor cursor, CXCursor parent, CXClientData data)
{
    auto attribs = (std::vector<ASTAttribEntry>*)data;

    CXCursorKind kind = clang_getCursorKind(cursor);
    if (kind == CXCursor_AnnotateAttr)
    {
        attribs->push_back(ASTAttribEntry(parent, cursor));
    }

    return CXChildVisit_Continue;
}

CXChildVisitResult CursorVisitor(CXCursor cursor, CXCursor parent, CXClientData data)
{
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_EnumDecl ||
        kind == CXCursor_UnionDecl ||
        kind == CXCursor_StructDecl ||
        kind == CXCursor_ClassDecl ||
        kind == CXCursor_FieldDecl ||
        kind == CXCursor_FunctionDecl ||
        kind == CXCursor_ClassTemplate)
    {
        std::vector<ASTAttribEntry> attribs;
        clang_visitChildren(cursor, CollectTypeAttribs, &attribs);

        for (auto it : attribs)
        {
            CXString cxspelling = clang_getCursorSpelling(it.parent);
            const char* spelling = clang_getCString(cxspelling);
            CXString cxkind = clang_getCursorKindSpelling(clang_getCursorKind(it.parent));
            const char* kind = clang_getCString(cxkind);

            printf("Cursor spelling, kind: %s, %s\n", spelling, kind);

            clang_disposeString(cxspelling);
            clang_disposeString(cxkind);
        }

    }

    return CXChildVisit_Recurse;
}

int main(int argc, char** argv)
{
    CXIndex index = clang_createIndex(0, 1);
    Assert(index);
    CXTranslationUnit unit = nullptr;
    CXErrorCode resultTr = clang_parseTranslationUnit2FullArgv(index, nullptr, argv, argc, 0, 0, CXTranslationUnit_SkipFunctionBodies, &unit);

    CXCursor rootCursor = clang_getTranslationUnitCursor(unit);

    clang_visitChildren(rootCursor, CursorVisitor, nullptr);

    return 0;
}
