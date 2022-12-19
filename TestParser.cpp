#include <iostream>
#include <regex>

#include "shunting-yard.h"
using namespace cparse;

// 传递函数调用参数
struct STest
{
    int value;
};

packToken fibonacci(TokenMap scope) {
    int N = scope["N"].asInt();
    if (N == 0) return 0;
    if (N == 1) return 1;

    scope["N"] = N - 1;
    int result = fibonacci(scope).asInt();
    scope["N"] = N - 2;
    return result + fibonacci(scope).asInt();
}

packToken TestPoint(TokenMap scope) {
    TokenMap& para = scope["this"].asMap();
    auto test = para["point"].asPoint();
    STest *pTest = static_cast<STest*>(test);
    return pTest->value;
}


packToken TestGroupFun(TokenMap scope) {
    auto Key = scope["Key"].asString();
    auto Div = scope["Div"].asInt();
    auto Index = 0;
    auto& Paras = scope["this"].asMap();
    if (Paras.find("Index"))
    {
        Index = Paras["Index"].asInt();
    }

    std::cout << "Key=" << Key << " Div=" << Div << " Index=" << Index  << std::endl;

    return Index;
}

void Split(std::string& Str, std::string& Delim, std::vector<std::string>& Result)
{
    char* TempStr = new char[Str.length()+1];
    TempStr[Str.length()] = 0;
    memcpy(TempStr, Str.c_str(), Str.length());
    const char* TempDelim = Delim.c_str();

    char *Buf;
    char*  Token = strtok_s(TempStr, TempDelim, &Buf);
    while (Token != nullptr)
    {
        Result.push_back(Token);
        Token = strtok_s(nullptr, TempDelim, &Buf);
    }

    delete [] TempStr;
}

void SplitString(const std::string& Str, const std::string& Delim, std::vector<std::string>& Result)
{
    char* TempStr = new char[Str.length()+1];
    TempStr[Str.length()] = 0;
    memcpy(TempStr, Str.c_str(), Str.length());
    const char* TempDelim = Delim.c_str();

    char *Buf;
    char*  Token = strtok_s(TempStr, TempDelim, &Buf);
    while (Token != nullptr)
    {
        Result.push_back(Token);
        Token = strtok_s(nullptr, TempDelim, &Buf);
    }

    delete [] TempStr;
}

void GetPropertyIDList(const std::string &PropertyIDExp, std::vector<int> &ValueVec)
{
    std::string Delim(" ");
    std::vector<std::string> Result;
    SplitString(PropertyIDExp, Delim, Result);
    for (int i=0; i<Result.size(); i++)
    {
        auto &Str = Result[i];
        const auto Pos = Str.find("-");
        if ( Pos != std::string::npos)
        {
            auto SubStr = Str.substr(Pos+1);
            const auto Begin = atoi(Str.c_str());
            const auto End = atoi(SubStr.c_str());
            for (int j=Begin; j<=End; j++)
                ValueVec.push_back(j);
        }
        else
        {
            ValueVec.push_back(atoi(Str.c_str()));
        }
    }
}

void TestExpression()
{
    std::string Exp = "@id=5-8 10 15";
    const std::regex RegMatch(R"(@{1}[A-Za-z]+={1}"?[\d-\s]+"?$)");    // 非严格
    auto IsMatch = std::regex_match(Exp, RegMatch);
    auto Test = IsMatch;
}


bool IsMultiPropertyExpression(const std::string& Exp)
{
    const std::regex RegMatch(R"(@{1}[A-Za-z]+={1}[a-zA-Z\d,\s\*\?/"]+$)");    // 非严格
    return std::regex_match(Exp, RegMatch);
}

void TestMultiExpression()
{
    // std::string Exp = "@id=5,8,10,15";
    std::string Exp = "@GeoIcon=Hello/Test/Group/?1";
    auto IsMatch = IsMultiPropertyExpression(Exp);
    auto Test = IsMatch;
}


// 将*和？转化成正则
bool ProcessPattenInGroupRef(std::string& Reg, char C, const std::string& Rep)
{
    auto Pos = Reg.find_first_of(C);
    if (Pos != std::string::npos)
    {
        do
        {
            Reg.replace(Pos, 1,Rep);
            auto NewPos = Pos+Rep.length();
            Pos = Reg.find_first_of(C, NewPos);
        }
        while (Pos != std::string::npos);

        return true;
    }
    return false;
}

