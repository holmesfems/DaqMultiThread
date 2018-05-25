/*
* stringTool_util.h
* Original reference:https://github.com/holmesfems/PasswordBook/blob/master/src/stringTool_util.h
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/
//! this is a util file of "StringTool.h",don't include this directly

#ifdef _STRINGTOOL_H
#ifndef _STRINGTOOL_UTIL_H
#define _STRINGTOOL_UTIL_H
#include <sstream>
namespace StringTool
{
	template <typename T>
	std::string convertFrom(T from)
	{
		std::ostringstream oss;
		oss << from;
		return oss.str();
	}

	template <typename T>
	T convertTo(const std::string &str)
	{
		std::istringstream iss;
		iss.str(str);
		T ret;
		iss >> ret;
		return ret;
	}
}
#endif  //_STRINGTOOL_UTIL_H
#endif  //_STRINGTOOL_H