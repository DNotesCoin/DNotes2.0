#ifndef BITCOIN_TEST_TEST_BITCOIN_H
#define BITCOIN_TEST_TEST_BITCOIN_H


#include "txdb.h"
#include <fs.h>


extern fs::path pathTest;
extern CWallet* ptestWallet;

void TestThing();

#endif