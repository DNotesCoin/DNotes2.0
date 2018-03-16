

#ifndef INVOICEUTIL_H
#define INVOICEUTIL_H

#include "util.h"


/** Utility functions around invoice number
 */
namespace InvoiceUtil
{
    bool validateInvoiceNumber(std::string input);
    void parseInvoiceNumberAndAddress(std::string input, std::string& outAddress, std::string& outInvoiceNumber);
}

#endif // INVOICEUTIL_H
