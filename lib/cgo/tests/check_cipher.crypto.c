
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <criterion/criterion.h>
#include <criterion/new/assert.h>

#include "libskycoin.h"
#include "skyerrors.h"
#include "skystring.h"
#include "skytest.h"

#if __APPLE__
  #include "TargetConditionals.h"
#endif

TestSuite(cipher_crypto, .init = setup, .fini = teardown);

Test(cipher_crypto, TestNewPubKey) {
  unsigned char buff[101];
  GoSlice slice;
  cipher__PubKey pk;

  slice.data = buff;
  slice.cap = 101;

  randBytes(&slice, 31);
  slice.len = 31;
  unsigned int errorcode = SKY_cipher_NewPubKey(slice, &pk);
  cr_assert(errorcode == SKY_ErrInvalidLengthPubKey, "31 random bytes");

  randBytes(&slice, 32);
  errorcode = SKY_cipher_NewPubKey(slice, &pk);
  cr_assert(errorcode == SKY_ErrInvalidLengthPubKey, "32 random bytes");

  randBytes(&slice, 34);
  errorcode = SKY_cipher_NewPubKey(slice, &pk);
  cr_assert(errorcode == SKY_ErrInvalidLengthPubKey, "34 random bytes");

  slice.len = 0;
  errorcode = SKY_cipher_NewPubKey(slice, &pk);
  cr_assert(errorcode == SKY_ErrInvalidLengthPubKey, "0 random bytes");

  randBytes(&slice, 100);
  errorcode = SKY_cipher_NewPubKey(slice, &pk);
  cr_assert(errorcode == SKY_ErrInvalidLengthPubKey, "100 random bytes");

  randBytes(&slice, 33);
  errorcode = SKY_cipher_NewPubKey(slice, &pk);
  cr_assert(errorcode == SKY_OK, "33 random bytes");

  cr_assert(eq(u8[33], pk, buff));
}

Test(cipher_crypto, TestPubKeyFromHex) {
  cipher__PubKey p, p1;
  GoString s;
  unsigned char buff[51];
  char sbuff[101];
  GoSlice slice = { (void *)buff, 0, 51 };
  unsigned int errorcode;

  // Invalid hex
  s.n = 0;
  errorcode = SKY_cipher_PubKeyFromHex(s, &p1);
  cr_assert(errorcode == SKY_ErrInvalidLengthPubKey, "TestPubKeyFromHex: Invalid hex. Empty string");

  s.p = "cascs";
  s.n = strlen(s.p);
  errorcode = SKY_cipher_PubKeyFromHex(s, &p1);
  cr_assert(errorcode == SKY_ErrInvalidPubKey, "TestPubKeyFromHex: Invalid hex. Bad chars");

  // Invalid hex length
  randBytes(&slice, 33);
  errorcode = SKY_cipher_NewPubKey(slice, &p);
  cr_assert(errorcode == SKY_OK);
  strnhex(&p[0], sbuff, slice.len / 2);
  s.p = sbuff;
  s.n = strlen(s.p);
  errorcode = SKY_cipher_PubKeyFromHex(s, &p1);
  cr_assert(errorcode == SKY_ErrInvalidLengthPubKey, "TestPubKeyFromHex: Invalid hex length");

  // Valid
  strnhex(p, sbuff, sizeof(p));
  s.p = sbuff;
  s.n = strlen(s.p);
  errorcode = SKY_cipher_PubKeyFromHex(s, &p1);
  cr_assert(errorcode == SKY_OK, "TestPubKeyFromHex: Valid. No panic.");
  cr_assert(eq(u8[33], p, p1));
}

Test(cipher_crypto, TestPubKeyHex) {
  cipher__PubKey p, p2;
  GoString s3, s4;
  unsigned char buff[50];
  GoSlice slice = { buff, 0, 50};
  unsigned int errorcode;

  randBytes(&slice, 33);
  errorcode = SKY_cipher_NewPubKey(slice, &p);
  cr_assert(errorcode == SKY_OK);
  SKY_cipher_PubKey_Hex(&p, (GoString_ *) &s3);
  registerMemCleanup((void *) s3.p);
  errorcode = SKY_cipher_PubKeyFromHex(s3, &p2);
  cr_assert(errorcode == SKY_OK);
  cr_assert(eq(u8[33], p, p2));

  SKY_cipher_PubKey_Hex(&p2, (GoString_ *)&s4);
  registerMemCleanup((void *) s4.p);
  // TODO: Translate into cr_assert(eq(type(GoString), s3, s4));
  cr_assert(s3.n == s4.n);
  cr_assert(eq(str, ((char *) s3.p), ((char *) s4.p)));
}

