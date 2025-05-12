#include "../hpp/back.hpp"

#define MEOW fprintf(stderr, PNK  "MEOW\n" RESET);

#define PRNT_JMP_MARK(name, ...)    PlaceNumOp(line, fprintf(line->files.out, name "%lu:", node->id), __LINE__, 0, node); \
                                    fprintf(line->files.out, " " __VA_ARGS__ "\n");\

#define PRNT_JMP(name, ...)         PlaceNumOp(line, fprintf(line->files.out, name "%lu", node->id), __LINE__, 0, node); \
                                    fprintf(line->files.out, " " __VA_ARGS__ "\n");\

#define PRNT(name, ...)             PlaceNumOp(line, fprintf(line->files.out, name),                  __LINE__, 0, node); \
                                    fprintf(line->files.out, " " __VA_ARGS__ "\n");\

#define PRNT_CUSTOM(...)            PlaceNumOp(line, fprintf(line->files.out, __VA_ARGS__), __LINE__, 1, node)\

#define PRNT_DATA(name, ...)        PlaceSpaces(line, fprintf(line->files.out, name),                  __LINE__, 0); \
                                    fprintf(line->files.out, " " __VA_ARGS__ "\n");\

#define PRNT_CUSTOM_DATA(...)       PlaceSpaces(line, fprintf(line->files.out, __VA_ARGS__), __LINE__, 1)\


const char*   DFLT_HTML_FILE = "htmldump.html";
const char*   DFLT_SAVE_FILE = "save.txt";
const char*   DFLT_OUT_FILE  = "out.txt";
const char*   DFLT_DOT_FILE  = "./bin/dot.dot";
const char*   DFLT_LIB_FILE  = "./src/stdlib.s";

const int64_t MAX_IDS  = 16;

enum errors{

    OK  = 0,
    ERR = 1,

};

enum params{

    DFLT = 0,
    EQL  = 1,
    DEF  = 2

};

enum id_types{

    GLOBAL = 103,
    LOCAL  = 108,

    VAR    = 118,
    FUNC   = 102

};

static int      BackCtor         (line_t* line);
static int      BackProccess     (line_t* line);
static int      NodeProcess      (line_t* line, node_t* node, int param);
static int      SetNameTypes     (line_t* line, node_t* node);
static int      CreateAddr       (line_t* line, node_t* node);
static int      ProcessArgs      (line_t* line, node_t* node);

static int      ProcessIf        (line_t* line, node_t* node);
static int      ProcessDef       (line_t* line, node_t* node);
static int      ProcessWhile     (line_t* line, node_t* node);
static int      ProcessCall      (line_t* line, node_t* node);
static int      ProcessOp        (line_t* line, node_t* node);

static int      PlaceNumOp       (line_t* line, int numSpaces, int numLine, int param, node_t* node);
static int      HeaderProcess    (line_t* line);
static int      EndProcess        (line_t* line);
static int      PlaceSpaces      (line_t* line, int numSpaces, int numLine, int param);
static size_t   getFileSize      (const char* filename);

int main(){
    line_t* line = (line_t*)calloc(1, sizeof(*line));

    BackCtor(line);

    LoadTree(line);

    SetNameTypes(line, line->tree->root);
    DumpIds(line, stdout);

    HeaderProcess(line);
    BackProccess(line);
    EndProcess(line);

    DumpIds(line, stdout);

    return 0;
}


static int HeaderProcess(line_t* line){

    PRNT_DATA("section     .rodata", "HeaderProcessStart");
    PRNT_DATA("std:");
    PRNT_DATA("dq getNum");
    PRNT_DATA("dq printNum");
    PRNT_DATA("dq exitProg");
    PRNT_DATA(" ");

    PRNT_DATA("section      .text");
    PRNT_DATA("global       _start");
    PRNT_DATA("_start:");


    PRNT_DATA(" ", "HeaderProcessEnd");

    return OK;
}



