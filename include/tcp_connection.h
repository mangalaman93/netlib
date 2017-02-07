/* written by Aman Mangal <mangalaman93@gmail.com>
 * on July 27, 2014
 * TCP connection class, maintains a tcp connection socket
 * (connecting side)
 * NOTE THAT, only send and send_and_wait are THREAD SAFE
 */

#ifndef TCP_NETWORK_H
#define TCP_NETWORK_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <atomic>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "cryptor.h"
#include <deque>
#include <memory>
using boost::asio::ip::tcp;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
    tcp::socket _socket;
    boost::asio::streambuf _buf;
    bool _is_open;
    bool _syn_flag;
	unsigned _rport;
    std::atomic<uint64_t> _crid;
    boost::mutex _rmut;
    std::map<uint64_t, std::pair<boost::condition_variable*,
                                 std::vector<unsigned char>*> > _request_map;
    Cryptor _cryptor;
	std::deque<std::vector<unsigned char>*> _queue;
	boost::asio::io_service::strand _strand;
	void(*_rec_callback)(std::string ip, unsigned port,
        const std::vector<unsigned char>& data, uint64_t request_id);

  private:
	void write_impl(std::vector<unsigned char>* to_send);
	void write();
    void write_handler(const boost::system::error_code& ec,
        unsigned bytes_transferred, std::vector<unsigned char>* to_send);
    void length_handler(const boost::system::error_code& ec, unsigned size);
    void read_handler(const boost::system::error_code& ec, unsigned size,
        unsigned length, uint64_t request_id);

  public:
    typedef std::shared_ptr<TcpConnection> sptr;

  	// default constructor, needs an io service
  	// creates a connection with the server listening at given ip and port
    TcpConnection(boost::asio::io_service& io_service);

    // default destructor
    ~TcpConnection();

    // connects to the server
    bool connect(std::string ip, unsigned port);

    // sends the given data to the server synchronously
    void send_syn(const std::vector<unsigned char>& data);

    // sends the given data to the server asynchronously
    void send(const std::vector<unsigned char>& data, uint64_t request_id);

    // waits for timeout (in ms) time for the response before returning
    void send_and_wait(const std::vector<unsigned char>& data,
        std::vector<unsigned char>& response, unsigned timeout);

    // should only be used for receiving small data
    void read_syn(std::vector<unsigned char>& data, unsigned length);

    // calls the given function when full application level packet is received
    void read(unsigned rport,
        void(*)(std::string ip, unsigned port,
            const std::vector<unsigned char>& data, uint64_t request_id));

    // returns the tcp socket
    tcp::socket& get_socket() { return _socket;}

    // returns cryptor object
    Cryptor* get_cryptor() { return &_cryptor;}

    // bools whether the connection is open
    bool is_open() { return _is_open;}

    // ends the connection with the given ip and given port
    void disconnect();
};

#endif
