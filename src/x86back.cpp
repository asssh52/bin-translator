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

#define EMIT_LST(str, name, ...)    PutWord(line, str, sizeof(str) - 1);\
                                    PRNT(name, __VA_ARGS__);\

#define EMIT(str)               PutWord(line, str, sizeof(str) - 1);\

#define QWORD_NUM(num)          *(int64_t*)(line->x86buff + line->currAddr) = num;  line->currAddr += 8;
#define DWORD_NUM(num)          *(int*)    (line->x86buff + line->currAddr) = num;  line->currAddr += 4;
#define WORD_NUM(num)           *(int16_t*)(line->x86buff + line->currAddr) = num;  line->currAddr += 2;
#define BYTE_NUM(num)           *(char*)   (line->x86buff + line->currAddr) = num;  line->currAddr += 1;

#define PUSH_NUM(num)                       PushNum(line, num);
#define PUSH_REG(reg, name, ...)            PushReg(line, reg);             PRNT(name, __VA_ARGS__);
#define PUSH_MEM(mem)                       PushMem(line, mem);
#define PUSH_RBP(offset)                    PushMemRBP(line, offset);

#define POP_REG(reg, name, ...)             PopReg (line, reg);             PRNT(name, __VA_ARGS__);
#define POP_MEM(mem)                        PopMem(line, mem);
#define POP_RBP(offset)                     PopMemRBP(line, offset);

#define MOV_R2M(to, from)                   MovReg2Mem64(line, from, to, DIRECT);
#define MOV_M2R(to, from)                   MovReg2Mem64(line, to, from, REVERSED);
#define MOV_R2R(to, from, name, ...)        MovReg2Reg64(line, to, from);   PRNT(name, __VA_ARGS__);

//===============================================================================================================================================
//          NOTES
//
//  careful with .data in the end of stdlib.s
//  max func arg == 16, (change command for pushing [rbp - c])
//
//
//
//===============================================================================================================================================

const char*   DFLT_HTML_FILE = "htmldump.html";
const char*   DFLT_ELF_FILE  = "bobr";
const char*   DFLT_LST_FILE  = "bobr.lst";
const char*   DFLT_SAVE_FILE = "save.txt";
const char*   DFLT_OUT_FILE  = "out.txt";
const char*   DFLT_DOT_FILE  = "./bin/dot.dot";
const char*   DFLT_LIB_FILE  = "./src/stdlib.s";
const char*   DFLT_CMPL_FILE = "./src/stdlibCmpl.out";

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
static int PutWord                  (line_t* line, const char* str, int len);
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

int main(int argc, char* argv[]){
    line_t* line = (line_t*)calloc(1, sizeof(*line));

    printf("argc:%d, argv[1]:%s\n", argc, argv[1]);
    if (argc == 2) line->files.saveName = argv[1];

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


    printf(RED "'%c%c%c%c'\n" RESET, line->x86buff[0], line->x86buff[1], line->x86buff[2], line->x86buff[3]);

    DumpIds(line, stdout);

    BackDtor(line);

    return 0;
}


static int ElfHeaderProcess(line_t* line){

    line->elfEhdr.e_ident[0] = ELFMAG0;
    line->elfEhdr.e_ident[1] = ELFMAG1;
    line->elfEhdr.e_ident[2] = ELFMAG2;
    line->elfEhdr.e_ident[3] = ELFMAG3;
    line->elfEhdr.e_ident[4] = ELFCLASS64;
    line->elfEhdr.e_ident[5] = ELFDATA2LSB;
    line->elfEhdr.e_ident[6] = EV_CURRENT;
    line->elfEhdr.e_ident[7] = ELFOSABI_SYSV;

    line->elfEhdr.e_type        = ET_EXEC;
    line->elfEhdr.e_machine     = EM_X86_64;
    line->elfEhdr.e_version     = EV_CURRENT;
    line->elfEhdr.e_entry       = 0x0;                  // filled later
    line->elfEhdr.e_phoff       = sizeof(Elf64_Ehdr);   // pheader offset
    line->elfEhdr.e_shoff       = 0x0;
    line->elfEhdr.e_flags       = 0x0;
    line->elfEhdr.e_ehsize      = sizeof(Elf64_Ehdr);
    line->elfEhdr.e_phentsize   = sizeof(Elf64_Phdr);
    line->elfEhdr.e_phnum       = 0x2;                  // num
    line->elfEhdr.e_shentsize   = sizeof(Elf64_Shdr);
    line->elfEhdr.e_shnum       = 0x0;
    line->elfEhdr.e_shstrndx    = 0x0;

    return OK;
}

