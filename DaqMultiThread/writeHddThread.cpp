#include "writeHddThread.h"
#include <fstream>
#include <functional>
namespace WriteHddThread
{
	double_t checkA = -1.0;
	double_t checkB = 1.0;

	bool checkOnOff(double_t datapoint)
	{
		return ((checkA*datapoint + checkB) > 0);
	}

	WriteParameter::WriteParameter(boost::posix_time::ptime & ptime, int32_t dataSize, double_t * data)
	{
		this->ptime = ptime;
		this->dataSize = dataSize;
		this->data = std::vector<double_t>(data, data + dataSize);
	}

	WriteParameter::~WriteParameter()
	{
	}

	/**
	*	Write the digital data to a file
	*	Format:
	*	[Header]
	*	    [int32](Count of the body bock,not includes the header)
	*		[ptime](timestamp)
	*		[int8](first status(0 for off or 1 for on))
	*	[Body]
	*	    \Loop until rear
	*			[int32](position's difference since one change to the last change)
	*		[int32](rear's distance from last change
	*	Example:
	*	data=11000111000111000
	*	[Header]=6,(time),1
	*	[Body]=233333
	*	\input
	*		filename: the name of target file,open with "wb+"
	*		dataNumber: the number of data block to read ,0 or 1
	*		dataSize: buffer size of a data block
	*		ptime: timestamp
	*/
	int writeEncode(std::ofstream &ofs, WriteParameter &wp)
	{
		Header header;
		if (wp.dataSize == 0 || wp.data.empty()) throw "Ill data";
		std::vector<int32_t> body;
		//encode
		if (checkOnOff(wp.data[0]))
		{
			header.first = 1;
		}
		else
		{
			header.first = 0;
		}
		int32_t i = 0;
		int32_t offset = 1;
		bool status = !(header.first == 0);
		for (i = 1; i < wp.dataSize; i++, offset++)
		{
			if (checkOnOff(wp.data[i]) ^ status)
			{
				status = !status;
				body.push_back(offset);
				offset = 0;
			}
		}
		body.push_back(offset);
		header.bodySize = body.size();
		header.ptime = wp.ptime;
		//encode_end
		//writeFileMutex.lock();
		ofs.write((char*)&header, sizeof(Header));
		ofs.write((char*)body.data(), sizeof(int32_t)*body.size());
		//writeFileMutex.unlock();
		return 0;
	}

	int writeRaw(std::ofstream &ofs, WriteParameter &wp)
	{
		ofs.write((char*)wp.data.data(), sizeof(double_t)*wp.dataSize);
		return 0;
	}

	WriteHddThread::WriteHddThread(boost::filesystem::path targetFilePath, int writeFlag):
		_targetFilePath(targetFilePath),
		_writeFlag(writeFlag)
	{
		_writeCmd = HOLD;
		_writeThread = new std::thread(std::bind(&WriteHddThread::_threadFunction, this));
	}


	WriteHddThread::~WriteHddThread()
	{
		//std::cout << "calling Destructor of WriteHddThread" << std::endl;
		_writeCmd = EXIT;
		if (_writeThread)
		{
			_writeThread->join();
			delete _writeThread;
		}
	}

	void WriteHddThread::push(WriteParameter &wp)
	{
		_dataQueue.push(std::move(wp));
	}

	void WriteHddThread::push(boost::posix_time::ptime &ptime, int32_t dataSize, double_t* data)
	{
		WriteParameter *wp = new WriteParameter(ptime, dataSize, data);
		push(*wp);
	}

	void  WriteHddThread::_threadFunction()
	{
		boost::filesystem::ofstream ofs_bit;
		if (_writeFlag & BIT)
		{
			ofs_bit.open(_targetFilePath.replace_extension("dat"), std::ios::binary | std::ios::app);
			if (!ofs_bit.is_open()) std::cout << "Error opening file:" << _targetFilePath.string() << std::endl;
		}
		boost::filesystem::ofstream ofs_raw;
		if (_writeFlag & RAW)
		{
			ofs_raw.open(_targetFilePath.replace_extension("raw"), std::ios::binary | std::ios::app);
			if (!ofs_raw.is_open()) std::cout << "Error opening raw file:" << _targetFilePath.string() << std::endl;
		}
		while (true)
		{
			if (_writeCmd == EXIT && _dataQueue.size() == 0) break;
			if (_dataQueue.size() > 0)
			{
				if (_writeFlag & BIT)
					writeEncode(ofs_bit, _dataQueue.front());
				if (_writeFlag & RAW)
					writeRaw(ofs_raw, _dataQueue.front());
				_dataQueue.pop();
			}
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		if (ofs_bit.is_open())
			ofs_bit.close();
		if (ofs_raw.is_open())
			ofs_raw.close();
	}
}