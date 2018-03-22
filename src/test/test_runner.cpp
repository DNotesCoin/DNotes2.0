#define BOOST_TEST_MODULE DNotes Test Suite

#include <boost/test/unit_test.hpp>

#include <test/test_runner.h>

#include "chainparams.h"
#include "util.h"
#include "miner.h"
#include "txdb.h"
#include <fs.h>


fs::path pathTest;
CWallet* ptestWallet;

void CreateTestFolders()
{
    pathTest = fs::temp_directory_path() / strprintf("test_bitcoin_%lu", (unsigned long)GetTime());
    fs::create_directories(pathTest);

    SoftSetArg("-datadir", pathTest.string());
}

void DeleteTestFolders()
{
    fs::remove_all(pathTest);
}

void SetupDatabase()
{
    bitdb.Open(GetDataDir());
    CTxDB txdb("cr");
    LoadBlockIndex();
}

void CleanupDatabase()
{
}

void SetupWallet()
{
    std::string strWalletFileName = "wallet.dat";
    ptestWallet = new CWallet(strWalletFileName);
    RegisterWallet(ptestWallet);
}

void CleanupWallet()
{
    UnregisterWallet(ptestWallet);
    bitdb.Flush(true);
    delete ptestWallet;
    ptestWallet = NULL;
}

struct TestingSetup {
    TestingSetup() {
        SelectParams(CChainParams::REGTEST);
        CreateTestFolders();
        SetupDatabase();
        SetupWallet();

        //fPrintToConsole = true;
    }
    ~TestingSetup()
    {
        CleanupDatabase();
        CleanupWallet();
        DeleteTestFolders();
    }
};

BOOST_GLOBAL_FIXTURE(TestingSetup);

void Shutdown(void* parg)
{
  exit(0);
}

std::string string_to_hex(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return "0x" + output;
}

//void StartShutdown()
//{
//  exit(0);
//}
