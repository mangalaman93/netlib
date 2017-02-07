#include "ed25519.h"

#ifndef ED25519_NO_SEED

#ifdef _WIN32
#include <Windows.h>
#include <Wincrypt.h>

# include <inttypes.h>

#ifdef __cplusplus_winrt
# include <winstring.h>
# include <roapi.h>
# include <windows.security.cryptography.h>
# include <windows.storage.h>
# include <windows.storage.streams.h>
#include <inspectable.h>

using namespace ABI::Windows::Security::Cryptography;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::Streams;

#endif

#else
#include <stdio.h>
#endif

int ed25519_create_seed(unsigned char *seed) {
#ifdef _WIN32
	
	//uint8_t *p_buf = (uint8_t *)seed;
	
#ifdef __cplusplus_winrt

	static const WCHAR *className = L"Windows.Security.Cryptography.CryptographicBuffer";
	const UINT32 clen = wcslen(className);

	HSTRING hClassName = NULL;
	HSTRING_HEADER header;
	HRESULT hr = WindowsCreateStringReference(className, clen, &header, &hClassName);
	if (hr) {
		WindowsDeleteString(hClassName);
		return 1;
	}

	ICryptographicBufferStatics *cryptoStatics = NULL;
	hr = RoGetActivationFactory(hClassName, IID_ICryptographicBufferStatics, (void**)&cryptoStatics);
	WindowsDeleteString(hClassName);

	if (hr)
		return 1;

	IBuffer *buffer = NULL;
	hr = cryptoStatics->GenerateRandom(32, &buffer);
	if (hr)
		return 1;
	UINT32 olength;
	unsigned char *rnd = NULL;
	hr = cryptoStatics->CopyToByteArray(buffer, &olength, (BYTE**)&rnd);
	memcpy(seed, rnd, 32);

#else

	
	HCRYPTPROV prov;

	 if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))  {
		 return 1;
		 }

		 if (!CryptGenRandom(prov, 32, seed))  {
		 CryptReleaseContext(prov, 0);
		 return 1;
		 }

		 CryptReleaseContext(prov, 0);

#endif

#else
	FILE *f = fopen("/dev/urandom", "rb");

	if (f == NULL) {
		return 1;
	}

	fread(seed, 1, 32, f);
	fclose(f);
#endif

	return 0;
}

#endif