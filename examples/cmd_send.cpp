/* written by Aman Mangal
   on August 23, 2014
   client to test memory leak
*/

#include "base_network.h"
#include <cstdlib>
#define PORT 5000

void receive_handler(std::string address, unsigned port,
        const std::vector<unsigned char>& data, uint64_t request_id) {
    std::cout<<"received data from: "<<address<<":"<<port;
    std::cout<<" and data is: \"";
    for(unsigned i=0; i<data.size(); i++) {
        std::cout<<data[i];
    }
    std::cout<<"\""<<std::endl;
}

char randchar() {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const unsigned max_index = (sizeof(charset) - 1);
    return charset[rand() % max_index];
}

std::string random_string(unsigned length) {
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}

int main()
{
    srand(time(NULL));
    int input=3, length, port;
    std::string address;
    std::string data;
    std::vector<unsigned char> vdata;
    BaseNetwork net(PORT, receive_handler);

    while(input != 0) {
        std::cout<<"0 -- exit"<<std::endl;
        std::cout<<"1 -- send data to any server"<<std::endl;
        std::cout<<"2 -- disconnect with a server"<<std::endl;
        std::cout<<"Enter a command: ";
        std::cin>>input;

        switch(input) {
            case 0:
                break;
            case 1:
                std::cout<<"Enter the ip: ";
                std::cin>>address;
                std::cout<<"Enter the port number: ";
                std::cin>>port;
                std::cout<<"Enter the length of the data to be sent: ";
                std::cin>>length;
                data = random_string(length);
                vdata.assign(data.begin(), data.end());
                net.send(address, port, vdata);
                break;
            case 2:
                std::cout<<"Enter the ip: ";
                std::cin>>address;
                std::cout<<"Enter the port number: ";
                std::cin>>port;
                net.disconnect(address, port);
                break;
            default:
                break;
        }
    }
}
