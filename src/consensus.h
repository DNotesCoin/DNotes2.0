#ifndef CONSENSUS_H
#define CONSENSUS_H

#include "main.h"
#include "wallet.h"

namespace Consensus
{
    bool ValidateReward(CBlockIndex* proposedBlockIndex, CBlock& proposedBlock, int64_t fees, int64_t stakeReward);
}

#endif // CONSENSUS_H
