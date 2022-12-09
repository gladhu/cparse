#include <iostream>
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

    return 0;
}

int main() {
    
    // 将函数定义放到全局
    TokenMap& global = TokenMap::default_global();
    global["TestPoint"] = CppFunction(&TestPoint, {""}, "TestPoint");

    STest test;
    void *p = &test;
    calculator c1;
    c1.compile("TestPoint() < 5");

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
    

    // {
    //     TokenMap& global = TokenMap::default_global();
    //     global["fibonacci"] = CppFunction(&fibonacci, { "N", "div", "Name" }, "fib_name");
    //     std::cout << "fibonacci(10)=" << calculator::calculate("fibonacci(10, 3, hello) > 0") << std::endl;
    // }

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
        calc.compile("TestGroup(key, 3)");

        for (int i=0; i<5; i++)
        {
            vars["Index"] = i;
            try
            {
                calc.eval(vars, true);

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