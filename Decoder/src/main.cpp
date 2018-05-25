
#include <stdio.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <vector>
#include <algorithm>
#include <sstream>
//#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include "stringTool.h"
#include "writeHddThread.h"
using int32 = int;
using float64 = double;
using int8 = char;

#define DAQmxErrChk(functionCall) { if( DAQmxFailed(error=(functionCall)) ) { goto Error; } 
//ParamFilter
using Filter = std::vector<std::string>;

//Default parameters
//Task parameters
int32       error = 0;
TaskHandle  taskHandle = 0;
char        errBuff[2048] = { '\0' };
int32       i;

//Channel parameters
//char       chan[] = "Dev1/ai0";
std::string channel = "Dev1/ai0";
float64     min = -1.0;
float64     max = 6.0;

//Timing parameters
//char        source[] = "OnboardClock";
std::string source = "OnboardClock";
int32       samplesPerChan = 80000;
float64     sampleRate = 80000.0;

// Data read parameters
int32       bufferSize = 80814;
//float64     *data1;
//float64     *data2;
//float64     *(data[2]);
int32       pointsToRead = -1;
float64     timeout = 10.0; //sec

// ReadTime
std::atomic<int32> readTime;//sec,-1 for infinity
std::atomic<int32> nowReadTime;

//SaveConfig::Config *config;
const std::string configFileName = "parameter.conf";
std::string savePath = ".";
//Mutex
//std::mutex writeFileMutex;
//std::mutex parameterMutex;
//std::mutex readDataMutex[2];

//CheckOnOff
double_t &checkA = WriteHddThread::checkA;
double_t &checkB = WriteHddThread::checkB;

//Flags
std::atomic<int> readStatus;
const int HOLD = 0;
const int DO = 1;
const int EXIT = -1;

bool use_TCP = false;

//ip config
std::string ip_host = "127.0.0.1";
//Tcp parameters
uint16_t tcpPort = 23333;

std::future<bool> readStartFlag;
std::promise<bool> readStartFlag_writer;
//std::atomic<bool> isReading;


std::string saveMode = "BIT";

int output_local(std::string msg)
{
	std::cout << boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::local_time()) << "\t" << msg << std::endl;
	std::cout.flush();
	return 0;
}

int output_toClient(std::string msg)
{
	TcpServer::TcpServer *server = tcpServer;
	if (server)
	{
		if (server->is_online(0))
			server->send(msg);
	}
	return 0;
}

int output(std::string msg)
{
	output_local(msg);
	output_toClient(msg);
	return 0;
}

//!Save the parameter in paramHelper to a config file
nlohmann::json paramHelperToJson(ParamSet::ParamHelper &ph)
{
	nlohmann::json json;
	for (auto item : ph.bindlist())
	{
		switch (item.second.second)
		{
		case ParamSet::ParamHelper::INTEGER:
			json[item.first] = *((int*)item.second.first);
			break;
		case ParamSet::ParamHelper::TEXT:
			json[item.first] = *((std::string*)item.second.first);
			break;
		case ParamSet::ParamHelper::FLOAT64:
			json[item.first] = *((double*)item.second.first);
			break;
		}
	}
	return json;
}


int saveConfig()
{
	configJson = paramHelperToJson(*paramHelper);
	std::ofstream ofs(configFileName);
	if (ofs.is_open())
	{
		ofs << configJson.dump(4);
		output_local("Save config succeed!");
	}
	else
	{
		output_local((boost::format("Can't open file: %s") % configFileName).str());
	}
	ofs.close();
	return 0;
}

//!Load parameters from config file to paramHelper
int loadConfig()
{
	std::ifstream ifs(configFileName);

	if (ifs.is_open())
	{ //file exists
		try
		{
			output_local("Read Configuation file succeed!");
			configJson.clear();
			ifs >> configJson;
			ParamSet::VariableBind bind = paramHelper->bindlist();
			for (auto &item : configJson.items())
			{
				if (bind.find(item.key()) == bind.cend())
				{
					output_local((boost::format("Error occured: key \"%s\" is not valid") % item.key()).str());
					continue;
				}
				ParamSet::VariableBindItem binditem = bind[item.key()];
				switch (binditem.second)
				{
				case ParamSet::ParamHelper::INTEGER:
					*((int*)binditem.first) = configJson[item.key()].get<int>();
					break;
				case ParamSet::ParamHelper::TEXT:
					*((std::string*)binditem.first) = configJson[item.key()].get<std::string>();
					break;
				case ParamSet::ParamHelper::FLOAT64:
					*((double*)binditem.first) = configJson[item.key()].get<double>();
					break;
				}
			}
		}
		catch(std::exception e)
		{
			output_local((boost::format("Error occured while reading config file:%s") % e.what()).str());
		}
	}
	else
	{ //file not exists
		saveConfig();
	}
	ifs.close();
	return 0;
}

int createDir(std::string dir)
{
	boost::filesystem::path path(dir);
	if (boost::filesystem::exists(path))
	{
		if (!boost::filesystem::is_directory(path))
		{
			output((boost::format("%s is not a directory name!") % path.string()).str());
			return 1;
		}
		return 0;
	}
	else
	{
		return boost::filesystem::create_directories(path) ? 0 : 1;
	}
}

