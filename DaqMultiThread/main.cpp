/*********************************************************************
*
* ANSI C Example program:
*    acquireNScans.c
*
* Example Category:
*    AI
*
* Description:
*    This example demonstrates how to acquire a finite amount of data
*    using the DAQ device's internal clock.
*
* Instructions for Running:
*    1. Select the physical channel to correspond to where your signal
*       is input on the DAQ device.
*    2. Enter the minimum and maximum voltage range.
*       Note: For better accuracy try to match the input range to the
*       expected voltage level of the measured signal.
*    3. Set the number of samples to acquire per channel.
*    4. Set the rate of the acquisiton.
*       Note: The rate should be AT LEAST twice as fast as the maximum
*       frequency component of the signal being acquired.
*
* Steps:
*    1. Create a task.
*    2. Create an analog input voltage channel.
*    3. Set the rate for the sample clock. Additionally, define the
*       sample mode to be finite.
*    4. Call the Start function to start the acquistion.
*    5. Read all of the waveform data.
*    6. Call the Clear Task function to stop the acquistion.
*    7. Display an error if any.
*
* I/O Connections Overview:
*    Make sure your signal input terminal matches the Physical Channel
*    I/O Control. In this case wire your signal to the ai0 pin on your
*    DAQ Device. By default, this will also be the signal used as the
*    analog start trigger.
*
* Recommended use:
*    Call Configure and Start functions.
*    Call Read function.
*    Call Stop function at the end.
*
*********************************************************************/

