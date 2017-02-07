/* written by Aman Mangal <mangalaman93@gmail.com>
 * on July 14, 2014
 */

#include "base_network.h"
#include <cassert>

void __sleep__(unsigned ms) {
#if defined _WIN32 || defined WIN32 || defined OS_WIN64 || defined _WIN64 || defined WIN64 || defined WINNT
    Sleep(ms);
#else
#include <unistd.h>
    sleep(ms);
#endif
}

BaseNetwork::BaseNetwork(unsigned port,
                         void(*rec)(std::string, unsigned,
                                    const std::vector<unsigned char>& data, uint64_t request_id),
                         unsigned num_io_threads) {
    _con_io_service = new boost::asio::io_service();
    _data_io_service = new boost::asio::io_service();
    _strand = new boost::asio::strand(*_con_io_service);
    _asocket = new tcp::acceptor(*_con_io_service);
    _con_work = std::make_shared<boost::asio::io_service::work>(*_con_io_service);
    _data_work = std::make_shared<boost::asio::io_service::work>(*_data_io_service);

    // socket variable
    _port = port;
    _rec_callback = rec;

    // creates io threads
    for(unsigned i=0; i<num_io_threads-1; i++) {
        _io_threads.create_thread(boost::bind(&boost::asio::io_service::run,
                                              _data_io_service));
    }
    _io_threads.create_thread(boost::bind(&boost::asio::io_service::run,
                                          _con_io_service));

    try {
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), _port);
        _asocket->open(ep.protocol());
        _asocket->set_option(tcp::acceptor::reuse_address(true));
        _asocket->bind(ep);
        _asocket->listen();
        begin_accept();
    } catch(std::exception const&  e) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<"unable to listen on port "<<_port<<std::endl;
        std::cout<<"because: "<<e.what()<<std::endl;
    }
}

std::pair<uint32_t, int> BaseNetwork::make_ip_port_pair(std::string ip,
        int port) {
    uint32_t _ip;
    unsigned char *p = (unsigned char*)&_ip;
    unsigned char num = 0;

    for(unsigned index=0; index<ip.length(); index++) {
        if(ip[index] == '.') {
            *p = num;
            p++;
            num = 0;
        } else {
            assert(ip[index] >= '0' && ip[index] <= '9');
            num = num * 10 + ip[index];
        }
    }
    *p = num;
    p++;

    assert(p == ((unsigned char*)(&_ip+1)));
    return std::make_pair(_ip, port);
}

void BaseNetwork::begin_accept() {
    TcpConnection::sptr tcon;
    tcon = std::make_shared<TcpConnection>(*_data_io_service);

    _asocket->async_accept(tcon->get_socket(),
                           _strand->wrap(std::bind(&BaseNetwork::accept_handler,
                                         this, std::placeholders::_1, tcon->shared_from_this())));
}

void BaseNetwork::accept_handler(const boost::system::error_code& ec,
                                 TcpConnection::sptr tcon) {
    if(!ec) {
        begin_accept();

        try {
            unsigned lport;
            std::vector<unsigned char> vlport;
            tcon->read_syn(vlport, 4);
            lport = ntohl(*((unsigned*)vlport.data()));

            tcp::endpoint endpoint = tcon->get_socket().remote_endpoint();
            std::string s = endpoint.address().to_string();
            std::pair<uint32_t, int> ep = make_ip_port_pair(s, lport);

            // reading and sending public keys
            std::vector<unsigned char> other_public_key;
            tcon->read_syn(other_public_key, 32);
            tcon->send_syn(tcon->get_cryptor()->get_public_key());
            tcon->get_cryptor()->generate_shared_secret(other_public_key);

            if(_cpool.count(ep) == 0 || _cpool[ep] == NULL) {
                _cpool[ep] = tcon->shared_from_this();
                tcon->read(lport, _rec_callback);
            } else if(_cpool[ep]->is_open()) {
                tcon->disconnect();
            } else {
                _cpool[ep]->disconnect();
                _cpool[ep] = tcon->shared_from_this();
                tcon->read(lport, _rec_callback);
            }
        } catch(std::exception const& e) {
            std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
            std::cout<<e.what()<<std::endl;
            tcon->disconnect();
        }
    } else {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<ec<<std::endl;
        tcon->disconnect();
    }
}