static int EndProcess(line_t* line){
    PRNT_DATA("call [std + 16]", "EndProcessStart");

    FILE* fileLib = fopen(DFLT_LIB_FILE, "r");
    size_t size = getFileSize(DFLT_LIB_FILE);
    void* buffPtr = calloc(size, sizeof(char));

    fread(buffPtr, sizeof(char), size, fileLib);
    fwrite(buffPtr, sizeof(char), size, line->files.out);

    PRNT_DATA("section .data");

    for (int i = 0; i < line->numId; i++){
        PRNT_CUSTOM_DATA("_%d: dq '0'", i);
    }

    PRNT_DATA(" ", "EndProcessEnd");

    return OK;
}



static size_t getFileSize (const char* filename){

    struct stat st;
    stat(filename, &st);
    size_t size = st.st_size;

    return size;
}



static int BackCtor(line_t* line){
    line->tree = (tree_t*)  calloc(1, sizeof(*line->tree));
    line->id   = (names_t*) calloc(MAX_IDS,  sizeof(*line->id));

    line->files.html = fopen(line->files.htmlName, "w");
    if (!line->files.html) line->files.html = fopen(DFLT_HTML_FILE, "w");

    line->files.save = fopen(line->files.saveName, "r");
    if (!line->files.save) line->files.save = fopen(DFLT_SAVE_FILE, "r");
    if (!line->files.save) {
        fprintf(stderr, RED "cannot find input file\n" RESET);
        return ERR;
    }

    line->files.out = fopen(line->files.outName, "w");
    if (!line->files.out) line->files.out = fopen(DFLT_OUT_FILE, "w");

    line->files.dotName = DFLT_DOT_FILE;

    line->freeAddr = 1;

    return OK;
}

static int BackProccess(line_t* line){

    NodeProcess(line, line->tree->root, DFLT);

    return OK;
}

static int NodeProcess(line_t* line, node_t* node, int param){

    int type = node->type;
    int op = node->data.op;

    // =
    if (type == T_OPR && op == O_EQL){
        NodeProcess(line, node->right, DFLT);
        NodeProcess(line, node->left, EQL);
    }

    // IF
    else if(type == T_OPR && op == O_IFB){
        ProcessIf(line, node);
    }

    // WHILE
    else if(type == T_OPR && op == O_WHB){
        ProcessWhile(line, node);
    }

    //DEF FUNC
    else if(type == T_OPR && op == O_DEF){
        ProcessDef(line, node);
    }

    //CALL
    else if( type == T_OPR && op == O_CAL){
        ProcessCall(line, node);
    }

    // OTHER
    else if (type == T_OPR && (op == O_ADD || op == O_SUB || op == O_DIV ||   // +  -  /
                               op == O_MUL || op == O_LES || op == O_LSE ||   // *  <  <=
                               op == O_MOR || op == O_MRE || op == O_EQQ ||   // >  >= ==
                               op == O_NEQ || op == O_POW)){                  // != ^

        if (node->right)  NodeProcess(line, node->right, DFLT);
        if (node->left)   NodeProcess(line, node->left, DFLT);
    }

    else{
        if (node->left)   NodeProcess(line, node->left, DFLT);
        if (node->right)  NodeProcess(line, node->right, DFLT);
    }

    // ID
    if (type == T_ID && param == EQL){
        if (line->id[node->data.id].visibilityType == GLOBAL && line->id[node->data.id].memAddr == 0){

            PRNT("pop rax", " ");
            PRNT_CUSTOM("mov qword [_%d], rax", node->data.id);

        }

        else if (line->id[node->data.id].visibilityType == GLOBAL && line->id[node->data.id].memAddr != 0){
            PRNT("pop rax", " ");
            PRNT_CUSTOM("mov qword [_%d], rax", node->data.id);
        }

        else if (line->id[node->data.id].visibilityType == LOCAL){
            PRNT_CUSTOM("pop qword [rbp - %d]", line->id[node->data.id].memAddr * 8); //???
        }
    }

    else if (type == T_ID && param != EQL){
        if (line->id[node->data.id].visibilityType == GLOBAL){
            PRNT_CUSTOM("push qword [_%d]", node->data.id);
        }

        else{
            PRNT_CUSTOM("push qword [rbp - %d]", line->id[node->data.id].memAddr * 8);
        }
    }
    //ID



    // NUM
    if (type == T_NUM){
        PRNT_CUSTOM("push %ld", (int64_t)node->data.num);
    }
    // NUM



    //OP
    if (type == T_OPR){
        ProcessOp(line, node);
    }
    //OP


    return OK;
}