bool IsMatch(const std::string& Str, const std::string& Patten)
{
    auto Reg(Patten);
    const bool SpecialPatten = ProcessPattenInGroupRef(Reg, '*', "\\S*")
    || ProcessPattenInGroupRef(Reg, '?', "\\S{1}");
    if (SpecialPatten)
    {
        Reg = "^" + Reg +"$";

        const std::regex RegMatch(Reg);
        return std::regex_match(Str, RegMatch);
    }
    else
    {
        return Str == Patten;
    }
    
}

void TestStringMatch()
{
    {
        // std::string String1("/foo/");
        // std::string String2("/foo/a");
        // std::string String3("/foo/*");
        // std::string String4("/foo");
        // std::string String5("/foo/");
        std::string String6("/foo/21");

        std::string Pattern("/foo/**");

        // auto m1 = IsMatch(String1, Pattern);
        // auto m2 = IsMatch(String2, Pattern);
        // auto m3 = IsMatch(String3, Pattern);
        // auto m4 = IsMatch(String4, Pattern);
        // auto m5 = IsMatch(String5, Pattern);
        auto m6 = IsMatch(String6, Pattern);

        int v = 0;
    }

}

int main_match()
{
    TestStringMatch();

    TestMultiExpression();

#if 0
    std::string Str("@id=\"5-8 10 15\"");
    std::string Delim("@=");
    std::vector<std::string> Result;
    Split(Str, Delim, Result);

    const std::string &PropertyName = Result[0];
    std::string &PropertyIDs = Result[1];
    if (PropertyIDs.length() >=2 && PropertyIDs[0] == 0x22 && PropertyIDs[PropertyIDs.length()-1] == 0x22)
    {
        PropertyIDs = PropertyIDs.substr(1, PropertyIDs.length()-2);
    }

    std::vector<int> ValueVec;
    GetPropertyIDList(Result[1], ValueVec);
#endif
    
    // TestExpression();
    return 0;
}


int main() {
    
    // 将函数定义放到全局
    TokenMap& global = TokenMap::default_global();
    global["TestPoint"] = CppFunction(&TestPoint, {""}, "TestPoint");

    STest test;
    void *p = &test;
    calculator c1;
    c1.compile("TestPoint() + 0.5 < 5");

    // 参数由参数传递
    TokenMap paraMap;
    // paraMap["TestPoint"] = CppFunction(&TestPoint, {""}, "TestPoint");
    paraMap["point"] = p;
    for (int i=0; i<10;  i++)
    {
        test.value = i;
        // auto value = calculator::calculate();

        std::cout << "value=" << test.value << " compare=" << c1.eval(paraMap) << std::endl; // 
    }
    

    {
        TokenMap& global = TokenMap::default_global();
        global["fibonacci"] = CppFunction(&fibonacci, { "N", "div", "Name" }, "fib_name");
        std::cout << "fibonacci(10)=" << calculator::calculate("fibonacci(10, 3, hello) > 0") << std::endl;
    }

    // {
    //     TokenMap vars;
    //     vars["fibonacci"] = CppFunction(&fibonacci, { "N", "div", "Name" });
    //     vars["N"] = 12;
    //     std::cout << "fibonacci(10)=" << calculator::calculate("fibonacci(10, 3, hello)", &vars) << std::endl;
    // }


    //{
    //    TokenMap vars;
    //    vars["TestGroup"] = CppFunction(&TestGroupFun, { "Key", "Div", "Name" });
    //    std::cout << "TestGroup(key, div)=" << calculator::calculate("TestGroup(key, 3)", &vars) << std::endl;
    //}

    {
        TokenMap vars;
        vars["TestGroup"] = CppFunction(&TestGroupFun, { "Key", "Div", "Index2" });
        //global["TestGroup"] = CppFunction(&TestGroupFun, { "Key", "Div", "Name" });
    
        calculator calc;
        calc.compile("TestGroup(key, 3)-1");
    
        for (int i=0; i<5; i++)
        {
            vars["Index"] = i;
            try
            {
               std::cout << i << " "<< calc.eval(vars, true) << std::endl;
            }
            catch (std::exception* e)
            {
                auto exp = e->what();
            }
        }
    
    
        //std::cout << "TestGroup(key, div)=" << calculator::calculate("TestGroup(key, 3)", &vars) << std::endl;
    }

    return 0;
}