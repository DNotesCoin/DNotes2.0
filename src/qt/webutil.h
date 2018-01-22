#ifndef WEBUTIL_H
#define WEBUTIL_H

/** Utility functions used by the Bitcoin Qt UI.
 */
namespace WebUtil
{
    // Get http response from url
    std::string getHttpsResponseFromUrl(std::string host, std::string path);
    std::string getHttpResponseFromUrl(std::string host, std::string path);
} // namespace WebUtil

#endif // WEBUTIL_H