Test(cipher_crypto, TestPubKeyVerify) {
  cipher__PubKey p;
  unsigned char buff[50];
  GoSlice slice = { buff, 0, 50 };
  unsigned int errorcode;

  int i = 0;
  for (; i < 10; i++) {
    randBytes(&slice, 33);
    errorcode = SKY_cipher_NewPubKey(slice, &p);
    cr_assert(errorcode == SKY_OK);
    errorcode = SKY_cipher_PubKey_Verify(&p);
    cr_assert(errorcode == SKY_ErrInvalidPubKey);
  }
}

Test(cipher_crypto, TestPubKeyVerifyNil) {
  cipher__PubKey p = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0
  };
  unsigned int errorcode;

  errorcode = SKY_cipher_PubKey_Verify(&p);
  cr_assert(errorcode == SKY_ErrInvalidPubKey);
}

Test(cipher_crypto, TestPubKeyVerifyDefault1) {
  cipher__PubKey p;
  cipher__SecKey s;

  SKY_cipher_GenerateKeyPair(&p, &s);
  unsigned int errorcode = SKY_cipher_PubKey_Verify(&p);
  cr_assert(errorcode == SKY_OK);
}

Test(cipher_crypto, TestPubKeyVerifyDefault2) {
  cipher__PubKey p;
  cipher__SecKey s;
  int i;

  for (i = 0; i < 1024; ++i) {
    SKY_cipher_GenerateKeyPair(&p, &s);
    unsigned int errorcode = SKY_cipher_PubKey_Verify(&p);
    cr_assert(errorcode == SKY_OK);
  }
}

Test(cipher_crypto, TestPubKeyToAddressHash) {
  cipher__PubKey p;
  cipher__SecKey s;
  cipher__Ripemd160 h;

  SKY_cipher_GenerateKeyPair(&p, &s);
  SKY_cipher_PubKey_ToAddressHash(&p, &h);
  // TODO: Translate code snippet
  //
  // x := sha256.Sum256(p[:])
  // x = sha256.Sum256(x[:])
  // rh := ripemd160.New()
  // rh.Write(x[:])
  // y := rh.Sum(nil)
  // assert.True(t, bytes.Equal(h[:], y))
  //
  //
}

Test(cipher_crypto, TestPubKeyToAddress) {
  cipher__PubKey p;
  cipher__SecKey s;
  cipher__Address addr;
  cipher__Ripemd160 h;
  int errorcode;

  SKY_cipher_GenerateKeyPair(&p, &s);
  SKY_cipher_AddressFromPubKey(&p, &addr);
  errorcode = SKY_cipher_Address_Verify(&addr, &p);
  cr_assert(errorcode == SKY_OK);
}

Test(cipher_crypto, TestPubKeyToAddress2) {
  cipher__PubKey p;
  cipher__SecKey s;
  cipher__Address addr;
  GoString_ addrStr;
  int i, errorcode;

  for (i = 0; i < 1024; i++) {
    SKY_cipher_GenerateKeyPair(&p, &s);
    SKY_cipher_AddressFromPubKey(&p, &addr);
    //func (self Address) Verify(key PubKey) error
    errorcode = SKY_cipher_Address_Verify(&addr, &p);
    cr_assert(errorcode == SKY_OK);
    SKY_cipher_Address_String(&addr, &addrStr);
    registerMemCleanup((void *) addrStr.p);
    errorcode = SKY_cipher_DecodeBase58Address(
        *((GoString*)&addrStr), &addr);
    //func DecodeBase58Address(addr string) (Address, error)
    cr_assert(errorcode == SKY_OK);
  }
}

