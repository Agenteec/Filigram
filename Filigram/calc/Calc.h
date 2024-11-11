#pragma once
#include <muParser.h>
#include <string>
#include <vector>
#include <map>


void calculate(std::pair<std::vector<double>, std::vector<double>>& dots, const std::string& expression = "sin(x)", double xBegin = -2 * 3.14159265359, double xEnd = 2 * 3.14159265359, double xStep = 0.1, const std::map<std::string, double>& coefficients = {});