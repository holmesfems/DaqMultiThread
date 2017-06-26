/*
* CmdHelper.cpp
* Created at 2017/06/25
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/

#include "CmdHelper.h"
#include <typeinfo>
#include <stdlib.h>
#include <stdio.h>
#include "stringTool.h"

namespace CmdHelper
{
	CmdHelper::CmdHelper(CmdMap &cmdMap):
		_cmdMap(cmdMap)
	{
	}

	CmdHelper::CmdHelper(CmdHelper &cmdHelper) :
		_cmdMap(cmdHelper._cmdMap)
	{
	}

	CmdHelper::~CmdHelper()
	{
	}

	void CmdHelper::registCmd(std::string cmd, CmdHandler handler,
		std::string help_string)
	{
		if (help_string.empty()) {
			help_string = "No help";
		}
		_cmdMap[std::move(cmd)] = std::make_pair(handler, std::move(help_string));
	}

	std::string CmdHelper::exec(const std::string &cmd)
	{
		std::string tcmd = StringTool::strTrim(cmd);
		if (tcmd != "")
		{
			CmdSend tosend = _str_to_cmd(tcmd);
			return dispatchCmd(tosend.first, tosend.second);
		}
	}

	std::string CmdHelper::dispatchCmd(const std::string & cmd, ParamSet::Params & split_param)
	{
		auto handler = _cmdMap[cmd];
		if (std::get<0>(handler)) {
			return std::get<0>(handler)(split_param);
		}
		return "unknown command";
	}

	CmdSend CmdHelper::_str_to_cmd(const std::string &cmdstr)
	{
		std::string trimCmd = StringTool::strTrim(cmdstr);
		bool first = true;
		// bool inStr=false;
		// bool inEsc=false;
		// bool inSpace=false;
		// bool needEnd=true;
		bool inEqual = false;
		const char *data = trimCmd.c_str();
		size_t i, maxi = trimCmd.length();
		std::ostringstream oss;
		std::string key, value;
		std::string cmd;
		ParamSet::Params ret;
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
			if (data[i] == _devide) {
				// if(inSpace) continue;
				do {
					i++;
				} while (i < maxi && data[i] == _devide);
				i -= 1;
				if (first) {
					cmd = oss.str();
					oss.str("");
					first = false;
					continue;
				}
				value = oss.str();
				ParamSet::ParamItem item(key, value);
				ret.push_back(item);
				key = "";
				value = "";
				oss.str("");
				// inSpace = true;
				continue;
			}
			if (data[i] == _equal) {
				//assert(!first);
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
		if (first && oss.str() != "") {
			cmd = oss.str();
		}
		else if (value == "" && oss.str() != "") {
			value = oss.str();
			ParamSet::ParamItem item(key, value);
			ret.push_back(item);
		}
#ifdef DEBUG
		std::cout << cmd << ":" << std::endl;
		for (auto item : ret) {
			std::cout << item.first << "," << item.second << std::endl;
		}
#endif
		return CmdSend(cmd, ret);
	}

}