Test(cipher_crypto, TestMustNewSecKey) {
  unsigned char buff[101];
  GoSlice b;
  cipher__SecKey sk;
  int errorcode;

  b.data = buff;
  b.cap = 101;

  randBytes(&b, 31);
  errorcode = SKY_cipher_NewSecKey(b, &sk);
  cr_assert(errorcode == SKY_ErrInvalidLengthSecKey);

  randBytes(&b, 33);
  errorcode = SKY_cipher_NewSecKey(b, &sk);
  cr_assert(errorcode == SKY_ErrInvalidLengthSecKey);

  randBytes(&b, 34);
  errorcode = SKY_cipher_NewSecKey(b, &sk);
  cr_assert(errorcode == SKY_ErrInvalidLengthSecKey);

  randBytes(&b, 0);
  errorcode = SKY_cipher_NewSecKey(b, &sk);
  cr_assert(errorcode == SKY_ErrInvalidLengthSecKey);

  randBytes(&b, 100);
  errorcode = SKY_cipher_NewSecKey(b, &sk);
  cr_assert(errorcode == SKY_ErrInvalidLengthSecKey);

  randBytes(&b, 32);
  errorcode = SKY_cipher_NewSecKey(b, &sk);
  cr_assert(errorcode == SKY_OK);
  cr_assert(eq(u8[32], sk, buff));
}

Test(cipher_crypto, TestMustSecKeyFromHex) {
  GoString str;
  cipher__SecKey sk, sk1;
  unsigned int buff[50];
  GoSlice b;
  char strBuff[101];
  GoString s;
  int errorcode;

  // Invalid hex
  s.p = "";
  s.n = strlen(s.p);
  errorcode = SKY_cipher_SecKeyFromHex(s, &sk);
  cr_assert(errorcode == SKY_ErrInvalidLengthSecKey);

  s.p = "cascs";
  s.n = strlen(s.p);
  errorcode = SKY_cipher_SecKeyFromHex(s, &sk);
  cr_assert(errorcode == SKY_ErrInvalidSecKeyHex);

  // Invalid hex length
  b.data = buff;
  b.cap = 50;
  randBytes(&b, 32);
  errorcode = SKY_cipher_NewSecKey(b, &sk);
  cr_assert(errorcode == SKY_OK);
  strnhex(sk, strBuff, 16);
  s.p = strBuff;
  s.n = strlen(strBuff);
  errorcode = SKY_cipher_SecKeyFromHex(s, &sk1);
  cr_assert(errorcode == SKY_ErrInvalidLengthSecKey);

  // Valid
  strnhex(sk, strBuff, 32);
  s.p = strBuff;
  s.n = strlen(strBuff);
  errorcode = SKY_cipher_SecKeyFromHex(s, &sk1);
  cr_assert(errorcode == SKY_OK);
  cr_assert(eq(u8[32], sk, sk1));
}

Test(cipher_crypto, TestSecKeyHex) {
  cipher__SecKey sk, sk2;
  unsigned char buff[101];
  char strBuff[50];
  GoSlice b;
  GoString str, h;
  int errorcode;

  b.data = buff;
  b.cap = 50;
  h.p = strBuff;
  h.n = 0;

  randBytes(&b, 32);
  SKY_cipher_NewSecKey(b, &sk);
  SKY_cipher_SecKey_Hex(&sk, (GoString_ *)&str);
  registerMemCleanup((void *) str.p);

  // Copy early to ensure memory is released
  strncpy((char *) h.p, str.p, str.n);
  h.n = str.n;

  errorcode = SKY_cipher_SecKeyFromHex(h, &sk2);
  cr_assert(errorcode == SKY_OK);
  cr_assert(eq(u8[32], sk, sk2));
}

Test(cipher_crypto, TestSecKeyVerify) {
  cipher__SecKey sk;
  cipher__PubKey pk;
  int errorcode;

  // Empty secret key should not be valid
  memset(sk, 0, 32);
  errorcode = SKY_cipher_SecKey_Verify(&sk);
  cr_assert(errorcode == SKY_ErrInvalidSecKey);

  // Generated sec key should be valid
  SKY_cipher_GenerateKeyPair(&pk, &sk);
  errorcode = SKY_cipher_SecKey_Verify(&sk);
  cr_assert(errorcode == SKY_OK);

  // Random bytes are usually valid
}

