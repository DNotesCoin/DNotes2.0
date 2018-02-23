
using namespace std;


#include "main.h"
#include "script.h"

namespace CRISP 
{
    std::map<CTxDestination, int64_t> GetStartingAddressBalances(int blockHeight);
    std::map<CTxDestination, int64_t> CalculateAddressBalanceDeltas(int startingHeight, int endingHeight);
    std::map<CTxDestination, int64_t> CalculateAddressPayouts(std::map<CTxDestination, int64_t> startingAddressBalances, std::map<CTxDestination, int64_t> addressBalanceDeltas);

    //returns address balances to set on the 
    std::map<CTxDestination, int64_t> AddCRISPPayouts(int currentBlockHeight, CTransaction coinbaseTransaction)
    {
        bool makeCRISPPayouts = false;
        bool makeCRISPCatchupPayouts = false;
        //make CRISP payouts every CRISPPayoutInterval blocks
        if(currentBlockHeight > Params().CRISPPayoutInterval() && currentBlockHeight % Params().CRISPPayoutInterval() == Params().CRISPPayoutLag())
        {
            makeCRISPPayouts = true;
        }
        else if(false) //todo: if we had 10k crisp payouts in the previous block, re-do above calculation as of that previous block, 
        {
            makeCRISPPayouts = true;
            makeCRISPCatchupPayouts = true;
        }

        if(makeCRISPPayouts)
        {
            int crispPayoutBlockHeight = currentBlockHeight;
            if(makeCRISPCatchupPayouts)
            {
                //if we're catching up, determine the original block that the payouts should have been for
                crispPayoutBlockHeight = currentBlockHeight % Params().CRISPPayoutInterval() - Params().CRISPPayoutLag();
            }

            std::map<CTxDestination, int64_t> startingBalances = GetStartingAddressBalances(crispPayoutBlockHeight);

            int deltaStartingHeight, deltaEndingHeight;
            deltaStartingHeight = crispPayoutBlockHeight - Params().CRISPPayoutInterval() - Params().CRISPPayoutLag();
            deltaEndingHeight = crispPayoutBlockHeight - Params().CRISPPayoutLag() - 1;
            std::map<CTxDestination, int64_t> balanceDeltas = CalculateAddressBalanceDeltas(deltaStartingHeight, deltaEndingHeight); 
            std::map<CTxDestination, int64_t> payouts = CalculateAddressPayouts(startingBalances, balanceDeltas);

            //iterate through payouts to build vouts for each (up to maximum)
            //build up list of CRISP payouts from recent blocks to avoid double paying them

            if(!makeCRISPCatchupPayouts)
            {
                std::map<CTxDestination, int64_t> currentBalances;
                //combine initial balance and delta and store that value into the block (if we're not playing catchup)
                
                //throw std::runtime_error("not yet implemented");
                return currentBalances; //what i know about heap, pointers, and c++ memory tells me that this will not work how i want.
            }   

        }
    
        std::map<CTxDestination, int64_t> currentBalances;
        return currentBalances; 
    }

    std::map<CTxDestination, int64_t> GetStartingAddressBalances(int blockHeight)
    {
        int blockHeightForAddressBalances = blockHeight - Params().CRISPPayoutInterval();

        //get address balances as of blockheight - Params().CRISPPayoutInterval - Params().CRISPPayoutLag - 1
        //which should be stored in block: blockheight - Params().CRISPPayoutInterval
        if(blockHeightForAddressBalances > Params().CRISPPayoutInterval()){
            //get a block.
            CBlock block;
            CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
            while (pblockindex->nHeight > blockHeightForAddressBalances)
                pblockindex = pblockindex->pprev;

            uint256 hash = *pblockindex->phashBlock;

            pblockindex = mapBlockIndex[hash];
            block.ReadFromDisk(pblockindex, false);

            return block.addressBalances;
        }
        
        std::map<CTxDestination, int64_t> addressBalances; 
        return addressBalances;
    }

    std::map<CTxDestination, int64_t> CalculateAddressBalanceDeltas(int startingHeight, int endingHeight)
    {
        std::map<CTxDestination, int64_t> addressBalanceDeltas;

        //iterate through blocks backward from tip.
        LogPrint("crisp", "starting balance delta loop");
        CBlock block;
        CBlockIndex *pblockindex = mapBlockIndex[hashBestChain];
        while (pblockindex->nHeight >= startingHeight)
        {
            LogPrint("crisp", "balance delta loop %d", pblockindex->nHeight);
            pblockindex = pblockindex->pprev;
            if (pblockindex->nHeight <= endingHeight)
            {
                uint256 hash = *pblockindex->phashBlock;



                pblockindex = mapBlockIndex[hash];
                block.ReadFromDisk(pblockindex, true);

                //transactions
                BOOST_FOREACH (CTransaction &transaction, block.vtx)
                {
                    //inputs
                    BOOST_FOREACH (CTxIn &input, transaction.vin)
                    {
                        //addressBalanceDeltas.count(NoTxDestination)
                        CTransaction inputTransaction;
                        uint256 hashBlock = 0;
                        if (GetTransaction(input.prevout.hash, inputTransaction, hashBlock))
                        {
                            txnouttype type;
                            vector<CTxDestination> addresses;
                            int nRequired;
                            CTxOut originatingOutput = inputTransaction.vout[input.prevout.n];

                            if (!ExtractDestinations(originatingOutput.scriptPubKey, type, addresses, nRequired))
                            {
                            }

                            BOOST_FOREACH (const CTxDestination &addr, addresses)
                            {
                                //a.push_back(CBitcoinAddress(addr).ToString());
                            }
                        }
                        else
                        {
                            //something has gone wrong.
                        }
                    }

                    //outputs
                    BOOST_FOREACH (CTxOut &output, transaction.vout)
                    {
                    }
                }
            }
        }

        return addressBalanceDeltas;
    }

    std::map<CTxDestination, int64_t> CalculateAddressPayouts(std::map<CTxDestination, int64_t> startingAddressBalances, std::map<CTxDestination, int64_t> addressBalanceDeltas)
    {
        std::map<CTxDestination, int64_t> addressPayouts; 
        


        return addressPayouts;
    }
}