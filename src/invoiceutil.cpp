
#include "util.h"
#include "invoiceutil.h"


namespace InvoiceUtil {

    bool validateInvoiceNumber(std::string input)
    {
        int size = input.size();
        if(size > 32) {
            return false;
        }

        for(int idx=0; idx < size; ++idx)
        {
            int ch = input.at(idx);

            if(((ch >= '0' && ch<='9') ||
            (ch >= 'a' && ch<='z') ||
            (ch >= 'A' && ch<='Z') ||
            (ch == '-')))
            {
                // Alphanumeric or -
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    void parseInvoiceNumberAndAddress(std::string input, std::string& outAddress, std::string& outInvoiceNumber)
    {
        size_t plusIndex = input.find('+');
        if(plusIndex == string::npos)
        {   //no +
            outAddress = input;
            outInvoiceNumber = "";
        }
        else
        {   //+
            outAddress = input.substr(0, plusIndex);
            outInvoiceNumber = input.substr(plusIndex + 1, input.size());
        }
    }

}