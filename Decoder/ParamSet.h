/*
* paramSet.h
* Modified from https://github.com/holmesfems/PasswordBook/blob/master/src/cmdSeparator.h
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/

#ifndef _PARAMSET_H
#define _PARAMSET_H
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace ParamSet
{
	using ParamItem = std::pair<std::string, std::string>;
	using Params = std::vector<ParamItem>;
	using VariableBindItem = std::pair<void *, int8_t>;
	using VariableBind = std::map<std::string, VariableBindItem>;
	//! The class to help setting a lot of variables
	/*!
	* While running the main application,this class should be constructed first,
	* all bound variables will be set easily
	*/
	class ParamHelper
	{
	public:
		static const int8_t INTEGER = 1;
		static const int8_t TEXT = 2;
		static const int8_t FLOAT64 = 3;

		ParamHelper() {}
		void bind(const std::string &key, void *target, int8_t type = TEXT);
		void bind(const char *key, void *target, int8_t type = TEXT);
		void set(Params &params);
		inline VariableBind bindlist() { return _vb; }
	private:
		VariableBind _vb;
	};

	//! Change a equation string to the ParamItem
	ParamItem strToItem(const std::string &equation, char escape = '\\', char equal = '=');

	//! Change a equation string to the ParamItem
	ParamItem strToItem(const char *equation, char escape = '\\', char equal = '=');

}
#endif

