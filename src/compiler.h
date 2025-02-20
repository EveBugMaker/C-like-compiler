#ifndef COMPILER_H
#define COMPILER_H

#endif // COMPILER_H

#pragma once
#include <bits/stdc++.h>
#include <iostream>
#include <unordered_set>
using namespace std;

const vector<string> key_word = {"int", "void", "if", "else", "while", "return"};

// 定义文法产生式结构体
struct Production
{
    string left;          // 左部
    vector<string> right; // 右部

    Production()
    {
    }
    Production(string l, vector<string> r) : left(l), right(r) {}

    bool operator==(const Production &other) const
    {
        return (
            left == other.left &&
            right == other.right);
    }
};

const int base = 100;

enum TOKENCODE
{
    TK_UNDEF = 0,
    // 关键字
    KW_INT,
    KW_VOID,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_RETURN,

    // 算符
    TK_PLUS,   // '+'  plus
    TK_MINUS,  // '-'  minus
    TK_STAR,   // '*'  star
    TK_DIV,    // '/'  divide
    TK_ASSIGN, // '='  assign
    TK_EQ,     // "==" equal
    TK_GT,     // '>'  greater than
    TK_GEQ,    // ">=" greater than and equal
    TK_LT,     // '<'  less than
    TK_LEQ,    // "<=" less than and equal
    TK_NEQ,    // "!=" not equal

    // bool
    TK_NOT, // "!"  not
    TK_AND, // "&&" and
    TK_OR,  // "||" or

    // 分隔符
    TK_SEM,    // ';'  semicolon
    TK_COM,    // ','  comma
    TK_DSLASH, // "//" double slash
    TK_LSLASH, // "/*" left slash
    TK_RSLASH, // "*/" right slash
    TK_LPA,    // '('  left parentheses
    TK_RPA,    // ')'  right parentheses
    TK_LBRK,   // '['  left bracket
    TK_RBRK,   // ']'  right bracket
    TK_LBRC,   // '{'  left brace
    TK_RBRC,   // '}'  right brace

    // 标识符
    TK_IDENT, // 自定义变量名、函数名等

    // 数字
    TK_DIG, //  0-9 digit

    // 结束符
    TK_END, // '#'

    // 辅助符号
    EPSILON,
    };

const map<string, TOKENCODE> token_mp = {
    {"+", TK_PLUS},
    {"-", TK_MINUS},
    {"*", TK_STAR},
    {"/", TK_DIV},
    {"!=", TK_NEQ},
    {">", TK_GT},
    {">=", TK_GEQ},
    {"<", TK_LT},
    {"<=", TK_LEQ},
    {"=", TK_ASSIGN},
    {"==", TK_EQ},
    {"#", TK_END},
    {"//", TK_DSLASH},
    {"/*", TK_LSLASH},
    {"*/", TK_RSLASH},
    {",", TK_COM},
    {";", TK_SEM},
    {"(", TK_LPA},
    {")", TK_RPA},
    {"[", TK_LBRK},
    {"]", TK_RBRK},
    {"{", TK_LBRC},
    {"}", TK_RBRC},
    {"idt", TK_IDENT},
    {"dig", TK_DIG},
    {"int", KW_INT},
    {"void", KW_VOID},
    {"if", KW_IF},
    {"else", KW_ELSE},
    {"while", KW_WHILE},
    {"return", KW_RETURN},
    {"", EPSILON},
    {"!", TK_NOT},
    {"||", TK_OR},
    {"&&", TK_AND},
    };

const int delta = 2;

const map<int, string> mp = {
    {1 + delta, "func"},
    {7 + delta, "get"},
    {8 + delta, "get"},
    {9 + delta, "declaration"},
    {15 + delta, "statements"},
    {21 + delta, "if"},
    {22 + delta, "if_else"},
    {23 + delta, "while"},
    {24 + delta, "return_void"},
    {25 + delta, "return"},
    {26 + delta, "assign"},
    // {27, "bool_expression"},
    // {28, "expression"},
    {31 + delta, "or"},
    {33 + delta, "and"},
    {35 + delta, "not"},
    {36 + delta, "bool"},
    {37 + delta, "relop"},
    {45 + delta, "cal_merge"},
    {48 + delta, "cal_merge"},
    {49 + delta, "cal_merge"},
    {51 + delta, "cal_merge"},
    {54 + delta, "cal_merge"},
    {55 + delta, "cal_merge"},
    {46 + delta, "operator"},
    {47 + delta, "operator"},
    {52 + delta, "operator"},
    {53 + delta, "operator"},
    {56 + delta, "dig"},
    {58 + delta, "idt"},
    {10 + delta, "bracket"},
    {11 + delta, "bracket"},
    {57 + delta, "bracket"},
    {59 + delta, "call"},
    {64 + delta, "param_list"},
    {65 + delta, "param1"},
    {66 + delta, "param2"},
    {67 + delta, "M"},
    {68 + delta, "N"},
};

struct FuncInfo
{
    string type;
    string name;
    int startAdd;
};

struct Token_table
{
    int line;
    string type;
    string token;
};

//void process(const vector<Token_table> &input);

void debug();

void init();

extern vector<Production> productions;
extern vector<map<string, pair<int, int>>> ACTION;
extern vector<map<string, int>> GOTO;

extern unordered_set<string> G;

extern vector<Production> productions;
extern vector<map<string, pair<int, int>>> ACTION;
extern vector<map<string, int>> GOTO;

extern unordered_set<string> G;

using info = pair<int, bool>;

struct info_code
{
    int num;
    info dst, l, r;
    info_code(int num) : num(num)
    {
        dst = l = r = {-1, false};
    };
};

struct Block
{
public:
    vector<info_code> icodes;
    int next1; // 跳转
    int next2; // 后继
};

struct ObjectCode
{
    string op;
    string src1;
    string src2;
    string src3;
    ObjectCode(const string &OP, const string &s1, const string &s2 = "", const string &s3 = "") : op(OP), src1(s1), src2(s2), src3(s3) {}

    friend ostream &operator<<(ostream &out, ObjectCode cd)
    {
        out << cd.op << ' ' << cd.src1;
        if (cd.src2 != "")
        {
            out << ", " << cd.src2;
        }
        if (cd.src3 != "")
        {
            out << ", " << cd.src3;
        }
        return out;
    }
};