static int CreateAddr(line_t* line, node_t* node){

    line->id[node->data.id].memAddr = line->freeAddr;
    line->freeAddr += 1;

    return OK;
}

static int ProcessArgs(line_t* line, node_t* node){
    node_t* nodeSpec = node->left;

    int paramCount = 0;

    node_t* nodeComma = nodeSpec;

    while (nodeComma->right && nodeComma->right->type == T_OPR && nodeComma->right->data.op == O_CMA){
        paramCount += 1;

        nodeComma = nodeComma->right;
        node_t* nodeArg = nodeComma->left;

        line->id[nodeArg->data.id].visibilityType   = LOCAL;
        line->id[nodeArg->data.id].memAddr          = paramCount;
    }

    line->id[nodeSpec->left->data.id].stackFrameSize = paramCount;
    line->id[nodeSpec->left->data.id].numParams = paramCount;

    return OK;
}

static int SetNameTypes(line_t* line, node_t* node){

    if (node->type == T_ID && node->parent && (node->parent->data.op == O_DFS || node->parent->data.op == O_CSP)){
        line->id[node->data.id].idType = FUNC;
        line->id[node->data.id].visibilityType = FUNC;

    }

    else if (node->type == T_ID && node->parent && node->parent->data.op != O_CMA){
        line->id[node->data.id].visibilityType = GLOBAL;
        line->id[node->data.id].idType = VAR;
    }

    else if (node->type == T_ID){
        line->id[node->data.id].idType = VAR;
    }

    if (node->left)  SetNameTypes(line, node->left);
    if (node->right) SetNameTypes(line, node->right);

    return OK;
}

static int PlaceNumOp(line_t* line, int numSpaces, int numLine, int param, node_t* node){

    for (int i = 0; i < 32 - numSpaces; i++){
        fprintf(line->files.out, " ");
    }

    fprintf(line->files.out, ";line:%.3d\t", numLine);

    fprintf(line->files.out, "node:%.3lu\t", node->id);

    if (param) fprintf(line->files.out, "\n");

    return OK;
}

static int PlaceSpaces(line_t* line, int numSpaces, int numLine, int param){

    for (int i = 0; i < 32 - numSpaces; i++){
        fprintf(line->files.out, " ");
    }

    fprintf(line->files.out, ";line:%.3d\t", numLine);

    fprintf(line->files.out, "        \t");

    if (param) fprintf(line->files.out, "\n");

    return OK;
}

static int ProcessIf(line_t* line, node_t* node){

    NodeProcess(line, node->left, DFLT);

    PRNT    ("pop rax", "НАЧАЛО ИФА");
    PRNT    ("cmp rax, 0");
    PRNT_JMP("je end_if", " ");

    NodeProcess(line, node->right, DFLT);

    PRNT_JMP_MARK("end_if", " ");

    return OK;
}

static int ProcessWhile(line_t* line, node_t* node){
    PRNT_JMP_MARK("while", "НАЧАЛО ЦИКЛА");

    NodeProcess(line, node->left, DFLT);

    PRNT    ("pop rax");
    PRNT    ("cmp rax, 0");
    PRNT_JMP("je end_while", " ");

    NodeProcess(line, node->right, DFLT);

    PRNT_JMP("jmp while", " ");
    PRNT_JMP_MARK("end_while", " ");

    return OK;
}

static int ProcessDef(line_t* line, node_t* node){
    ProcessArgs(line, node);

    PRNT(" ", "НАЧАЛО ОПРЕДЕЛЕНИЯ ФУНКЦИИ");
    PRNT_CUSTOM("jmp def_func_end%d",   node->left->left->data.id);
    PRNT_CUSTOM("def_func%d:", node->left->left->data.id);

    PRNT("push rbp", " ");
    PRNT("mov rbp, rsp", " ");
    PRNT_CUSTOM("sub rsp, %d", line->id[node->left->left->data.id].stackFrameSize * 8 + 8);

    int params = line->id[node->left->left->data.id].numParams;
    for (int i = params; i > 0; i--){
        PRNT_CUSTOM("push qword [meow + %d]", i * 8);
        PRNT_CUSTOM("pop qword [rbp - %d]", i * 8);
    }

    NodeProcess(line, node->right, DFLT);

    PRNT_CUSTOM("def_func_end%d:", node->left->left->data.id);

    return OK;
}

