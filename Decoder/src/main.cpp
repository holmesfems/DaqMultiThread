
#include <stdio.h>
#include <iostream>
#include <queue>
#include <vector>
#include <sstream>
//#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <fstream>

#include "writeHddThread.h"
using int32 = int;
using float64 = double;
using int8 = char;

int32       bufferSize = 80814;

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
