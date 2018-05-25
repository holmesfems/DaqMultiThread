/*
* stringTool.h
* Original reference:https://github.com/holmesfems/PasswordBook/blob/master/src/stringTool.h
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/

#ifndef _STRINGTOOL_H
#define _STRINGTOOL_H
#include <string>
#include <vector>
namespace StringTool
{
	//! The function to split a string using delim sign
	/*!
	* \param s The string to be splitted
	* \param delim The delim sign
	* \return The vector includes all items that splitted not including the delim
	* sign.
	*/
	std::vector<std::string> strSplit(const std::string &s, const std::string &delim);

	//! The function to seperate a string from a certain locate
	/*!
	* \param str The string to be seperated
	* \param locat The start location to be seperated
	* \param length The length of seperating sign
	* \return The vector contains two items of seperating result
	*/
	std::vector<std::string> strSep(const std::string &str, size_t locate, size_t length);

	//! The function to trim a string
	/*!
	* \param str The string to be trimmed
	* \param trim The trimming sign
	* \return A new string which is the result of trimming
	*/
	std::string strTrim(const std::string &str, const std::string trim = "\n\t ");

	//! The convert function from type T to std::string
	template <class T>
	std::string convertFrom(T from);

	//! The convert function from std::string to type T
	template <class T>
	T convertTo(const std::string &str);

	//! The convert function from binary data to a binary-expressing string
	/*!
	* For example,the data 0x1f,0xa2 will be convert to string "1FA2"
	* \param binary The binary data that convert from
	* \param length The length of "binary"
	* \return A new string which is the result of convert
	*/
	std::string binToStr(const char *binary, int length);

	//! The convert function from a binary-expressing string to binary data
	/*!
	* Please free the pointer that returned if you won't use it again
	* \sa binToStr()
	* \param str The string that convert from
	* \param length The length of the data will be recorded to this pointer
	* \return A new char siries which is the result of convert
	*/
	char *strToBin(const std::string &str, int *length = NULL);

	//! The function that add a bracket to a string
	/*!
	* For example,bracket("abcd") will return R"("abcd")"
	* \param str The string to be added a bracket
	* \return A new string which is the result
	*/
	std::string bracket(const std::string &str);
}
#include "stringTool_util.h"
#endif
