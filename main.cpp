#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <limits>
#include <thread>

using boost::asio::ip::tcp;
using std::cout;

class HttpConnector
{
public:
	HttpConnector(boost::asio::io_service& ios, uint32_t start, uint32_t end)
		: io_service(ios)
		, start_range(start)
		, end_range(end)
		, current_ip(start - 1)
		, socket(io_service)
	{
		connect_to_next_ip();
//		std::thread th{ [this]() {this->io_service.run(); } };
//		th.detach();
	}

	void run()
	{
		io_service.run();
	}
private:
	uint32_t get_next_ip()
	{
		++current_ip;
		if (current_ip > end_range)
			return 0;		// Invalid IP

		return current_ip;
	}

	void connect_to_next_ip()
	{
		uint32_t next_ip = get_next_ip();

		if (next_ip == 0)
			return;

		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address(boost::asio::ip::address_v4(next_ip)), 80);
		socket.async_connect(endpoint, boost::bind(&HttpConnector::handle_connect, this, boost::asio::placeholders::error));
	}

	void handle_connect(const boost::system::error_code& error_code)
	{
		if (error_code)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(10));
//			socket.close();
			connect_to_next_ip();
			return;
		}

		get_ashiyane_page();
	}

	void get_ashiyane_page()
	{
		try {
			boost::asio::streambuf request;
			std::ostream request_stream(&request);
			request_stream << "GET /themedata/secserv.jpg HTTP/1.0\r\n";
			request_stream << "Host: ashiyane.ir\r\n";
			request_stream << "Accept: */*\r\n";
			request_stream << "Connection: close\r\n\r\n";

			// Send the request.
			boost::asio::write(socket, request);

			// Read the response status line. The response streambuf will automatically
			// grow to accommodate the entire line. The growth may be limited by passing
			// a maximum size to the streambuf constructor.
			boost::asio::streambuf response;
			boost::asio::read_until(socket, response, "\r\n");

			// Check that response is OK.
			std::istream response_stream(&response);
			std::string http_version;
			response_stream >> http_version;
			unsigned int status_code;
			response_stream >> status_code;
			std::string status_message;
			std::getline(response_stream, status_message);
			if (!response_stream || http_version.substr(0, 5) != "HTTP/" || status_code != 200)
				cout << status_code << std::endl;
			else if (status_code == 200)
				print_ip(current_ip);

			socket.close();
			connect_to_next_ip();
		}
		catch (std::exception& e)
		{
			std::cout << "Exception: " << e.what() << "\n";
		}

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

	boost::asio::io_service& io_service;
	uint32_t start_range;
	uint32_t end_range;
	uint32_t current_ip;
	boost::asio::ip::tcp::socket socket;
};

union IP
{
	uint32_t raw;
	uint8_t numbers[4];
};
int main(int argc, char* argv[])
{
	try
	{
		uint32_t start = std::stol(argv[1]);
		uint32_t end = std::stol(argv[2]);
		boost::asio::io_service io_service;
		int number_of_threads = 10;
		std::thread** threads;
		HttpConnector** connectors;
		threads = new std::thread*[number_of_threads];
		connectors = new HttpConnector*[number_of_threads];

		int sections = (end - start + 1) / number_of_threads;
		for (int i = 0; i < number_of_threads; ++i)
		{
			uint32_t start_section = i * sections + start;
			uint32_t end_section = (i + 1) * sections + start;
			connectors[i] = new HttpConnector(io_service, start_section, end_section);
			threads[i] = new std::thread([connectors, i]() {connectors[i]->run(); });
		}

		for (int i = 0; i < number_of_threads; ++i)
			threads[i]->join();

		std::cout << "Terminating...." << std::endl;
		for (int i = 0; i < number_of_threads; ++i)
		{
			delete connectors[i];
			delete threads[i];
		}

		delete []threads;
		delete []connectors;
		//io_service.run();
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}

	return 0;
}
