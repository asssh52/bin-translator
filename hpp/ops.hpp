#pragma once

const int OP_LEN = 16;

typedef struct opName_t{

    char    name[OP_LEN];
    int     opNum;
    int     len;
    char    stdname[OP_LEN];
    int     stdlen;

} opName_t;

enum nodeTypes{

    T_NUM = 1,
    T_ID  = 2,
    T_OPR = 3

};

enum nodeOperations{

    O_ADD = 43,     // +
    O_SUB = 45,     // -
    O_MUL = 42,     // *
    O_DIV = 47,     // /
    O_EQL = 61,     // =

    O_LES = 109,    // m
    O_LSE = 110,    // n
    O_MOR = 111,    // o
    O_MRE = 112,    // p
    O_EQQ = 113,    // q
    O_NEQ = 114,    // r
    O_POW = 94,     // ^
    O_SQT = 83,     // S

    O_SIN = 115,    // s
    O_COS = 99,     // c
    O_TAN = 116,    // t
    O_OBR = 40,     // (
    O_CBR = 41,     // )
    O_IFB = 73,     // I
    O_IFC = 79,     // O
    O_PNT = 80,     // P
    O_TRM = 36,     // $
    O_SEP = 59,     // ;
    O_WHB = 87,     // W
    O_WHC = 88,     // X
    O_DEF = 100,    // d
    O_DFS = 120,    // x
    O_CMA = 44,     // ,
    O_CAL = 67,     // C
    O_CSP = 119,    // w
    O_RET = 82,     // R

};

const opName_t opList[] = {

    {"+",           O_ADD, 1,   "+",            1   },
    {"-",           O_SUB, 1,   "-",            1   },
    {"*",           O_MUL, 1,   "*",            1   },
    {"/",           O_DIV, 1,   "/",            1   },
    {"=",           O_EQL, 1,   "=",            1   },
    {"less",        O_LES, 4,   "less",         4   },
    {"less_equal",  O_LSE, 10,  "less_equal",  10   },
    {"more",        O_MOR, 4,   "more",         4   },
    {"more_equal",  O_MRE, 10,  "more_equal",  10   },
    {"equal",       O_EQQ, 5,   "==",           2   },
    {"not_equal",   O_NEQ, 9,   "!=",           2   },
    {"pow",         O_POW, 3,   "^",            1   },
    {"sqrt",        O_SQT, 4,   "sqrt",         4   },
    {"sin",         O_SIN, 3,   "NAN",         -1   },
    {"cos",         O_COS, 3,   "NAN",         -1   },
    {"tan",         O_TAN, 3,   "NAN",         -1   },
    {"(",           O_OBR, 1,   "NAN",         -1   },
    {")",           O_CBR, 1,   "NAN",         -1   },
    {"skibidi",     O_IFB, 7,   "if",           2   },
    {"ohio",        O_IFC, 4,   "NAN",         -1   },
    {"sigma",       O_PNT, 5,   "print",        5   },
    {"@",           O_TRM, 1,   "terminal",     8   },
    {"$",           O_SEP, 1,   ";",            1   },
    {"amogus",      O_WHB, 6,   "while",        5   },
    {"imposter",    O_WHC, 8,   "NAN",         -1   },
    {"kfc",         O_DEF, 3,   "def",          3   },
    {"xd",          O_DFS, 2,   "spec",         4   },
    {"whopper",     O_CMA, 7,   ",",            1   },
    {"wakie",       O_CAL, 5,   "call",         4   },
    {"wakye",       O_CSP, 5,   "spec",         4   },
    {"banana",      O_RET, 6,   "return",       6   }

};

const opName_t typeList[] = {

    {"ID",     T_ID,  2, "NAN"},
    {"NUM",    T_NUM, 3, "NAN"},
    {"OP",     T_OPR, 2, "NAN"},

};
