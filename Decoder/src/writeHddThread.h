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
#include <boost/filesystem.hpp>
namespace WriteHddThread
{
	class Header
	{
	public:
		int32_t bodySize;
		boost::posix_time::ptime ptime;
		int8_t first;
	};

	const int BIT = 0x1;
	const int RAW = 0x10;

	const int HOLD = 0;
	const int DO = 1;
	const int EXIT = -1;

	// CheckOnOff:Ax+B>0?On:Off
	extern double_t  checkA;
	extern double_t  checkB;
}
#endif //_WRITEHDDTHREAD_H

