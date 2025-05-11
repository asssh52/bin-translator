#pragma once

#include "../hpp/structs.hpp"
#include "../hpp/colors.hpp"
#include "../hpp/ops.hpp"

const int MAX_TKNS_DMP = 64;

enum readerParams{
    ROOT  = 1,
    LEFT  = 2,
    RIGHT = 3,
};

int TreeDump            (line_t* line);
int TokensDump          (line_t* line);
int HTMLDumpGenerate    (line_t* line);
int HTMLGenerateHead    (line_t* line);

int DumpIds             (line_t* line, FILE* file);

int LoadTree            (line_t* line);
int FindOp              (char* word, int length);
int FindOpStd           (char* word, int len);
int FindOpByNum         (int num);

int CreateId    (line_t* line, char* word, int len);
int FindId      (line_t* line, char* word, int len);
