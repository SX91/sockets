#ifndef PROJECT_ERROR_H
#define PROJECT_ERROR_H

#include <exception>
#include <stdexcept>


class SocketError : public std::exception {
public:
    SocketError(std::string what, int error_code)
            : m_Message(std::move(what)), m_ErrorCode(error_code)
    {}

    SocketError(const SocketError&) = default;
    SocketError(SocketError&&) = default;

    SocketError & operator=(const SocketError&) = default;
    SocketError & operator=(SocketError&&) = default;

    int code() const noexcept {
        return m_ErrorCode;
    }

    const char * what() const noexcept override {
        return m_Message.c_str();
    }

private:
    std::string m_Message;
    int m_ErrorCode;
};

#endif //PROJECT_ERROR_H
