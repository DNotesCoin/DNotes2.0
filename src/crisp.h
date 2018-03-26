#ifndef CRISP_H
#define CRISP_H

#include "main.h"
#include "wallet.h"

namespace CRISP
{
    bool BlockShouldHaveAddressBalances(int currentBlockHeight, bool &addCatchupAddressBalances);
    bool BlockShouldHaveCRISPPayouts(int currentBlockHeight, bool& makeCRISPCatchupPayouts);
    std::map<CTxDestination, int64_t> AddCRISPPayouts(int currentBlockHeight, CTransaction& coinbaseTransaction);
}

#endif // CRISP_H
