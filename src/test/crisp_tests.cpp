#include <boost/test/unit_test.hpp>

#include "main.h"
#include "miner.h"


CBlockIndex genesisBlockIndex;
CBlockIndex tipBlockIndex;

BOOST_AUTO_TEST_SUITE(crisp_tests)

BOOST_AUTO_TEST_CASE(functional_simple)
{
        //create some blocks
        CBlockIndex blockIndex;
        //CBlock block = new CBlock();

        blockIndex.pnext = &blockIndex;
        blockIndex.pprev = &blockIndex;
        blockIndex.phashBlock = new uint256();
        blockIndex.nHeight = 1;
        CPubKey pubKey = CPubKey();

        pindexBest = &blockIndex;
/*
        for( int i = 0; i < 100; i++)
        {
                CBlock* b = CreateNewBlock(pubKey, false, 0);
        }
*/
        std::vector<CBlock> blockChain;
        for(int i = 0; i < 100; i++)
        {
                CBlock block;
                block.nBits = 1;
                blockChain.push_back(block);
        }

        BOOST_CHECK(blockIndex.pnext->pnext->pnext->nHeight == 1);


        //delete block;
        //delete blockIndex;
}


BOOST_AUTO_TEST_CASE(performance_simple)
{
       
}


BOOST_AUTO_TEST_SUITE_END()