Test(cipher_crypto, TestECDHonce) {
  cipher__PubKey pub1, pub2;
  cipher__SecKey sec1, sec2;
  unsigned char buff1[50], buff2[50];
  GoSlice_ buf1, buf2;

  buf1.data = buff1;
  buf1.len = 0;
  buf1.cap = 50;
  buf2.data = buff2;
  buf2.len = 0;
  buf2.cap = 50;

  SKY_cipher_GenerateKeyPair(&pub1, &sec1);
  SKY_cipher_GenerateKeyPair(&pub2, &sec2);

  SKY_cipher_ECDH(&pub2, &sec1, &buf1);
  SKY_cipher_ECDH(&pub1, &sec2, &buf2);

  // ECDH shared secrets are 32 bytes SHA256 hashes in the end
  cr_assert(eq(u8[32], buff1, buff2));
}

Test(cipher_crypto, TestECDHloop) {
  int i;
  cipher__PubKey pub1, pub2;
  cipher__SecKey sec1, sec2;
  unsigned char buff1[50], buff2[50];
  GoSlice_ buf1, buf2;

  buf1.data = buff1;
  buf1.len = 0;
  buf1.cap = 50;
  buf2.data = buff2;
  buf2.len = 0;
  buf2.cap = 50;

  for (i = 0; i < 128; i++) {
    SKY_cipher_GenerateKeyPair(&pub1, &sec1);
    SKY_cipher_GenerateKeyPair(&pub2, &sec2);
    SKY_cipher_ECDH(&pub2, &sec1, &buf1);
    SKY_cipher_ECDH(&pub1, &sec2, &buf2);
    cr_assert(eq(u8[32], buff1, buff2));
  }
}

Test(cipher_crypto, TestNewSig) {
  unsigned char buff[101];
  GoSlice b;
  cipher__Sig s;
  int errorcode;

  b.data = buff;
  b.len = 0;
  b.cap = 101;

  randBytes(&b, 64);
  errorcode = SKY_cipher_NewSig(b, &s);
  cr_assert(errorcode == SKY_ErrInvalidLengthSig);

  randBytes(&b, 66);
  errorcode = SKY_cipher_NewSig(b, &s);
  cr_assert(errorcode == SKY_ErrInvalidLengthSig);

  randBytes(&b, 67);
  errorcode = SKY_cipher_NewSig(b, &s);
  cr_assert(errorcode == SKY_ErrInvalidLengthSig);

  randBytes(&b, 0);
  errorcode = SKY_cipher_NewSig(b, &s);
  cr_assert(errorcode == SKY_ErrInvalidLengthSig);

  randBytes(&b, 100);
  errorcode = SKY_cipher_NewSig(b, &s);
  cr_assert(errorcode == SKY_ErrInvalidLengthSig);

  randBytes(&b, 65);
  errorcode = SKY_cipher_NewSig(b, &s);
  cr_assert(errorcode == SKY_OK);
  cr_assert(eq(u8[65], buff, s));
}

Test(cipher_crypto, TestMustSigFromHex) {
  unsigned char buff[101];
  char strBuff[257];
  GoSlice b = { buff, 0, 101 };
  GoString str;
  cipher__Sig s, s2;
  int errorcode;

  // Invalid hex
  str.p = "";
  str.n = strlen(str.p);
  errorcode = SKY_cipher_SigFromHex(str, &s2);
  cr_assert(errorcode == SKY_ErrInvalidLengthSig);

  str.p = "cascs";
  str.n = strlen(str.p);
  errorcode = SKY_cipher_SigFromHex(str, &s2);
  cr_assert(errorcode == SKY_ERROR);

  // Invalid hex length
  randBytes(&b, 65);
  errorcode = SKY_cipher_NewSig(b, &s);
  cr_assert(errorcode == SKY_OK);
  str.p = strBuff;
  str.n = 0;
  strnhex(s, (char *) str.p, 32);
  str.n = strlen(str.p);
  errorcode = SKY_cipher_SigFromHex(str, &s2);
  cr_assert(errorcode == SKY_ErrInvalidLengthSig);

  // Valid
  strnhex(s, (char *) str.p, 65);
  str.n = strlen(str.p);
  errorcode = SKY_cipher_SigFromHex(str, &s2);
  cr_assert(errorcode == SKY_OK);
  cr_assert(eq(u8[65], s2, s));
}

