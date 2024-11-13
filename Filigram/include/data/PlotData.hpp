#pragma once
#include <string>
#include <vector>
#include <map>

class PlotData
{
public:
	PlotData() {}
    PlotData(double xStep = 0, double xBegin = 0, double xEnd = 0, const std::map<std::string, double>& coefficients = {},const std::string& expression = "sin(x)", const std::pair<std::vector<double>, std::vector<double>>& dots = {}) :
        xStep(xStep),
        xBegin(xBegin),
        xEnd(xEnd),
        coefficients(coefficients),
        expression(expression),
        dots(dots)
    {}

    std::map<std::string, double> coefficients = {};
    std::pair<std::vector<double>, std::vector<double>> dots = {};
    std::string expression = "sin(x)";
    double xBegin = 0;
    double xEnd = 0;
    double xStep = 0;
};