static int ProgrammHeaderProcess(line_t* line){

    line->elfPhdr.p_type    = PT_LOAD;
    line->elfPhdr.p_flags   = PF_X + PF_W + PF_R;
    line->elfPhdr.p_offset  = OFFSET;
    line->elfPhdr.p_vaddr   = MEM_START;
    line->elfPhdr.p_paddr   = MEM_START;
    line->elfPhdr.p_filesz  = 0x0;  //filled later
    line->elfPhdr.p_memsz   = 0x0;  //filled later
    line->elfPhdr.p_align   = OFFSET;

    line->currAddr   = 0x1000;

    return OK;
}

static int StdlibProcess(line_t* line){

    FILE* cmpld = fopen(DFLT_CMPL_FILE, "r");
    size_t cmpldFileSize = getFileSize(DFLT_CMPL_FILE);

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
    }

    line->globalVars = line->currAddr;
    line->currAddr    += line->numId * 8; // space for global variables
    line->entryPoint = line->currAddr;

    return OK;
}

static int EndElfProcess(line_t* line){
    size_t size = line->currAddr - OFFSET;

    printf(BLU "entry point: %lx\n" RESET, line->entryPoint - OFFSET + MEM_START);

    line->elfPhdr.p_filesz  = size;
    line->elfPhdr.p_memsz   = size;

    line->elfEhdr.e_entry = line->entryPoint - OFFSET + MEM_START;

    memcpy(line->x86buff, &(line->elfEhdr), sizeof(Elf64_Ehdr));
    memcpy(line->x86buff + 0x40, &(line->elfPhdr), sizeof(Elf64_Phdr));

    return OK;
}

static int PutWord(line_t* line, const char* str, int len){
    memcpy(line->x86buff + line->currAddr, str, len);
    line->currAddr += (len);

    return OK;
}


static int PushReg(line_t* line, int reg){
    BYTE_NUM(PUSHR + reg);

    return OK;
}

//addr example: mem == line->globalVar + 8 * Num
static int PushMem(line_t* line, int mem){
    int adr = line->currAddr;
    int offset = mem - (adr + 6);

    EMIT("\xFF"); //opcode
    EMIT("\x35"); // WHY???
    DWORD_NUM(offset);

    return OK;
}

static int PushMemRBP(line_t* line, int offset){
    EMIT("\xFF"); //opcode
    EMIT("\x75"); // WHY???

    BYTE_NUM(offset);

    return OK;
}

static int PushNum(line_t* line, int num){
    EMIT("\x68"); //opcode

    DWORD_NUM(num);

    return OK;
}

static int PopReg(line_t* line, int reg){
    BYTE_NUM(POPR + reg);

    return OK;
}

static int PopMem(line_t* line, int mem){
    int adr = line->currAddr;
    int offset = mem - (adr + 6);

    EMIT("\x8F"); //opcode
    EMIT("\x05");
    DWORD_NUM(offset);

    return OK;
}

static int PopMemRBP(line_t* line, char offset){
    EMIT("\x8F"); //opcode
    EMIT("\x45");

    BYTE_NUM(offset);

    return OK;
}

static int MovReg2Reg64(line_t* line, int from, int to){
    EMIT("\x48"); // REX.W
    EMIT("\x89"); // reg to reg/mem (move opcode)

    char modRM = 0b11000000;
    modRM |= from;
    modRM |= to << 3;

    BYTE_NUM(modRM);

    return OK;
}

