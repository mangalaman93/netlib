#include <cryptopp/hex.h>
#include "cryptor.h"
#include <iostream>
#include <vector>

int main() {
    Cryptor ec, dc;
    dc.generate_shared_secret(ec.get_public_key());
    ec.generate_shared_secret(dc.get_public_key());

    std::string s("This is test data!");
    std::vector<unsigned char> data(s.begin(), s.end());
    ec.encrypt(data);

    // Pretty print cipher text
    std::string encoded;
    CryptoPP::StringSource(data.data(), data.size(), true,
                           new CryptoPP::HexEncoder(
                               new CryptoPP::StringSink(encoded)
                           ) // HexEncoder
                          ); // StringSource
    std::cout<<"plain text: "<<s<<endl;
    std::cout<<"cipher text: "<<encoded<<endl;

    ec.decrypt(data);
    std::cout<<"plain text: ";
    std::cout.write((const char*)data.data(), data.size());
    std::cout<<std::endl;
    return 0;
}
