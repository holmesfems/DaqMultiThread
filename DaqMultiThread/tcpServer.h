//#pragma once
/*
* tcpserver version 180418.rev3
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
#include <atomic>
#include <future>

namespace TcpServer
{
	class TcpServer
	{
	public:
		TcpServer(boost::asio::io_service &io_service, uint16_t port, std::ostream *os = NULL);
		~TcpServer() {};
		void start_accept();
		void send(std::string msg); 
		//void stop_accept();
		int status = 0;
		bool is_connected(int timeout = 100);
		std::string lastRecv(int timeout = 1000);
		std::string waitRecv();
		bool waitAllDone(int timeout = 100);
		void exit();
		static const int LOST = -1;
		static const int EXIT = -2;
		static const int ONLINE = 1;
		static const int OFFLINE = 0;
		static const int FAIL = -3;

		static const std::string EXIT_MSG;
		static const std::string LOST_MSG;
	private:
		boost::asio::io_service& _io;
		boost::asio::ip::tcp::acceptor _acceptor;
		boost::asio::ip::tcp::socket _socket;
		boost::asio::streambuf _receive_buff;
		std::queue<std::string> _msgQueue;
		std::ostream *_os;
		uint16_t _port;
		std::future<std::string> _receive_msg;
		std::promise<std::string> _receive_msg_writer;
		
		std::future<int> _connection_status;
		std::promise<int> _connection_status_writer;

		//Flags
		std::future<bool> _doneFlag;
		std::promise<bool> _doneFlag_writer;

		std::atomic<bool> _writeDone;

		std::queue<std::string> _recvQueue;

		//uint16_t _port;

		void _on_accept(const boost::system::error_code &err);
		void _async_receive();
		void _on_receive(const boost::system::error_code &err, size_t bytes_transferred);
		void _async_write();
		void _on_write(const boost::system::error_code &err);
		void _refresh_doneFlag();
		void _done();
		void _clear_recvMsg();
		void _refresh_recvMsg();

//		void _on_offline();
//		void _start_listen();
	};
}
#endif //_TCPSERVER_H
