/* written by Aman Mangal <mangalaman93@gmail.com>
 * on July 27, 2014
 * TCP connection class, maintains a tcp connection socket
 * (connection sending side)
*/

#include "tcp_connection.h"

void rec_callback(std::string ip, unsigned port,
                  const std::vector<unsigned char>& data, uint64_t request_id) {
    // do nothing
}

uint64_t htonll(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return n;
#else
    return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}

uint64_t ntohll(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return n;
#else
    return (((uint64_t)ntohl(n)) << 32) + ntohl(n >> 32);
#endif
}

TcpConnection::TcpConnection(boost::asio::io_service& io_service)
    : _strand(io_service), _socket(io_service) {
    _is_open = false;
    _syn_flag = true;
    _rport = 0;
    _crid = 1;
    _rec_callback = rec_callback;
}

void TcpConnection::write_impl(std::vector<unsigned char>* to_send) {
    _queue.push_back(to_send);

    if(_queue.size() > 1) {
        return;
    }

    write();
}

void TcpConnection::write() {
    std::vector<unsigned char>* to_send = _queue.front();
    boost::asio::async_write(_socket,
                             boost::asio::buffer(*to_send),
                             _strand.wrap(std::bind(&TcpConnection::write_handler,
                                          shared_from_this(), std::placeholders::_1,
                                          std::placeholders::_2, to_send)));
}

void TcpConnection::write_handler(const boost::system::error_code& ec,
                                  unsigned bytes_transferred, std::vector<unsigned char>* to_send) {
    _queue.pop_front();
    memset(to_send->data(), '\0', to_send->size());
    delete to_send;

    if(ec) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => "<<ec<<std::endl;
    }

    if(!_queue.empty()) {
        write();
    }
}

void TcpConnection::length_handler(const boost::system::error_code& ec,
                                   unsigned size) {
    if(!ec) {
        const unsigned char *rbuf;
        rbuf = boost::asio::buffer_cast<const unsigned char*>(_buf.data());
        uint32_t len = ntohl(*((uint32_t*)rbuf));
        uint64_t request_id = ntohll(*((uint64_t*)(rbuf+4)));
        _buf.consume(12);

        if(len == 0) {
            std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
            std::cout<<"unknown length packet, closing connection!"<<std::endl;
            _is_open = false;
        } else {
            _buf.prepare(len);
            boost::asio::async_read(_socket, _buf,
                                    boost::asio::transfer_exactly(len),
                                    _strand.wrap(std::bind(&TcpConnection::read_handler,
                                                 shared_from_this(), std::placeholders::_1,
                                                 std::placeholders::_2, len, request_id)));
        }
    } else if(ec == boost::asio::error::eof) {
        std::cout<<"Info "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<"Connection closed"<<std::endl;
        _is_open = false;
    } else {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<ec<<std::endl;
        _is_open = false;
    }
}

void TcpConnection::read_handler(const boost::system::error_code& ec,
                                 unsigned size, unsigned length, uint64_t request_id) {
    if(_rport == 0) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<"receive port of remote endpoint undefined!"<<std::endl;
        _is_open = false;
    } else if(!ec && _buf.size() == length) {
        const unsigned char* data;
        data = boost::asio::buffer_cast<const unsigned char*>(_buf.data());

        // 0 => normal data packet
        // 1 => request packet
        if((request_id & 0x01) == 0) {
            std::vector<unsigned char> received;
            received.reserve(length);
            for(unsigned i=0; i<length; i++) {
                received.push_back(data[i]);
            }
            _cryptor.decrypt(received);
            _rec_callback(_socket.remote_endpoint().address().to_string(),
                          _rport, received, request_id);
            memset(received.data(), '\0', length);
        } else {
            // i.e. response packet
            request_id = request_id >> 1;

            boost::condition_variable* cond = NULL;
            std::vector<unsigned char>* received = NULL;
            bool flag = false;

            // do..while so that lock guard is destroyed
            do {
                std::map<uint64_t, std::pair<boost::condition_variable*,
                    std::vector<unsigned char>*> >::iterator it;
                boost::lock_guard<boost::mutex> lock(_rmut);
                it = _request_map.find(request_id);
                if(it != _request_map.end()) {
                    cond = it->second.first;
                    received = it->second.second;
                    _request_map.erase(it);
                    flag = true;
                }
            } while(false);

            // if request id in fact exists
            if(flag) {
                received->resize(0);
                received->reserve(length);
                for(unsigned i=0; i<length; i++) {
                    received->push_back(data[i]);
                }
                _cryptor.decrypt(*received);

                boost::lock_guard<boost::mutex> lock(_rmut);
                cond->notify_one();
            } else {
                std::cout<<"Info "<<__FILE__<<":"<<__LINE__<<" => ";
                std::cout<<"received response for non-existing ";
                std::cout<<"request id: "<<request_id<<std::endl;
            }
        }

        _buf.consume(length);
        _buf.prepare(12);
        boost::asio::async_read(_socket, _buf,
                                boost::asio::transfer_exactly(12),
                                _strand.wrap(std::bind(&TcpConnection::length_handler,
                                             shared_from_this(),	std::placeholders::_1,
                                             std::placeholders::_2)));
    } else if(ec == boost::asio::error::eof) {
        std::cout<<"Info "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<"Connection closed"<<std::endl;
        _is_open = false;
    } else {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<ec<<std::endl;
        _is_open = false;
    }
}

