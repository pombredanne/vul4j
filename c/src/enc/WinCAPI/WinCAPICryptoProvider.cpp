/*
 * Copyright 2002-2004 The Apache Software Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * imitations under the License.
 */

/*
 * XSEC
 *
 * WinCAPICryptoProvider := Provider to support Windows Crypto API
 *
 * Author(s): Berin Lautenbach
 *
 * $Id$
 *
 */

#include <xsec/framework/XSECError.hpp>


#include <xsec/enc/WinCAPI/WinCAPICryptoProvider.hpp>
#include <xsec/enc/WinCAPI/WinCAPICryptoX509.hpp>
#include <xsec/enc/WinCAPI/WinCAPICryptoKeyDSA.hpp>
#include <xsec/enc/WinCAPI/WinCAPICryptoKeyRSA.hpp>
#include <xsec/enc/WinCAPI/WinCAPICryptoHash.hpp>
#include <xsec/enc/WinCAPI/WinCAPICryptoHashHMAC.hpp>
#include <xsec/enc/WinCAPI/WinCAPICryptoSymmetricKey.hpp>
#include <xsec/enc/XSCrypt/XSCryptCryptoBase64.hpp>
#include <xsec/enc/XSECCryptoException.hpp>

#include <xercesc/util/Janitor.hpp>

XSEC_USING_XERCES(ArrayJanitor);

static char s_xsecKeyStoreName[] = "ApacheXML-SecurityKeyStore";

WinCAPICryptoProvider::WinCAPICryptoProvider(
						LPCSTR provDSSName, 
						LPCSTR provRSAName) {

	if (!CryptAcquireContext(&m_provDSS,
		NULL,
		provDSSName,
		PROV_DSS,
		CRYPT_VERIFYCONTEXT)) 
	{
		throw XSECException(XSECException::InternalError,
			"WinCAPICryptoProvider() - Error obtaining default PROV_DSS");
	}

	if (!CryptAcquireContext(&m_provRSA,
		NULL,
		provRSAName,
		PROV_RSA_AES,
		CRYPT_VERIFYCONTEXT)) 
	{
		// Check of we maybe don't understand AES

		DWORD error = GetLastError();
		if (error == NTE_PROV_TYPE_NOT_DEF) {

			// This system does not have AES!
			m_haveAES = false;
			m_provRSAType = PROV_RSA_FULL;

			if (!CryptAcquireContext(&m_provRSA,
				NULL,
				provRSAName,
				PROV_RSA_FULL,
				CRYPT_VERIFYCONTEXT)) 
			{

				throw XSECException(XSECException::InternalError,
					"WinCAPICryptoProvider() - Error obtaining default PROV_RSA_FULL");
			}

		}
		
		else {

			throw XSECException(XSECException::InternalError,
				"WinCAPICryptoProvider() - Error obtaining default PROV_RSA_AES");
		}
	}

	else {
		m_haveAES = true;
		m_provRSAType = PROV_RSA_AES;
	}

	// Now obtain our internal (library) key store

	if (!CryptAcquireContext(&m_provApacheKeyStore,
		s_xsecKeyStoreName,
		provRSAName,
		m_provRSAType,
		CRYPT_MACHINE_KEYSET)) 
	{

		// Try to create
		if (!CryptAcquireContext(&m_provApacheKeyStore,
			s_xsecKeyStoreName,
			provRSAName,
			PROV_RSA_FULL,
			CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET)) {

			throw XSECException(XSECException::InternalError,
				"WinCAPICryptoProvider() - Error obtaining generating internal key store for PROV_RSA_FULL");
		}
		else {
			HCRYPTKEY k;
			if (!CryptGenKey(m_provApacheKeyStore, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, &k)) {
				throw XSECException(XSECException::InternalError,
					"WinCAPICryptoProvider() - Error generating internal key set for PROV_RSA_FULL");
			}
			CryptDestroyKey(k);
		}
	}

	// Copy parameters for later use

	if (provDSSName != NULL)
		m_provDSSName = strdup(provDSSName); 
	else
		m_provDSSName = NULL;

	if (provRSAName != NULL)
		m_provRSAName = strdup(provRSAName);
	else
		m_provRSAName = NULL;

}

WinCAPICryptoProvider::~WinCAPICryptoProvider() {


	CryptReleaseContext(m_provRSA, 0);
	CryptReleaseContext(m_provDSS, 0);
	CryptReleaseContext(m_provApacheKeyStore, 0);

}

const XMLCh * WinCAPICryptoProvider::getProviderName() {

	return DSIGConstants::s_unicodeStrPROVWinCAPI;

}


// Hashing classes

XSECCryptoHash	* WinCAPICryptoProvider::hashSHA1() {

	WinCAPICryptoHash * ret;

	XSECnew(ret, WinCAPICryptoHash(m_provDSS, XSECCryptoHash::HASH_SHA1));

	return ret;

}

