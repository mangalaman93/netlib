/* written by Aman Mangal <mangalaman93@gmail.com>
 * on Oct 7, 2014
 */

#include "cryptor.h"

Cryptor::Cryptor() {
	flag = false;
	public_key = vector<unsigned char>(32, 0);
	private_key = vector<unsigned char>(64, 0);
	seed = vector<unsigned char>(32, 0);

	memset(iv, 0, CryptoPP::AES::BLOCKSIZE);
	ed25519_create_seed(seed.data());
	ed25519_create_keypair(public_key.data(), private_key.data(), seed.data());
}

void Cryptor::generate_shared_secret(const vector<unsigned char>& other_public_key) {
	unsigned char temp_key[32];
	unsigned char temp_hash[64];
	ed25519_key_exchange(temp_key, other_public_key.data(),
		private_key.data());
	sha512(temp_key, 32, temp_hash);

	// using last 32 bytes as the key, neglecting rest
	encryptor.SetKeyWithIV(temp_hash+32, 32, iv);
	decryptor.SetKeyWithIV(temp_hash+32, 32, iv);
	flag = true;
}

void Cryptor::encrypt(vector<unsigned char>& data, unsigned offset) {
	if(flag) {
		string *cypher_text = new string();
		CryptoPP::StringSource(data.data()+offset, data.size()-offset, true,
			new CryptoPP::StreamTransformationFilter(encryptor,
				new CryptoPP::StringSink(*cypher_text)));
		memcpy(data.data()+offset, cypher_text->c_str(), data.size()-offset);
		delete cypher_text;
	}
}

void Cryptor::decrypt(vector<unsigned char>& data) {
	if(flag) {
		string *recovered_text = new string();
		CryptoPP::StringSource(data.data(), data.size(), true,
	         new CryptoPP::StreamTransformationFilter(decryptor,
	             new CryptoPP::StringSink(*recovered_text)));
		memcpy(data.data(), recovered_text->c_str(), data.size());
		delete recovered_text;
	}
}
