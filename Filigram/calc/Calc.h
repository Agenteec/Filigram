#ifdef _WIN32
#ifdef FILIGRAMCALC_EXPORTS
#define FILIGRAMCALC_API __declspec(dllexport)
#else
#define FILIGRAMCALC_API __declspec(dllimport)
#endif
#else
#define FILIGRAMCALC_API
#endif
#pragma once
#include <muParser.h>
#include <string>
#include <vector>
#include <map>


FILIGRAMCALC_API void calculate(std::pair<std::vector<double>, std::vector<double>>& dots, const std::string& expression = "sin(x)", double xBegin = -2 * 3.14159265359, double xEnd = 2 * 3.14159265359, double xStep = 0.1, const std::map<std::string, double>& coefficients = {});