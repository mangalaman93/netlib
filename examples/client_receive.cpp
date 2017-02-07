/* written by Aman Mangal
   on July 14, 2014
   server to test basic network class
*/

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
    BaseNetwork net(5001, receive_handler);
    net.join();

    return 0;
}
