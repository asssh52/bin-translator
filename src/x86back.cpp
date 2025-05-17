#include "../hpp/back.hpp"
#include "../hpp/emitter.hpp"

#define MEOW fprintf(stderr, PNK  "MEOW\n" RESET);

#define PRNT_JMP_MARK(name, ...)    PlaceNumOp(line, fprintf(line->files.out, name "%lu:", node->id), __LINE__, 0, node);\
                                    fprintf(line->files.out, " " __VA_ARGS__ "\n");\

#define PRNT_JMP(name, ...)         PlaceNumOp(line, fprintf(line->files.out, name "%lu", node->id), __LINE__, 0, node);\
                                    fprintf(line->files.out, " " __VA_ARGS__ "\n");\

#define PRNT(name, ...)             PlaceNumOp(line, fprintf(line->files.out, name),                  __LINE__, 0, node);\
                                    fprintf(line->files.out, " " __VA_ARGS__ "\n");\

#define PRNT_CUSTOM(...)            PlaceNumOp(line, fprintf(line->files.out, __VA_ARGS__), __LINE__, 1, node)

#define PRNT_DATA(name, ...)        PlaceSpaces(line, fprintf(line->files.out, name),                  __LINE__, 0); \
                                    fprintf(line->files.out, " " __VA_ARGS__ "\n");\

#define PRNT_CUSTOM_DATA(...)       PlaceSpaces(line, fprintf(line->files.out, __VA_ARGS__), __LINE__, 1)

#define QWORD(num)              PutWord(line, num, 8, DIRECT);
#define DWORD(num)              PutWord(line, num, 4, DIRECT);
#define WORD(num)               PutWord(line, num, 2, DIRECT);
#define BYTE(num)               PutWord(line, num, 1, DIRECT);

#define QWORD_NUM(num)          PutWord(line, num, 8, REVERSED);
#define DWORD_NUM(num)          PutWord(line, num, 4, REVERSED);
#define WORD_NUM(num)           PutWord(line, num, 2, REVERSED);
#define BYTE_NUM(num)           PutWord(line, num, 1, REVERSED);

#define PUSH_NUM(num)           PushNum(line, num);
#define PUSH_REG(reg)           PushReg(line, reg);
#define PUSH_MEM(mem)           PushMem(line, mem);
#define PUSH_RBP(offset)        PushMemRBP(line, offset);

#define POP_REG(reg)            PopReg (line, reg);
#define POP_MEM(mem)            PopMem(line, mem);
#define POP_RBP(offset)         PopMemRBP(line, offset);

#define MOV_R2M(to, from)      MovReg2Mem64(line, from, to, DIRECT);
#define MOV_M2R(to, from)      MovReg2Mem64(line, to, from, REVERSED);
#define MOV_R2R(to, from)      MovReg2Reg64(line, to, from);

//===============================================================================================================================================
//          NOTES
// todo: backDtor
//  careful with .data in the end of stdlib.s
//  max func arg - 16, (change command for pushing [rbp - c])
//
//
//
//===============================================================================================================================================

const char*   DFLT_HTML_FILE = "htmldump.html";
const char*   DFLT_ELF_FILE  = "bobr";
const char*   DFLT_SAVE_FILE = "save.txt";
const char*   DFLT_OUT_FILE  = "out.txt";
const char*   DFLT_DOT_FILE  = "./bin/dot.dot";
const char*   DFLT_LIB_FILE  = "./src/stdlib.s";

const int64_t X86BUFF_SIZE      = 0x10000;
const int64_t OFFSET            = 0x1000;
const int64_t MAX_IDS           = 16;
const int64_t MAX_ARGS          = 16;
const int     STDLIB_NUM_FUNC   = 3;

const uint64_t MEM_START        = 0x401000;

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

enum emitParams{

    DIRECT = 0,
    REVERSED  = 1

};

static int      BackCtor         (line_t* line);
static int      BackDtor         (line_t* line);
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

static int ElfHeaderProcess         (line_t* line);
static int EndElfProcess            (line_t* line);
static int ProgrammHeaderProcess    (line_t* line);
static int PutWord                  (line_t* line, int64_t num, int bytes, int order);
static int StdlibProcess            (line_t* line);
static int PopReg                   (line_t* line, int reg);
static int PushReg                  (line_t* line, int reg);
static int MovReg2Reg64             (line_t* line, int from, int to);
static int MovReg2Mem64             (line_t* line, int from, int to, int order);
static int PushMem                  (line_t* line, int mem);
static int PushMemRBP               (line_t* line, int offset);
static int PushNum                  (line_t* line, int num);
static int PopMem                   (line_t* line, int mem);
static int PopMemRBP                (line_t* line, char offset);
static int PushNum                  (line_t* line, int num);

