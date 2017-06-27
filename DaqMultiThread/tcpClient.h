//#pragma once
/*
* tcpClient.h
* Created at 2017/06/25
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/
#ifndef _TCPCLIENT_H
#define _TCPCLIENT_H

#include <iostream>
#include <queue>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace TcpClient
{
	class TcpClient
	{

	public:
		TcpClient(boost::asio::io_service& io_service);
		~TcpClient() {}
		void connect(std::string ip_address, uint16_t port);
		void send(std::string msg);
		int status = 0;
		static const int LOST = -1;
		static const int EXIT = -2;
		static const int ONLINE = 1;
		static const int OFFLINE = 0;
		static const int FAIL = -3;
	private:
		boost::asio::io_service &_io;
		boost::asio::ip::tcp::socket _socket;
		boost::asio::io_service::strand _strand;
		std::queue<std::string> _msgQueue;
		boost::asio::streambuf _receive_buff;

		void _on_connect(const boost::system::error_code &err);
		void _async_receive();
		void _on_receive(const boost::system::error_code &err, size_t bytes_transferred);
		void _async_write();
		void _on_write(const boost::system::error_code &err);

	};
}

#endif
