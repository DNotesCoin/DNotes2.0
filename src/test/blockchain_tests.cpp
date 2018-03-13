#include <boost/test/unit_test.hpp>

#include "main.h"
#include "miner.h"
#include "txdb.h"
#include <fs.h>
#include <test/test_runner.h>


//CBlockIndex genesisBlockIndex;
//CBlockIndex tipBlockIndex;

BOOST_AUTO_TEST_SUITE(blockchain_tests)

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

BOOST_AUTO_TEST_CASE(serialize_block)
{
        //Test that a block can be serialized and deserialized and maintain values
        CBlock block = CBlock();
        block.nTime = 123;
        block.nNonce = 567;
        block.nBits = 345345345;

        CTxDestination addr1 = CKeyID(uint160(1));
        block.addressBalances[addr1] = 123;
        
        std::stringstream ss;
        block.Serialize(ss, 1, 1);
        
        string serializedString = ss.str();
        string hexString = string_to_hex(serializedString);

        BOOST_TEST_MESSAGE(hexString);

        CBlock newBlock = CBlock();
        newBlock.Unserialize(ss, 1, 1);

        uint64_t x1 = block.addressBalances[addr1];
        uint64_t x2 = newBlock.addressBalances[addr1];
        BOOST_TEST_MESSAGE(x1);
        BOOST_TEST_MESSAGE(x2);

        std::stringstream ss2;
        newBlock.Serialize(ss2, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss2.str()));

        BOOST_CHECK(block.nBits == newBlock.nBits);
        BOOST_CHECK(block.addressBalances.size() == newBlock.addressBalances.size());
        BOOST_CHECK(block.addressBalances[addr1] == newBlock.addressBalances[addr1]);
}


BOOST_AUTO_TEST_CASE(serialize_transaction)
{
        //Test that a transaction can be serialized and maintain vins and vouts
        CTransaction tx = CTransaction();
        tx.nTime = 123;

        CTxIn input = CTxIn();
        input.nSequence = 678;
        //input.prevout = COutPoint();
        /*
         CScript scriptSig;
        unsigned int nSequence;
        */
        CTxOut output = CTxOut();
        output.nValue = 1234;
        /* int64_t nValue;
        std::string invoiceNumber;
        CScript scriptPubKey;
        */
        tx.vin.push_back(input);
        tx.vout.push_back(output);

        std::stringstream ss;
        tx.Serialize(ss, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss.str()));

        CTransaction newTx = CTransaction();
        newTx.Unserialize(ss, 1, 1);

        std::stringstream ss2;
        newTx.Serialize(ss2, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss2.str()));

        BOOST_CHECK(tx.nTime == newTx.nTime);
        BOOST_CHECK(1 == newTx.vin.size());
        BOOST_CHECK(1 == newTx.vout.size());
}

BOOST_AUTO_TEST_CASE(serialize_variant_map)
{
        //Test that a map <CTxDestination,int> can be serialized and deserialized and maintain values
        std::map<CTxDestination, int64_t> map;
        CTxDestination addr1 = CKeyID(uint160(1));
        map[addr1] = 123;
        
        std::stringstream ss;
        Serialize(ss, map, 1, 1);
        
        string serializedString = ss.str();
        BOOST_TEST_MESSAGE(string_to_hex(serializedString));

        std::map<CTxDestination, int64_t> newMap;
        Unserialize(ss, newMap, 1, 1);

        std::stringstream ss2;
        Serialize(ss2, newMap, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss2.str()));

        BOOST_CHECK(map.size() == newMap.size());
        BOOST_CHECK(map[addr1] == newMap[addr1]);
}

BOOST_AUTO_TEST_CASE(serialize_int_map)
{
        //Test that a map <int,int> can be serialized and deserialized and maintain values
        std::map<int64_t, int64_t> map;
        int64_t addr1 = 1;
        map[addr1] = 123;
        
        std::stringstream ss;
        Serialize(ss, map, 1, 1);
        
        string serializedString = ss.str();
        BOOST_TEST_MESSAGE(string_to_hex(serializedString));

        std::map<int64_t, int64_t> newMap;
        Unserialize(ss, newMap, 1, 1);

        std::stringstream ss2;
        Serialize(ss2, newMap, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss2.str()));

        BOOST_CHECK(map.size() == newMap.size());
        BOOST_CHECK(map[addr1] == newMap[addr1]);
}

BOOST_AUTO_TEST_CASE(aaaa)
{

        //
        int64_t nStart = GetTime();
        CBlockIndex* pindexPrev = pindexBest;
        unsigned int nExtraNonce = 0;
        int64_t nFees = 0;
        auto_ptr<CBlock> pblocktemplate(CreateNewBlock(ptestWallet->vchDefaultKey, false, &nFees));
        CBlock *pblock = pblocktemplate.get();

        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);
        uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();
        uint256 hash;

        while(true)
        {
                if (hash <= hashTarget)
                {
                        if(CheckWork(pblock, *ptestWallet)){
                                break;
                        }
                }
            
	        ++pblock->nNonce;    
        }
        BOOST_CHECK(true);

        //fs::remove_all(pathTemp);
        
}




BOOST_AUTO_TEST_SUITE_END()
