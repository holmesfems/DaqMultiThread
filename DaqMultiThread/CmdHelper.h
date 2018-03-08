//#pragma once
/*
* CmdHelper.h
* Created at 2017/06/25
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/
#ifndef _CMDHELPER_H
#define _CMDHELPER_H
#include <iostream>
#include <string>
#include "ParamSet.h"
#include <map>
namespace CmdHelper
{
	using CmdSend = std::pair<std::string, ParamSet::Params>;
	using CmdHandler = std::function<std::string(ParamSet::Params &)>;
	using CmdWithHelp = std::pair<CmdHandler, std::string>;
	using CmdMap = std::map<std::string, CmdWithHelp>;
	class CmdHelper
	{
	public:
		CmdHelper(CmdMap &cmdMap);
		CmdHelper(CmdHelper &cmdHelper);
		~CmdHelper();
		//! Executing a certain command
		/*!
		* \return 0 if should exitting application, >0 if done normally, <0 if
		*   some error occured
		*/
		std::string exec(const std::string &cmd);

		//! regist command with name 'cmd'
		/*!
		*  corresponding function is like:
		*
		*  ```
		*  int _cmd_func(Params &split_param) {...}
		*  ```
		*/
		void registCmd(std::string cmd, CmdHandler handler,
			std::string help_string = "");
		//inline void registCmd(std::string cmd, int (CmdHelper::*f)(ParamSet::Params &),
			//std::string help_string = "");

		int show_help(std::istream& is) { return 0; }

	protected:
		//! dispatch cmd to corresponding handler
		std::string dispatchCmd(const std::string &cmd, ParamSet::Params &split_param);
		//void initCmdDict();
		//void initPMDB(const boost::filesystem::path &dbpath);
		//void initPG(const boost::filesystem::path &config);

	private:
		//! Convert a string to command
		static CmdSend _str_to_cmd(const std::string &str);
		static const char _devide = ' ';
		static const char _escape = '\\';
		// static const char _strBracket='"';
		static const char _equal = '=';
		CmdMap &_cmdMap;

		//int _cmd_generate(Params &param);
		//int _cmd_exit(Params &param);
		//int _cmd_help(Params &param);
		//// int _cmd_opendb(Params &param);
		//int _cmd_save_password_for_domain(Params &param);
		//int _cmd_load_password(Params &param);
		//int _cmd_list_item(Params &param);
	};
}
#endif // !_CMDHELPER_H

