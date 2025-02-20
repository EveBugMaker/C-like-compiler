#include "compiler.h"
#include "mainwindow.h"

extern vector<Code> codes;
extern vector<FuncInfo> funcDeclaration;

bool is_number(const string &str)
{
    if (str == "")
        return false;
    else
        return str[0] >= '0' && str[0] <= '9';
}

bool is_var(const string &str)
{

    return str != "" && !is_number(str);
}

ObjectCodeGenerator::ObjectCodeGenerator()
{
    for (int i = 16; i <= 23; i++)
        freeReg.insert(i);
}

void ObjectCodeGenerator::get_basic_blocks()
{
    vector<int> entrys;
    int n = codes.size();
    int lNum = 0;
    for (int i = 0; i < funcDeclaration.size(); i++)
    {
        FuncInfo func = funcDeclaration[i];
        entrys.clear();
        entrys.push_back(func.startAdd);
        nowFunc = func.name;
        for (int j = func.startAdd + 1; j < n && j < (i == funcDeclaration.size() - 1 ? n : funcDeclaration[i + 1].startAdd); j++)
        {
            if (codes[j].op[0] == 'j')
            {
                int to = atoi(codes[j].dst.c_str()) - base;
                if (to >= 0)
                    entrys.push_back(to);
            }
            if (codes[j - 1].op[0] == 'j' && codes[j - 1].op != "j")
                entrys.push_back(j);
            if (codes[j - 1].op == "call")
                entrys.push_back(j);
        }
        sort(entrys.begin(), entrys.end());
        entrys.erase(unique(entrys.begin(), entrys.end()), entrys.end());
        map<int, int> entryToBlock;
        entryToBlock[0] = 0;
        for (int j = 1; j < entrys.size(); j++)
        {
            jmp_map[to_string(entrys[j] + base)] = "L" + to_string(lNum++);
            entryToBlock[entrys[j]] = j;
        }
        int blockCnt = entrys.size();
        funcBlocks[nowFunc].resize(blockCnt);
        for (int j = 0; j < blockCnt; j++)
        {
            funcBlocks[nowFunc][j].next1 = -1;
            if (j < blockCnt - 1)
                funcBlocks[nowFunc][j].next2 = j + 1;
            else
                funcBlocks[nowFunc][j].next2 = -1;
            int end;
            if (j == blockCnt - 1)
            {
                if (i == funcDeclaration.size() - 1)
                    end = codes.size();
                else
                    end = funcDeclaration[i + 1].startAdd;
            }
            else
                end = entrys[j + 1];
            for (int k = entrys[j]; k < end; k++)
            {
                funcBlocks[nowFunc][j].icodes.push_back({k});
                if (codes[k].op[0] == 'j')
                {
                    funcBlocks[nowFunc][j].next1 = entryToBlock[atoi(codes[k].dst.c_str()) - base];
                    break;
                }
                if (codes[k].op == "call")
                    break;
            }
        }
    }
}