Test(cipher_crypto, TestSigHex) {
  unsigned char buff[66];
  GoSlice b = {buff, 0, 66};
  char strBuff[150],
  strBuff2[150];
  GoString str = {NULL, 0},
           str2 = {NULL, 0};
  cipher__Sig s, s2;
  int errorcode;

  randBytes(&b, 65);
  errorcode = SKY_cipher_NewSig(b, &s);

  cr_assert(errorcode == SKY_OK);
  SKY_cipher_Sig_Hex(&s, (GoString_ *) &str);
  registerMemCleanup((void *) str.p);
  errorcode = SKY_cipher_SigFromHex(str, &s2);

  cr_assert(errorcode == SKY_OK);
  cr_assert(eq(u8[65], s, s2));

  SKY_cipher_Sig_Hex(&s2, (GoString_ *) &str2);
  registerMemCleanup((void *) str2.p);
  cr_assert(eq(type(GoString), str, str2));
}

Test(cipher_crypto, TestChkSig) {
  cipher__PubKey pk, pk2;
  cipher__SecKey sk, sk2;
  cipher__Address addr, addr2;
  unsigned char buff[257];
  GoSlice b = { buff, 0, 257 };
  cipher__SHA256 h, h2;
  cipher__Sig sig, sig2;
  int errorcode;

  SKY_cipher_GenerateKeyPair(&pk, &sk);
  errorcode = SKY_cipher_PubKey_Verify(&pk);
  cr_assert(errorcode == SKY_OK);
  errorcode = SKY_cipher_SecKey_Verify(&sk);
  cr_assert(errorcode == SKY_OK);

  SKY_cipher_AddressFromPubKey(&pk, &addr);
  errorcode = SKY_cipher_Address_Verify(&addr, &pk);
  cr_assert(errorcode == SKY_OK);
  randBytes(&b, 256);
  SKY_cipher_SumSHA256(b, &h);
  SKY_cipher_SignHash(&h, &sk, &sig);
  errorcode = SKY_cipher_ChkSig(&addr, &h, &sig);
  cr_assert(errorcode == SKY_OK);

  // Empty sig should be invalid
  memset(&sig, 0, sizeof(sig));
  errorcode = SKY_cipher_ChkSig(&addr, &h, &sig);
  cr_assert(errorcode == SKY_ErrInvalidSigForPubKey);

  // Random sigs should not pass
  int i;
  for (i = 0; i < 100; i++) {
    randBytes(&b, 65);
    SKY_cipher_NewSig(b, &sig);
    errorcode = SKY_cipher_ChkSig(&addr, &h, &sig);
    cr_assert(errorcode != SKY_OK); // One of many error codes
  }

  // Sig for one hash does not work for another hash
  randBytes(&b, 256);
  SKY_cipher_SumSHA256(b, &h2);
  SKY_cipher_SignHash(&h2, &sk, &sig2);
  errorcode = SKY_cipher_ChkSig(&addr, &h2, &sig2);
  cr_assert(errorcode == SKY_OK);
  errorcode = SKY_cipher_ChkSig(&addr, &h, &sig2);
  cr_assert(errorcode == SKY_ErrInvalidAddressForSig);
  errorcode = SKY_cipher_ChkSig(&addr, &h2, &sig);
  cr_assert(errorcode != SKY_OK); // One of many error codes

  // Different secret keys should not create same sig
  SKY_cipher_GenerateKeyPair(&pk2, &sk2);
  SKY_cipher_AddressFromPubKey(&pk2, &addr2);
  memset(&h, 0, sizeof(h));
  SKY_cipher_SignHash(&h, &sk, &sig);
  SKY_cipher_SignHash(&h, &sk2, &sig2);
  errorcode = SKY_cipher_ChkSig(&addr, &h, &sig);
  cr_assert(errorcode == SKY_OK);
  errorcode = SKY_cipher_ChkSig(&addr2, &h, &sig2);
  cr_assert(errorcode == SKY_OK);
  cr_assert(not(eq(u8[65], sig, sig2)));

  randBytes(&b, 256);
  SKY_cipher_SumSHA256(b, &h);
  SKY_cipher_SignHash(&h, &sk, &sig);
  SKY_cipher_SignHash(&h, &sk2, &sig2);
  errorcode = SKY_cipher_ChkSig(&addr, &h, &sig);
  cr_assert(errorcode == SKY_OK);
  errorcode = SKY_cipher_ChkSig(&addr2, &h, &sig2);
  cr_assert(errorcode == SKY_OK);
  cr_assert(not(eq(u8[65], sig, sig2)));

  // Bad address should be invalid
  errorcode = SKY_cipher_ChkSig(&addr, &h, &sig2);
  cr_assert(errorcode == SKY_ErrInvalidAddressForSig);
  errorcode = SKY_cipher_ChkSig(&addr2, &h, &sig);
  cr_assert(errorcode == SKY_ErrInvalidAddressForSig);
}

