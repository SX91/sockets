#include "Reactor.h"

#include <iostream>
#include <unistd.h>


std::ostream &operator<<(std::ostream &os, const Event &rhs) {
    switch (rhs) {
        case Event::ReadReady:
            return os << "ReadReady";
        case Event::WriteReady:
            return os << "WriteReady";
        case Event::HangUp:
            return os << "HangUp";
        case Event::Error:
            return os << "Error";
    }

    return os << "InvalidEvent(" << static_cast<int>(rhs) << ")";
}


Ready::Ready(uint32_t flags) : m_Flags(flags)
{}

Ready Ready::operator|(const Ready &other) const noexcept {
    return Ready(m_Flags | other.m_Flags);
}

Ready &Ready::operator|=(const Ready &other) noexcept {
    m_Flags |= other.m_Flags;
    return *this;
}

uint32_t Ready::bits() const noexcept {
    return m_Flags;
}

bool Ready::contains(const Ready &other) const noexcept {
    return (m_Flags & other.m_Flags) != 0;
}

bool Ready::isReadable() const noexcept {
    return (m_Flags & READABLE) != 0;
}

bool Ready::isWritable() const noexcept {
    return (m_Flags & WRITABLE) != 0;
}

bool Ready::isHup() const noexcept {
    return (m_Flags & HUP) != 0;
}

Ready Ready::readable() noexcept {
    return Ready(READABLE);
}

Ready Ready::writable() noexcept {
    return Ready(WRITABLE);
}

Ready Ready::hup() noexcept {
    return Ready(HUP);
}

PollOpt &PollOpt::operator|=(const PollOpt &other) noexcept {
    m_Options |= other.m_Options;
    return *this;
}

PollOpt PollOpt::operator|(const PollOpt &other) const noexcept {
    return PollOpt(m_Options | other.m_Options);
}

uint32_t PollOpt::operator*() const noexcept {
    return bits();
}

uint32_t PollOpt::bits() const noexcept {
    return m_Options;
}

bool PollOpt::isEdge() const noexcept {
    return (m_Options & EDGE) != 0;
}

bool PollOpt::isLevel() const noexcept {
    return (m_Options & EDGE) == 0;
}

bool PollOpt::isOneshot() const noexcept {
    return (m_Options & ONESHOT) != 0;
}

PollOpt PollOpt::edge() noexcept {
    return PollOpt(EDGE);
}

PollOpt PollOpt::level() noexcept {
    return PollOpt(0);
}

PollOpt PollOpt::oneshot() noexcept {
    return PollOpt(ONESHOT);
}

PollOpt::PollOpt(uint32_t opts) noexcept
        : m_Options(opts)
{}

Reactor::Reactor() noexcept
        : m_EpollFd(epoll_create1(0))
{}

Reactor::~Reactor() {
    close(m_EpollFd);
}

void Reactor::exec() {
    execute();
}

void Reactor::stop() noexcept {
    if (m_Running)
        m_Running = false;
}

bool Reactor::running() const noexcept {
    return m_Running;
}

void Reactor::subscribe(SocketDescr fd,
                        const Ready &interest,
                        const PollOpt &options,
                        HandlerFunc &&handler)
{
    int action = 0;

    auto handler_it = m_FdHandlerMap.find(fd);
    if (handler_it == m_FdHandlerMap.end()) {
        action = EPOLL_CTL_ADD;
        m_FdHandlerMap.insert(FdHandlerMap::value_type(fd, handler));
    } else {
        action = EPOLL_CTL_MOD;
        handler_it->second = handler;
    }

    epoll_event ev;
    ev.data.fd = fd;
    ev.events = options.bits() | interest.bits();

    if (EPOLL_ERR == epoll_ctl(m_EpollFd, action, fd, &ev)) {
        throw std::runtime_error("could not set ReadyRead handler");
    }
}

void Reactor::unsubscribe(SocketDescr fd) {
    auto iter = m_FdHandlerMap.find(fd);
    if (iter != m_FdHandlerMap.end()) {
        if (epoll_ctl(m_EpollFd, EPOLL_CTL_DEL, fd, nullptr) == EPOLL_ERR)
            std::cerr << "ERROR: "
                      << "could not remove file descriptor from epoll"
                      << std::endl;
//            throw std::runtime_error("could not delete fd from epoll");
        m_FdHandlerMap.erase(iter);
    }
}

bool Reactor::contains(SocketDescr fd) const noexcept {
    return (m_FdHandlerMap.find(fd) != m_FdHandlerMap.end());
}

void Reactor::execute() {
    epoll_event event_list[MAX_EVENTS];

    m_Running = true;
    while (m_Running) {
        int event_count = 0;
        do {
            event_count = epoll_wait(m_EpollFd, event_list, MAX_EVENTS, -1);
        } while (event_count < 0 && errno == EINTR);

        if (event_count == EPOLL_ERR) {
            std::cerr << "ERROR: epoll error " << errno << std::endl;
            continue;
        }

        for (int i = 0; i < event_count; i++) {
            epoll_event &ev = event_list[i];
            auto callback = m_FdHandlerMap[ev.data.fd];

            if (ev.events & EPOLLIN) {
                callback(Event::ReadReady, ev.data.fd);
            } else if (ev.events & EPOLLOUT) {
                callback(Event::WriteReady, ev.data.fd);
            } else if (ev.events & EPOLLHUP) {
                callback(Event::HangUp, ev.data.fd);
            } else if (ev.events & EPOLLRDHUP) {
                callback(Event::HangUp, ev.data.fd);
            } else if (ev.events & EPOLLERR) {
                callback(Event::Error, ev.data.fd);
            } else {
                std::cerr << "ERROR: "
                          << "unsupported event " << ev.events
                          << " occurred on fd " << ev.data.fd
                          << std::endl;
            }
        }
    }
}
