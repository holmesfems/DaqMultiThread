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
//#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <fstream>
#include "saveConfig.h"
#include "ParamSet.h"
#include "stringTool.h"
#include "CmdHelper.h"
#include "tcpServer.h"
#include "tcpClient.h"
#include "writeHddThread.h"

#define DAQmxErrChk(functionCall) { if( DAQmxFailed(error=(functionCall)) ) { goto Error; } 
//ParamFilter
using Filter = std::vector<std::string>;

//Default parameters
//Tcp parameters
uint16_t tcpPort = 23333;

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
int32       readSets = 10;

// ReadTime
std::atomic<int32> readTime;//sec,-1 for infinity

//SaveConfig::Config *config;
ParamSet::ParamHelper *paramHelper = NULL;
ParamSet::ParamHelper *autoParams = NULL;
const std::string configFileName = "parameter.conf";

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

std::atomic<TcpServer::TcpServer*> tcpServer;

//!Save the parameter in paramHelper to a config file
int saveConfig()
{
	SaveConfig::Config config;
	for (auto item : paramHelper->bindlist())
	{
		switch (item.second.second)
		{
		case ParamSet::ParamHelper::INTEGER:
			config.pushItem(SaveConfig::ConfigItem(item.first, StringTool::convertFrom<int32>(*(int32*)(item.second.first))));
			break;
		case ParamSet::ParamHelper::TEXT:
			config.pushItem(SaveConfig::ConfigItem(item.first, (*(std::string*)(item.second.first))));
			break;
		case ParamSet::ParamHelper::FLOAT64:
			config.pushItem(SaveConfig::ConfigItem(item.first, StringTool::convertFrom<double_t>(*(double_t*)(item.second.first))));
			break;
		}
	}
	config.save(configFileName);
}

//!Load parameters from config file to paramHelper
int loadConfig()
{
	SaveConfig::Config config;
	if (config.load(configFileName) < 0)
	{
		//make initial config
		saveConfig();
	}
	else
	{
		//Read params from config file
		ParamSet::Params params;
		for (auto item : config.getList())
		{
			params.push_back(ParamSet::ParamItem(item.key, item.value));
		}
		paramHelper->set(params);
	}
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
	//Set flag
	readStatus = DO;
	return "start reading\n";
}

//!Stop reading from DAQ device
std::string stopRead(ParamSet::Params &params)
{
	readStatus = HOLD;
	return "stop reading\n";
}

std::string exitRead(ParamSet::Params &params)
{
	readStatus = EXIT;
	return "exit reading\n";
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
		paramHelper->bind("readSets", &readSets, ParamSet::ParamHelper::INTEGER);
		paramHelper->bind("checkA", &checkA, ParamSet::ParamHelper::FLOAT64);
		paramHelper->bind("checkB", &checkB, ParamSet::ParamHelper::FLOAT64);
		paramHelper->bind("tcpPort", &tcpPort, ParamSet::ParamHelper::INTEGER);
	}
	if (!autoParams)
	{
		autoParams = new ParamSet::ParamHelper();
		autoParams->bind("readTime", &readTime, ParamSet::ParamHelper::INTEGER);
	}
	loadConfig();
	cmdHelper.registCmd("read", &startRead, "start read daq");
	cmdHelper.registCmd("stop", &stopRead, "stop read daq");
	cmdHelper.registCmd("exit", &exitRead, "exit read daq");
	//data[0] = new float64[bufferSize];
	//data[1] = new float64[bufferSize];
	//writeCmd = HOLD;
	readStatus = HOLD;
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
		}
	}
	delete[] buffer;
	return 0;
}

void tcpThread()
{
	while (true)
	{
		boost::asio::io_service io_service;
		TcpServer::TcpServer server(io_service, tcpPort, cmdHelper, &std::cout);
		tcpServer = &server;
		server.start_accept();
		io_service.run();
		std::cout << "server status = " << server.status << std::endl;
		tcpServer = NULL;
		if (server.status == TcpServer::TcpServer::EXIT) break;
	}
}

