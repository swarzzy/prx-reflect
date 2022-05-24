#define attrib(...) __attribute__((annotate("__metaprogram " #__VA_ARGS__)))
#define metaprogram_visible attrib("metaprogram_visible")

typedef unsigned int u32;

struct metaprogram_visible StructForwardDecl;

enum metaprogram_visible EnumNoTypedef : long
{
    Boo, Bar
};

typedef enum metaprogram_visible : long
{
    Boo2 = -1,
    Bar2 = 1
} EnumWithTypedef;

typedef void(FunctionProto)(int i);

typedef struct metaprogram_visible
{
    signed short int signedShortInt;
    unsigned long int unsignedLongInt;
    signed long int signedLongInt;
    signed char signedChar;     // SChar
    char justChar;              // Char_S
    char signed charSigned;     // SChar
    unsigned char unsignedChar; // Uchar
    char unsigned charUnsigned; // Uchar
    int *intPtr;
    int **intPtrPtr;
    int *intPtrArray[10];
    int intArray[11];
    u32 u32Typedef;
    void (*functionNoProto)();
    FunctionProto* functionProto;
    int arrayUnknownSize[];
} TestFields;

struct metaprogram_visible StructWithoutTypedef
{
    int a;
    float b;
};

typedef struct metaprogram_visible
{
    int a;
    float b;
} StructWithTypedef;

typedef struct metaprogram_visible _NamedStructWithTypedef
{
    int a;
    float b;
} NamedStructWithTypedef;

struct metaprogram_visible StructDefinesTest
{
    struct StructWithoutTypedef structWithoutTypedef;
    StructWithTypedef structWithTypedef;
    NamedStructWithTypedef namedStructWithTypedef;
    struct _NamedStructWithTypedef _namedStructWithTypedef;
};

struct metaprogram_visible AnonymousNestedStructTest
{
    struct
    {
        int a;
        float c;
    } foo;
    int bar;
};

struct metaprogram_visible AnonymousEnumStructTest
{
    enum {A, B} anonumousEnum;
};