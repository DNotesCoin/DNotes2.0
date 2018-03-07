#include <boost/math/special_functions/round.hpp>
  
using namespace std;

#include "main.h"
#include "script.h"

namespace CRISP
{
std::map<CTxDestination, int64_t> GetStartingAddressBalances(int blockHeight);
std::map<CTxDestination, int64_t> CalculateAddressBalanceDeltas(int startingHeight, int endingHeight);
std::map<CTxDestination, int64_t> CalculateAddressPayouts(std::map<CTxDestination, int64_t> &startingAddressBalances, std::map<CTxDestination, int64_t> &addressBalanceDeltas);

bool BlockShouldHaveCRISPPayouts(int currentBlockHeight, bool &makeCRISPCatchupPayouts)
{
    makeCRISPCatchupPayouts = false;
    bool makeCRISPPayouts = false;
    //make CRISP payouts every CRISPPayoutInterval blocks
    if (currentBlockHeight > Params().CRISPPayoutInterval() && currentBlockHeight % Params().CRISPPayoutInterval() == Params().CRISPPayoutLag())
    {
        makeCRISPPayouts = true;
    }
    else if (false) //todo: if we had 10k crisp payouts in the previous block, re-do above calculation as of that previous block,
    {
        makeCRISPPayouts = true;
        makeCRISPCatchupPayouts = true;
    }

    return makeCRISPPayouts;
}

std::map<CTxDestination, int64_t> AddCRISPPayouts(int currentBlockHeight, CTransaction &coinbaseTransaction)
{
    bool makeCRISPCatchupPayouts = false;
    bool makeCRISPPayouts = BlockShouldHaveCRISPPayouts(currentBlockHeight, makeCRISPCatchupPayouts);

    if (makeCRISPPayouts)
    {
        int crispPayoutBlockHeight = currentBlockHeight;
        if (makeCRISPCatchupPayouts)
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
        std::map<CTxDestination, int64_t>::iterator payoutIterator = payouts.begin();
        while (payoutIterator != payouts.end())
        {
            CTxDestination address = payoutIterator->first;
            int64_t payoutAmount = payoutIterator->second;

            CTxOut crispPayout;
            crispPayout.scriptPubKey.SetDestination(address);
            crispPayout.nValue = payoutAmount;
            coinbaseTransaction.vout.push_back(crispPayout);

            //TODO consider 10k maximum and previous block payouts
            payoutIterator++;
        }

        //TODO: think about sorting the payouts

        //return address balances to store in the block
        if (!makeCRISPCatchupPayouts)
        {
            std::map<CTxDestination, int64_t> currentBalances;
            //combine initial balance and delta and store that value into the block (if we're not playing catchup)

            std::map<CTxDestination, int64_t>::iterator it1 = startingBalances.begin();
            while (it1 != startingBalances.end())
            {
                CTxDestination address = it1->first;
                int64_t value = it1->second;

                currentBalances[address] += value;
                it1++;
            }

            std::map<CTxDestination, int64_t>::iterator it2 = balanceDeltas.begin();
            while (it2 != balanceDeltas.end())
            {
                CTxDestination address = it2->first;
                int64_t value = it2->second;

                currentBalances[address] += value;
                it2++;
            }

            return currentBalances; //am i using the right parameter and return types? * vs & vs normal?
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
    if (blockHeightForAddressBalances > Params().CRISPPayoutInterval())
    {
        //get a block.
        CBlock block;
        CBlockIndex *pblockindex = mapBlockIndex[hashBestChain];
        while (pblockindex->nHeight > blockHeightForAddressBalances)
            pblockindex = pblockindex->pprev;

        uint256 hash = *pblockindex->phashBlock;

        pblockindex = mapBlockIndex[hash];
        block.ReadFromDisk(pblockindex, true);

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
                    uint256 hashBlock = 0;
                    if (GetTransaction(input.prevout.hash, inputTransaction, hashBlock))
                    {
                        CTxOut originatingOutput = inputTransaction.vout[input.prevout.n];

                        if (ExtractDestinations(originatingOutput.scriptPubKey, type, addresses, nRequired))
                        {
                            BOOST_FOREACH (const CTxDestination &addr, addresses)
                            {
                                inputsAddressesInThisTransaction.insert(addr);

                                addressBalanceDeltas[addr] -= originatingOutput.nValue;
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
                            if (transaction.IsCoinBase() || inputsAddressesInThisTransaction.count(addr) > 0)
                            {
                                addressBalanceDeltas[addr] += output.nValue;
                            }
                        }
                    }
                }
            }
        }
    }

    return addressBalanceDeltas;
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

        if (balanceDelta >= 0)
        {
            
            addressPayouts[address] = boost::math::round(matureBalance * Params().CRISPPayoutPercentage());
        }
        else if (matureBalance + balanceDelta > 0)
        {
            addressPayouts[address] = boost::math::round((matureBalance + balanceDelta) * Params().CRISPPayoutPercentage());
        }
        else if (matureBalance + balanceDelta < 0)
        {
            //no payout
        }

        startingBalanceIterator++;
    }

    return addressPayouts;
}
}