void ObjectCodeGenerator::analyse_basic_blocks()
{
    for (auto &[func, blocks] : funcBlocks)
    {
        nowFunc = func;
        int blockCnt = blocks.size();
        vector<set<string>> inL(blockCnt), outL(blockCnt), def(blockCnt), use(blockCnt);
        for (int i = 0; i < blockCnt; i++)
        {
            auto block = blocks[i];
            for (auto infoCode : block.icodes)
            {
                Code code = codes[infoCode.num];
                if (code.op[0] != 'j' && code.op != "call" && code.op != "get" && use[i].count(code.dst) == 0)
                    def[i].insert(code.dst);
                if (is_var(code.src1) && def[i].count(code.src1) == 0)
                    use[i].insert(code.src1);
                if (is_var(code.src2) && def[i].count(code.src2) == 0)
                    use[i].insert(code.src2);
            }
            inL[i] = use[i];
        }

        // 确定inL和outL
        int tag = 1;
        while (tag)
        {
            tag = 0;
            for (int i = 0; i < blockCnt; i++)
            {
                auto block = blocks[i];
                if (block.next1 != -1)
                {
                    for (auto var : inL[block.next1])
                    {
                        if (outL[i].insert(var).second)
                        {
                            if (def[i].count(var) == 0)
                                inL[i].insert(var);
                            tag = 1;
                        }
                    }
                }
                if (block.next2 != -1)
                {
                    for (auto var : inL[block.next2])
                    {
                        if (outL[i].insert(var).second)
                        {
                            if (def[i].count(var) == 0)
                                inL[i].insert(var);
                            tag = 1;
                        }
                    }
                }
            }
        }
        funcIn[func] = inL;
        funcOut[func] = outL;
        funcSymbol[func].resize(blocks.size());
        for (int i = 0; i < blocks.size(); i++)
        {
            for (auto var : outL[i]) // outL中都是出基本块的活跃变量
            {
                funcSymbol[func][i][var].push_back({-1, true});
            }
        }

        /* 这里有问题 */
        for (int i = 0; i < blocks.size(); i++)
        {
            auto &block = blocks[i];
            for (int j = block.icodes.size() - 1; j >= 0; j--)
            {
                info_code &iCode = block.icodes[j];
                Code code = codes[iCode.num];
                if (code.op == "j" || code.op == "call")
                    continue;
                string A, B, C;
                A = code.dst, B = code.src1, C = code.src2;
                if (is_var(B))
                {
                    if (funcSymbol[nowFunc][i][B].size() == 0)
                        funcSymbol[nowFunc][i][B].push_back({-1, false});
                    iCode.l = *funcSymbol[nowFunc][i][B].rbegin();
                    funcSymbol[nowFunc][i][B].push_back({j, true});
                }
                if (is_var(C))
                {
                    if (funcSymbol[nowFunc][i][C].size() == 0)
                        funcSymbol[nowFunc][i][C].push_back({-1, false});
                    iCode.r = *funcSymbol[nowFunc][i][C].rbegin();
                    funcSymbol[nowFunc][i][C].push_back({j, true});
                }
                if (is_var(A) && code.op[0] != 'j')
                {
                    if (funcSymbol[nowFunc][i][A].size() == 0)
                        funcSymbol[nowFunc][i][A].push_back({-1, false});
                    iCode.dst = *funcSymbol[nowFunc][i][A].rbegin();
                    funcSymbol[nowFunc][i][A].push_back({-1, false});
                }
            }
        }
    }
}

int ObjectCodeGenerator::alloc_reg()
{
    int R;
    // 先找空的
    if (freeReg.size())
    {
        R = *freeReg.rbegin();
        freeReg.erase(R);
        return R;
    }

    int _max = -1;
    int blockIndex = nowBlock - funcBlocks[nowFunc].begin();
    for (auto [i, v] : Rvalue)
    {
        int tag = 1;
        int _min = 2e9;
        for (auto var : Rvalue[i])
        {
            if (Avalue[var].size() <= 1)
                tag = 0;
            if (funcSymbol[nowFunc][blockIndex][var].rbegin()->first != -1)
                _min = min(funcSymbol[nowFunc][blockIndex][var].rbegin()->first, _min);
        }
        if (tag) // Rvalue[R]中的所有变量都在别处记录了
        {
            R = i;
            break;
        }
        if (_min > _max)
        {
            R = i;
            _max = _min;
        }
    }

    // 处理原住民
    for (auto var : Rvalue[R])
    {
        Avalue[var].erase(R);
        if (Avalue[var].size() == 0)
        {
            int tag = 1, tag1 = 0;
            for (auto it = nowCode; it != nowBlock->icodes.end(); it++)
            {
                if (codes[it->num].src1 == var || codes[it->num].src2 == var)
                {
                    tag = 1;
                    tag1 = 1;
                    break;
                }
                if (codes[it->num].dst == var)
                {
                    tag = 0;
                    tag1 = 1;
                    break;
                }
            }

            if (!tag1)
            {
                int bIndex = nowBlock - funcBlocks[nowFunc].begin();
                if (funcOut[nowFunc][bIndex].count(var))
                    tag = 1;
                else
                    tag = 0;
            }

            if (tag)
                store_var(R, var);
        }
    }
    Rvalue[R].clear();

    return R;
}

int ObjectCodeGenerator::alloc_reg(const string &var)
{
    if (is_number(var))
    {
        if (var == "0")
            return 0;
        else
        {
            int R = alloc_reg();
            objCodes.push_back(ObjectCode("addi", "$" + to_string(R), "$0", var));
            return R;
        }
    }
    for (int R : Avalue[var])
    {
        if (R != -1)
            return R;
    }

    int R = alloc_reg();
    objCodes.push_back(ObjectCode("lw", "$" + to_string(R), to_string(varOffset[var]) + "($sp)"));
    Avalue[var].insert(R);
    Rvalue[R].clear();
    Rvalue[R].insert(var);

    return R;
}

int ObjectCodeGenerator::get_reg()
{
    Code code = codes[nowCode->num];
    string A = code.dst, B = code.src1, C = code.src2;
    if (is_var(B))
    {
        for (int R : Avalue[B])
        {
            if (R == -1)
                continue;
            if (Rvalue[R].size() == 1)
            {
                if (A == B || nowCode->l.second == false)
                {
                    Rvalue[R].insert(A);
                    Avalue[A].insert(R);
                    return R;
                }
            }
        }
    }

    int R = alloc_reg();
    Avalue[A].insert(R);
    Rvalue[R].insert(A);
    return R;
}

