//#pragma once
/*
* tcpServer.h
* Created at 2017/06/25
* Copyright (C) 2017 zhai <holmesfems@gmail.com>
*
* Distributed under terms of the MIT license.
*/
#ifndef _TCPSERVER_H
#define _TCPSERVER_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <queue>
#include "CmdHelper.h"

namespace TcpServer
{
	class TcpServer
	{
	public:
		TcpServer(boost::asio::io_service &io_service, uint16_t port, CmdHelper::CmdHelper &cmdHelper, std::ostream *os = NULL);
		~TcpServer() {};
		void start_accept();
		void send(std::string msg); 
		//void stop_accept();
		int status = 0;
		static const int LOST = -1;
		static const int EXIT = -2;
		static const int ONLINE = 1;
		static const int OFFLINE = 0;
		static const int FAIL = -3;
	private:
		boost::asio::io_service& _io_service;
		boost::asio::ip::tcp::acceptor _acceptor;
		boost::asio::ip::tcp::socket _socket;
		boost::asio::streambuf _receive_buff;
		std::queue<std::string> _msgQueue;
		std::ostream *_os;
		uint16_t _port;
		//uint16_t _port;
		CmdHelper::CmdHelper &_cmdHelper;
		void _on_accept(const boost::system::error_code &err);
		void _async_receive();
		void _on_receive(const boost::system::error_code &err, size_t bytes_transferred);
		void _async_write();
		void _on_write(const boost::system::error_code &err);
//		void _on_offline();
//		void _start_listen();
	};
}
#endif //_TCPSERVER_H