XSECCryptoHash * WinCAPICryptoProvider::hashHMACSHA1() {

	WinCAPICryptoHashHMAC * ret;

	XSECnew(ret, WinCAPICryptoHashHMAC(m_provDSS, XSECCryptoHash::HASH_SHA1));

	return ret;

}

XSECCryptoHash	* WinCAPICryptoProvider::hashMD5() {

	WinCAPICryptoHash * ret;

	XSECnew(ret, WinCAPICryptoHash(m_provDSS, XSECCryptoHash::HASH_MD5));

	return ret;

}

XSECCryptoHash * WinCAPICryptoProvider::hashHMACMD5() {

	WinCAPICryptoHashHMAC * ret;

	XSECnew(ret, WinCAPICryptoHashHMAC(m_provDSS, XSECCryptoHash::HASH_MD5));

	return ret;

}

XSECCryptoKeyDSA * WinCAPICryptoProvider::keyDSA() {
	
	WinCAPICryptoKeyDSA * ret;

	XSECnew(ret, WinCAPICryptoKeyDSA(m_provDSS));

	return ret;

}

XSECCryptoKeyRSA * WinCAPICryptoProvider::keyRSA() {
	
	WinCAPICryptoKeyRSA * ret;

	XSECnew(ret, WinCAPICryptoKeyRSA(m_provRSA));

	return ret;

}


XSECCryptoX509 * WinCAPICryptoProvider::X509() {

	WinCAPICryptoX509 * ret;

	XSECnew(ret, WinCAPICryptoX509(m_provRSA, m_provDSS));

	return ret;

}

XSECCryptoBase64 * WinCAPICryptoProvider::base64() {

	// The Windows CAPI does not provide a Base64 decoder/encoder.
	// Use the internal implementation.

	XSCryptCryptoBase64 * ret;

	XSECnew(ret, XSCryptCryptoBase64());

	return ret;

}

XSECCryptoSymmetricKey	* WinCAPICryptoProvider::keySymmetric(XSECCryptoSymmetricKey::SymmetricKeyType alg) {

	// Only temporary

	WinCAPICryptoSymmetricKey * ret;
	
	XSECnew(ret, WinCAPICryptoSymmetricKey(m_provApacheKeyStore, alg));

	return ret;

}

unsigned int WinCAPICryptoProvider::getRandom(unsigned char * buffer, unsigned int numOctets) {

	if (!CryptGenRandom(m_provApacheKeyStore, numOctets, buffer)) {
		throw XSECException(XSECException::InternalError,
			"WinCAPICryptoProvider() - Error generating Random data");
	}

	return numOctets;
}



// --------------------------------------------------------------------------------
//     Translate a Base64 number to a Windows (little endian) integer
// --------------------------------------------------------------------------------

BYTE * WinCAPICryptoProvider::b642WinBN(const char * b64, unsigned int b64Len, unsigned int &retLen) {

	BYTE * os;
	XSECnew(os, BYTE[b64Len]);
	ArrayJanitor<BYTE> j_os(os);

	// Decode
	XSCryptCryptoBase64 b;

	b.decodeInit();
	retLen = b.decode((unsigned char *) b64, b64Len, os, b64Len);
	retLen += b.decodeFinish(&os[retLen], b64Len - retLen);

	BYTE * ret;
	XSECnew(ret, BYTE[retLen]);

	BYTE * j = os;
	BYTE * k = ret + retLen - 1;

	for (unsigned int i = 0; i < retLen ; ++i)
		*k-- = *j++;

	return ret;

}

// --------------------------------------------------------------------------------
//     Translate a Windows integer to a Base64 I2OSP number 
// --------------------------------------------------------------------------------

unsigned char * WinCAPICryptoProvider::WinBN2b64(BYTE * n, DWORD nLen, unsigned int &retLen) {


	// First reverse
	BYTE * rev;;
	XSECnew(rev, BYTE[nLen]);
	ArrayJanitor<BYTE> j_rev(rev);

	BYTE * j = n;
	BYTE * k = rev + nLen - 1;

	for (unsigned int i = 0; i < nLen ; ++i)
		*k-- = *j++;

	
	unsigned char * b64;
	// Naieve length calculation
	unsigned int bufLen = nLen * 2 + 4;

	XSECnew(b64, unsigned char[bufLen]);
	ArrayJanitor<unsigned char> j_b64(b64);

	XSCryptCryptoBase64 b;

	b.encodeInit();
	retLen = b.encode(rev, (unsigned int) nLen, b64, bufLen);
	retLen += b.encodeFinish(&b64[retLen], bufLen - retLen);

	j_b64.release();
	return b64;

}