void ObjectCodeGenerator::store_var(int reg, const string &var)
{
    if (varOffset.count(var))
    {
        objCodes.push_back(ObjectCode("sw", "$" + to_string(reg), to_string(varOffset[var]) + "($sp)"));
    }
    else
    {
        varOffset[var] = top;
        top += 4;
        objCodes.push_back(ObjectCode("sw", "$" + to_string(reg), to_string(varOffset[var]) + "($sp)"));
    }
    Avalue[var].insert(-1);
}

bool ObjectCodeGenerator::generate_objCode()
{
    Code code = codes[nowCode->num];

    if (code.op[0] != 'j' && code.op != "call")
    {
        if (is_var(code.src1) && Avalue[code.src1].empty())
        {
            QMessageBox::warning(nullptr, "Error", QString::fromStdString("引用了未赋值的变量：" + code.src1));
            return false;
        }
        if (is_var(code.src2) && Avalue[code.src2].empty())
        {
            QMessageBox::warning(nullptr, "Error", QString::fromStdString("引用了未赋值的变量：" + code.src2));
            return false;
        }
    }

    if (nowCode == nowBlock->icodes.begin() && nowBlock != funcBlocks[nowFunc].begin())
        objCodes.push_back(ObjectCode(jmp_map[to_string(base + nowCode->num)] + ":", ""));

    if (code.op == "j")
    {
        objCodes.push_back(ObjectCode("j", jmp_map[code.dst]));
    }
    else if (code.op[0] == 'j')
    {
        string op;
        string src1, src2;
        if (code.op == "j==")
            op = "beq";
        else if (code.op == "j!=")
            op = "bne";
        else if (code.op == "j>")
            op = "bgt";
        else if (code.op == "j>=")
            op = "bge";
        else if (code.op == "j<")
            op = "blt";
        else if (code.op == "j<=")
            op = "ble";
        else if (code.op == "jnz")
            op = "bne", src2 = "$0";
        src1 = "$" + to_string(alloc_reg(code.src1));
        if (code.op != "jnz")
            src2 = "$" + to_string(alloc_reg(code.src2));
        objCodes.push_back(ObjectCode(op, src1, src2, jmp_map[code.dst]));
        if (!nowCode->l.second)
            release_var(code.src1);
        if (!nowCode->r.second)
            release_var(code.src2);
    }
    else if (code.op == "par")
    {
        parList.push_back({code.src1, nowCode->l.second});
    }
    else if (code.op == "call")
    {
        for (int i = 0; i < parList.size(); i++)
        {
            int R = alloc_reg(parList[i].first);
            objCodes.push_back(ObjectCode("sw", "$" + to_string(R), to_string(top + (i + 2) * 4) + "($sp)"));
            if (!parList[i].second)
                release_var(parList[i].first);
        }
        parList.clear();

        objCodes.push_back(ObjectCode("sw", "$sp", to_string(top) + "($sp)"));
        objCodes.push_back(ObjectCode("addi", "$sp", "$sp", to_string(top)));

        objCodes.push_back(ObjectCode("jal", code.dst));

        objCodes.push_back(ObjectCode("lw", "$sp", "0($sp)"));
    }
    else if (code.op == "get")
    {
        varOffset[code.dst] = top;
        top += 4;
        Avalue[code.dst].insert(-1);
    }
    else if (code.op == ":=")
    {
        int R;
        if (code.src1 == "@")
            R = 2;
        else
            R = alloc_reg(code.src1);
        Rvalue[R].insert(code.dst);
        Avalue[code.dst].clear();
        Avalue[code.dst].insert(R);
    }
    else if (code.op == "return")
    {
        if (code.dst != "")
        {
            if (is_number(code.dst))
                objCodes.push_back(ObjectCode("addi", "$2", "$0", code.dst));
            else if (is_var(code.dst))
            {
                int R = *Avalue[code.dst].begin();
                if (R == -1)
                    objCodes.push_back(ObjectCode("lw", "$2", to_string(varOffset[code.dst]) + "($sp)"));
                else
                    objCodes.push_back(ObjectCode("add", "$2", "$0", "$" + to_string(R)));
            }
        }
        objCodes.push_back(ObjectCode("lw", "$ra", "4($sp)"));
        objCodes.push_back(ObjectCode("jr", "$ra"));
    }
    else if (code.op == "+" || code.op == "-" || code.op == "*" || code.op == "/")
    {
        string src1 = "$" + to_string(alloc_reg(code.src1));
        string src2 = "$" + to_string(alloc_reg(code.src2));
        string dst = "$" + to_string(get_reg());
        if (code.op == "+")
        {
            objCodes.push_back(ObjectCode("add", dst, src1, src2));
        }
        else if (code.op == "-")
        {
            objCodes.push_back(ObjectCode("sub", dst, src1, src2));
        }
        else if (code.op == "*")
        {
            objCodes.push_back(ObjectCode("mul", dst, src1, src2));
        }
        else if (code.op == "/")
        {
            objCodes.push_back(ObjectCode("div", src1, src2));
            objCodes.push_back(ObjectCode("mflo", dst));
        }
        if (!nowCode->l.second)
            release_var(code.src1);
        if (!nowCode->r.second)
            release_var(code.src2);
    }
    if (funcSymbol[nowFunc][nowBlock - funcBlocks[nowFunc].begin()].count(code.dst))
        funcSymbol[nowFunc][nowBlock - funcBlocks[nowFunc].begin()][code.dst].pop_back();
    if (funcSymbol[nowFunc][nowBlock - funcBlocks[nowFunc].begin()].count(code.src1))
        funcSymbol[nowFunc][nowBlock - funcBlocks[nowFunc].begin()][code.src1].pop_back();
    if (funcSymbol[nowFunc][nowBlock - funcBlocks[nowFunc].begin()].count(code.src2))
        funcSymbol[nowFunc][nowBlock - funcBlocks[nowFunc].begin()][code.src2].pop_back();

    return true;
}

