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
}