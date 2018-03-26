#include <boost/test/unit_test.hpp>

#include "main.h"
#include "miner.h"
#include "txdb.h"
#include <fs.h>
#include <test/test_runner.h>


BOOST_AUTO_TEST_SUITE(blockchain_tests)

void addBlocksToChain(int toBlockHeight)
{
        int currentHeight = pindexBest->nHeight;
        while(currentHeight < toBlockHeight - 1)
        {
                currentHeight = pindexBest->nHeight;
                
                CBlockIndex *pindexPrev = pindexBest;
                unsigned int nExtraNonce = 0;
                int64_t nFees = 0;
                CPubKey newCoinbaseKey;
                ptestWallet->GetKeyFromPool(newCoinbaseKey);
                auto_ptr<CBlock> pblocktemplate(CreateNewBlock(newCoinbaseKey, false, &nFees));
                CBlock *pblock = pblocktemplate.get();

                IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);
                uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();
                uint256 hash;

                while (true)
                {
                        hash = pblock->GetPoWHash();
                        if (hash <= hashTarget)
                        {
                                if (CheckWork(pblock, *ptestWallet))
                                {
                                        break;
                                }
                        }

                        ++pblock->nNonce;
                        if (pblock->nTime > FutureDrift((int64_t)pblock->vtx[0].nTime))
                        {
                                break; //coinbase timestamp vs block timestamp will be out of sync and cause the block to be rejected, so we must regenerate it occasionally.
                        }
                }
        }
}
/*
BOOST_AUTO_TEST_CASE(add_block_to_chain)
{
        //check current tip is the genesis block
        CBlockIndex *pgenesisBlockIndex = mapBlockIndex[hashBestChain];
        CBlockIndex *pblockindex = mapBlockIndex[hashBestChain];
        BOOST_TEST_MESSAGE(pblockindex->phashBlock->ToString());
        BOOST_CHECK(pblockindex->phashBlock->ToString() == GENESIS_BLOCK_HASH);

        //mine new block
        
        //check the current tip is not the genesis block
        pblockindex = mapBlockIndex[hashBestChain];
        BOOST_TEST_MESSAGE(pblockindex->phashBlock->ToString());
        BOOST_CHECK(pblockindex->phashBlock->ToString() != GENESIS_BLOCK_HASH);

        //disconnect tip block from chain
        CTxDB txdb;
        pblock->DisconnectBlock(txdb, pblockindex); //only updates disk representation, in memory must be done manually.
        if (!txdb.WriteHashBestChain(pblockindex->pprev->GetBlockHash()))
        {
                BOOST_FAIL("error updating hash best chain on disk");
        }
        pblockindex->pprev->pnext = NULL;
        hashBestChain = *(pgenesisBlockIndex->phashBlock);

        //check the current tip is the genesis block
        pblockindex = mapBlockIndex[hashBestChain];
        BOOST_TEST_MESSAGE(pblockindex->phashBlock->ToString());
        BOOST_CHECK(pblockindex->phashBlock->ToString() == GENESIS_BLOCK_HASH);
}
*/

BOOST_AUTO_TEST_CASE(crisp_simple_test)
{
        /* works somtimes, but then breaks and re-running the test doesn't work... 
        CBlockIndex *pblockindex = mapBlockIndex[hashBestChain];
        BOOST_CHECK(pblockindex->phashBlock->ToString() == GENESIS_BLOCK_HASH);

        addBlocksToChain(250);

        pblockindex = mapBlockIndex[hashBestChain];
        BOOST_CHECK(pblockindex->phashBlock->ToString() != GENESIS_BLOCK_HASH);
        BOOST_CHECK(pblockindex->nHeight == 250);

        CBlockIndex* pcrispBlockIndex = mapBlockIndex[hashBestChain];
        while (pcrispBlockIndex->nHeight > 225)
                pcrispBlockIndex = pcrispBlockIndex->pprev;

        CBlock crispBlock;
        crispBlock.ReadFromDisk(pcrispBlockIndex, true);
                
        BOOST_CHECK(crispBlock.addressBalances.size() > 1);
        */
}




BOOST_AUTO_TEST_SUITE_END()
