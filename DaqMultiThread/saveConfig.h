/*
* saveConfig.h
* Created on 2017/03/25 20:23:49
* Original reference:https://github.com/holmesfems/PasswordBook/blob/master/src/saveConfig.h
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/

#ifndef _SAVECONFIG_H
#define _SAVECONFIG_H
#include <string>
#include <vector>
namespace SaveConfig
{
	//! An item of config
	class ConfigItem
	{
	public:
		//! Constructor
		ConfigItem();

		//! Constructor,initializing key and value
		ConfigItem(const std::string &key, const std::string &value);

		//! key of the item
		std::string key;

		//! value of the item
		std::string value;
	};

	//! This class is to support reading and writing to config file
	/*!
	* The format of the config file will be always be "[Key] = [Value]".
	* The "#" will be recognized as comment.
	* All other lines which out of the format above will be ignored
	*/
	using ConfigList = std::vector<ConfigItem>;
	class Config
	{
	public:
		//! Constructor
		Config() {}
		//! Push an item to configList
		int pushItem(ConfigItem item);

		//! Load config from a certain file
		int load(const std::string &filename);

		//! Save config to a certain file
		int save(const std::string &filename);

		//! Get the value of config by key
		std::string getValueByKey(const std::string &key);

		//! Get the value of config by index
		std::string getValueByIndex(int index);

		//! Get the key of config by index
		std::string getKeyByIndex(int index);

		//! Get the index of config by key
		int getIndexByKey(const std::string &key);

		//! Set the value of config by key
		/*!
		If the is no key hit,this function will create a new item and
		add it to configList
		*/
		int setValueByKey(const std::string &value, const std::string &key);

		//! Set the value of config by index
		/*!
		* \sa setValueByKey()
		*/
		int setValueByIndex(const std::string &value, int index);
		// int deleteByIndex(int index);

		//! Get the list of config item
		inline ConfigList getList() { return _configList; }
	private:
		//! The vector to save all config items
		ConfigList _configList;
	};
}

#endif  // _SAVECONFIG_H