Test(cipher_crypto, TestSignHash, 
  #if __linux__ 
    .signal=SIGABRT 
  #elif __APPLE__
    #if TARGET_OS_MAC
    .exit_code=2
    #endif
  #endif
  ) {
  cipher__PubKey pk;
  cipher__SecKey sk;
  cipher__Address addr;
  unsigned char buff[257];
  GoSlice b = { buff, 0, 101 };
  cipher__SHA256 h;
  cipher__Sig sig, sig2;
  int errorcode;

  SKY_cipher_GenerateKeyPair(&pk, &sk);
  SKY_cipher_AddressFromPubKey(&pk, &addr);

  randBytes(&b, 256);
  SKY_cipher_SumSHA256(b, &h);
  SKY_cipher_SignHash(&h, &sk, &sig);
  memset((void *) &sig2, 0, 65);
  cr_assert(not(eq(u8[65], sig2, sig)));
  errorcode = SKY_cipher_ChkSig(&addr, &h, &sig);
  cr_assert(errorcode == SKY_OK);
}

Test(cipher_crypto, TestPubKeyFromSecKey) {
  cipher__PubKey pk, pk2;
  cipher__SecKey sk;
  unsigned char buff[101];
  GoSlice b = { buff, 0, 101 };
  int errorcode;

  SKY_cipher_GenerateKeyPair(&pk, &sk);
  errorcode = SKY_cipher_PubKeyFromSecKey(&sk, &pk2);
  cr_assert(errorcode == SKY_OK);
  cr_assert(eq(u8[33], pk, pk2));

  memset(&sk, 0, sizeof(sk));
  errorcode = SKY_cipher_PubKeyFromSecKey(&sk, &pk);
  cr_assert(errorcode == SKY_ErrPubKeyFromNullSecKey);

  randBytes(&b, 99);
  errorcode = SKY_cipher_NewSecKey(b, &sk);
  cr_assert(errorcode == SKY_ErrInvalidLengthSecKey);

  randBytes(&b, 31);
  errorcode = SKY_cipher_NewSecKey(b, &sk);
  cr_assert(errorcode == SKY_ErrInvalidLengthSecKey);
}

Test(cipher_crypto, TestPubKeyFromSig) {
  cipher__PubKey pk, pk2;
  cipher__SecKey sk;
  cipher__SHA256 h;
  cipher__Sig sig;
  unsigned char buff[257];
  GoSlice b = { buff, 0, 257 };
  int errorcode;

  SKY_cipher_GenerateKeyPair(&pk, &sk);

  randBytes(&b, 256);
  SKY_cipher_SumSHA256(b, &h);
  SKY_cipher_SignHash(&h, &sk, &sig);
  errorcode = SKY_cipher_PubKeyFromSig(&sig, &h, &pk2);

  cr_assert(errorcode == SKY_OK);
  cr_assert(eq(u8[33], pk, pk2));

  memset(&sig, 0, sizeof(sig));
  errorcode = SKY_cipher_PubKeyFromSig(&sig, &h, &pk2);
  cr_assert(errorcode == SKY_ErrInvalidSigForPubKey);
}

