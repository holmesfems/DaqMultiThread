/*
* tcpClient.cpp
* Created at 2017/06/25
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/

#include "tcpClient.h"
namespace TcpClient
{
	TcpClient::TcpClient(boost::asio::io_service & io_service):
		_io_service(io_service),
		_socket(io_service)
	{
	}

	void TcpClient::connect(std::string ip_address, uint16_t port)
	{
		_socket.async_connect(
			boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port),
			boost::bind(&TcpClient::TcpClient::_on_connect, this, boost::asio::placeholders::error));
	}

	void TcpClient::_on_connect(const boost::system::error_code & err)
	{
		if (err) {
			std::cout << "connect failed : " << err.message() << std::endl;
		}
		else {
			std::cout << "connected" << std::endl;
			status = ONLINE;
			std::string msg;
			//_async_receive();
			std::getline(std::cin, msg);
			if (msg == "exit") status = EXIT;
			//std::cin >> msg;
			//std::cout << "sending:" << msg << std::endl;
			_async_write(msg + "\n");
			std::cout.flush();
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		std::cout << "exit_on_connect" << std::endl;
		std::cout.flush();
	}

	void TcpClient::_async_receive()
	{
		
		/*boost::asio::async_read_until(
			_socket,
			_receive_buff,
			"\n",
			boost::bind(&TcpClient::_on_receive, this,
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));*/
		size_t bytes_transferred;
		boost::system::error_code err;
		bytes_transferred = boost::asio::read_until(_socket, _receive_buff, "\n", err);
		_on_receive(err, bytes_transferred);
	}

	void TcpClient::_on_receive(const boost::system::error_code &err, size_t bytes_transferred)
	{
		if (err && err != boost::asio::error::eof) {
			std::cout << "receive failed: " << err.message() << std::endl;
		}
		else {
			std::cout << "recieve succeed " << "length = " << bytes_transferred << std::endl;
			std::string data = std::string(boost::asio::buffer_cast<const char*>(_receive_buff.data()), bytes_transferred);
			data = data.substr(0, data.length() - 1);
			std::cout << "reply = \"" << data << "\"" << std::endl;
			_receive_buff.consume(_receive_buff.size());
			//if(status == ONLINE)
				//_async_receive();
		}
	}

	void TcpClient::_async_write(std::string msg)
	{
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
			if (status == ONLINE)
			{
				_async_receive();
				std::string msg;
				std::getline(std::cin, msg);
				_async_write(msg + "\n");
				if (msg == "exit")
				{
					status = EXIT;
				}
			}
		}
		std::cout << "exit_on_write" << std::endl;
	}

	
}