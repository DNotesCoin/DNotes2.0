#ifndef BITCOIN_TEST_TEST_BITCOIN_H
#define BITCOIN_TEST_TEST_BITCOIN_H


#include "txdb.h"
#include <fs.h>

static const string GENESIS_BLOCK_HASH = "0000074d707edc8763d1f1a295f474e59a8e827d73b150bef1611279c08fc068";

extern fs::path pathTest;
extern CWallet* ptestWallet;

void TestThing();
std::string string_to_hex(const std::string& input);

#endif