Test(cipher_crypto, TestVerifySignature) {
  cipher__PubKey pk, pk2;
  cipher__SecKey sk, sk2;
  cipher__SHA256 h, h2;
  cipher__Sig sig, sig2;
  unsigned char buff[257];
  GoSlice b = { buff, 0, 257 };
  int errorcode;

  SKY_cipher_GenerateKeyPair(&pk, &sk);
  randBytes(&b, 256);
  SKY_cipher_SumSHA256(b, &h);
  randBytes(&b, 256);
  SKY_cipher_SumSHA256(b, &h2);
  SKY_cipher_SignHash(&h, &sk, &sig);
  errorcode = SKY_cipher_VerifySignature(&pk, &sig, &h);
  cr_assert(errorcode == SKY_OK);

  memset(&sig2, 0, sizeof(sig2));
  errorcode = SKY_cipher_VerifySignature(&pk, &sig2, &h);
  cr_assert(errorcode == SKY_ErrInvalidSigForPubKey);

  errorcode = SKY_cipher_VerifySignature(&pk, &sig, &h2);
  cr_assert(errorcode == SKY_ErrPubKeyRecoverMismatch);

  SKY_cipher_GenerateKeyPair(&pk2, &sk2);
  errorcode = SKY_cipher_VerifySignature(&pk2, &sig, &h);
  cr_assert(errorcode == SKY_ErrPubKeyRecoverMismatch);

  memset(&pk2, 0, sizeof(pk2));
  errorcode = SKY_cipher_VerifySignature(&pk2, &sig, &h);
  cr_assert(errorcode == SKY_ErrPubKeyRecoverMismatch);
}

Test(cipher_crypto, TestGenerateKeyPair) {
  cipher__PubKey pk;
  cipher__SecKey sk;
  int errorcode;

  SKY_cipher_GenerateKeyPair(&pk, &sk);
  errorcode = SKY_cipher_PubKey_Verify(&pk);
  cr_assert(errorcode == SKY_OK);
  errorcode = SKY_cipher_SecKey_Verify(&sk);
  cr_assert(errorcode == SKY_OK);
}

Test(cipher_crypto, TestGenerateDeterministicKeyPair) {
  cipher__PubKey pk;
  cipher__SecKey sk;
  unsigned char buff[33];
  GoSlice seed = { buff, 0, 33 };
  int errorcode;

  // TODO -- deterministic key pairs are useless as is because we can't
  // generate pair n+1, only pair 0
  randBytes(&seed, 32);
  SKY_cipher_GenerateDeterministicKeyPair(seed, &pk, &sk);
  errorcode = SKY_cipher_PubKey_Verify(&pk);
  cr_assert(errorcode == SKY_OK);
  errorcode = SKY_cipher_SecKey_Verify(&sk);
  cr_assert(errorcode == SKY_OK);

  SKY_cipher_GenerateDeterministicKeyPair(seed, &pk, &sk);
  errorcode = SKY_cipher_PubKey_Verify(&pk);
  cr_assert(errorcode == SKY_OK);
  errorcode = SKY_cipher_SecKey_Verify(&sk);
  cr_assert(errorcode == SKY_OK);
}

Test(cipher_crypto, TestSecKeTest) {
  cipher__PubKey pk;
  cipher__SecKey sk;
  int errorcode;

  SKY_cipher_GenerateKeyPair(&pk, &sk);
  errorcode = SKY_cipher_TestSecKey(&sk);
  cr_assert(errorcode == SKY_OK);

  memset(&sk, 0, sizeof(sk));
  errorcode = SKY_cipher_TestSecKey(&sk);
  cr_assert(errorcode == SKY_ErrInvalidSecKyVerification);
}

Test(cipher_crypto, TestSecKeyHashTest) {
  cipher__PubKey pk;
  cipher__SecKey sk;
  cipher__SHA256 h;
  unsigned char buff[257];
  GoSlice b = { buff, 0, 257};
  int errorcode;

  SKY_cipher_GenerateKeyPair(&pk, &sk);
  randBytes(&b, 256);
  SKY_cipher_SumSHA256(b, &h);
  errorcode = SKY_cipher_TestSecKeyHash(&sk, &h);
  cr_assert(errorcode == SKY_OK);


  memset(&sk, 0, sizeof(sk));
  errorcode = SKY_cipher_TestSecKeyHash(&sk, &h);
  cr_assert(errorcode == SKY_ErrInvalidSecKyVerification);
}

