#include <QApplication>

#include "webutil.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>

using std::string;

namespace WebUtil 
{
    string getHttpResponseFromUrl(string host, string path)
    {
        try
        {
            string port = "80";
            boost::system::error_code ec;
            using namespace boost::asio;
            using boost::asio::ip::tcp;

            // what we need
            boost::asio::io_service io_service;

            tcp::resolver resolver(io_service);
            tcp::resolver::query query(host, "http");
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

            tcp::socket socket(io_service);
            boost::asio::connect(socket, endpoint_iterator);

            // send request
            std::string request("GET " + path + " HTTP/1.1\r\n"
                                "Host: " + host + "\r\n"
                                "Accept: */*\r\n"
                                "Connection: close\r\n\r\n");
            boost::asio::write(socket, buffer(request));

            // read response
            std::string response;

            do {
                char buf[1024];
                size_t bytes_transferred = socket.receive(buffer(buf), {}, ec);
                if (!ec) response.append(buf, buf + bytes_transferred);
            } while (!ec);

            return response ;
        }
        catch(std::exception const& e)
        {
            return "";
        }
    }

    string getHttpsResponseFromUrl(string host, string path)
    {
        try
        {
            string port = "443";
            boost::system::error_code ec;
            using namespace boost::asio;
            using namespace boost::asio::ssl;

            // what we need
            io_service svc;
            ip::tcp::resolver resolver(svc);
            ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve({host, port});

            ssl::context ctx(svc, (ssl::context::method) 10); //ssl::context::method::sslv23_client = 10
            ssl::stream<ip::tcp::socket> ssock(svc, ctx);

            boost::asio::connect(ssock.lowest_layer(), endpoint_iterator);

            ssock.handshake((ssl::stream_base::handshake_type) 0); //ssl::stream_base::handshake_type::client = 0

            // send request
            std::string request("GET " + path + " HTTP/1.1\r\n"
                                "Host: " + host + "\r\n"
                                "Accept: */*\r\n"
                                "Connection: close\r\n\r\n");
            boost::asio::write(ssock, buffer(request));

            // read response
            std::string response;

            do {
                char buf[1024];
                size_t bytes_transferred = ssock.read_some(buffer(buf), ec);
                if (!ec) response.append(buf, buf + bytes_transferred);
            } while (!ec);

            return response ;
        }
        catch(std::exception const& e)
        {
            return "";
        }
    }


} // namespace WebUtil

