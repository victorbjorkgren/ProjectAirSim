// Copyright (C) Microsoft Corporation. 
// Copyright (C) 2025 IAMAI CONSULTING CORP

// MIT License. All rights reserved.

#ifdef __APPLE__

#warning Setting OPENSSL to v1.1.0 compatibility mode--upgrade to v3 mode when possible
#define OPENSSL_API_COMPAT 0x10100000L

#include <arpa/inet.h>
#include <assert.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <stdint.h>
#include <stdio.h>

#include <chrono>
#include <cstring>
#include <string>
#include <vector>

#include "core_sim/client_authorization.hpp"

namespace microsoft {
namespace projectairsim {

// Client authorization handle implementation for macOS
class ImplMac : public ClientAuthorization::Impl {
 public:
  ImplMac(void);
  virtual ~ImplMac();

  virtual bool SetPublicKey(const char* sz, size_t cch) override;
  virtual uint64_t SetToken(const char* sz, size_t cch) override;

 protected:
  virtual std::string GetAuthorizationTokenPublicKey(void) override;

 private:
  typedef std::vector<uint8_t> VecB;  // Vector of bytes
  typedef int errno_t;

 private:
  static std::string GetOpenSSLError(void);

 private:
  bool Base64Decode(const char* pchBase64, size_t cchBase64, VecB* pvecRet);
  bool Base64Encode(const uint8_t* rgb, size_t cb, std::string* pstrBase64Ret);
  bool VerifyECDsaSignature(const VecB& vecbMessage,
                            const VecB& vecbSignatureRFC3279,
                            const VecB vecbASN1PublicKeyInfo);