bool ObjectCodeGenerator::generate_block_objCodes()
{
    Avalue.clear();
    Rvalue.clear();
    for (auto var : funcIn[nowFunc][nowBlock - funcBlocks[nowFunc].begin()])
    {
        Avalue[var].insert(-1);
    }

    freeReg.clear();
    for (int i = 16; i <= 23; i++)
        freeReg.insert(i);

    bool ret;
    for (auto it = nowBlock->icodes.begin(); it != nowBlock->icodes.end(); it++)
    {
        nowCode = it;
        if (it == nowBlock->icodes.end() - 1)
        {
            if (codes[nowCode->num].op[0] == 'j' || codes[nowCode->num].op == "call" || codes[nowCode->num].op == "return")
            {
                store_outvar();
                ret = generate_objCode();
            }
            else
            {
                ret = generate_objCode();
                store_outvar();
            }
        }
        else
            ret = generate_objCode();
        if (!ret)
            return false;
    }

    return true;
}

bool ObjectCodeGenerator::generate_func_objCodes()
{
    varOffset.clear();
    objCodes.push_back(ObjectCode(nowFunc + ":", ""));
    objCodes.push_back(ObjectCode("sw", "$ra", "4($sp)"));
    top = 8;
    for (auto it = funcBlocks[nowFunc].begin(); it != funcBlocks[nowFunc].end(); it++)
    {
        nowBlock = it;
        bool ret = generate_block_objCodes();
        if (!ret)
            return false;
    }

    return true;
}

void ObjectCodeGenerator::generate_objCodes()
{
    objCodes.push_back(ObjectCode("j", "main"));
    for (auto func : funcDeclaration)
    {
        nowFuncInfo = func;
        nowFunc = func.name;
        generate_func_objCodes();
    }
}

void ObjectCodeGenerator::process(QTextBrowser *obj_code, TextBrowserStreambuf *textBrowserStreambuf_obj_code)
{
    get_basic_blocks();
    analyse_basic_blocks();
    generate_objCodes();
    show_codes(obj_code, textBrowserStreambuf_obj_code);
}

void ObjectCodeGenerator::release_var(const string &var)
{
    for (auto R : Avalue[var])
    {
        if (R == -1)
            continue;
        Rvalue[R].erase(var);
        if (Rvalue[R].size() == 0)
        {
            freeReg.insert(R);
        }
    }
    Avalue[var].clear();
}

void ObjectCodeGenerator::store_outvar()
{
    for (auto var : funcOut[nowFunc][nowBlock - funcBlocks[nowFunc].begin()])
    {
        int R;
        for (auto i : Avalue[var])
        {
            if (i == -1)
            {
                R = -1;
                break;
            }
            else
                R = i;
        }
        if (R != -1)
            store_var(R, var);
    }
}

void ObjectCodeGenerator::show_codes(QTextBrowser *obj_code, TextBrowserStreambuf *textBrowserStreambuf_obj_code)
{
    obj_code->clear();
    cout.rdbuf(textBrowserStreambuf_obj_code);
    for (auto cd : objCodes)
    {
        cout << cd << endl;
    }
}
