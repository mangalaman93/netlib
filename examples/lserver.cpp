/* written by Aman Mangal <mangalaman93@gmail.com>
 * on Oct 28, 2014
 * server to test large number of connections
 */

#include "base_network.h"

const int PORT = 8000;
BaseNetwork *rnet;

void receive_handler(std::string address, unsigned port,
		const std::vector<unsigned char>& data, uint64_t request_id) {
	uint32_t i = *((unsigned*)data.data());
	uint32_t j = *((unsigned*)(data.data()+4));
	std::cout<<"received confirmation for "<<i<<","<<j<<std::endl;

	std::vector<unsigned char> to_send(data.begin(), data.begin()+8);
	rnet->send(address, port, to_send);
}

int main() {
	rnet = new BaseNetwork(PORT, receive_handler);
	rnet->join();
}