void BaseNetwork::connect(std::string ip, unsigned port) {
    std::pair<uint32_t, int> ep = make_ip_port_pair(ip, port);

    if(_cpool.count(ep) > 0 && _cpool[ep] != NULL &&
            _cpool[ep]->is_open()) {
        return;
    } else {
        TcpConnection::sptr tcon(NULL);
        tcon = std::make_shared<TcpConnection>(*_data_io_service);
        if(!tcon->connect(ip, port)) {
            std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
            std::cout<<"unable to connect and send data!"<<std::endl;
            return;
        }

        uint32_t to_send = htonl(_port);
        std::vector<unsigned char> vto_send;
        vto_send.reserve(4);
        for(unsigned i=0; i<4; i++) {
            vto_send.push_back(((unsigned char*)&to_send)[i]);
        }
        tcon->send_syn(vto_send);

        // reading and sending public keys
        std::vector<unsigned char> other_public_key;
        tcon->send_syn(tcon->get_cryptor()->get_public_key());
        tcon->read_syn(other_public_key, 32);
        tcon->get_cryptor()->generate_shared_secret(other_public_key);

        _cpool[ep] = tcon;
        tcon->read(port, _rec_callback);
    }
}

void BaseNetwork::send_helper(std::pair<uint32_t, int> ep,
                              const std::vector<unsigned char>& data, uint64_t request_id) {
    _cpool[ep]->send(data, request_id);
}

void BaseNetwork::join() {
    _io_threads.join_all();
}

void BaseNetwork::send(std::string ip, unsigned port,
                       const std::vector<unsigned char>& data, uint64_t request_id) {
    // making sure that the connection doesn't exist already
    std::pair<uint32_t, int> ep = make_ip_port_pair(ip, port);

    if(_cpool.count(ep) > 0 && _cpool[ep] != NULL &&
            _cpool[ep]->is_open()) {
        _cpool[ep]->send(data, request_id);
    } else {
        // post connect
        _strand->post(std::bind(&BaseNetwork::connect, this, ip, port));
        _strand->post(std::bind(&BaseNetwork::send_helper, this, ep, data,
                                request_id));
    }
}

// send and receive a response
void BaseNetwork::send_and_wait_for_response(std::string ip, unsigned port,
        const std::vector<unsigned char>& data,
        std::vector<unsigned char>& response, unsigned timeout) {
    // making sure that the connection doesn't exist already
    std::pair<uint32_t, int> ep = make_ip_port_pair(ip, port);

    if(_cpool.count(ep) > 0 && _cpool[ep] != NULL &&
            _cpool[ep]->is_open()) {
        _cpool[ep]->send_and_wait(data, response, timeout);
    } else {
        // post connect
        _strand->post(std::bind(&BaseNetwork::connect, this, ip, port));

        // busy wait
        unsigned total_sleep = 0;
        while(true) {
            if(_cpool.count(ep) > 0 && _cpool[ep] != NULL &&
                    _cpool[ep]->is_open()) {
                _cpool[ep]->send_and_wait(data, response, timeout);
                break;
            }

            __sleep__(CONNECT_WAIT_TIME);
            total_sleep += CONNECT_WAIT_TIME;
            if(total_sleep > MAX_CONNECT_TIME) {
                std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
                std::cout<<"unable to connect to "<<ip<<":"<<port<<std::endl;
                break;
            }
        }
    }
}

void BaseNetwork::stop() {
    try {
        _con_work.reset();
        _data_work.reset();
        _data_io_service->stop();
        _con_io_service->stop();

        while(true) {
            if(_data_io_service->stopped() && _con_io_service->stopped()) {
                break;
            }
            __sleep__(WAIT_TIME);
        }

        _io_threads.interrupt_all();
        _asocket->close();

        std::map<std::pair<uint32_t, int>, TcpConnection::sptr>::iterator it;
        for (it = _cpool.begin(); it != _cpool.end(); ++it) {
            it->second.reset();
        }
    } catch(std::exception const& e) {
        std::cout<<"Error "<<__FILE__<<":"<<__LINE__<<" => ";
        std::cout<<e.what()<<std::endl;
    }
}

BaseNetwork::~BaseNetwork() {
    delete _asocket;
    delete _con_io_service;
    delete _data_io_service;
    delete _strand;
}
