#ifndef CRISP_H
#define CRISP_H

#include "main.h"
#include "wallet.h"
#include "crisp.h"

namespace CRISP
{
    std::map<CTxDestination, int64_t> AddCRISPPayouts(int currentBlockHeight, CTransaction coinbaseTransaction);
}

#endif // CRISP_H
