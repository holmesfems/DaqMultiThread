/*
* paramSet.cpp
* Modified from https://github.com/holmesfems/PasswordBook/blob/master/src/cmdSeparator.cpp
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/
#include "ParamSet.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include "stringTool.h"
namespace ParamSet
{
	const std::string space = " ";
	void ParamHelper::bind(const std::string &key, void *target, int8_t type)
	{
		VariableBindItem vbi(target, type);
		_vb[key] = vbi;
		//_vb.insert(std::pair<std::string, VariableBindItem>(key, vbi));
	}

	void ParamHelper::bind(const char *key, void *target, int8_t type)
	{
		std::string keystr = key;
		bind(keystr, target, type);
	}

	void ParamHelper::set(Params &params)
	{
		for (auto item : params) {
			VariableBind::iterator iter;
			if ((iter = _vb.find(item.first)) != _vb.end()) {
				VariableBindItem vbi = iter->second;
				switch (vbi.second) {
				case INTEGER:
					*((int32_t *)(vbi.first)) = StringTool::convertTo<int32_t>(item.second);
					break;
				case TEXT:
					*((std::string *)(vbi.first)) = item.second;
					break;
				case FLOAT64:
					*((double_t *)(vbi.first)) = StringTool::convertTo<double_t>(item.second);
					break;
				}
			}
		}
	}

	ParamItem strToItem(const std::string &equation, char _escape, char _equal)
	{
		// static const char _strBracket='"';
		std::string trimCmd = StringTool::strTrim(equation);
		// bool first = true;
		// bool inStr=false;
		// bool inEsc=false;
		// bool inSpace=false;
		// bool needEnd=true;
		bool inEqual = false;
		const char *data = trimCmd.c_str();
		size_t i, maxi = trimCmd.length();
		std::ostringstream oss;
		std::string key = "" , value;
		// std::string cmd;
		//Params ret;
		for (i = 0; i < maxi; i++) {
#ifdef DEBUG
			std::cout << '\'' << data[i] << '\'' << std::endl;
#endif
			if (data[i] == _escape) {
				i += 1;
				//assert(!(i >= maxi));
				char *hexcode;
				std::ostringstream oss2;
				switch (data[i]) {
				case 'v':
					oss << '\v';
					break;
				case 'n':
					oss << '\n';
					break;
				case 't':
					oss << '\t';
					break;
				case 'r':
					oss << '\r';
					break;
				case 'x':
					//assert(!(i + 2 >= maxi));
					oss2 << data[i + 1] << data[i + 2];
					hexcode = StringTool::strToBin(oss2.str());
					oss << hexcode[0];
					free(hexcode);
					i += 2;
					break;
				default:
					oss << data[i];
				}
				// inEsc = false;
				continue;
			}
			if (data[i] == _equal) {
				//assert(!(i == maxi - 1));
				if (!inEqual) {
					key = oss.str();
					oss.str("");
					inEqual = true;
				}
				else {
					oss << data[i];
				}
				continue;
			}
			oss << data[i];
		// TODO
		}  // for
		if (value == "" && oss.str() != "") {
			value = oss.str();
			//ParamItem item(key, value);
			//ret.push_back(item);
		}
#ifdef DEBUG
		std::cout << cmd << ":" << std::endl;
		for (auto item : ret) {
			std::cout << item.first << "," << item.second << std::endl;
		}
#endif
		return ParamItem(key, value);
	}

	ParamItem strToItem(const char *equation, char _escape, char _equal)
	{
		std::string strequ = std::string(equation);
		return strToItem(strequ,_escape,_equal);
	}
}