//give addr example: to == line->globalVar + 8 * Num
static int MovReg2Mem64(line_t* line, int from, int to, int order){
    int adr = line->currAddr;

    EMIT("\x48"); // REX.W

    if (!order){
        EMIT("\x89"); // reg to reg/mem (move opcode)
    }
    else{
        EMIT("\x8B");
    }
    char modRM = 0b00000101;
    modRM |= from << 3;

    BYTE_NUM(modRM);

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
    EMIT("\xE8");

    int offset = OFFSET + line->stdFuncsOffset[2] - (adr + CALL_SIZE);
    DWORD_NUM(offset);

    line->currAddr += CALL_SIZE;
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

    if (line->files.save) fclose(line->files.save);
    if (line->files.elf) fclose(line->files.elf);
    if (line->files.html) fclose(line->files.html);
    if (line->files.out) fclose(line->files.out);
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
        int memaddr = line->globalVars + 8 * node->data.id;

        if (line->id[node->data.id].visibilityType == GLOBAL && line->id[node->data.id].memAddr == 0){

            POP_REG(RAX,                        "pop rax");
            MOV_R2M(memaddr, mRAX); PRNT_CUSTOM("mov qword [_%d], rax", node->data.id);
        }

        else if (line->id[node->data.id].visibilityType == GLOBAL && line->id[node->data.id].memAddr != 0){

            POP_REG(RAX,                        "pop rax");
            MOV_R2M(memaddr, mRAX); PRNT_CUSTOM("mov qword [_%d], rax", node->data.id);
        }

        else if (line->id[node->data.id].visibilityType == LOCAL){
            int addr = line->id[node->data.id].memAddr * 8  + 8;
            POP_RBP(addr); PRNT_CUSTOM("pop qword [rbp + %d]", addr);
        }
    }

    else if (type == T_ID && param != EQL){
        if (line->id[node->data.id].visibilityType == GLOBAL){
            int memaddr = line->globalVars + 8 * node->data.id;
            PUSH_MEM(memaddr); PRNT_CUSTOM("push qword [_%d]", node->data.id);
        }

        else{
            int rbpaddr = line->id[node->data.id].memAddr * 8 + 8;
            PUSH_RBP(rbpaddr); PRNT_CUSTOM("push qword [rbp + %d]", rbpaddr);
        }
    }
    //ID



    // NUM
    if (type == T_NUM){
        int num = (int)node->data.num;
        PUSH_NUM(num); PRNT_CUSTOM("push %d", num);
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

    POP_REG(mRAX,                               "pop rax", "НАЧАЛО ИФА");
    EMIT_LST("\x48\x83\xF8\x00",                "cmp rax, 0");
    int adr = line->currAddr;
    EMIT(JZ); PRNT_JMP(                         "je end_if", " ");
    int ptr = line->currAddr;
    EMIT("\x00\x00\x00\x00"); // space for filling later

    NodeProcess(line, node->right, DFLT);

    *(int*)(line->x86buff + ptr) = (line->currAddr) - (adr + JZ_SIZE);
    PRNT_JMP_MARK(                              "end_if", " ");


    return OK;
}

static int ProcessWhile(line_t* line, node_t* node){

    int begin = line->currAddr; PRNT_JMP_MARK(  "while", "НАЧАЛО ЦИКЛА");

    NodeProcess(line, node->left, DFLT);

    POP_REG(mRAX,                               "pop rax");
    EMIT_LST("\x48\x83\xF8\x00",                "cmp rax, 0");
    int adr = line->currAddr;
    EMIT(JZ); PRNT_JMP(                         "jz end_while", " ");
    int ptr = line->currAddr;
    EMIT("\x00\x00\x00\x00"); // space for filling later

    NodeProcess(line, node->right, DFLT);

    int jmpAddr = line->currAddr;
    EMIT(JMP); PRNT_JMP(                        "jmp while", "");
    DWORD_NUM((begin) - (jmpAddr + JMP_SIZE));
    *(int*)(line->x86buff + ptr) = (line->currAddr) - (adr + JZ_SIZE);
    PRNT_JMP_MARK(                              "end_while", " ");

    return OK;
}

static int ProcessDef(line_t* line, node_t* node){
    ProcessArgs(line, node);
    int funcId = node->left->left->data.id;

    PRNT(" ", "НАЧАЛО ОПРЕДЕЛЕНИЯ ФУНКЦИИ");
    int adr = line->currAddr;
    EMIT(JMP); PRNT_CUSTOM(                     "jmp def_func_end%d", funcId);
    int jmpAdr = line->currAddr;
    EMIT("\x00\x00\x00\x00");


    PRNT_CUSTOM(                                "def_func%d:", funcId);
    line->id[funcId].memAddr = line->currAddr;

    PUSH_REG(RBP,                               "push rbp");
    MOV_R2R(mRBP, mRSP,                         "mov rbp, rsp");

    NodeProcess(line, node->right, DFLT);

    *(int*)(line->x86buff + jmpAdr) = (line->currAddr) - (adr + JMP_SIZE);
    PRNT_CUSTOM(                                "def_func_end%d:", funcId);

    return OK;
}

static int ProcessCall(line_t* line, node_t* node){
    int funcId      = node->left->left->data.id;
    int frameSize   = line->id[funcId].stackFrameSize * 8;
    PRNT        (" ", "Я НАЧАЛО ВЫЗОВ ФУНКЦИИ");

    NodeProcess(line, node->left->right, DFLT);

    int callAdr = line->currAddr;
    int calleeAdr = line->id[node->left->left->data.id].memAddr;
    EMIT(CALL); PRNT_CUSTOM(                    "call def_func%d", funcId);
    DWORD_NUM(calleeAdr - (callAdr + CALL_SIZE));
    EMIT("\x48\x83\xC4"); PRNT_CUSTOM (         "add rsp, %d", frameSize);
    BYTE_NUM(frameSize);
    PUSH_REG(RAX,                               "push rax");

    return OK;
}