int main(){
    line_t* line = (line_t*)calloc(1, sizeof(*line));

    BackCtor(line);

    LoadTree(line);

    SetNameTypes(line, line->tree->root);
    DumpIds(line, stdout);


    // elf creating
    ElfHeaderProcess(line);
    ProgrammHeaderProcess(line);
    StdlibProcess(line);

    // elf creating

    // nasm listing compilation
    HeaderProcess(line);
    BackProccess(line);
    EndProcess(line);
    // nasm listing compilation

    EndElfProcess(line);

    printf("globalVars:0x%lx\n",line->globalVars);

    fwrite(line->x86buff, X86BUFF_SIZE, 1, line->files.elf);


    printf(RED "'%c %c %c %c'\n" RESET, line->x86buff[0], line->x86buff[1], line->x86buff[2], line->x86buff[3]);

    DumpIds(line, stdout);

    BackDtor(line);

    return 0;
}


static int ElfHeaderProcess(line_t* line){

    QWORD(0x7f454c4602010100); // [0-3] SIGN [4-7] x64 LITTLE-ENDIAN VER. 1 UNIX
    QWORD(0x0);                // PADDING BYTE
    QWORD(0x02003e0001000000); // [0-1] FILE TYPE [2-3] SYSTEM (x86-64) [4-7] VER
    QWORD(0x0010400000000000); // ENTRY POINT!!!!!! addr 18

    QWORD(0x4000000000000000); // PROGRAMM HEADER OFFSET
    QWORD(0x0);                // SECTION HEADER OFFSET
    QWORD(0x0000000040003800); // [0-3] CPU FLAGS [4-5] PROGRAMM HEADER SIZE [6-7] HEADER SIZE
    QWORD(0x0200400000000000); // [0-1] PROGRAMM HEADER COUNT [2-3] SECTION HEADER SIZE [4-5] SECTION HEADER COUNT [6-7] INDEX IN .shstrtab


    return OK;
}

static int ProgrammHeaderProcess(line_t* line){

    QWORD(0x0100000007000000);  // [0-3] p_type [4-7] p_flags
    QWORD_NUM(OFFSET);          // offset
    QWORD_NUM(MEM_START);       // virtual address
    QWORD_NUM(MEM_START);       // physical address

    QWORD(0x8050000000000000);  // file size!!! addr 60
    QWORD(0x8050000000000000);  // mem size     addr 68
    QWORD_NUM(OFFSET);          // alignment

    line->currAddr   = 0x1000;

    return OK;
}

static int StdlibProcess(line_t* line){

    FILE* cmpld = fopen("./src/stdlibCmpl.out", "r");
    size_t cmpldFileSize = getFileSize("./src/stdlibCmpl.out");

    char* cmpldBuff = (char*) calloc(X86BUFF_SIZE, 1);
    fread(cmpldBuff, cmpldFileSize, 1, cmpld);

    size_t libSize = *(uint64_t*)(cmpldBuff + 0x98);    // all after headers

    printf(GRN "libSize: 0x%lx\n" RESET, libSize);

    memcpy(line->x86buff + line->currAddr, cmpldBuff + OFFSET, libSize);

    free(cmpldBuff);

    line->currAddr   = OFFSET + libSize;

    size_t stdFuncs  = OFFSET + 0xf; //!!!!!!!!!!!!!!!!!!!!!!! 0xf - mov; call; call;
    line->stdFuncs = stdFuncs;
    line->stdFuncsOffset = (int64_t*)calloc(STDLIB_NUM_FUNC, sizeof(*line->stdFuncsOffset));

    for (int i = 0; i < STDLIB_NUM_FUNC; i++){
        line->stdFuncsOffset[i] = *((int64_t*)(line->x86buff + stdFuncs) + i);
        line->stdFuncsOffset[i] -= MEM_START;

        printf(PNK "stdFunc(%d):%lx, adr:%lx\n" RESET, i, line->stdFuncsOffset[i], stdFuncs + 8 * i);
    }

    line->globalVars = line->currAddr;
    line->currAddr    += line->numId * 8; // space for global variables
    line->additionalBuff = line->currAddr;
    line->currAddr    += MAX_ARGS * 8;
    line->entryPoint = line->currAddr;

    return OK;
}

static int EndElfProcess(line_t* line){
    size_t size = line->currAddr - OFFSET;

    *(int64_t*)(line->x86buff + 0x18) = line->entryPoint - OFFSET + MEM_START;

    printf(BLU "entry point: %lx\n" RESET, line->entryPoint - OFFSET + MEM_START);
    *(int64_t*)(line->x86buff + 0x60) = size;
    *(int64_t*)(line->x86buff + 0x68) = size;

    return OK;
}