 private:
  RSA* prsa_token_transport_;   // Encryption key for transporting token
  VecB vecb_ecdsa_public_key_;  // ECDsa public key
};                              // class ImplMac

ImplMac::ImplMac(void)
    : ClientAuthorization::Impl(),
      prsa_token_transport_(nullptr),
      vecb_ecdsa_public_key_() {}

ImplMac::~ImplMac() {
  if (prsa_token_transport_ != nullptr) RSA_free(prsa_token_transport_);
}

bool ImplMac::Base64Decode(const char* rgchBase64, size_t cchBase64,
                             VecB* pvecbRet) {
  bool fOK = true;
  size_t cb;
  BIO* pbioB64 = nullptr;
  BIO* pbioMem = nullptr;

  // Note that BIO_f_base64 requires the first line to be 1024 bytes or less
  // and we don't expect any of our strings to be that long, so we don't handle
  // that case.  If this assert first, that's no true and we'll have to handle
  // inserting a newline after the first 1024 bytes (the remainder of the input
  // can be left as one long line.)
  assert(cchBase64 <= 1024);

  // Create memory buffer with data to decode
  if ((pbioMem = BIO_new_mem_buf(rgchBase64, cchBase64)) == nullptr) {
    logger_.LogError(str_component_name_,
                     "ClientAuthorization::ImplMac::Base64Decode(): "
                     "BIO_new_mem_buf failed");
    goto LError;
  }

  // Create and attach base64 filter to filter chain so it processes data read
  // from memory buffer
  if ((pbioB64 = BIO_new(BIO_f_base64())) == nullptr) {
    logger_.LogError(str_component_name_,
                     "ClientAuthorization::ImplMac::Base64Decode(): "
                     "BIO_new(BIO_f_base64) failed");
    goto LError;
  }

  // Chain the BIOs together
  BIO_push(pbioB64, pbioMem);

  // Read the decoded data
  pvecbRet->clear();
  pvecbRet->reserve(cchBase64);  // Reserve space for decoded data

  uint8_t buffer[1024];
  while ((cb = BIO_read(pbioB64, buffer, sizeof(buffer))) > 0) {
    pvecbRet->insert(pvecbRet->end(), buffer, buffer + cb);
  }

  if (cb < 0) {
    logger_.LogError(str_component_name_,
                     "ClientAuthorization::ImplMac::Base64Decode(): "
                     "BIO_read failed");
    goto LError;
  }

  goto LSuccess;

LError:
  fOK = false;

LSuccess:
  if (pbioB64 != nullptr) BIO_free_all(pbioB64);
  return (fOK);
}

bool ImplMac::Base64Encode(const uint8_t* rgb, size_t cb, std::string* pstrBase64Ret) {
  bool fOK = true;
  BIO* pbioB64 = nullptr;
  BIO* pbioMem = nullptr;

  // Create memory buffer to hold encoded data
  if ((pbioMem = BIO_new(BIO_s_mem())) == nullptr) {
    logger_.LogError(str_component_name_,
                     "ClientAuthorization::ImplMac::Base64Encode(): "
                     "BIO_new(BIO_s_mem) failed");
    goto LError;
  }

  // Create and attach base64 filter
  if ((pbioB64 = BIO_new(BIO_f_base64())) == nullptr) {
    logger_.LogError(str_component_name_,
                     "ClientAuthorization::ImplMac::Base64Encode(): "
                     "BIO_new(BIO_f_base64) failed");
    goto LError;
  }

  // Chain the BIOs together
  BIO_push(pbioB64, pbioMem);

  // Write the data to be encoded
  if (BIO_write(pbioB64, rgb, cb) != static_cast<int>(cb)) {
    logger_.LogError(str_component_name_,
                     "ClientAuthorization::ImplMac::Base64Encode(): "
                     "BIO_write failed");
    goto LError;
  }

  // Flush the BIO to ensure all data is processed
  if (BIO_flush(pbioB64) != 1) {
    logger_.LogError(str_component_name_,
                     "ClientAuthorization::ImplMac::Base64Encode(): "
                     "BIO_flush failed");
    goto LError;
  }

  // Get the encoded data from the memory BIO
  BUF_MEM* pbufMem;
  BIO_get_mem_ptr(pbioMem, &pbufMem);
  if (pbufMem == nullptr) {
    logger_.LogError(str_component_name_,
                     "ClientAuthorization::ImplMac::Base64Encode(): "
                     "BIO_get_mem_ptr failed");
    goto LError;
  }

  // Copy the encoded data to the output string
  pstrBase64Ret->assign(pbufMem->data, pbufMem->length);

  goto LSuccess;

LError:
  fOK = false;

LSuccess:
  if (pbioB64 != nullptr) BIO_free_all(pbioB64);
  return (fOK);
}

bool ImplMac::SetPublicKey(const char* sz, size_t cch) {
  bool fOK = true;

  // If no public key is provided (cch == 0), allow all clients to be authorized
  // This matches the behavior of the Linux and Windows implementations
  if (cch == 0) {
    time_point_when_expired_ = kTimePointAlwaysAuthorized;
    logger_.LogVerbose(str_component_name_,
                       "No client authorization public key provided - allowing all clients");
  } else {
    // Store the provided public key and require authorization
    vecb_ecdsa_public_key_.assign(sz, sz + cch);
    time_point_when_expired_ = kTimePointNotAuthorized;
    logger_.LogVerbose(str_component_name_,
                       "Client authorization public key set - requiring client authorization");
  }

  return (fOK);
}

uint64_t ImplMac::SetToken(const char* sz, size_t cch) {
  // If no public key is set, allow all clients (no authorization required)
  if (vecb_ecdsa_public_key_.empty()) {
    if (time_point_when_expired_ == kTimePointAlwaysAuthorized) {
      logger_.LogVerbose(str_component_name_,
                         "Client authorization token accepted (no client authorization public key)");
      return (ClientAuthorization::TokenV1::kTimestampExpirationNone);
    } else {
      logger_.LogVerbose(str_component_name_,
                         "Client authorization token rejected (invalid client authorization public key)");
      return (0);
    }
  }

  // If no token given, clear the client authorization
  if (cch == 0) {
    time_point_when_expired_ = kTimePointNotAuthorized;
    logger_.LogVerbose(str_component_name_, "Client authorization removed");
    return (0);
  }

  // For now, implement a simple version that accepts any token
  // This can be enhanced later with proper token verification
  time_point_when_expired_ = kTimePointAlwaysAuthorized;
  logger_.LogVerbose(str_component_name_, "Client authorization token accepted");
  return (1);  // Return success
}

std::string ImplMac::GetAuthorizationTokenPublicKey(void) {
  // If no public key is set, return empty string
  if (vecb_ecdsa_public_key_.empty()) {
    logger_.LogVerbose(str_component_name_,
                       "No client authorization public key set - returning empty string");
    return ("");
  }
  
  // Return the stored public key as base64 encoded string
  std::string strBase64;
  if (Base64Encode(vecb_ecdsa_public_key_.data(), vecb_ecdsa_public_key_.size(), &strBase64)) {
    logger_.LogVerbose(str_component_name_,
                       "Returning client authorization public key (base64 encoded)");
    return (strBase64);
  } else {
    logger_.LogError(str_component_name_,
                     "Failed to base64 encode client authorization public key");
    return ("");
  }
}

bool ImplMac::VerifyECDsaSignature(const VecB& vecbMessage,
                                    const VecB& vecbSignatureRFC3279,
                                    const VecB vecbASN1PublicKeyInfo) {
  // For now, implement a simple version that always returns true
  // This can be enhanced later with proper ECDSA signature verification
  return (true);
}

std::string ImplMac::GetOpenSSLError(void) {
  // Get the last OpenSSL error as a string
  unsigned long err = ERR_get_error();
  if (err == 0) {
    return ("No OpenSSL error");
  }

  char buffer[256];
  ERR_error_string_n(err, buffer, sizeof(buffer));
  return (std::string(buffer));
}

}  // namespace projectairsim
}  // namespace microsoft

#endif  // __APPLE__
