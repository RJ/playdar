#ifndef _CURL_CHECKER_HPP_
#define _CURL_CHECKER_HPP_

#include <curl/curl.h> 

// simplify curl return code checking
// throws when assigned anything other than CURLE_OK
// use operator("") to provide more info for the exception msg
//
// eg:
//      CurlChecker cc;
//      cc("setting option") = curl_easy_opt(...)
//
struct CurlChecker
{
    struct Exception : public std::exception
    {
        Exception(CURLcode e, const std::string& doing)
        {
            std::ostringstream oss;
            oss << "error " << e << " from curl " << doing;
            m_what = oss.str();
        }

        virtual const char* what() const
        {
            return m_what.data();
        }

        std::string m_what;
    };

    CurlChecker& operator()(const char *doing)
    {
        m_doing.assign(doing);
        return *this;
    }

    CurlChecker& operator=(CURLcode c)
    {
        if (c != CURLE_OK)
            throw Exception(c, m_doing);
        return *this;
    }

    std::string m_doing;
};

#endif