static int PutWord(line_t* line, int64_t num, int bytes, int order){

    int64_t buff = num;

    if (!order)
        for (int i = 0; i < bytes; i++){
            *(line->x86buff + line->currAddr + i) = ((char*)&buff)[bytes - 1 - i];
        }
    else{
        for (int i = 0; i < bytes; i++){
            *(line->x86buff + line->currAddr + i) = ((char*)&buff)[i];
        }
    }

    line->currAddr += bytes;

    return OK;
}


static int PushReg(line_t* line, int reg){
    BYTE(PUSHR + reg);

    return OK;
}

//give addr example: to == line->globalVar + 8 * Num
static int PushMem(line_t* line, int mem){
    int adr = line->currAddr;

    BYTE(0xFF); //opcode
    BYTE(0x35); // WHY???

    int offset = mem - (adr + 6);

    printf("offset in pushMem:0x%x, 0d%d\n", offset, offset);

    DWORD_NUM(offset);

    return OK;
}

static int PushMemRBP(line_t* line, int offset){
    BYTE(0xFF); //opcode
    BYTE(0x75); // WHY???

    BYTE(offset);

    return OK;
}

static int PushNum(line_t* line, int num){
    BYTE(0x68); //opcode

    DWORD_NUM(num);

    return OK;
}

static int PopReg(line_t* line, int reg){
    BYTE(POPR + reg);

    return OK;
}

static int PopMem(line_t* line, int mem){
    int adr = line->currAddr;

    BYTE(0x8F); //opcode
    BYTE(0x05);

    int offset = mem - (adr + 6);

    DWORD_NUM(offset);

    return OK;
}

static int PopMemRBP(line_t* line, char offset){
    BYTE(0x8F); //opcode
    BYTE(0x45);

    BYTE(offset);

    return OK;
}

static int MovReg2Reg64(line_t* line, int from, int to){
    BYTE(0x48); // REX.W
    BYTE(0x89); // reg to reg/mem (move opcode)

    char modRM = 0b11000000;
    modRM |= from;
    modRM |= to << 3;

    BYTE(modRM);

    return OK;
}

//give addr example: to == line->globalVar + 8 * Num
static int MovReg2Mem64(line_t* line, int from, int to, int order){
    int adr = line->currAddr;

    BYTE(0x48); // REX.W

    if (!order){
        BYTE(0x89); // reg to reg/mem (move opcode)
    }
    else{
        BYTE(0x8B);
    }
    char modRM = 0b00000101;
    modRM |= from << 3;

    BYTE(modRM);

    int offset = to - (adr + 7); // 7 == length of command

    DWORD_NUM(offset);

    return OK;
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


    //bin, call stdExit
    int adr = line->currAddr;
    BYTE(0xE8);

    int offset = OFFSET + line->stdFuncsOffset[2] - (adr + 5);
    DWORD_NUM(offset);

    line->currAddr += 5;
    //bin

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

    line->files.elf = fopen(line->files.elfName, "w");
    if (!line->files.elf) line->files.elf = fopen(DFLT_ELF_FILE, "w");

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

    line->x86buff = (char*)calloc(X86BUFF_SIZE, sizeof(char));
    line->currAddr = 0;

    return OK;
}

static int BackDtor(line_t* line){

    fclose(line->files.save);
    fclose(line->files.elf);
    fclose(line->files.html);
    fclose(line->files.out);
    free(line->x86buff);
    free(line->tree);
    free(line->id);

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

            POP_REG(RAX);  //bin
            MOV_R2M(line->globalVars + 8 * node->data.id, mRAX);

        }

        else if (line->id[node->data.id].visibilityType == GLOBAL && line->id[node->data.id].memAddr != 0){
            PRNT("pop rax", " ");
            PRNT_CUSTOM("mov qword [_%d], rax", node->data.id);

            POP_REG(RAX);  //bin
            MOV_R2M(line->globalVars + 8 * node->data.id, mRAX);
        }

        else if (line->id[node->data.id].visibilityType == LOCAL){
            PRNT_CUSTOM("pop qword [rbp - %d]", line->id[node->data.id].memAddr * 8); //???

            POP_RBP(line->id[node->data.id].memAddr * 8 * (-1)); //bin
        }
    }

    else if (type == T_ID && param != EQL){
        if (line->id[node->data.id].visibilityType == GLOBAL){
            PRNT_CUSTOM("push qword [_%d]", node->data.id);

            PUSH_MEM(line->globalVars + 8 * node->data.id); //bin
        }

        else{
            PRNT_CUSTOM("push qword [rbp - %d]", line->id[node->data.id].memAddr * 8);

            PUSH_RBP(line->id[node->data.id].memAddr * 8 * (-1)); //bin
        }
    }
    //ID



    // NUM
    if (type == T_NUM){
        PRNT_CUSTOM("push %ld", (int64_t)node->data.num);

        PUSH_NUM((int)node->data.num);  //casting may cause problems
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


    //bin
    POP_REG(mRAX);
    DWORD(0x4883F800); // cmp rax, 0
    int adr = line->currAddr;
    WORD(JZ);  // jz
    int ptr = line->currAddr;
    DWORD(0x0); // space for filling later
    //bin

    NodeProcess(line, node->right, DFLT);

    //bin
    *(int*)(line->x86buff + ptr) = (line->currAddr) - (adr + 6);
    //bin

    PRNT_JMP_MARK("end_if", " ");

    return OK;
}

