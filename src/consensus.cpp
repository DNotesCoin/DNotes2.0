
using namespace std;

#include "main.h"
#include "script.h"
#include "crisp.h"

namespace Consensus
{

bool ValidateReward(CBlockIndex *proposedBlockIndex, CBlock &proposedBlock, int64_t fees, int64_t stakeReward)
{
    bool makeCRISPCatchupPayouts;

    if (CRISP::BlockShouldHaveCRISPPayouts(proposedBlockIndex->nHeight, makeCRISPCatchupPayouts))
    {
        CTransaction calculatedCRISPCoinbase = CTransaction();
        std::map<CTxDestination, int64_t> calculatedAddressBalances;
        calculatedAddressBalances = CRISP::AddCRISPPayouts(proposedBlockIndex->nHeight, calculatedCRISPCoinbase);
        CTransaction proposedCRISPCoinbase = proposedBlock.vtx[0];

        //add extra output for coinbase reward
        CTxOut coinbaseOutput = CTxOut();
        calculatedCRISPCoinbase.vout.insert(calculatedCRISPCoinbase.vout.begin(), coinbaseOutput);

        //validate our calculated crisp payouts match the proposed block's
        if (proposedCRISPCoinbase.vout.size() != calculatedCRISPCoinbase.vout.size())
        {
            return proposedBlock.DoS(50, error("ConnectBlock() : coinbase CRISP Payout invalid."));
        }

        for (uint i = 0; i < calculatedCRISPCoinbase.vout.size(); i++)
        {
            CTxOut calculatedOutput = calculatedCRISPCoinbase.vout[i];
            CTxOut proposedOutput = proposedCRISPCoinbase.vout[i];

            if (i == 0)
            {
                int64_t calculatedFirstOutputValue = 0; //POS has a 0 value output
                if (proposedBlock.IsProofOfWork())
                {
                    calculatedFirstOutputValue = GetProofOfWorkReward(fees);
                }

                if (proposedOutput.nValue != calculatedFirstOutputValue)
                {
                    return proposedBlock.DoS(50, error("ConnectBlock() : coinbase reward exceeded (actual=%d vs calculated=%d)",
                                                       proposedOutput.nValue,
                                                       calculatedFirstOutputValue));
                }
            }
            else
            {
                if (calculatedOutput.nValue != proposedOutput.nValue || calculatedOutput.scriptPubKey != proposedOutput.scriptPubKey)
                {
                    return proposedBlock.DoS(50, error("ConnectBlock() : coinbase CRISP Payout invalid."));
                }
            }
        }

        if (makeCRISPCatchupPayouts)
        {
            if (proposedBlock.addressBalances.size() != 0)
            {
                return proposedBlock.DoS(50, error("ConnectBlock() : addressBalance should be empty for non CRISP Blocks."));
            }
        }
        else
        {
            //validate our calculated addressBalances match the proposed block's
            if (calculatedAddressBalances.size() != proposedBlock.addressBalances.size())
            {
                return proposedBlock.DoS(50, error("ConnectBlock() : addressBalance invalid."));
            }

            std::map<CTxDestination, int64_t>::iterator it = calculatedAddressBalances.begin();
            while (it != calculatedAddressBalances.end())
            {
                CTxDestination address = it->first;
                int64_t payoutAmount = it->second;

                if (proposedBlock.addressBalances[address] != payoutAmount) //more robust?
                {
                    return proposedBlock.DoS(50, error("ConnectBlock() : coinbase CRISP Payout invalid."));
                }

                it++;
            }
        }
    }
    else
    {
        if (proposedBlock.addressBalances.size() != 0)
        {
            return proposedBlock.DoS(50, error("ConnectBlock() : addressBalance should be empty for non CRISP Blocks."));
        }
    }

    if (proposedBlock.IsProofOfWork())
    {
        int64_t nReward = GetProofOfWorkReward(fees);
        if (proposedBlock.vtx[0].vout[0].nValue != nReward)
        {
            return proposedBlock.DoS(50, error("ConnectBlock() : coinbase reward exceeded (actual=%d vs calculated=%d)",
                                               proposedBlock.vtx[0].vout[0].nValue,
                                               nReward));
        }
        /*
                //Check coinbase reward
                if (vtx[0].GetValueOut() > nReward)
                return DoS(50, error("ConnectBlock() : coinbase reward exceeded (actual=%d vs calculated=%d)",
                    vtx[0].GetValueOut(),
                    nReward));
                    */
    }
    if (proposedBlock.IsProofOfStake())
    {
        //removed coinage, it's from peercoin and we don't make use of it anymore.
        int64_t nCalculatedStakeReward = GetProofOfStakeReward(proposedBlockIndex->pprev, 0, fees);

        if (stakeReward > nCalculatedStakeReward)
        {
            return proposedBlock.DoS(100, error("ConnectBlock() : coinstake pays too much(actual=%d vs calculated=%d)", stakeReward, nCalculatedStakeReward));
        }
    }

    return true;
}
}