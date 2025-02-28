#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLabel>
#include <QTextBrowser>
#include <QPushButton>
#include <QTextStream>
// #include <QTextCodec>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDebug>
#include "compiler.h"

//string epsilon_ = to_string(0x03B5);

class TextBrowserStreambuf : public std::streambuf
{
public:
    TextBrowserStreambuf(QTextBrowser* textBrowser) : textBrowser_(textBrowser)
    {
    }

protected:
    virtual int_type overflow(int_type c = traits_type::eof())
    {
        if (c != traits_type::eof())
        {
            char ch = traits_type::to_char_type(c);
            textBrowser_->moveCursor(QTextCursor::End);
            textBrowser_->insertPlainText(QString(ch));
        }
        return c;
    }

private:
    QTextBrowser* textBrowser_;
};

struct Code
{
    string op;
    string src1;
    string src2;
    string dst;

    Code(){}

    Code(const string &op, const string &src1, const string &src2, const string &dst)
        : op(op), src1(src1), src2(src2), dst(dst){};

    friend ostream &operator<<(ostream &out, const Code &cd)
    {
        out << "("
            << cd.op << ","
            << (cd.src1 == "" ? "_" : cd.src1) << ","
            << (cd.src2 == "" ? "_" : cd.src2) << ","
            << (cd.dst == "" ? "_" : cd.dst) << ")";
        return out;
    }
};

class ObjectCodeGenerator
{
public:
    static const int regs_num = 32;
    map<string, string> jmp_map;
    map<string, vector<Block>> funcBlocks;
    map<string, vector<map<string, vector<info>>>> funcSymbol;
    map<string, set<int>> Avalue;
    map<int, set<string>> Rvalue;
    map<string, vector<set<string>>> funcOut;
    map<string, vector<set<string>>> funcIn;
    string nowFunc;
    FuncInfo nowFuncInfo;
    set<int> freeReg;
    vector<Block>::iterator nowBlock;
    vector<info_code>::iterator nowCode;
    vector<ObjectCode> objCodes;
    map<string, int> varOffset;
    vector<pair<string, bool>> parList;
    int top;

    // function
    ObjectCodeGenerator();
    void get_basic_blocks();
    void analyse_basic_blocks();
    int  alloc_reg();
    int  alloc_reg(const string &var);
    void store_var(int reg, const string &var);
    bool generate_objCode();
    bool generate_block_objCodes();
    bool generate_func_objCodes();
    void generate_objCodes();
    int  get_reg();
    void process(QTextBrowser* obj_code, TextBrowserStreambuf* textBrowserStreambuf_obj_code);
    void release_var(const string &var);
    void store_outvar();
    void show_codes(QTextBrowser* obj_code, TextBrowserStreambuf* textBrowserStreambuf_obj_code);
};

extern vector<Code> codes;

extern vector<string> declarations;

extern vector<string> vars;

extern vector<FuncInfo> funcDeclaration;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct State
{
    string place;
    vector<int> true_list;
    vector<int> false_list;
    vector<int> next_list;
    vector<string> par_list;
    int next_quad;
    int line_count;

    State(const string &place = "", const int line_count = 0, const vector<int> &true_list = {}, const vector<int> &false_list = {}, const vector<int> &next_list = {}, const int &next_quad = {}, const vector<string> &par_list = {})
        : place(place), true_list(true_list), false_list(false_list), next_list(next_list), par_list(par_list), next_quad(next_quad), line_count(line_count){};

    friend ostream &operator<<(ostream &out, const State &st)
    {
        out << st.place << endl;
        return out;
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void dockInit();
    int main1(TextBrowserStreambuf *textBrowserStreambuf_state,TextBrowserStreambuf* textBrowserStreambuf_symbol,TextBrowserStreambuf* textBrowserStreambuf_input,TextBrowserStreambuf* textBrowserStreambuf_code,TextBrowserStreambuf* textBrowserStreambuf_obj_code,int go_end);
    void process(const vector<Token_table> &input,TextBrowserStreambuf *textBrowserStreambuf_state,TextBrowserStreambuf* textBrowserStreambuf_symbol,TextBrowserStreambuf* textBrowserStreambuf_input,TextBrowserStreambuf* textBrowserStreambuf_code,TextBrowserStreambuf* textBrowserStreambuf_obj_code,int go_end);
    bool assign_statement(State src, State dst);
    bool declaration_statement(State v);
    bool is_init;

public slots:
    void help();
    void newFile();//新建文件槽函数
    void read();
    void show_yufa(); // 语法分析界面
    void show_code(); // 中间代码界面
    void show_obj_code(); // 目标代码界面
    void save();//保存槽函数
    void save2();//另存为槽函数

    //打开文件槽函数
    void on_actionOpen_triggered();
    //新建文件槽函数
    void on_actionNew_triggered();
    //    //全部执行槽函数
    //    void on_actionRun_triggered();
    //    //单步执行槽函数
    //    void on_actionOne_triggered();
    //保存文件槽函数
    void on_actionSave_triggered();
    //另存为文件槽函数
    void on_actionSave2_triggered();
    //工具栏槽函数
    void on_actionTool_triggered(bool checked);
    //编译结果槽函数
    void on_actionBian_triggered(bool checked);
    //关于Qt槽函数
    void on_actionQt_triggered();
    //关于作者槽函数
    void on_actionAuthor_triggered();
    //关闭程序槽函数
    void on_actionClose_triggered();

    void on_actionSave_obj_code_triggered();
    void on_actionSave_code_triggered();

    void save_code();
    void save_obj_code();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::MainWindow *ui;
    string fileName;//打开文件名
    QDockWidget *leftdock,*rightdock;//dock窗口
    QTextEdit *textEdit;//edit窗口
    QToolBar *rightToolBar;//右侧窗口工具栏
    QTextBrowser *input_,*state,*symbol;//语法分析窗口
    QTextBrowser *code;//中间代码窗口
    QTextBrowser *obj_code;
    QPushButton *portBtn_yufa, *portBtn_code, *portBtn_obj_code;//rightdock中的控制按钮
    QPushButton *btn_save_code, *btn_save_obj_code;
    QLabel *Linput,*Lstate,*Lsymbol,*Lcode, *Lobjcode;//各个textbrowser的标签
    QPushButton *portBtn_run,*portBtn_one;//全部执行，单步执行两个按钮
    int one_used;
    bool done;
    //连接信号和槽函数
    void myConnect();
};
#endif // MAINWINDOW_H