bool TcpConnection::connect(std::string ip, unsigned port) {
    try {
        _is_open = true;
        tcp::endpoint ep(boost::asio::ip::address::from_string(ip), port);
        _socket.connect(ep);

        // Adding Keepalive flag
        int32_t nsock = _socket.native_handle();
        int32_t timeout = 20;
        int32_t cnt = 2;
        int32_t interval = 2;
        _socket.set_option(boost::asio::socket_base::keep_alive(true));

#if defined _WIN32 || defined WIN32 || defined OS_WIN64 || defined _WIN64 || defined WIN64 || defined WINNT
        int32_t timeout_milli = timeout * 1000;
        setsockopt(nsock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_milli,
                   sizeof(timeout_milli));
        setsockopt(nsock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_milli,
                   sizeof(timeout_milli));
#else
        setsockopt(nsock, SOL_TCP, TCP_KEEPIDLE, &timeout, sizeof(timeout));
        setsockopt(nsock, SOL_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
        setsockopt(nsock, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
#endif
    } catch (std::exception const& e) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<e.what()<<std::endl;
        _is_open = false;
    }

    _syn_flag = true;
    return _is_open;
}

// @warning doesn't work for large data size
void TcpConnection::send_syn(const std::vector<unsigned char>& data) {
    if(!_is_open) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<"trying to send data on a closed connection!"<<std::endl;
        return;
    }

    if(!_syn_flag) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<"sync send not allowed on this connection!"<<std::endl;
        return;
    }

    // sending synchronously
    std::vector<unsigned char> to_send(data);
    _cryptor.encrypt(to_send);
    boost::asio::write(_socket, boost::asio::buffer(to_send));
    memset(to_send.data(), '\0', data.size());
}

void TcpConnection::send(const std::vector<unsigned char>& data,
                         uint64_t request_id) {
    if(!_is_open) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<"trying to send data on a closed connection!"<<std::endl;
        return;
    }

    _syn_flag = false;

    std::vector<unsigned char>* to_send = new std::vector<unsigned char>();
    to_send->reserve(12+data.size());

    // sending length
    uint32_t _length = htonl(data.size());
    for(unsigned i=0; i<4; i++) {
        to_send->push_back(((unsigned char*)&_length)[i]);
    }

    // sending request id
    // the last bit is 1 for response, 0 for request (hack)
    // request id is 0 for a normal packet
    if(request_id != 0) {
        // assert((request_id & 0x01) == 0);
        request_id += 1;
    }
    uint64_t _rid = htonll(request_id);
    for(unsigned i=0; i<8; i++) {
        to_send->push_back(((unsigned char*)&_rid)[i]);
    }

    // sending data
    for(unsigned i=0; i<data.size(); i++) {
        to_send->push_back(data[i]);
    }
    _cryptor.encrypt(*to_send, 12);
    _strand.post(std::bind(&TcpConnection::write_impl, shared_from_this(),
                           to_send));
}

void TcpConnection::send_and_wait(const std::vector<unsigned char>& data,
                                  std::vector<unsigned char>& response, unsigned timeout) {
    uint64_t request_id;

    request_id = ++_crid;
    send(data, (request_id<<1)-1);

    boost::condition_variable* cond = new boost::condition_variable();
    // critical section
    boost::unique_lock<boost::mutex> lock(_rmut);
    _request_map[request_id] = std::make_pair(cond, &response);
    bool er = cond->timed_wait(lock, boost::posix_time::milliseconds(timeout));
    // critical section finished

    // if timed out
    if(!er) {
        _request_map.erase(request_id);
        response.resize(0);
    }

    delete cond;
}

// @warning doesn't work for large data size
void TcpConnection::read_syn(std::vector<unsigned char>& data,
                             unsigned length) {
    _is_open = true;

    if(!_syn_flag) {
        throw "synchronous read not allowed on this connection!";
    }

    _buf.prepare(length);
    try {
        if(length == boost::asio::read(_socket, _buf,
                                       boost::asio::transfer_exactly(length))) {

            const unsigned char* _d;
            _d = boost::asio::buffer_cast<const unsigned char*>(_buf.data());
            data.reserve(length);
            data.assign(_d, _d+length);
            _buf.consume(length);
        } else {
            throw "could not read specified length";
        }
    } catch(std::exception const& e) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<e.what()<<std::endl;
        _is_open = false;
    }
}

void TcpConnection::read(unsigned rport,
                         void(*rec)(std::string ip, unsigned port,
                                    const std::vector<unsigned char>& data, uint64_t request_id)) {
    _is_open = true;
    _syn_flag = false;
    _rport = rport;
    _rec_callback = rec;

    _buf.prepare(12);
    boost::asio::async_read(_socket, _buf, boost::asio::transfer_exactly(12),
                            _strand.wrap(std::bind(&TcpConnection::length_handler,
                                         shared_from_this(), std::placeholders::_1,
                                         std::placeholders::_2)));
}

// close the connection
void TcpConnection::disconnect() {
    try {
        _is_open = false;
        // _socket.cancel();
        _socket.close();
    } catch (std::exception const& e) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<e.what()<<std::endl;
    }
}

TcpConnection::~TcpConnection() {
    disconnect();

    while(!_queue.empty()) {
        std::vector<unsigned char>* to_send = _queue.front();
        _queue.pop_front();
        memset(to_send->data(), '\0', to_send->size());
        delete to_send;
    }

    std::map<uint64_t, std::pair<boost::condition_variable*,
        std::vector<unsigned char>*> >::iterator it;
    for(it=_request_map.begin(); it!=_request_map.end(); ++it) {
        delete it->second.first;
    }
}
