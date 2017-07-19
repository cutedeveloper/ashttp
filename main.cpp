#include <iostream>
#include <thread>

#include "HttpConnector.h"

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
			threads[i] = new std::thread([connectors, i]() {connectors[i]->start(); });
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
