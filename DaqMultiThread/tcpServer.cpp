/*
* tcpServer.cpp
* version 180418rev4
* Created at 2017/06/25
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/
#include "tcpServer.h"
#include <sstream>

namespace TcpServer
{
	const std::string TcpServer::EXIT_MSG("EXIT SERVER");
	const std::string TcpServer::LOST_MSG("LOST CONNECTION");

	TcpServer::TcpServer(boost::asio::io_service &io_service, uint16_t port, std::ostream *os)
		: _io(io_service),
		_socket(io_service),
		_os(os),
		_port(port),
		_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	{
		if (_os == NULL) //dummy
			_os = new std::ostringstream();
		_writeDone = true;
	}

	void TcpServer::start_accept()
	{
		_connection_status = _connection_status_writer.get_future().share();
		_acceptor.async_accept(
			_socket,
			boost::bind(&TcpServer::_on_accept, this, boost::asio::placeholders::error));
	}

	void TcpServer::send(std::string msg)
	{
		_msgQueue.push(msg);
		_refresh_recvMsg();
		if (_writeDone)
			_io.post(boost::bind(&TcpServer::_async_write, this));
	}

	bool TcpServer::is_connected(int timeout)
	{
		try
		{
			if (timeout < 0 || _connection_status.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::ready)
			{
				return (_connection_status.get()) == ONLINE;
			}
			else
			{
				return false;
			}
		}
		catch (std::future_error e)
		{
			return status == ONLINE;
		}
	}

	bool TcpServer::is_online(int timeout)
	{
		if (is_connected(timeout))
		{
			return status == ONLINE;
		}
		else
		{
			return false;
		}
	}

	std::string TcpServer::lastRecv(int timeout)
	{
		_refresh_doneFlag();
		std::string ret;
		if (timeout < 0 || _receive_msg.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::ready)
		{
			try
			{
				ret = _receive_msg.get();
			}
			catch (std::future_error e)
			{
				std::ostringstream oss;
				oss << "Future Error:" << e.what();
				ret = oss.str();
			}
		}
		else
		{
			ret = "MSG NOT VALID (Timeout)";
		}
		_done();
		return ret;
	}

	std::string TcpServer::waitRecv()
	{
		_refresh_doneFlag();
		_clear_recvMsg();
		std::string ret;
		try
		{
			ret = _receive_msg.get();
		}
		catch (std::future_error e)
		{
			std::ostringstream oss;
			oss << "Future error:" << e.what();
			ret = oss.str();
		}
		_done();
		return ret;
	}

	bool TcpServer::waitAllDone(int timeout)
	{
		if (!_doneFlag.valid())
		{
			return true;
		}
		if (timeout < 0 || _doneFlag.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::ready)
		{
			try
			{
				return _doneFlag.get();
			}
			catch (std::future_error e)
			{
				std::ostringstream oss;
				oss << "Future Error:" << e.what();
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	void TcpServer::exit()
	{
		try
		{
			status = EXIT;
			_connection_status_writer.set_value(status);
		}
		catch (std::future_error e)
		{
			if (e.code() != std::future_errc::promise_already_satisfied)
			{
				*_os << "Future error:";
				*_os << e.what();
			}
		}
		try
		{
			_receive_msg_writer.set_value(EXIT_MSG);
		}
		catch (std::future_error e)
		{
			if (e.code() != std::future_errc::promise_already_satisfied)
			{
				*_os << "Future error:";
				*_os << e.what();
			}
		}
		waitAllDone();
		_io.stop();
	}

	void TcpServer::_on_accept(const boost::system::error_code &err)
	{
		if (err)
		{
			*_os << "Failed to accept : " << err.message() << std::endl;
			status = FAIL;
			_connection_status_writer.set_value(status);
		}
		else
		{
			*_os << "Connection accepted" << std::endl;
			status = ONLINE;
			_connection_status_writer.set_value(status);
			_async_receive();
		}
	}

	void TcpServer::_async_receive()
	{
		boost::asio::async_read_until(
			_socket,
			_receive_buff,
			"\n",
			boost::bind(&TcpServer::_on_receive, this,
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

	void TcpServer::_on_receive(const boost::system::error_code &err, size_t bytes_transferred)
	{
		//std::cout << "now status = " << status << std::endl;
		//std::cout.flush();
		if (status == EXIT) return;
		if (err && err != boost::asio::error::eof) {
			*_os << "receive failed: " << err.message() << std::endl;
			status = FAIL;
			//start_accept();
		}
		else {
			
			std::string data = std::string(boost::asio::buffer_cast<const char*>(_receive_buff.data()), bytes_transferred);
			data = data.substr(0, data.length() - 1);
			
			if (bytes_transferred == 0 && err == boost::asio::error::eof)
			{
				*_os << "connection lost!" << std::endl;
				if (status != EXIT)
				{
					status = LOST;
					//start_accept();
				}
				
				try
				{
					_receive_msg_writer.set_value(LOST_MSG);
				}
				catch (std::future_error e)
				{
					if (e.code() != std::future_errc::promise_already_satisfied)
					{
						*_os << "Future error:";
						*_os << e.what();
					}
				}
					
//				_on_offline();
			}
			else
			{
				*_os << "recieve succeed " << "length = " << bytes_transferred << std::endl;
				*_os << "cmd = \"" << data << "\"" << std::endl;
				try 
				{
					_receive_msg_writer.set_value(data);
				}
				catch (std::future_error e)
				{
					if (e.code() != std::future_errc::promise_already_satisfied)
					{
						*_os << "Promise error occured:" << e.what() << std::endl;
					}
					else
					{
						*_os << "Already satisfied!" << std::endl;
					}
				}
				if (!(data == "exit"))
				{
					_receive_buff.consume(_receive_buff.size());
					_async_receive();
				}
				else
				{
					status = EXIT;
				}
			}
		}
	}

	void TcpServer::_async_write()
	{
		if (_msgQueue.empty())
		{
			_writeDone = true;
			return;
		}
		_writeDone = false;
		std::string msg = _msgQueue.front();
		_msgQueue.pop();
		msg = msg + "\n";
		boost::asio::async_write(
			_socket,
			boost::asio::buffer(msg.c_str(), msg.length()),
			boost::bind(&TcpServer::_on_write, this,
				boost::asio::placeholders::error));
	}

	void TcpServer::_on_write(const boost::system::error_code & err)
	{
		if (err)
		{
			*_os << "write failed" << std::endl;
		}
		else
		{
			*_os << "write succeed" << std::endl;
			if (!_msgQueue.empty())
				_async_write();
			else
				_writeDone = true;
		}
	}

	void TcpServer::_refresh_doneFlag()
	{
		std::promise<bool> newpromise;
		_doneFlag_writer.swap(newpromise);
		_doneFlag = _doneFlag_writer.get_future().share();
	}

	void TcpServer::_done()
	{
		try
		{
			_doneFlag_writer.set_value(true);
		}
		catch (std::future_error e)
		{
			if (e.code() != std::future_errc::promise_already_satisfied)
			{
				*_os << "Promise error occured: " << e.what() << std::endl;
			}
		}
	}

	void TcpServer::_clear_recvMsg()
	{
		std::promise<std::string> newpromise;
		_receive_msg_writer.swap(newpromise);
		_receive_msg = _receive_msg_writer.get_future();
	}

	void TcpServer::_refresh_recvMsg()
	{
		if (!_receive_msg.valid() || _receive_msg.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
		{
			_clear_recvMsg();
		}
	}
}