#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>

typedef struct files{

    const char* inputName;
    FILE* input;

    const char* dotName;
    FILE* dot;

    const char* htmlName;
    FILE* html;

    const char* saveName;
    FILE* save;

    const char* elfName;
    FILE* elf;

    const char* outName;
    FILE* out;

} files_t;

typedef union data_t{

    double num;
    int op;
    int id;

} data_t;

typedef struct node_t{

    node_t*     parent;
    node_t*     left;
    node_t*     right;

    int         type;
    uint64_t    id;

    data_t      data;

} node_t;

typedef struct tree{

    int64_t numElem;
    int64_t numDump;

    node_t* root;

} tree_t;

typedef struct names_t{

    char*       name;
    uint64_t    len;

    int         visibilityType; // global/local
    int         idType;         // func/var
    int         stackFrameSize; // for funcs
    int         numParams;      // for funcs

    int         memAddr;

} names_t;

typedef struct line{

    //SYNTAXIS
    char*       buffer;
    uint64_t    ptr;

    //LEXICAL
    node_t*     tokens;
    uint64_t    tptr;

    //NAMETABLE
    names_t*    id;
    uint64_t    numId;

    files_t     files;

    tree_t*     tree;

    //BACKEND
    int         freeAddr;
    char*       x86buff;
    size_t      currAddr;
    size_t      fileSize;
    size_t      entryPoint;
    size_t      globalVars;
    size_t      stdFuncs;
    size_t      additionalBuff;
    int64_t*    stdFuncsOffset;

    //LEXICAL
    bool        err;

} line_t;
