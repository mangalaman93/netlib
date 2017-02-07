/* written by Aman Mangal <mangalaman93@gmail.com>
 * on Oct 7, 2014
 * Provides cryptography tools
 * reference http://www.codeproject.com/Articles/16465/Product-Keys-Based-on-the-Advanced-Encryption-Stan
 */

#include <cryptopp/aes.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <ed25519/ed25519.h>
#include <ed25519/sha512.h>
#include <vector>
using namespace std;

class Cryptor
{
	bool flag;
	vector<unsigned char> public_key;
	vector<unsigned char> private_key;
	vector<unsigned char> seed;
	CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption encryptor;
	CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption decryptor;
	byte iv[CryptoPP::AES::BLOCKSIZE];

  public:
	Cryptor();
	~Cryptor() { ;}
	void generate_shared_secret(const vector<unsigned char>& other_public_key);
	const vector<unsigned char> get_public_key() { return public_key;}
	void encrypt(vector<unsigned char>& data, unsigned offset=0);
	void decrypt(vector<unsigned char>& data);
};
