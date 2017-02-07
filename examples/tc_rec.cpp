#include <iostream>
#include "tcp_connection.h"

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

    tcp::acceptor acceptor(is, tcp::endpoint(tcp::v4(), 8080));
    acceptor.accept(tcon->get_socket());
    tcon->read(8080, receive_handler);

#if defined _WIN32 || defined WIN32 || defined OS_WIN64 || defined _WIN64 || defined WIN64 || defined WINNT
	Sleep(5000);
#else
#include <unistd.h>
    sleep(5);
#endif

    std::string data = "this is test data";
    std::vector<unsigned char> to_send(data.begin(), data.end());
    tcon->send(to_send, 0);

    boost::asio::io_service::work work(is);
    is.run();

    return 0;
}