static int ProcessWhile(line_t* line, node_t* node){
    PRNT_JMP_MARK("while", "НАЧАЛО ЦИКЛА");

    //bin
    int begin = line->currAddr;
    //bin

    NodeProcess(line, node->left, DFLT);

    PRNT    ("pop rax");
    PRNT    ("cmp rax, 0");
    PRNT_JMP("je end_while", " ");

    //bin
    POP_REG(mRAX);
    DWORD(0x4883F800); // cmp rax, 0
    int adr = line->currAddr;
    WORD(JZ);  // jz
    int ptr = line->currAddr;
    DWORD(0x0); // space for filling later
    //bin

    NodeProcess(line, node->right, DFLT);

    //bin
    int jmpAddr = line->currAddr;
    BYTE(JMP);
    DWORD_NUM((begin) - (jmpAddr + JMP_SIZE));
    *(int*)(line->x86buff + ptr) = (line->currAddr) - (adr + JZ_SIZE);
    //bin

    PRNT_JMP("jmp while", " ");
    PRNT_JMP_MARK("end_while", " ");

    return OK;
}

static int ProcessDef(line_t* line, node_t* node){
    ProcessArgs(line, node);

    PRNT(" ", "НАЧАЛО ОПРЕДЕЛЕНИЯ ФУНКЦИИ");
    PRNT_CUSTOM("jmp def_func_end%d",   node->left->left->data.id);

    //bin
    int adr = line->currAddr;
    BYTE(JMP);
    int jmpAdr = line->currAddr;
    DWORD(0x0);
    //bin

    PRNT_CUSTOM("def_func%d:", node->left->left->data.id);

    //bin
    line->id[node->left->left->data.id].memAddr = line->currAddr; //save address
    //bin

    PRNT("push rbp", " ");
    PRNT("mov rbp, rsp", " ");
    PRNT_CUSTOM("sub rsp, %d", line->id[node->left->left->data.id].stackFrameSize * 8 + 8);

    int params = line->id[node->left->left->data.id].numParams;
    for (int i = params; i > 0; i--){
        PRNT_CUSTOM("push qword [meow + %d]", i * 8);
        PRNT_CUSTOM("pop qword [rbp - %d]", i * 8);
    }

    //bin
    PUSH_REG(RBP);
    MOV_R2R(mRBP, mRSP);
    WORD(0x4881); BYTE(0xEC); // sub rsp, num32
    DWORD_NUM(line->id[node->left->left->data.id].stackFrameSize * 8 + 8);

    for (int i = params; i > 0; i--){
        PUSH_MEM(line->additionalBuff + 8 * i);
        POP_RBP(-8 * i);
    }
    //bin

    NodeProcess(line, node->right, DFLT);

    //bin
    *(int*)(line->x86buff + jmpAdr) = (line->currAddr) - (adr + JMP_SIZE);
    //bin

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

    //bin

    for (int i = params; i > 0; i--){
        POP_MEM(line->additionalBuff + 8 * i);
    }
    int callAdr = line->currAddr;
    int calleeAdr = line->id[node->left->left->data.id].memAddr;
    BYTE(CALL); DWORD_NUM(calleeAdr - (callAdr + CALL_SIZE));
    PUSH_REG(RAX);
    //bin

    return OK;
}

