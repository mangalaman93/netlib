/* written by Aman Mangal <mangalaman93@gmail.com>
 * on July 20, 2014
 * client to test basic network class
 * [large number of connections test]
 */

#include <algorithm>
#include <atomic>
#include <boost/thread.hpp>
#include <cstdlib>
#include <cstring>
#include <random>
#include "base_network.h"

#define DATA_SIZE (1024*4)

std::string ADDRESS = "127.0.0.1";
const int PORT = 8000;
const int NUM_THREADS = 10;
const int NUM_STRINGS = 10;
std::vector<unsigned char> cdata[NUM_THREADS][NUM_STRINGS];
boost::thread* threads[NUM_THREADS];
int total_errors = 0;
BaseNetwork* rnet;
std::atomic<int> rcount(0);

#define handle_error(en, msg) \
do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void receive_handler(std::string address, unsigned port,
                     const std::vector<unsigned char>& data, uint64_t request_id) {
    uint32_t i = *((unsigned*)data.data());
    uint32_t j = *((unsigned*)(data.data()+4));

    bool flag = true;
    for(unsigned k = 0; k < data.size()-8; k++) {
        if(cdata[i][j][k] != data[8+k]) {
            flag = false;
            break;
        }
    }

    if(!flag) {
        std::cout<<"sent data is different than received data!"<<std::endl;
        std::cout<<"received data from: "<<address<<":"<<port;
        std::cout<<" and data is: "<<"\"";
        std::cout.write((const char*)(data.data()+8), data.size()-8);
        std::cout<<"\""<<std::endl<<"sent data: ";
        std::cout.write((const char*)cdata[i][j].data(), cdata[i][j].size());
        std::cout<<std::endl;
        std::cout<<"number of errors till: "<<++total_errors<<std::endl;
        std::cout<<"----------------------------------------------"<<std::endl;
    }

    rnet->send(address, port, data);
}

void receive_handler2(std::string address, unsigned port,
                      const std::vector<unsigned char>& data, uint64_t request_id) {
    uint32_t i = *((unsigned*)data.data());
    uint32_t j = *((unsigned*)(data.data()+4));

    std::cout<<"received confirmation for "<<i<<","<<j<<std::endl;
    rcount++;
}

unsigned char randchar() {
    const unsigned char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const unsigned max_index = (sizeof(charset)-1);
    return charset[rand() % max_index];
}

void run(int id) {
    srand(time(NULL));
    BaseNetwork net(PORT+id+1, receive_handler2);

    std::vector<unsigned char> to_send;
    for(int i = 0; i<NUM_STRINGS; i++) {
        cdata[id][i].reserve(DATA_SIZE);
        for(int j = 0; j < DATA_SIZE; j++) {
            cdata[id][i].push_back(randchar());
        }

        to_send.resize(0);
        to_send.reserve(DATA_SIZE+8);
        for(int j = 0; j<4; j++) {
            to_send.push_back(((char*)&id)[j]);
        }
        for(int j = 0; j<4; j++) {
            to_send.push_back(((char*)&i)[j]);
        }
        for(unsigned j = 0; j<DATA_SIZE; j++) {
            to_send.push_back(cdata[id][i][j]);
        }

        net.send(ADDRESS, PORT, to_send);
    }

    while (true) {
        if (rcount.load() >= NUM_STRINGS*NUM_THREADS) {
            break;
        }
        __sleep__(1000);
    }

    net.stop();
    net.join();
}

int main()
{
    rnet = new BaseNetwork(PORT, receive_handler);

    for(int i = 0; i<NUM_THREADS; i++) {
        threads[i] = new boost::thread(run, i);
        std::cout<<"sender thread: "<<threads[i]->get_id()<<std::endl;
    }

    while(true) {
        if(rcount.load() >= NUM_STRINGS*NUM_THREADS) {
            break;
        }
        __sleep__(1000);
    }
    rnet->stop();

    for(int i = 0; i<NUM_THREADS; i++) {
        threads[i]->join();
    }
    rnet->join();

    for(int i = 0; i<NUM_THREADS; i++) {
        delete threads[i];
    }
    delete rnet;
}
