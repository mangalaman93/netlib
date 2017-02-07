#include <iostream>
#include "tcp_connection.h"
using namespace std;

void receive_handler(std::string address, unsigned port,
                     const std::vector<unsigned char>& data, uint64_t request_id) {
    std::cout<<"received data from: "<<address<<":"<<port;
    std::cout<<" and data is: \"";
    std::cout.write((const char*)data.data(), data.size());
    std::cout<<"\""<<std::endl;
}

int main() {
    boost::asio::io_service is;
    TcpConnection::sptr tcon = std::make_shared<TcpConnection>(is);

    tcon->connect("127.0.0.1", 8080);
    tcon->read(8080, receive_handler);
    std::string data = "this is test data";
    std::vector<unsigned char> to_send(data.begin(), data.end());
    tcon->send(to_send, 0);

    boost::asio::io_service::work work(is);
    is.run();

    return 0;
}
