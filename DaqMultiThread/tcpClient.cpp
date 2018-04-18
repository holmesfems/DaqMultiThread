/*
* tcpClient.cpp
* version 180418
* Created at 2017/06/25
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/

#include "tcpClient.h"
#include <thread>
#include <chrono>
#include <sstream>

namespace TcpClient
{
	TcpClient::TcpClient(boost::asio::io_service & io_service):
		_io(io_service),
		_socket(io_service),
		_strand(io_service)
	{
	}

	void TcpClient::connect(std::string ip_address, uint16_t port)
	{
		std::promise<int> csw;
		_connection_status_writer.swap(csw);
		_connection_status = _connection_status_writer.get_future();
		_socket.async_connect(
			boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port),
			boost::bind(&TcpClient::TcpClient::_on_connect, this, boost::asio::placeholders::error));
	}

	void TcpClient::send(std::string msg)
	{
		bool is_sending = !_msgQueue.empty();
		_msgQueue.push(msg);
		std::promise<std::string> newWriter;
		_receive_msg_writer.swap(newWriter);
		_receive_msg = _receive_msg_writer.get_future(); // reset future-promise
		if(!is_sending)
			_io.post(boost::bind(&TcpClient::_async_write, this));
	}

	bool TcpClient::is_connected(int timeout) 
	{
		try
		{
			if (_connection_status.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::ready)
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

	std::string TcpClient::lastRecv(int timeout)
	{

		if (_receive_msg.wait_for(std::chrono::milliseconds(timeout)) == std::future_status::ready)
		{
			try
			{
				return _receive_msg.get();
			}
			catch (std::future_error e)
			{
				std::ostringstream oss;
				oss << "Future Error:" << e.what();
				return oss.str();
			}
		}
		else
		{
			return "MSG NOT VALID (Timeout)";
		}
	}

	void TcpClient::exit()
	{
		_io.stop();
	}

	void TcpClient::_on_connect(const boost::system::error_code & err)
	{
		if (err) {
			std::cout << "connect failed : " << err.message() << std::endl;
			status = FAIL;
			_connection_status_writer.set_value(status);
		}
		else {
			std::cout << "connected" << std::endl;
			status = ONLINE;
			_connection_status_writer.set_value(status);
			_async_receive();
		}
	}

	void TcpClient::_async_receive()
	{
		boost::asio::async_read_until(
			_socket,
			_receive_buff,
			"\n",
			boost::bind(&TcpClient::_on_receive, this,
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

	void TcpClient::_on_receive(const boost::system::error_code &err, size_t bytes_transferred)
	{
		if (status < 1) return;
		if (err && err != boost::asio::error::eof) {
			std::cout << "receive failed: " << err.message() << std::endl;
			status = -1;
		}
		else {
			std::cout << "recieve succeed " << "length = " << bytes_transferred << std::endl;
			if (bytes_transferred == 0 && err == boost::asio::error::eof)
			{
				std::cout << "Connection lost!" << std::endl;
				status = LOST;
				return;
			}
			std::string data = std::string(boost::asio::buffer_cast<const char*>(_receive_buff.data()), bytes_transferred);
			data = data.substr(0, data.length() - 1);
			std::cout << "reply = \"" << data << "\"" << std::endl;
			try
			{
				_receive_msg_writer.set_value(data); //Msg is ready
			}
			catch (std::future_error e)
			{
				if (e.code() != std::future_errc::promise_already_satisfied)
				{
					std::cout << "Promise error occured: " << e.what() << std::endl;
				}
			}
			_receive_buff.consume(_receive_buff.size());
			_async_receive();
			//if(status == ONLINE)
				//_async_receive();
		}
	}

	void TcpClient::_async_write()
	{
		if (_msgQueue.empty()) return;
		std::string msg = _msgQueue.front();
		_msgQueue.pop();
		if (msg == "exit") status = -1;
		if (msg == "clientExit") status = -1;
		msg = msg + "\n";
		boost::asio::async_write(
			_socket,
			boost::asio::buffer(msg.c_str(), msg.length()),
			boost::bind(&TcpClient::_on_write, this,
				boost::asio::placeholders::error));
	}

	void TcpClient::_on_write(const boost::system::error_code & err)
	{
		if (err)
		{
			std::cout << "write failed: " << err.message() << std::endl;
		}
		else
		{
			std::cout << "write succeed" << std::endl;
			if (!_msgQueue.empty()) _async_write();
		}
		//std::cout << "exit_on_write" << std::endl;
	}
}