static int ProcessOp(line_t* line, node_t* node){
    int adr = 0, offset = 0;

    switch (node->data.op){
        // +
        case O_ADD:
            PRNT("pop rax", "ПЛЮС");
            PRNT("pop rbx", " ");
            PRNT("add rax, rbx", " ");
            PRNT("push rax", " ");

            POP_REG(mRAX);
            POP_REG(mRBX);
            WORD(0x4801); BYTE(0xD8); // add rax, rbx
            PUSH_REG(RAX);

            break;

        // -
        case O_SUB:
            PRNT("pop rax", "МИНУС");
            PRNT("pop rbx", " ");
            PRNT("sub rax, rbx", " ");
            PRNT("push rax", " ");

            POP_REG(mRAX);
            POP_REG(mRBX);
            WORD(0x4829); BYTE(0xD8); // sub rax, rbx
            PUSH_REG(RAX);

            break;

        // *
        case O_MUL:
            PRNT("pop rax", "УМНОЖИТЬ");
            PRNT("pop rbx", " ");
            PRNT("imul rax, rbx", " ");
            PRNT("push rax", " ");

            POP_REG(mRAX);
            POP_REG(mRBX);
            DWORD(0x480FAFC3); // imul rax, rbx
            PUSH_REG(RAX);

            break;

        // /
        case O_DIV:
            PRNT("pop rax", "ДЕЛИТЬ");
            PRNT("cmp rax, 0", "");
            PRNT_JMP("jl div", "");
            PRNT("xor rdx, rdx", "");
            PRNT_JMP("jmp div_end", "");
            PRNT_JMP_MARK("div", "");
            PRNT("mov rdx, -1", "");
            PRNT_JMP_MARK("div_end", "");
            PRNT("pop rbx", " ");
            PRNT("idiv rbx", " ");
            PRNT("push rax", " ");

            //bin
            POP_REG(mRAX);
            DWORD(0x4883F800);                              // cmp rax, 0
            WORD(JL); DWORD_NUM(0x3 + JMP_SIZE);            // 0x3 == SIZE_XOR
            WORD(0x4831); BYTE(0xD2);                       // xor rdx, rdx
            BYTE(JMP); DWORD_NUM(0x7);                      // 0x7 == SIZE_MOV
            WORD(0x48C7); BYTE(0xC2); DWORD(0xFFFFFFFF);    // mov rdx, -1
            POP_REG(mRBX);
            WORD(0x48F7); BYTE(0xFB);                       // idiv rbx
            PUSH_REG(RAX);
            //bin

            break;

        // print
        case O_PNT:
            PRNT("pop rsi", "ПЕЧАТЬ");
            PRNT("call [std + 8]", "ПЕЧАТЬ");

            //bin
            POP_REG(mRSI);

            adr = line->currAddr;
            BYTE(0xE8);     // call

            offset = OFFSET + line->stdFuncsOffset[1] - (adr + 5);
            DWORD_NUM(offset);
            //bin

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

            //bin
            POP_REG(mRAX);
            MOV_R2R(mRSP, mRBP);
            POP_REG(mRBP);
            BYTE(RET);
            //bin

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

            //bin
            POP_REG(mRAX);
            POP_REG(mRBX);
            WORD(0x4839); BYTE(0xD8); // cmp rax, rbx
            WORD(JL); DWORD_NUM(JMP_SIZE + PUSH_NUM_SIZE);
            PUSH_NUM(0x00);
            BYTE(JMP); DWORD_NUM(PUSH_NUM_SIZE);
            PUSH_NUM(0x01);
            //bin

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
            PRNT_JMP_MARK("eq_end", "РАВНО КОНЕЦ");

            //bin
            POP_REG(mRAX);
            POP_REG(mRBX);
            WORD(0x4839); BYTE(0xD8); // cmp rax, rbx
            WORD(JZ); DWORD_NUM(JMP_SIZE + PUSH_NUM_SIZE);
            PUSH_NUM(0x00);
            BYTE(JMP); DWORD_NUM(PUSH_NUM_SIZE);
            PUSH_NUM(0x01);
            //bin

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
            PRNT("pxor xmm0, xmm0", " ");
            PRNT("pop rax", " ");
            PRNT("cvtsi2sd xmm0, rax", " ");
            PRNT("sqrtsd xmm0, xmm0", " ");
            PRNT("cvttsd2si rax, xmm0", " ");
            PRNT("push rax", " ");

            //bin
            DWORD(0x660FEFC0);              // pxor xmm0, xmm0
            POP_REG(mRAX);
            DWORD(0xF2480F2A); BYTE(0xC0);  // cvtsi2sd xmm0, rax
            DWORD(0xF20F51C0);              // sqrtsd xmm0, xmm0
            DWORD(0xF2480F2C); BYTE(0xC0);  // cvttsd2si rax, xmm0
            PUSH_REG(RAX);
            //bin

            break;

        default:
            PRNT("unknown node", " ");
            break;
    }

    return OK;
}
