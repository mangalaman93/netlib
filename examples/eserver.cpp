/* written by Aman Mangal <mangalaman93@gmail.com>
 * on Oct 28, 2014
 * server to test large number of connections
 */

#include "base_network.h"
#include <boost/crc.hpp>
#include <cassert>

const int PORT = 8000;
BaseNetwork *rnet;

void receive_handler(std::string address, unsigned port,
		const std::vector<unsigned char>& data, uint64_t request_id) {
	uint32_t i = ntohl(*((unsigned*)data.data()));
	uint32_t j = ntohl(*((unsigned*)(data.data()+4)));
	int expected = ntohl(*((unsigned*)(data.data()+8)));

	boost::crc_32_type result;
	result.process_bytes(data.data()+12, data.size()-12);
	int calculated = result.checksum();
	if(expected == calculated) {
		std::cout<<"received data "<<i<<","<<j<<std::endl;
	} else {
		std::cout<<"error data "<<i<<","<<j<<"=>";
		std::cout<<expected<<","<<calculated<<std::endl;
	}
}

int main() {
	rnet = new BaseNetwork(PORT, receive_handler, 10);
	rnet->join();
}
