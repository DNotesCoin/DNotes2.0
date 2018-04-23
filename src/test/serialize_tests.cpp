#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

#include "serialize.h"
#include "main.h"
#include "test/test_runner.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(serialize_tests)

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

BOOST_AUTO_TEST_CASE(serialize_block_max_size)
{
        CBlock block = CBlock();
        block.nTime = 123;
        block.nNonce = 567;
        block.nBits = 345345345;

        for(int i = 1; i <= 10000; i++)
        {
            CTxDestination addr1 = CKeyID(uint160(i));
            block.addressBalances[addr1] = 1234567890123;
        }

        //needs proper transaction inputs and ouputs

        for(int i = 1; i <= 100; i++)
        {
            CTransaction tx = CTransaction();
            tx.nTime = 123;

            CTxIn input = CTxIn();
            input.nSequence = 678;

            CTxOut output = CTxOut();
            output.nValue = 1234;

            tx.vin.push_back(input);
            tx.vout.push_back(output);

            block.vtx.push_back(tx);
        }

        BOOST_TEST_MESSAGE(::GetSerializeSize(block, SER_DISK, CLIENT_VERSION));

        std::stringstream ss;
        block.Serialize(ss, 1, 1);
        
        BOOST_TEST_MESSAGE(ss.str().length());
}

BOOST_AUTO_TEST_CASE(serialize_block_test_byte_offsets)
{
        CBlock block = CBlock();
        std::stringstream ss;
        ss.str("");
        block.Serialize(ss, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss.str()));

        CTxDestination addr1 = CKeyID(uint160(1));
        block.addressBalances[addr1] = 123;
        
        ss.str("");
        block.Serialize(ss, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss.str()));
        
        CTxDestination addr2 = CKeyID(uint160(2));
        block.addressBalances[addr2] = 123;
        
        ss.str("");
        block.Serialize(ss, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss.str()));
        
        CTransaction tx = CTransaction();
        CTxIn input = CTxIn();
        input.nSequence = 678;
        CTxOut output = CTxOut();
        output.nValue = 1234;

        tx.vin.push_back(input);
        tx.vout.push_back(output);

        block.vtx.push_back(tx);

        ss.str("");
        block.Serialize(ss, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss.str()));

        block.vtx.push_back(tx);

        ss.str("");
        block.Serialize(ss, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss.str()));
}


BOOST_AUTO_TEST_CASE(serialize_block_test_byte_offsets_invoice1)
{
        CBlock block = CBlock();
        std::stringstream ss;

        CTransaction tx = CTransaction();
        CTxIn input = CTxIn();
        input.nSequence = 678;
        CTxOut output = CTxOut();
        output.nValue = 1234;
        output.invoiceNumber = "";

        tx.vin.push_back(input);
        tx.vout.push_back(output);

        block.vtx.push_back(tx);

        ss.str("");
        block.Serialize(ss, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss.str()));
}

BOOST_AUTO_TEST_CASE(serialize_block_test_byte_offsets_invoice2)
{
        CBlock block = CBlock();
        std::stringstream ss;

        CTransaction tx = CTransaction();
        CTxIn input = CTxIn();
        input.nSequence = 678;
        CTxOut output = CTxOut();
        output.nValue = 1234;

        tx.vin.push_back(input);
        tx.vout.push_back(output);

        block.vtx.push_back(tx);

        ss.str("");
        block.Serialize(ss, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss.str()));
}

BOOST_AUTO_TEST_CASE(varints)
{
    // encode

    CDataStream ss(SER_DISK, 0);
    CDataStream::size_type size = 0;
    for (int i = 0; i < 100000; i++) {
        ss << VARINT(i);
        size += ::GetSerializeSize(VARINT(i), 0, 0);
        BOOST_CHECK(size == ss.size());
    }

    for (uint64_t i = 0;  i < 100000000000ULL; i += 999999937) {
        ss << VARINT(i);
        size += ::GetSerializeSize(VARINT(i), 0, 0);
        BOOST_CHECK(size == ss.size());
    }

    // decode
    for (int i = 0; i < 100000; i++) {
        int j;
        ss >> VARINT(j);
        BOOST_CHECK_MESSAGE(i == j, "decoded:" << j << " expected:" << i);
    }

    for (uint64_t i = 0;  i < 100000000000ULL; i += 999999937) {
        uint64_t j;
        ss >> VARINT(j);
        BOOST_CHECK_MESSAGE(i == j, "decoded:" << j << " expected:" << i);
    }

}

BOOST_AUTO_TEST_CASE(genesis_block)
{
        CBlock block = Params().GenesisBlock();
        std::stringstream ss;
        ss.str("");
        block.Serialize(ss, 1, 1);
        BOOST_TEST_MESSAGE(string_to_hex(ss.str()));
}


BOOST_AUTO_TEST_SUITE_END()
