/* written by Aman Mangal <mangalaman93@gmail.com>
 * on July 14, 2014
 * Basic network class, provides send/receive functions
 * manages the connection objects itself (not transparent)
 */

#ifndef BASE_NETWORK_H
#define BASE_NETWORK_H

#define CONNECT_WAIT_TIME 500
#define MAX_CONNECT_TIME 2000
#define WAIT_TIME 1000

#include <boost/thread/thread.hpp>
#include "tcp_connection.h"
using boost::asio::ip::tcp;

void __sleep__(unsigned ms);

class BaseNetwork
{
    unsigned _port;
    void (*_rec_callback)(std::string ip, unsigned port,
        const std::vector<unsigned char>& data, uint64_t request_id);
    boost::thread_group _io_threads;
    boost::asio::io_service *_con_io_service;
    boost::asio::io_service *_data_io_service;
	std::shared_ptr<boost::asio::io_service::work> _con_work;
	std::shared_ptr<boost::asio::io_service::work> _data_work;
    boost::asio::strand *_strand;
    std::map<std::pair<uint32_t, int>, TcpConnection::sptr> _cpool;
    tcp::acceptor *_asocket;

  private:
    std::pair<uint32_t, int> make_ip_port_pair(std::string ip, int port);
    void begin_accept();
    void accept_handler(const boost::system::error_code& ec,
        TcpConnection::sptr tcon);
	void connect(std::string ip, unsigned port);
	void send_helper(std::pair<uint32_t, int> ep,
		const std::vector<unsigned char>& data, uint64_t request_id);

  public:
    // default constructor
    BaseNetwork(unsigned port, void(*)(std::string ip, unsigned port,
        const std::vector<unsigned char>& data, uint64_t request_id),
        unsigned num_io_threads=5);

    // default destructor
    ~BaseNetwork();

    // block until all the io threads exit
    void join();

    // send given length of data to given ip and port
    void send(std::string ip, unsigned port,
        const std::vector<unsigned char>& data, uint64_t request_id=0);

    // send and receive a response in time timeout (in ms)
    // if response lengtj is 0 => timeout occurred
    void send_and_wait_for_response(std::string ip, unsigned port,
        const std::vector<unsigned char>& data,
        std::vector<unsigned char>& response, unsigned timeout);

	// exit
	void stop();
};

#endif
