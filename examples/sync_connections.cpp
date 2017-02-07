/* written by Aman Mangal <mangalaman93@gmail.com>
 * on Oct 28, 2014
 * client to test large number of connections
 */

#include <algorithm>
#include <boost/thread.hpp>
#include <cstdlib>
#include <random>
#include "base_network.h"

#define DATA_SIZE (1024*1024)
std::string ADDRESS = "127.0.0.1";
const int PORT = 8000;
const int NUM_THREADS = 10;
const int NUM_STRINGS = 3;

boost::thread* threads[NUM_THREADS];
BaseNetwork* rnet;

void receive_handler(std::string address, unsigned port,
		const std::vector<unsigned char>& data, uint64_t request_id) {
	uint32_t i = *((unsigned*)data.data());
	uint32_t j = *((unsigned*)(data.data()+4));

	std::cout<<"received confirmation for "<<i<<","<<j<<std::endl;
	std::vector<unsigned char> to_send(data.begin(), data.begin()+8);
	rnet->send(address, port, to_send, request_id);
}

void receive_handler2(std::string address, unsigned port,
		const std::vector<unsigned char>& data, uint64_t request_id) {
	assert(0);
}

unsigned char randchar() {
	const unsigned char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	const unsigned max_index = (sizeof(charset)-1);
	return charset[rand() % max_index];
}

void run(int id) {
	BaseNetwork net(PORT+id+1, receive_handler2);
	srand(time(NULL));

	std::vector<unsigned char> to_send;
	for(int i = 0; i<NUM_STRINGS; i++) {
		to_send.resize(0);
		to_send.reserve(DATA_SIZE+8);
		for(int j=0; j<4; j++) {
			to_send.push_back(((char*)&id)[j]);
		}
		for(int j=0; j<4; j++) {
			to_send.push_back(((char*)&i)[j]);
		}
		for(unsigned j=0; j<DATA_SIZE; j++) {
			to_send.push_back(randchar());
		}

		std::vector<unsigned char> response;
		net.send_and_wait_for_response(ADDRESS, PORT, to_send, response, 3000);
		if(response.size() == 0) {
			std::cout<<"not gonna receive response for "<<id<<","<<i<<std::endl;
		}
	}

	net.join();
}

int main() {
	rnet = new BaseNetwork(PORT, receive_handler);

	for (int i = 0; i<NUM_THREADS; i++) {
		threads[i] = new boost::thread(run, i);
		std::cout<<"sender thread: "<<threads[i]->get_id()<<std::endl;
	}

	for (int i = 0; i<NUM_THREADS; i++) {
		threads[i]->join();
	}

	rnet->join();
	delete rnet;
}