static int ProcessOp(line_t* line, node_t* node){
    int adr = 0, offset = 0;

    switch (node->data.op){
        // +
        case O_ADD:

            POP_REG(mRAX,               "pop rax");
            POP_REG(mRBX,               "pop rbx");
            EMIT_LST("\x48\x01\xD8",    "add rax, rbx");
            PUSH_REG(RAX,               "push rax");

            break;

        // -
        case O_SUB:

            POP_REG(mRAX,               "pop rax");
            POP_REG(mRBX,               "pop rbx");
            EMIT_LST("\x48\x29\xD8",    "sub rax, rbx");
            PUSH_REG(RAX,               "push rax");

            break;

        // *
        case O_MUL:

            POP_REG(mRAX,                   "pop rax");
            POP_REG(mRBX,                   "pop rbx");
            EMIT_LST("\x48\x0F\xAF\xC3",    "imul rax, rbx");
            PUSH_REG(RAX,                   "push rax");

            break;

        // /
        case O_DIV:

            POP_REG(mRAX,                                   "pop rax");
            EMIT_LST("\x48\x83\xF8\x00",                    "cmp rax, 0");
            EMIT(JL); PRNT_JMP(                             "jl div", "");
            DWORD_NUM(0x3 + JMP_SIZE); // 0x3 == SIZE_XOR
            EMIT_LST("\x48\x31\xD2",                        "xor rdx, rdx");
            EMIT(JMP); PRNT_JMP(                            "jmp div_end", "");
            DWORD_NUM(0x7);            // 0x7 == SIZE_MOV
            PRNT_JMP_MARK("div", "");
            EMIT_LST("\x48\xC7\xC2\xFF\xFF\xFF\xFF",        "mov rdx, -1");
            POP_REG(mRBX,                                   "pop rbx");
            EMIT_LST("\x48\xF7\xFB",                        "idiv rbx");
            PUSH_REG(RAX,                                   "push rax");

            break;

        // print
        case O_PNT:

            POP_REG(mRSI,           "pop rsi");
            adr = line->currAddr;
            EMIT_LST("\xE8",        "call [std + 8]");
            offset = OFFSET + line->stdFuncsOffset[1] - (adr + CALL_SIZE);
            DWORD_NUM(offset);

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

            POP_REG(mRAX,       "pop rax");
            MOV_R2R(mRSP, mRBP, "mov rsp, rbp");
            POP_REG(mRBP,       "pop rbp");
            EMIT_LST(RET,       "ret");

            break;

        // call
        case O_CAL:
            PRNT(" ", " ");
            break;

        // <
        case O_LES:

            POP_REG(mRAX,                       "pop rax");
            POP_REG(mRBX,                       "pop rbx");
            EMIT_LST("\x48\x39\xD8",            "cmp rax, rbx");
            EMIT(JL); PRNT_JMP(                 "jl less", "");
            DWORD_NUM(JMP_SIZE + PUSH_NUM_SIZE);
            PUSH_NUM(0x00) PRNT(                "push 0d");
            EMIT(JMP); PRNT_JMP(                "jmp less_end", "");
            DWORD_NUM(PUSH_NUM_SIZE);
            PRNT_JMP_MARK(                      "less", "");
            PUSH_NUM(0x01) PRNT(                "push 1d");
            PRNT_JMP_MARK(                      "less_end", "");

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

            POP_REG(mRAX,                       "pop rax");
            POP_REG(mRBX,                       "pop rbx");
            EMIT_LST("\x48\x39\xD8",            "cmp rax, rbx");
            EMIT(JZ); PRNT_JMP(                 "jz eq", "");
            DWORD_NUM(JMP_SIZE + PUSH_NUM_SIZE);
            PUSH_NUM(0x0); PRNT(                "push 0d");
            EMIT(JMP); PRNT_JMP(                "jmp eq_end", "");
            DWORD_NUM(PUSH_NUM_SIZE);
            PRNT_JMP_MARK(                      "eq", "");
            PUSH_NUM(0x01);PRNT(                "push 1d");
            PRNT_JMP_MARK(                      "eq_end", "");

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

            EMIT_LST("\x66\x0F\xEF\xC0",            "pxor xmm0, xmm0");
            POP_REG(mRAX,                           "pop rax");
            EMIT_LST( "\xF2\x48\x0F\x2A\xC0",       "cvtsi2sd xmm0, rax");
            EMIT_LST("\xF2\x0F\x51\xC0",            "sqrtsd xmm0, xmm0");
            EMIT_LST( "\xF2\x48\x0F\x2C\xC0",       "cvttsd2si rax, xmm0");
            PUSH_REG(RAX,                           "push rax");

            break;

        default:
            PRNT("unknown node", " ");
            break;
    }

    return OK;
}