int output(std::string msg)
{
	std::cout << msg << std::endl;
	std::cout.flush();
	TcpServer::TcpServer *server = tcpServer;
	if (server)
	{
		if (server->status == TcpServer::TcpServer::ONLINE)
			server->send(msg);
	}
	return 0;
}

int read()
{
	initialize();
	float64 *rdata;
	std::thread _tcpThread(tcpThread);
	while (true)
	{
		if (readStatus == HOLD)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		if (readStatus == EXIT) break;
		if (readStatus != DO) break;
		//Number of writing  HDD thread
		size_t wthreadNum = std::count(channel.begin(), channel.end(), ',') + 1;
		using sharePtr_wthread = std::shared_ptr<WriteHddThread::WriteHddThread>;
		std::vector<sharePtr_wthread> wthreads(wthreadNum);
		for (int i = 0; i < wthreadNum; i++)
		{
			targetFileName = (boost::format("BS%s_%d.dat") % boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::local_time()) % (i + 1)).str();
			wthreads[i] = sharePtr_wthread(new WriteHddThread::WriteHddThread(targetFileName));
		}
		if (DAQmxBaseCreateTask("", &taskHandle) < 0)
			throw "Error in creating task";
		if (DAQmxBaseCreateAIVoltageChan(taskHandle, channel.c_str(), "", DAQmx_Val_RSE, min, max, DAQmx_Val_Volts, NULL) < 0)
			throw "Error in creating AIVoltageChan";
		if (DAQmxBaseCfgSampClkTiming(taskHandle, source.c_str(), sampleRate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, samplesPerChan) < 0)
			throw "Error in CfgSampClkTiming";
		DAQmxBaseStartTask(taskHandle);
		pointsToRead = samplesPerChan;
		int j;
		output((boost::format("start reading, readTime = %d") % readTime).str());
		for (j = 0; j < readTime || readTime < 0; j++)
		{
			int32	pointsRead = 0;
			rdata = new float64[bufferSize*wthreadNum];
			boost::posix_time::ptime ptime = boost::posix_time::microsec_clock::local_time();
			DAQmxBaseReadAnalogF64(taskHandle, pointsToRead, timeout, DAQmx_Val_GroupByChannel, rdata, bufferSize*wthreadNum, &pointsRead, NULL);
			auto tmp = rdata;
			for (int i = 0; i < wthreadNum; i++)
			{
				wthreads[i]->push(ptime, pointsRead, tmp);
				tmp += pointsRead;
			}
			delete rdata;
			//dataQueue.push(std::move(*wp));
			if (readStatus != DO) readTime = 0;
		}
		//writeCmd = EXIT;
		readStatus = HOLD;
		if (taskHandle != 0)
		{
			DAQmxBaseStopTask(taskHandle);
			DAQmxBaseClearTask(taskHandle);
			taskHandle = 0;
		}
		//_writeThread.join();
		output((boost::format("stop reading, total read for: %d seconds") % j).str());
	}
	std::cout << "exit reading" << std::endl;
	_tcpThread.join();
//Error:
	/*
	if (DAQmxFailed(error))
		DAQmxBaseGetExtendedErrorInfo(errBuff, 2048);

	if (error)
		printf("DAQmxBase Error %ld: %s\n", error, errBuff);
	*/
	return 0;
}

int makeClient()
{
	std::string msg;
	boost::asio::io_service io_service;
	TcpClient::TcpClient client(io_service);
	client.connect("127.0.0.1", tcpPort);
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
		client.send(msg);
		if (msg == "exit") break;
		if (msg == "clientExit") break;
	}
	std::cout << "client exit" << std::endl;
	thread.join();
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		read();
		return 0;
	}
	std::string cmd = argv[1];
	if (cmd == "-d")
	{
		if (argc == 4)
		{
			std::string source = argv[2];
			std::string target = argv[3];
			decode(source, target);
			return 0;
		}
	}
	else if (cmd == "-dr")
	{
		if (argc == 4)
		{
			std::string source = argv[2];
			std::string target = argv[3];
			decode_raw(source, target);
			return 0;
		}
	}
	else if (cmd == "-c")
	{
		makeClient();
		return 0;
	}
	std::cout << "Wrong use\n" << std::endl;
	return 0;
}