/*
* Modified from sample code given by NI
* Original reference: acquireNScans.c
* Modifier: zhai <holmesfems@gmail.com>
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/

#include <NIDAQmxBase.h>
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
#include <fstream>
#include "ParamSet.h"
#include "stringTool.h"
#include "CmdHelper.h"
#include "tcpServer.h"
#include "tcpClient.h"
#include "writeHddThread.h"
#include "json.hpp"

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
ParamSet::ParamHelper *paramHelper = NULL;
ParamSet::ParamHelper *autoParams = NULL;
const std::string configFileName = "parameter.conf";
nlohmann::json configJson;

//Mutex
//std::mutex writeFileMutex;
//std::mutex parameterMutex;
//std::mutex readDataMutex[2];

//TargetFileName
std::string targetFileName = "";

//CmdHelper
CmdHelper::CmdMap cmdMap;
CmdHelper::CmdHelper cmdHelper(cmdMap);

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

std::atomic<TcpServer::TcpServer*> tcpServer;

//future
std::shared_future<TcpServer::TcpServer*> tcpServer_future;
std::promise<TcpServer::TcpServer*> tcpServer_promise;

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

int output(std::string msg)
{
	output_local(msg);
	TcpServer::TcpServer *server = tcpServer;
	if (server)
	{
		if (server->is_online(0))
			server->send(msg);
	}
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

int readThread()
{
	//Number of writing  HDD thread
	do
	{
		//refresh flag
		//isReading = false;
		if (use_TCP)
		{
			std::promise<bool> newpromise;
			readStartFlag_writer.swap(newpromise);
			readStartFlag = readStartFlag_writer.get_future();
			// wait for flag is set
			if (!readStartFlag.get()) break;
		}
		size_t wthreadNum = std::count(channel.begin(), channel.end(), ',') + 1;
		using sharePtr_wthread = std::shared_ptr<WriteHddThread::WriteHddThread>;
		std::vector<sharePtr_wthread> wthreads(wthreadNum);
		std::vector<int32_t> writeFlag(wthreadNum);
		if (saveMode == "BIT")
		{
			for (int i = 0; i < wthreadNum; i++)
			{
				writeFlag[i] = WriteHddThread::BIT;
			}
		}
		else if (saveMode == "RAW")
		{
			for (int i = 0; i < wthreadNum; i++)
			{
				writeFlag[i] = WriteHddThread::RAW;
			}
		}
		else
		{
			for (int i = 0; i < wthreadNum; i++)
			{
				writeFlag[i] = 0;
			}
			std::vector<std::string> delim = StringTool::strSplit(saveMode, ",");
			if (delim.size() != wthreadNum)
			{
				output("Wrong use of saveMode!");
				return -1;
			}
			for (int i = 0; i < wthreadNum; i++)
			{
				int32_t getFlag = 0;
				std::vector<std::string> delim2 = StringTool::strSplit(delim[i], "|");
				for (auto item : delim2)
				{
					if (item == "RAW")
					{
						getFlag = WriteHddThread::RAW;
					}
					else if (item == "BIT")
					{
						getFlag = WriteHddThread::BIT;
					}
					else
					{
						output("Wrong use of saveMode!");
						readStatus = HOLD;
						continue;
					}
					if ((writeFlag[i] & getFlag) == 0)
					{
						writeFlag[i] += getFlag;
					}
					else
					{
						output("Wrong use of saveMode!");
						readStatus = HOLD;
						continue;
					}
				}
			}
		}
		for (int i = 0; i < wthreadNum; i++)
		{
			targetFileName = (boost::format("BS%s_%d.dat") % boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::local_time()) % (i + 1)).str();
			wthreads[i] = sharePtr_wthread(new WriteHddThread::WriteHddThread(targetFileName, writeFlag[i]));
		}
		if (DAQmxBaseCreateTask("", &taskHandle) < 0)
			throw "Error in creating task";
		if (DAQmxBaseCreateAIVoltageChan(taskHandle, channel.c_str(), "", DAQmx_Val_RSE, min, max, DAQmx_Val_Volts, NULL) < 0)
			throw "Error in creating AIVoltageChan";
		if (DAQmxBaseCfgSampClkTiming(taskHandle, source.c_str(), sampleRate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, samplesPerChan) < 0)
			throw "Error in CfgSampClkTiming";
		float64 *rdata;
		rdata = new float64[bufferSize*wthreadNum];
		pointsToRead = samplesPerChan;
		output((boost::format("start reading, readTime = %d") % readTime).str());
		//isReading = true;
		DAQmxBaseStartTask(taskHandle);
		for (nowReadTime = 0; nowReadTime < readTime || readTime < 0; nowReadTime++)
		{
			int32	pointsRead = 0;
			boost::posix_time::ptime ptime = boost::posix_time::microsec_clock::local_time();
			DAQmxBaseReadAnalogF64(taskHandle, pointsToRead, timeout, DAQmx_Val_GroupByChannel, rdata, bufferSize*wthreadNum, &pointsRead, NULL);
			auto tmp = rdata;
			for (int i = 0; i < wthreadNum; i++)
			{
				wthreads[i]->push(ptime, pointsRead, tmp);
				tmp += pointsRead;
			}
			//dataQueue.push(std::move(*wp));
			if (readStatus != DO) readTime = 0;
		}
		delete rdata;
		//writeCmd = EXIT;

		readStatus = HOLD;
		if (taskHandle != 0)
		{
			DAQmxBaseStopTask(taskHandle);
			DAQmxBaseClearTask(taskHandle);
			taskHandle = 0;
		}
		//_writeThread.join();
		output((boost::format("stop reading, total read for: %d seconds") % nowReadTime).str());

		//Error:
		/*
		if (DAQmxFailed(error))
		DAQmxBaseGetExtendedErrorInfo(errBuff, 2048);

		if (error)
		printf("DAQmxBase Error %ld: %s\n", error, errBuff);
		*/
	}while (use_TCP && readStatus != EXIT);
	output_local("Exit succeed");
	return 0;
}

//Start reading from DAQ device
std::string startRead(ParamSet::Params &params)
{
	readTime = -1;
	Filter filter;
	filter.push_back("readTime");
	filter.push_back("sampleRate");
	filter.push_back("samplesPerChan");
	filter.push_back("channel");
	filter.push_back("");
	filter.push_back("saveMode");
	ParamSet::Params filtered;
	for (auto item : params)
	{
		if (std::find(filter.begin(), filter.end(), item.first) != filter.end())
		{
			filtered.push_back(item);
		}
	}
	//default parameter = "readTime"
	autoParams->bind("", &readTime, ParamSet::ParamHelper::INTEGER);
	loadConfig();
	paramHelper->set(filtered);
	autoParams->set(filtered);
	std::string ret;
	//Set flag
	readStatus = DO;
	try
	{
		readStartFlag_writer.set_value(true);
		ret = "Start reading";
	}
	catch (std::future_error e)
	{
		if (e.code() == std::future_errc::promise_already_satisfied)
		{
			ret = "Already reading";
		}
		else
		{
			ret = "Future error occured:";
			ret += e.what();
		}
	}
	return ret;
}

