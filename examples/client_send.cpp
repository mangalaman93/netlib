/* written by Aman Mangal <mangalaman93@gmail.com>
   on July 14, 2014
   client to test basic network class
*/

#include <cstring>
#include "base_network.h"

void receive_handler(std::string address, unsigned port,
		const std::vector<unsigned char>& data, uint64_t request_id) {
    std::cout<<"received data from: "<<address<<":"<<port;
    std::cout<<" and data is: \"";
	for(unsigned i=0; i<data.size(); i++) {
		std::cout<<data[i];
	}
	std::cout<<"\""<<std::endl;
}

int main() {
	std::string s = "This is test data!";
	const std::vector<unsigned char> data(s.begin(), s.end());
	std::string address = "127.0.0.1";
	int port = 5001;

	BaseNetwork net(port+1, receive_handler);
	net.send(address, port, data);
	net.send(address, port, data);
	net.join();
}
