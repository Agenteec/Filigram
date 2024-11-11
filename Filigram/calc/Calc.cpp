#include "Calc.h"

void calculate(std::pair<std::vector<double>, std::vector<double>>& dots, const std::string& expression, double xBegin, double xEnd, double xStep, const std::map<std::string, double>& coefficients)
{
    mu::Parser parser;
    std::vector<std::shared_ptr<double>> cVals;
    for (const auto& [name, value] : coefficients) {
        cVals.push_back(std::make_shared<double>(value));
        parser.DefineVar(name, cVals.back().get());
    }




    parser.SetExpr(expression);

    double x = xBegin;
    parser.DefineVar("x", &x);

    for (; x <= xEnd; x += xStep) {
        
        double result = parser.Eval();
        dots.first.push_back(x);
        dots.second.push_back(result);
    }
}