//!Stop reading from DAQ device
std::string stopRead(ParamSet::Params &params)
{
	readStatus = HOLD;
	return "Stop reading";
}

std::string exitRead(ParamSet::Params &params)
{
	readStatus = EXIT;
	std::string ret;
	/*
	try
	{
		readStartFlag_writer.set_value(false);
		ret = "Try to exit reading";
	}
	catch (std::future_error e)
	{
		if (e.code() == std::future_errc::promise_already_satisfied)
		{
			ret = "Try to exit this reading";
		}
		else
		{
			ret = "Future error occured:";
			ret += e.what();
		}
	}
	//exit tcp server
	
	TcpServer::TcpServer *server = tcpServer;
	if (server)
	{
		server->exit();
	}*/
	ret = "Try to exit reading";
	return ret;
}

std::string getParam(ParamSet::Params &params)
{
	std::string paramName;
	ParamSet::ParamHelper ph;
	ph.bind("", &paramName, ParamSet::ParamHelper::TEXT);
	ph.set(params);
	nlohmann::json json;
	json["Params"] = paramHelperToJson(*paramHelper);
	json["Auto"] = paramHelperToJson(*autoParams);
	if (paramName.empty())
	{
		//show all params:
		output(json.dump(4));
		return "Get param done!";
	}
	else
	{
		for (auto item : json.items())
		{
			if (!item.value()[paramName].is_null())
			{
				return (boost::format("%s:%s = %s") % item.key() % paramName% item.value()[paramName].get<std::string>()).str();
			}
		}
		return "Can't find variable: " + paramName;
	}

}