int decode(std::string source, std::string target)
{
	//using DataPoint = std::pair < int32, int32 >;
	//std::vector<DataPoint> result;
	std::ofstream ofs;
	std::ifstream ifs;
	//writeFileMutex.lock();
	ifs.open(source, std::ios::binary);
	ofs.open(target);
	bool first = true;
	int32 position = 0;
	bool status;
	while (!ifs.eof())
	{
		//header
		using Header = WriteHddThread::Header;
		Header header;
		ifs.read((char*)&header, sizeof(Header));
		if (ifs.eof())
			break;
		ofs << "#" << boost::posix_time::to_iso_extended_string(header.ptime) << std::endl;
		if (first)
		{
			ofs << position << "\t" << int32(header.first) << "\n";
			status = !(header.first == 0);
			first = false;
		}
		else
		{
			if (status != !(header.first == 0))
			{
				int8 value = status ? 1 : 0;
				ofs << position - 1 << "\t" << int32(value) << std::endl;
				ofs << position << "\t" << 1 - value << std::endl;
				status = !status;
			}
		}
		int32 *buffer = new int32[header.bodySize];
		ifs.read((char*)buffer, sizeof(int32)*header.bodySize);
		int i;
		for (i = 0; i < header.bodySize - 1; i++)
		{
			position += buffer[i];
			int8 value = status ? 1 : 0;
			ofs << position - 1 << "\t" << int32(value) << std::endl;
			ofs << position << "\t" << 1 - value << std::endl;
			status = !status;
		}
		position += buffer[i];
		std::cout << "position:" << position << std::endl;
		delete buffer;
	}
	int8 value = status ? 1 : 0;
	position--;
	ofs << position << "\t" << int32(value) << std::endl;
	ifs.close();
	ofs.close();
	//writeFileMutex.unlock();
	return 0;
}

int decode_raw(std::string source, std::string target)
{
	std::ofstream ofs;
	std::ifstream ifs;
	ifs.open(source, std::ios::binary);
	ofs.open(target);
	int count = 0;
	double_t *buffer = new double_t[bufferSize];
	while (!ifs.eof())
	{
		
		ifs.read((char*)buffer, bufferSize * sizeof(double_t));
		for (int i = 0; i < ifs.gcount() / sizeof(double_t); i++)
		{
			ofs << count << "\t" << buffer[i] << std::endl;
			count++;
		}
	}
	delete[] buffer;
	return 0;
}

int timeDiff(std::string source, std::string target)
{
	//using DataPoint = std::pair < int32, int32 >;
	//std::vector<DataPoint> result;
	std::ofstream ofs;
	std::ifstream ifs;
	//writeFileMutex.lock();
	ifs.open(source, std::ios::binary);
	ofs.open(target);
	bool first = true;
	int32 position = 0;
	bool status;
	boost::posix_time::ptime ptime;
	while (!ifs.eof())
	{
		//header
		using Header = WriteHddThread::Header;
		Header header;
		ifs.read((char*)&header, sizeof(Header));
		if (ifs.eof())
			break;
    //ofs << "#" << boost::posix_time::to_iso_extended_string(header.ptime) << std::endl;
        if (first)
        {
	        ptime = header.ptime;
	        first = false;
        }
        else
        {
	        ofs << (header.ptime - ptime).total_microseconds() / 1000000.0 << std::endl;
	        ptime = header.ptime;
        }
        int32 *buffer = new int32[header.bodySize];
        ifs.read((char*)buffer, sizeof(int32)*header.bodySize);
        int i;
        for (i = 0; i < header.bodySize - 1; i++)
        {
	        position += buffer[i];
	        int8 value = status ? 1 : 0;
	        //ofs << position - 1 << "\t" << int32(value) << std::endl;
	        //ofs << position << "\t" << 1 - value << std::endl;
	        status = !status;
        }
        position += buffer[i];
        std::cout << "position:" << position << std::endl;
        delete buffer;
	}
	int8 value = status ? 1 : 0;
	position--;
	//ofs << position << "\t" << int32(value) << std::endl;
	ifs.close();
	ofs.close();
	//writeFileMutex.unlock();
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		return 0;
	}
	std::string cmd = argv[1];
	if (cmd == "-d")
	{
		/*
		if (argc == 4)
		{
			std::string source = argv[2];
			std::string target = argv[3];
			decode(source, target);
			return 0;
		}
		*/
		if (argc >= 3)
		{
			for (int i = 2; i < argc; i++)
			{
				std::string source = argv[i];
				std::string target = source + ".out";
				decode(source, target);
			}
			return 0;
		}
	}
	else if (cmd == "-dr")
	{
		if (argc >= 3)
		{
			for (int i = 2; i < argc; i++)
			{
				std::string source = argv[i];
				std::string target = source + ".out";
				decode_raw(source, target);
			}
			return 0;
		}
	}
	else if (cmd == "-dt")
	{
		if (argc >= 3)
		{
			for (int i = 2; i < argc; i++)
			{
				std::string source = argv[i];
				std::string target = source + ".tdiff";
				timeDiff(source, target);
			}
			return 0;
		}
	}
	std::cout << "Wrong use\n" << std::endl;
	return 0;
}
