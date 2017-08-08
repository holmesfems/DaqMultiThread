//#pragma once
/*
* writeHddThread
* Created at 2017/07/28
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/
#ifndef _WRITEHDDTHREAD_H
#define _WRITEHDDTHREAD_H
#include <iostream>
#include <queue>
#include <thread>
#include <atomic>
#include <boost/date_time/posix_time/posix_time.hpp>
namespace WriteHddThread
{
	class WriteParameter
	{
	public:
		int32_t dataSize;
		boost::posix_time::ptime ptime;
		std::vector<double_t> data;
		WriteParameter(boost::posix_time::ptime &ptime, int32_t dataSize = 0, double_t* data = NULL);
		~WriteParameter();
	};

	class Header
	{
	public:
		int32_t bodySize;
		boost::posix_time::ptime ptime;
		int8_t first;
	};

	const int BIT = 0x1;
	const int RAW = 0x10;

	class WriteHddThread
	{
	public:
		WriteHddThread(std::string &targetFileName,int writeFlag=BIT);
		~WriteHddThread();
		void push(WriteParameter& wp);
		void push(boost::posix_time::ptime &ptime, int32_t dataSize = 0, double_t* data = NULL);
		//void stop();
	
	private:
		std::thread *_writeThread;
		std::queue<WriteParameter> _dataQueue;
		std::atomic<int> _writeCmd;
		std::string _targetFileName;
		void _threadFunction();
		int _writeFlag;
	};


	const int HOLD = 0;
	const int DO = 1;
	const int EXIT = -1;

	// CheckOnOff:Ax+B>0?On:Off
	extern double_t  checkA;
	extern double_t  checkB;
}
#endif //_WRITEHDDTHREAD_H