int initialize()
{
	if (!paramHelper)
	{
		paramHelper = new ParamSet::ParamHelper();
		//initializing parameter binder
		paramHelper->bind("channel", &channel, ParamSet::ParamHelper::TEXT);
		paramHelper->bind("min", &min, ParamSet::ParamHelper::FLOAT64);
		paramHelper->bind("max", &max, ParamSet::ParamHelper::FLOAT64);
		paramHelper->bind("source", &source, ParamSet::ParamHelper::TEXT);
		paramHelper->bind("samplesPerChan", &samplesPerChan, ParamSet::ParamHelper::INTEGER);
		paramHelper->bind("sampleRate", &sampleRate, ParamSet::ParamHelper::FLOAT64);
		paramHelper->bind("bufferSize", &bufferSize, ParamSet::ParamHelper::INTEGER);
		paramHelper->bind("timeout", &timeout, ParamSet::ParamHelper::FLOAT64);
		//paramHelper->bind("readSets", &readSets, ParamSet::ParamHelper::INTEGER);
		paramHelper->bind("checkA", &checkA, ParamSet::ParamHelper::FLOAT64);
		paramHelper->bind("checkB", &checkB, ParamSet::ParamHelper::FLOAT64);
		paramHelper->bind("tcpPort", &tcpPort, ParamSet::ParamHelper::INTEGER);
		paramHelper->bind("ip_host", &ip_host, ParamSet::ParamHelper::TEXT);
		paramHelper->bind("saveMode", &saveMode, ParamSet::ParamHelper::TEXT);
	}
	if (!autoParams)
	{
		autoParams = new ParamSet::ParamHelper();
		autoParams->bind("readTime", &readTime, ParamSet::ParamHelper::INTEGER);
		autoParams->bind("nowReadTime", &nowReadTime, ParamSet::ParamHelper::INTEGER);
		//autoParams->bind("saveMode", &saveMode, ParamSet::ParamHelper::TEXT);
	}
	loadConfig();
	cmdHelper.registCmd("read", &startRead, "start read daq");
	cmdHelper.registCmd("stop", &stopRead, "stop read daq");
	cmdHelper.registCmd("exit", &exitRead, "exit read daq");
	cmdHelper.registCmd("get", &getParam, "get parameters");
	//data[0] = new float64[bufferSize];
	//data[1] = new float64[bufferSize];
	//writeCmd = HOLD;
	tcpServer_future = tcpServer_promise.get_future().share();
	readStatus = HOLD;
	//isReading = false;
	return 0;
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

void tcpThread()
{
	while (true)
	{
		use_TCP = true;
		boost::asio::io_service io_service;
		TcpServer::TcpServer server(io_service, tcpPort);
		tcpServer = &server;
		tcpServer_promise.set_value(&server);
		server.start_accept();
		io_service.run();
		std::cout << "server status = " << server.status << std::endl;
		if (!server.waitAllDone())
		{
			std::cout << "Error occured while waiting done" << std::endl;
		}
		tcpServer = NULL;
		
		if (server.status == TcpServer::TcpServer::EXIT) break;
	}
	tcpServer_promise.set_value(NULL);
	use_TCP = false;
}

void readByTcp()
{
	std::string cmd;
	std::string reply;
	std::thread _readThread(readThread);
	while (readStatus != EXIT)
	{
		TcpServer::TcpServer *server = tcpServer_future.get();
		std::promise<TcpServer::TcpServer*> newpromise;
		tcpServer_promise.swap(newpromise);
		tcpServer_future = tcpServer_promise.get_future().share();
		while (server != NULL)
		{
			if (server->is_connected(-1))
			{
				cmd = server->waitRecv();
				if (cmd != TcpServer::TcpServer::EXIT_MSG && cmd != TcpServer::TcpServer::LOST_MSG)
				{
					output_local("Received Command:" + cmd);
					reply = cmdHelper.exec(cmd);
					output(reply);
				}
			}
			else
			{
				break;
			}
		}
	}
	try
	{
		readStartFlag_writer.set_value(false);
		output_local("try to exit reading");
	}
	catch (std::future_error e)
	{
		if (e.code() == std::future_errc::promise_already_satisfied)
		{
			output_local("Now it's reading, please wait for it finishes work");
		}
		else
		{
			output_local((boost::format("Future error occured: %s") % e.what()).str());
		}
	}
	TcpServer::TcpServer* server = tcpServer;
	if (server)
	{
		server->exit();
	}
	_readThread.join();
}

int makeClient(std::string ip, uint16_t port)
{
	boost::asio::io_service io_service;
	TcpClient::TcpClient client(io_service);
	client.connect(ip, port);
	std::thread thread
	(
		[&io_service]()
	{
		io_service.run();
	});
	while (true)
	{
		std::string msg;
		std::getline(std::cin, msg);
		if (msg == "clientExit") break;
		client.send(msg);
		if (msg == "exit") break;
	}
	client.exit();
	std::cout << "client exit" << std::endl;
	thread.join();
	return 0;
}

int makeClient_sendCmd(std::string cmd)
{
	boost::asio::io_service io_service;
	TcpClient::TcpClient client(io_service);
	client.connect(ip_host, tcpPort);
	std::thread thread
	(
		[&io_service]()
	{
		io_service.run();
	});
	if (client.is_connected())
	{
		client.send(cmd);
		client.lastRecv();
	}
	else
	{
		std::cout << "Server not alive" << std::endl;
	}
	client.exit();
	std::cout << "client exit" << std::endl;
	thread.join();
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		initialize();
		std::thread _tcpThread(tcpThread);
		readByTcp();
		_tcpThread.join();
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
	else if (cmd == "-c")
	{
        if (argc >= 3)
        {
            ip_host = argv[2];
            if(argc >=4)
                tcpPort=std::stoi(argv[3]);
        }
		makeClient(ip_host,tcpPort);
		return 0;
	}
	else if (cmd == "-e")
	{
		initialize();
		std::ostringstream oss;
		for (int i = 2; i < argc; i++)
		{
			if (i != 2) oss << " ";
			oss << argv[i];
		}
		cmdHelper.exec(oss.str());
		readThread();
		return 0;
	}
	else if (cmd == "-ec")
	{
		initialize();
		std::ostringstream oss;
		for (int i = 2; i < argc; i++)
		{
			if (i != 2) oss << " ";
			oss << argv[i];
		}
		makeClient_sendCmd(oss.str());
		return 0;
	}
	std::cout << "Wrong use\n" << std::endl;
	return 0;
}
