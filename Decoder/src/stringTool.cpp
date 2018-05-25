/*
* stringTool.cpp
* Original reference:https://github.com/holmesfems/PasswordBook/blob/master/src/stringTool.cpp
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/

#include "stringTool.h"
#include <iostream>
#include <sstream>
#include <stdlib.h>

namespace StringTool
{
	std::vector<std::string> strSplit(const std::string &s, const std::string &delim)
	{
#ifdef DEBUG
		std::cout << "calling strsplit( " << s << " , " << delim << " )\n";
#endif
		std::vector<std::string> result;
		std::string::size_type pos = 0;
		while (pos != std::string::npos) {
			std::string::size_type p = s.find(delim, pos);
			if (p == std::string::npos) {
				result.push_back(s.substr(pos));
				break;
			}
			else {
				result.push_back(s.substr(pos, p - pos));
			}
			pos = p + delim.size();
		}
		return result;
	}

	std::vector<std::string> strSep(const std::string &str, size_t locate, size_t length)
	{
#ifdef DEBUG
		std::cout << "calling strSep(" << str << "," << locate << "," << length << ")" << std::endl;
#endif
		std::vector<std::string> result;
		result.push_back(str.substr(0, locate));
		result.push_back(str.substr(locate + length, str.length() - locate - length));
#ifdef DEBUG
		std::cout << "result:" << result[0] << " | " << result[1] << std::endl;
#endif
		return result;
	}

	std::string strTrim(const std::string &str, const std::string trim)
	{
		size_t start = str.find_first_not_of(trim);
		if (start == std::string::npos) return "";
		size_t last = str.find_last_not_of(trim);
		return str.substr(start, last - start + 1);
	}

	char binToHexChar(unsigned char c)
	{
		if (c > 0x0F) return 0;
		if (c < 0x0A) return 0x30 + c;
		return 0x37 + c;
	}

	std::string hex(char c)
	{
		unsigned char uc = (unsigned char)(c);
		std::ostringstream oss;
		unsigned char upper = (uc & (unsigned char)(0xF0)) >> 4;
		unsigned char lower = (uc & (unsigned char)(0x0F));
		oss << binToHexChar(upper) << binToHexChar(lower);
		return oss.str();
	}

	std::string binToStr(const char *binary, int length)
	{
		int i;
		std::ostringstream oss;
		for (i = 0; i < length; i++) {
			oss << hex(binary[i]);
		}
		return oss.str();
	}

	unsigned char hexCharToBin(char c)
	{
		if (c >= 0x30 && c <= 0x39) return c - 0x30;
		if (c >= 0x41 && c <= 0x46) return c - 0x37;
		if (c >= 0x61 && c <= 0x66) return c - 0x57;
		return (unsigned char)(0xff);
	}

	char *strToBin(const std::string &str, int *length)
	{
		const char *cstr = str.c_str();
		int maxi = str.length();
		if (maxi % 2 != 0) return NULL;
		char *ret = (char *)malloc((maxi / 2) * sizeof(char));
		int i;
		for (i = 0; i < maxi; i += 2) {
			unsigned char ucu = hexCharToBin(cstr[i]);
			unsigned char ucl = hexCharToBin(cstr[i + 1]);
			if (ucu == 0xff || ucl == 0xff) {
				free(ret);
				return NULL;
			}
			unsigned char uc = (ucu << 4) | ucl;
			ret[i / 2] = char(uc);
		}
		if (length != NULL) (*length) = maxi / 2;
		return ret;
	}

	std::string bracket(const std::string &str)
	{
		std::ostringstream oss;
		oss << "\"";
		oss << str;
		oss << "\"";
		return oss.str();
	}
}