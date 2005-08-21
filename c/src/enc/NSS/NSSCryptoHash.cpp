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
 * NSSCryptoHash := NSS Implementation of Message digests
 *
 * Author(s): Milan Tomic
 *
 * $Id$
 *
 */

#include <xsec/enc/NSS/NSSCryptoHash.hpp>
#include <xsec/enc/XSECCryptoException.hpp>

//#include <memory.h>

// Constructors/Destructors

NSSCryptoHash::NSSCryptoHash(HashType alg) {

  switch (alg) {

	case (XSECCryptoHash::HASH_SHA1) :
	
		mp_md = PK11_CreateDigestContext(SEC_OID_SHA1);
		break;

	case (XSECCryptoHash::HASH_MD5) :
	
		mp_md = PK11_CreateDigestContext(SEC_OID_MD5);
		break;

	default :

		mp_md = NULL;

	}

	if(!mp_md) {

		throw XSECCryptoException(XSECCryptoException::MDError,
			"NSS:Hash - Unknown algorithm"); 

	}

	m_hashType = alg;

	SECStatus s = PK11_DigestBegin(mp_md);

  if (s != SECSuccess) 
  {
    throw XSECCryptoException(XSECCryptoException::MDError,
			"NSS:Hash - Error initializing Message Digest"); 
  }
}


NSSCryptoHash::~NSSCryptoHash() {

	if (mp_md != 0)
		PK11_DestroyContext(mp_md, PR_TRUE);

}



// Hashing Activities
void NSSCryptoHash::reset(void) {

	if (mp_md != 0)
		PK11_DestroyContext(mp_md, PR_TRUE);

	SECStatus s = PK11_DigestBegin(mp_md);
  
  if (s != SECSuccess) 
  {
    throw XSECCryptoException(XSECCryptoException::MDError,
			"NSS:Hash - Error initializing Message Digest"); 
  }

}

void NSSCryptoHash::hash(unsigned char * data, 
								 unsigned int length) {

  SECStatus s = PK11_DigestOp(mp_md, data, length);

  if (s != SECSuccess) 
  {
    throw XSECCryptoException(XSECCryptoException::MDError,
			"NSS:Hash - Error Hashing Data"); 
  }

}

unsigned int NSSCryptoHash::finish(unsigned char * hash,
									   unsigned int maxLength) {

  unsigned int retLen = XSEC_MAX_HASH_SIZE;

  SECStatus s = PK11_DigestFinal(mp_md, m_mdValue, &retLen, XSEC_MAX_HASH_SIZE);

  if (s != SECSuccess) 
  {
    throw XSECCryptoException(XSECCryptoException::MDError,
			"NSS:Hash - Error getting hash value"); 
  }

  m_mdLen = retLen;

  retLen = (maxLength > m_mdLen ? m_mdLen : maxLength);
	memcpy(hash, m_mdValue, retLen);

	return (unsigned int) retLen;

}

// Get information

XSECCryptoHash::HashType NSSCryptoHash::getHashType(void) {

	return m_hashType;			// This could be any kind of hash

}

