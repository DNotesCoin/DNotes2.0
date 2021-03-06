#include <boost/math/special_functions/round.hpp>

using namespace std;

#include "main.h"
#include "script.h"
#include "txdb.h"

namespace CRISP
{
std::map<CTxDestination, int64_t> GetStartingAddressBalances(int blockHeight);
void CalculateAddressBalanceDeltas(int startingHeight, int endingHeight, std::map<CTxDestination, int64_t> &fullDeltas, std::map<CTxDestination, int64_t> &matureDeltas);
std::map<CTxDestination, int64_t> CalculateAddressPayouts(std::map<CTxDestination, int64_t> &startingAddressBalances, std::map<CTxDestination, int64_t> &addressBalanceDeltas);

bool PreviousBlockHadMaximumCRISPPayouts()
{
    CBlock block;
    if (hashBestChain != 0)
    {
        CBlockIndex *pblockindex = mapBlockIndex[hashBestChain];
        if (pblockindex != NULL)
        {
            uint256 hash = *pblockindex->phashBlock;

            pblockindex = mapBlockIndex[hash];
            block.ReadFromDisk(pblockindex, true);

            CTransaction coinBaseTransaction = block.vtx[0];
            if (coinBaseTransaction.vout.size() == Params().MaxCoinBaseOutputsPerBlock())
            {
                return true;
            }
        }
    }

    return false;
}

bool PreviousBlockHadMaximumAddressBalances()
{
    CBlock block;
    if (hashBestChain != 0)
    {
        CBlockIndex *pblockindex = mapBlockIndex[hashBestChain];
        if (pblockindex != NULL)
        {
            uint256 hash = *pblockindex->phashBlock;

            pblockindex = mapBlockIndex[hash];
            block.ReadFromDisk(pblockindex, true);

            if (block.addressBalances.size() == Params().MaxAddressBalancesPerBlock())
            {
                return true;
            }
        }
    }

    return false;
}

bool BlockShouldHaveCRISPPayouts(int currentBlockHeight, bool &makeCRISPCatchupPayouts)
{
    makeCRISPCatchupPayouts = false;
    bool makeCRISPPayouts = false;
    bool previousBlockHadMaximumCRISPPayouts = PreviousBlockHadMaximumCRISPPayouts();

    //make CRISP payouts every CRISPPayoutInterval blocks
    if (currentBlockHeight > Params().CRISPPayoutInterval() && currentBlockHeight % Params().CRISPPayoutInterval() == Params().CRISPPayoutLag())
    {
        makeCRISPPayouts = true;
    }
    else if (previousBlockHadMaximumCRISPPayouts)
    {
        makeCRISPPayouts = true;
        makeCRISPCatchupPayouts = true;
    }

    return makeCRISPPayouts;
}

bool BlockShouldHaveAddressBalances(int currentBlockHeight, bool &addCatchupAddressBalances)
{
    addCatchupAddressBalances = false;
    bool addAddressBalances = false;
    bool previousBlockHadMaximumAddressBalances = PreviousBlockHadMaximumAddressBalances();

    //add address balances every CRISPPayoutInterval blocks
    if (currentBlockHeight > Params().CRISPPayoutInterval() && currentBlockHeight % Params().CRISPPayoutInterval() == Params().CRISPPayoutLag())
    {
        addAddressBalances = true;
    }
    else if (previousBlockHadMaximumAddressBalances)
    {
        addAddressBalances = true;
        addCatchupAddressBalances = true;
    }

    return addAddressBalances;
}

std::map<CTxDestination, int64_t> AddCRISPPayouts(int currentBlockHeight, CTransaction &coinbaseTransaction)
{
    bool makeCRISPCatchupPayouts = false;
    bool addCatchupAddressBalances = false;
    bool makeCRISPPayouts = BlockShouldHaveCRISPPayouts(currentBlockHeight, makeCRISPCatchupPayouts);
    bool addAddressBalances = BlockShouldHaveAddressBalances(currentBlockHeight, addCatchupAddressBalances);
   
    int crispPayoutBlockHeight = currentBlockHeight;
    int deltaStartingHeight, deltaEndingHeight;
    std::map<CTxDestination, int64_t> matureBalanceDeltas;
    std::map<CTxDestination, int64_t> fullBalanceDeltas;
    std::map<CTxDestination, int64_t> startingBalances;
    if(makeCRISPPayouts || addAddressBalances)
    {
        //if we're catching up, determine the original block that the payouts should have been for
        crispPayoutBlockHeight = currentBlockHeight - (currentBlockHeight % Params().CRISPPayoutInterval() - Params().CRISPPayoutLag());

        deltaStartingHeight = crispPayoutBlockHeight - Params().CRISPPayoutInterval() - Params().CRISPPayoutLag();
        deltaEndingHeight = crispPayoutBlockHeight - Params().CRISPPayoutLag() - 1;
        startingBalances = GetStartingAddressBalances(crispPayoutBlockHeight);
        CalculateAddressBalanceDeltas(deltaStartingHeight, deltaEndingHeight, fullBalanceDeltas, matureBalanceDeltas);
    }

    if (makeCRISPPayouts)
    {
      
        std::map<CTxDestination, int64_t> payouts = CalculateAddressPayouts(startingBalances, matureBalanceDeltas);

        //iterate through payouts to build vouts for each (up to maximum)
        int payoutIndex = 0;
        int blocksBetweenCurrentAndCRISPPayout = currentBlockHeight - crispPayoutBlockHeight;
        int firstPayoutIndex = (blocksBetweenCurrentAndCRISPPayout * Params().MaxCoinBaseOutputsPerBlock()) - blocksBetweenCurrentAndCRISPPayout; //0 for no catchup
        int lastPayoutIndex = firstPayoutIndex + Params().MaxCoinBaseOutputsPerBlock() - 2;                                                       //+ 9998, 1 is reserved for coinbase, 1 is because the index is inclusive

        std::map<CTxDestination, int64_t>::iterator payoutIterator = payouts.begin();
        while (payoutIterator != payouts.end())
        {
            if (payoutIndex >= firstPayoutIndex && payoutIndex <= lastPayoutIndex)
            {
                CTxDestination address = payoutIterator->first;
                int64_t payoutAmount = payoutIterator->second;

                CTxOut crispPayout;
                crispPayout.scriptPubKey.SetDestination(address);
                crispPayout.nValue = payoutAmount;
                coinbaseTransaction.vout.push_back(crispPayout);
            }

            payoutIterator++;
            payoutIndex++;
        }

        //Note on order:
        //  maps are sorted by the key using the < operator by default, and the variant CTxDestination has an implemented < operator
        //  so the payouts are sorted by address
    }

    //return address balances to store in the block
    if (addAddressBalances)
    {
        std::map<CTxDestination, int64_t> currentBalances;
        //combine initial balance and delta and store that value into the block

        std::map<CTxDestination, int64_t>::iterator it1 = startingBalances.begin();
        while (it1 != startingBalances.end())
        {
            CTxDestination address = it1->first;
            int64_t value = it1->second;

            currentBalances[address] += value;
            it1++;
        }

        std::map<CTxDestination, int64_t>::iterator it2 = fullBalanceDeltas.begin();
        while (it2 != fullBalanceDeltas.end())
        {
            CTxDestination address = it2->first;
            int64_t value = it2->second;

            currentBalances[address] += value;
            it2++;
        }

        //trim 0 balances from list
        //std::map<CTxDestination, int64_t>::iterator it3 = currentBalances.begin();
        for (std::map<CTxDestination, int64_t>::iterator it3 = currentBalances.begin(); it3 != currentBalances.end() /* not hoisted */; /* no increment */)
        {
            int64_t value = it3->second;
            if(value == 0)
            {
                currentBalances.erase(it3++);    // or "it = m.erase(it)" since C++11
            }
            else
            {
                ++it3;
            }
        }

        //iterate through address balances and add the correct ones for this block to build vouts for each (up to maximum)
        int addressBalanceIndex = 0;
        int blocksBetweenCurrentAndCRISPPayout = currentBlockHeight - crispPayoutBlockHeight;
        int firstAddressBalanceIndex = (blocksBetweenCurrentAndCRISPPayout * Params().MaxAddressBalancesPerBlock()); //0 for no catchup
        int lastAddressBalanceIndex = firstAddressBalanceIndex + Params().MaxAddressBalancesPerBlock() - 1;          //+ 9999, 1 is because the index is inclusive
        std::map<CTxDestination, int64_t> returnedBalances;
        
        std::map<CTxDestination, int64_t>::iterator addressBalanceIterator = currentBalances.begin();
        while (addressBalanceIterator != currentBalances.end())
        {
            if (addressBalanceIndex >= firstAddressBalanceIndex && addressBalanceIndex <= lastAddressBalanceIndex)
            {
                CTxDestination address = addressBalanceIterator->first;
                int64_t balance = addressBalanceIterator->second;

                returnedBalances[address] = balance;
            }

            addressBalanceIterator++;
            addressBalanceIndex++;
        }

        return returnedBalances;
    }

    std::map<CTxDestination, int64_t> returnedBalances;
    return returnedBalances;
}

std::map<CTxDestination, int64_t> GetStartingAddressBalances(int blockHeight)
{
    int blockHeightForAddressBalances = blockHeight - Params().CRISPPayoutInterval();
    std::map<CTxDestination, int64_t> addressBalances;

    //collect address balances going back through time.

    //get address balances as of blockheight - Params().CRISPPayoutInterval - Params().CRISPPayoutLag - 1
    //which should be stored in block: blockheight - Params().CRISPPayoutInterval
    if (blockHeightForAddressBalances > Params().CRISPPayoutInterval())
    {
        //get a block.
        CBlock block;
        CBlockIndex *pblockindex = mapBlockIndex[hashBestChain];
        while (pblockindex->nHeight > blockHeightForAddressBalances)
        {
            pblockindex = pblockindex->pprev;
            uint256 hash = *pblockindex->phashBlock;

            pblockindex = mapBlockIndex[hash];
            block.ReadFromDisk(pblockindex, true);

            if(block.addressBalances.size() > 0)
            {
                addressBalances.insert(block.addressBalances.begin(), block.addressBalances.end());
            }
        }
    }
    return addressBalances;
}

void CalculateAddressBalanceDeltas(int startingHeight, int endingHeight, std::map<CTxDestination, int64_t> &fullDeltas, std::map<CTxDestination, int64_t> &matureDeltas)
{
    //iterate through blocks backward from tip.
    LogPrint("crisp", "starting balance delta loop");
    CBlock block;
    CBlockIndex *pblockindex = mapBlockIndex[hashBestChain];

    CTxDB txdb("r");        
    
    while (pblockindex && pblockindex->nHeight >= startingHeight)
    {
        LogPrint("crisp", "balance delta loop %d", pblockindex->nHeight);
        pblockindex = pblockindex->pprev;
        if (pblockindex && pblockindex->nHeight <= endingHeight)
        {
            uint256 hash = *pblockindex->phashBlock;

            pblockindex = mapBlockIndex[hash];
            block.ReadFromDisk(pblockindex, true);
            
            //transactions
            BOOST_FOREACH (CTransaction &transaction, block.vtx)
            {
                std::set<CTxDestination> inputsAddressesInThisTransaction;
                txnouttype type;
                vector<CTxDestination> addresses;
                int nRequired;

                //inputs
                BOOST_FOREACH (CTxIn &input, transaction.vin)
                {
                    CTransaction inputTransaction;
                    CTxIndex txindex;
                    if (inputTransaction.ReadFromDisk(txdb, input.prevout.hash, txindex))
                    {
                        CTxOut originatingOutput = inputTransaction.vout[input.prevout.n];

                        if (ExtractDestinations(originatingOutput.scriptPubKey, type, addresses, nRequired))
                        {
                            BOOST_FOREACH (const CTxDestination &addr, addresses)
                            {
                                inputsAddressesInThisTransaction.insert(addr);

                                fullDeltas[addr] -= originatingOutput.nValue;
                                matureDeltas[addr] -= originatingOutput.nValue;
                            }
                        }
                    }
                    else
                    {
                        //something has gone wrong. or this is a coinbase transaction
                    }
                }

                //outputs
                BOOST_FOREACH (CTxOut &output, transaction.vout)
                {
                    vector<CTxDestination> addresses;
                    if (ExtractDestinations(output.scriptPubKey, type, addresses, nRequired))
                    {
                        BOOST_FOREACH (const CTxDestination &addr, addresses)
                        {
                            fullDeltas[addr] += output.nValue;
                            if (transaction.IsCoinBase() || inputsAddressesInThisTransaction.count(addr) > 0)
                            {
                                matureDeltas[addr] += output.nValue;
                            }
                        }
                    }
                }
            }
        }
    }
}

std::map<CTxDestination, int64_t> CalculateAddressPayouts(std::map<CTxDestination, int64_t> &startingAddressBalances, std::map<CTxDestination, int64_t> &addressBalanceDeltas)
{
    std::map<CTxDestination, int64_t> addressPayouts;

    std::map<CTxDestination, int64_t>::iterator startingBalanceIterator = startingAddressBalances.begin();
    while (startingBalanceIterator != startingAddressBalances.end())
    {
        CTxDestination address = startingBalanceIterator->first;
        int64_t matureBalance = startingBalanceIterator->second;
        int64_t balanceDelta = addressBalanceDeltas[address];

        int64_t payout = 0;
        if (balanceDelta >= 0)
        {
            payout = boost::math::round(matureBalance * Params().CRISPPayoutPercentage());
        }
        else if (matureBalance + balanceDelta > 0)
        {
            payout = boost::math::round((matureBalance + balanceDelta) * Params().CRISPPayoutPercentage());
        }
        else if (matureBalance + balanceDelta < 0)
        {
            //no payout
        }

        if (payout > 0)
        {
            addressPayouts[address] = payout;
        }

        startingBalanceIterator++;
    }

    return addressPayouts;
}
}
