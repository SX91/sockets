#ifndef SOCKETS_REACTOR_H
#define SOCKETS_REACTOR_H

#include <ostream>
#include <functional>
#include <unordered_map>

#include <sys/epoll.h>
#include <memory>

enum class Event;

class Ready;

class PollOpt;

class Reactor;

using SocketDescr = int;
using HandlerFunc = std::function<void(Event, SocketDescr)>;
using FdHandlerMap = std::unordered_map<SocketDescr, HandlerFunc>;
using ReactorPtr = std::shared_ptr<Reactor>;
using ReactorWPtr = std::weak_ptr<Reactor>;

const int EPOLL_ERR = -1;
const SocketDescr SOCKET_ERR = -1;

const int MAX_EVENTS = 128;


enum class Event {
    ReadReady,
    WriteReady,
    HangUp,
    Error,
};


std::ostream &operator<<(std::ostream &os, const Event &rhs);


class Ready {
    static const uint32_t READABLE = EPOLLIN;
    static const uint32_t WRITABLE = EPOLLOUT;
    static const uint32_t HUP = EPOLLHUP;

public:
    explicit Ready(uint32_t flags = 0);

    Ready(const Ready &) = default;

    Ready(Ready &&) = default;

    Ready &operator=(const Ready &) = default;

    Ready &operator=(Ready &&) = default;

    Ready operator|(const Ready &other) const noexcept;

    Ready &operator|=(const Ready &other) noexcept;

    uint32_t bits() const noexcept;

    bool contains(const Ready &other) const noexcept;

    bool isReadable() const noexcept;

    bool isWritable() const noexcept;

    bool isHup() const noexcept;

    static Ready readable() noexcept;

    static Ready writable() noexcept;

    static Ready hup() noexcept ;

private:
    uint32_t m_Flags;
};


class PollOpt {
    static const uint32_t EDGE = EPOLLET;
    static const uint32_t ONESHOT = EPOLLONESHOT;

public:
    explicit PollOpt(uint32_t opts = 0) noexcept;

    PollOpt(const PollOpt &) = default;

    PollOpt(PollOpt &&) = default;

    PollOpt operator|(const PollOpt &other) const noexcept;

    PollOpt &operator|=(const PollOpt &other) noexcept;

    uint32_t operator*() const noexcept;

    uint32_t bits() const noexcept;

    bool isEdge() const noexcept;

    bool isLevel() const noexcept;

    bool isOneshot() const noexcept;

    static PollOpt edge() noexcept;

    static PollOpt level() noexcept;

    static PollOpt oneshot() noexcept;

private:
    uint32_t m_Options;
};


class Reactor {
public:
    Reactor() noexcept;

    Reactor(const Reactor &) = delete;

    ~Reactor();

    Reactor &operator=(const Reactor &) = delete;

    void exec();

    void stop() noexcept;

    bool running() const noexcept;

    void subscribe(SocketDescr fd,
                   const Ready &interest,
                   const PollOpt &options,
                   HandlerFunc &&handler);

    void unsubscribe(SocketDescr fd);

    bool contains(SocketDescr fd) const noexcept;

protected:
    void execute();

private:
    int m_EpollFd;
    FdHandlerMap m_FdHandlerMap;

    bool m_Running = false;
};

#endif //SOCKETS_REACTOR_H