static int ProcessCall(line_t* line, node_t* node){
    PRNT        (" ", "Я НАЧАЛО ВЫЗОВ ФУНКЦИЯ");

    NodeProcess(line, node->left->right, DFLT);

    int params = line->id[node->left->left->data.id].numParams; // num params

    for (int i = params; i > 0; i--){
        PRNT_CUSTOM("pop qword [meow + %d]", i * 8);
    }

    PRNT_CUSTOM ("call def_func%d", node->left->left->data.id);

    PRNT        ("push rax", " ");

    return OK;
}

static int ProcessOp(line_t* line, node_t* node){

    switch (node->data.op){
        // +
        case O_ADD:
            PRNT("pop rax", "ПЛЮС");
            PRNT("pop rbx", " ");
            PRNT("add rax, rbx", " ");
            PRNT("push rax", " ");

            break;

        // -
        case O_SUB:
            PRNT("pop rax", "МИНУС");
            PRNT("pop rbx", " ");
            PRNT("sub rax, rbx", " ");
            PRNT("push rax", " ");
            break;

        // *
        case O_MUL:
            PRNT("pop rax", "УМНОЖИТЬ");
            PRNT("pop rbx", " ");
            PRNT("imul rax, rbx", " ");
            PRNT("push rax", " ");
            break;

        // /
        case O_DIV:
            PRNT("div", " ");
            break;

        // print
        case O_PNT:
            PRNT("pop rsi", "ПЕЧАТЬ");
            PRNT("call [std + 8]", "ПЕЧАТЬ");
            break;

        // ;
        case O_SEP:
            PRNT(" ", " ");
            break;

        // =
        case O_EQL:
            PRNT(" ", " ");
            break;

        // if
        case O_IFB:
            PRNT(" ", "КОНЕЦ ИФА");
            break;

        // while
        case O_WHB:
            PRNT(" ", "КОНЕЦ ЦИКЛА");
            break;

        // function defenition
        case O_DEF:
            PRNT(" ", "КОНЕЦ ОПРЕДЕЛЕНИЯ ФУНКЦИИ");
            break;

        // return
        case O_RET:
            PRNT("pop rax", " ");
            PRNT("mov rsp, rbp", " ");
            PRNT("pop rbp", " ");
            PRNT("ret", " ");
            break;

        // call
        case O_CAL:
            PRNT(" ", " ");
            break;

        // <
        case O_LES:
            PRNT("pop rax", "МЕНЬШЕ");
            PRNT("pop rbx", " ");
            PRNT("cmp rax, rbx", " ");
            PRNT_JMP("jl less", "");
            PRNT("push 0d", " ");
            PRNT_JMP("jmp less_end", "");
            PRNT_JMP_MARK("less", "");
            PRNT("push 1d", "");
            PRNT_JMP_MARK("less_end", "МЕНЬШЕ КОНЕЦ");

            break;

        // <=
        case O_LSE:
            PRNT("less_equal", " ");
            break;

        // >
        case O_MOR:
            PRNT("more", " ");
            break;

        // >=
        case O_MRE:
            PRNT("more_equal", " ");
            break;

        // ==
        case O_EQQ:
            PRNT("pop rax", "РАВНО");
            PRNT("pop rbx", " ");
            PRNT("cmp rax, rbx", " ");
            PRNT_JMP("je eq", "");
            PRNT("push 0d", " ");
            PRNT_JMP("jmp eq_end", "");
            PRNT_JMP_MARK("eq", "");
            PRNT("push 1d", "");
            PRNT_JMP_MARK("eq_end", "РАНВО КОНЕЦ");
            break;

        // !=
        case O_NEQ:
            PRNT("not_equal", " ");
            break;

        // ^
        case O_POW:
            PRNT("power", " ");
            break;

        // ,
        case O_CMA:
            PRNT(" ", " ");
            break;

        // sqrt
        case O_SQT:
            PRNT("sqrt", " ");
            break;

        default:
            PRNT("unknown node", " ");
            break;
    }

    return OK;
}
