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
#include <algorithm>
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
		return "blank";
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
		std::ostringstream oss;
		std::string key, value;
		std::string cmd;
		ParamSet::Params ret;
        std::vector<std::string> splitCmd=StringTool::strSplit(trimCmd,std::string(1,_devide));
        cmd = splitCmd[0];
        std::for_each(splitCmd.cbegin()+1,splitCmd.cend(),[&ret](std::string item)
        {
            ret.push_back(ParamSet::strToItem(item,_escape,_equal));
        });
#ifdef DEBUG
		std::cout << cmd << ":" << std::endl;
		for (auto item : ret) {
			std::cout << item.first << "," << item.second << std::endl;
		}
#endif
		return CmdSend(cmd, ret);
	}

}
