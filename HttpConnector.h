#ifndef HTTP_CONNECTOR_H_
#define HTTP_CONNECTOR_H_

#include <boost/asio/deadline_timer.hpp>
#include <istream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;

class HttpConnector
{
	public:
		HttpConnector(boost::asio::io_service& ios, uint32_t start, uint32_t end)
			: stopped_(false)
			  , ios(ios)
			  , socket_(ios)
			  , deadline_(ios)
			  , start_range(start)
			  , end_range(end)
			  , current_ip(start-1)
	{
	}

		// Called by the user of the HttpConnector class to initiate the connection process.
		// The endpoint iterator will have been obtained using a tcp::resolver.
		void start()
		{
			start_connect();

			// Start the deadline actor. You will note that we're not setting any
			// particular deadline here. Instead, the connect and input actors will
			// update the deadline prior to each asynchronous operation.
			deadline_.async_wait(boost::bind(&HttpConnector::check_deadline, this));
			ios.run();
		}

		// This function terminates all the actors to shut down the connection. It
		// may be called by the user of the HttpConnector class, or by the class itself in
		// response to graceful termination or an unrecoverable error.
		void stop()
		{
			stopped_ = true;
			boost::system::error_code ignored_ec;
			socket_.close(ignored_ec);
			deadline_.cancel();
		}

	private:
		void start_connect()
		{
			uint32_t next_ip = get_next_ip();

			if (next_ip == 0)
				return stop();

			deadline_.expires_from_now(boost::posix_time::seconds(2));
			std::cout << "Connecting to " << next_ip << std::endl;
			boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address(boost::asio::ip::address_v4(next_ip)), 80);
			socket_.async_connect(endpoint, boost::bind(&HttpConnector::handle_connect, this, boost::asio::placeholders::error));
		}

		void handle_connect(const boost::system::error_code& ec)
		{
			if (stopped_)
				return;

			// The async_connect() function automatically opens the socket at the start
			// of the asynchronous operation. If the socket is closed at this time then
			// the timeout handler must have run first.
			if (!socket_.is_open())
			{
				std::cout << "Connect timed out\n";

				// Try the next available endpoint.
				start_connect();
			}

			// Check if the connect operation failed before the deadline expired.
			else if (ec)
			{
				std::cout << "Connect error: " << ec.message() << "\n";

				// We need to close the socket used in the previous connection attempt
				// before starting a new one.
				//socket_.close();

				// Try the next available endpoint.
				start_connect();
			}

			// Otherwise we have successfully established a connection.
			else
			{
				std::cout << "connection stablished" << std::endl;
				get_ashiyane_page();
			}
		}

		void get_ashiyane_page()
		{
			std::cout << "get ashiyane page\n";
			deadline_.expires_from_now(boost::posix_time::seconds(2));
			try {
				boost::asio::streambuf request;
				std::ostream request_stream(&request);
				request_stream << "HEAD /themedata/secserv.jpg HTTP/1.0\r\n";
				//request_stream << "Host: ashiyane.ir\r\n";
				request_stream << "Accept: */*\r\n";
				request_stream << "Connection: close\r\n\r\n";

				// Send the request.
				boost::asio::write(socket_, request);

				// Read the response status line. The response streambuf will automatically
				// grow to accommodate the entire line. The growth may be limited by passing
				// a maximum size to the streambuf constructor.
				boost::asio::streambuf response;
				std::cout << "Waiting to read" << std::endl;
				boost::asio::async_read_until(socket_, response_, "\r\n",
						boost::bind(&HttpConnector::handle_read_status_line, this,
							boost::asio::placeholders::error));

			}
			catch (std::exception& e)
			{
				std::cout << "Exception: " << e.what() << "\n";
			}

		}

		void handle_read_status_line(const boost::system::error_code& err)
		{
			if (!err)
			{
				std::istream response_stream(&response_);
				std::string http_version;
				response_stream >> http_version;
				unsigned int status_code;
				response_stream >> status_code;
				std::string status_message;
				std::getline(response_stream, status_message);
				if (status_code == 200)
				{
					print_ip(current_ip);
				}
				// Check that response is OK.

				socket_.close();
			}

			else
				std::cout << "read error: " << err.message() << std::endl;

			start_connect();
		}

		void print_ip(int ip)
		{
			unsigned char bytes[4];
			bytes[0] = ip & 0xFF;
			bytes[1] = (ip >> 8) & 0xFF;
			bytes[2] = (ip >> 16) & 0xFF;
			bytes[3] = (ip >> 24) & 0xFF;
			printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);
		}

		void check_deadline()
		{
			if (stopped_)
				return;

			// Check whether the deadline has passed. We compare the deadline against
			// the current time since a new asynchronous operation may have moved the
			// deadline before this actor had a chance to run.
			if (deadline_.expires_at() <= deadline_timer::traits_type::now())
			{
				std::cout << "dead line passed" << std::endl;
				// The deadline has passed. The socket is closed so that any outstanding
				// asynchronous operations are cancelled.
				socket_.close();

				// There is no longer an active deadline. The expiry is set to positive
				// infinity so that the actor takes no action until a new deadline is set.
				deadline_.expires_at(boost::posix_time::pos_infin);
			}

			// Put the actor back to sleep.
			deadline_.async_wait(boost::bind(&HttpConnector::check_deadline, this));
		}

		uint32_t get_next_ip()
		{
			++current_ip;
			if (current_ip > end_range)
				return 0;		// Invalid IP

			return current_ip;
		}

	private:
		boost::asio::io_service& ios;
		bool stopped_;
		uint32_t start_range;
		uint32_t end_range;
		uint32_t current_ip;
		tcp::socket socket_;
		deadline_timer deadline_;
		boost::asio::streambuf response_;
};

#endif
