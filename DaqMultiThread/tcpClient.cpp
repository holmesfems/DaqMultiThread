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
		_io(io_service),
		_socket(io_service),
		_strand(io_service)
	{
	}

	void TcpClient::connect(std::string ip_address, uint16_t port)
	{
		_socket.async_connect(
			boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port),
			boost::bind(&TcpClient::TcpClient::_on_connect, this, boost::asio::placeholders::error));
	}

	void TcpClient::send(std::string msg)
	{
		bool is_sending = !_msgQueue.empty();
		_msgQueue.push(msg);
		if(!is_sending)
			_io.post(boost::bind(&TcpClient::_async_write, this));
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
			std::string data = std::string(boost::asio::buffer_cast<const char*>(_receive_buff.data()), bytes_transferred);
			data = data.substr(0, data.length() - 1);
			std::cout << "reply = \"" << data << "\"" << std::endl;
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