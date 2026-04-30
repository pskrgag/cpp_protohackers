#pragma once
#include <cocur/net/tcp.h>
#include <cocur/scheduler/scheduler.h>
#include <cocur/scheduler/task.h>
#include <deque>
#include <format>
#include <map>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>

class ClientSocket {
public:
    ClientSocket(cocur::TcpClient client) : client_(std::move(client)), messages_{} {
    }

    cocur::Task<std::optional<std::string>> getMessage() {
        std::byte buffer[2000];
        bool to_do_last = false;

        if (messages_.size() == 0) {
            do {
                auto res = co_await client_.recv(buffer);
                if (res == 0) {
                    co_return {};
                }

                auto received_data = std::span(buffer, buffer + res);

                if (to_do_last) {
                    auto elem = std::ranges::find(received_data, static_cast<std::byte>('\n'));
                    auto idx = elem != std::end(received_data)
                                   ? std::distance(received_data.begin(), elem)
                                   : received_data.size();

                    messages_.back().append(std::string_view((char *)buffer, idx));
                    received_data = received_data.subspan(idx);
                }

                for (auto part : received_data | std::views::split(static_cast<std::byte>('\n'))) {
                    if (part.size() != 0)
                        messages_.emplace_back(part.begin(), part.end());
                }

                to_do_last = buffer[res - 1] != static_cast<std::byte>('\n');
            } while (to_do_last);
        }

        auto first = messages_.front();

        messages_.pop_front();
        co_return first;
    }

    cocur::Task<ssize_t> send(std::string_view data) {
        co_return co_await client_.send(data);
    }

private:
    cocur::TcpClient client_;
    std::deque<std::string> messages_;
};
class ChatRoom;
using Client = std::shared_ptr<ClientSocket>;

class UserHandle {
public:
    UserHandle(UserHandle &&other)
        : room_(std::exchange(other.room_, nullptr)), name_(other.name_),
          scheduler_(other.scheduler_) {
    }

    UserHandle operator=(UserHandle &&other) {
        return std::move(other);
    }

    ~UserHandle();

private:
    friend class ChatRoom;
    UserHandle(ChatRoom *room, std::string_view name, cocur::Scheduler<> &scheduler)
        : room_(room), name_(name), scheduler_(scheduler) {
    }

    ChatRoom *room_ = nullptr;
    std::string_view name_;
    cocur::Scheduler<> &scheduler_;
};

class ChatRoom {
public:
    ChatRoom(cocur::Scheduler<> &scheduler) : scheduler_(scheduler) {};

    cocur::Task<std::optional<UserHandle>> add(std::string_view name, Client socket) {
        {
            std::lock_guard _guard(mutex_);

            if (users_.contains(name))
                co_return std::nullopt;

            users_[name] = socket;
        }

        co_await announceNewUser(name);
        co_await sendCurrentUsers(socket, name);
        co_return UserHandle(this, name, scheduler_);
    }

    void remove(std::string_view name) {
        std::lock_guard _guard(mutex_);

        users_.erase(name);
    }

    cocur::Task<> sendMessage(std::string_view cur_name, std::string_view msg) {
        auto message = std::format("[{}] {}\n", cur_name, msg);

        co_await forEachMember([&](auto &name, auto &client) -> cocur::Task<> {
            if (name != cur_name)
                co_await client->send(message);
        });
    }

    size_t numUsers() {
        std::lock_guard _guard(mutex_);
        return users_.size();
    }

    friend class UserHandle;

private:
    template <typename T>
    cocur::Task<> forEachMember(T t) {
        std::lock_guard _guard(mutex_);

        for (auto &[name, client] : users_) {
            co_await t(name, client);
        }
    }
    cocur::Task<> announceNewUser(std::string_view cur_name) {
        auto message = std::format("* {} has entered the room\n", cur_name);

        co_await forEachMember([&](auto &name, auto &client) -> cocur::Task<> {
            if (name != cur_name)
                co_await client->send(message);
        });
    }

    cocur::Task<> userLeaves(std::string name) {
        auto message = std::format("* {} has leaved the room\n", name);

        co_await forEachMember(
            [&](auto &name, auto &client) -> cocur::Task<> { co_await client->send(message); });
    }

    cocur::Task<> sendCurrentUsers(Client &client, std::string_view cur_name) {
        if (numUsers() == 1)
            co_return;

        std::string message = "* The room contains: ";

        co_await forEachMember([&](auto &name, auto &_) -> cocur::Task<> {
            if (name != cur_name) {
                message.push_back(' ');
                message.append(name);
                co_return;
            }
        });

        message.push_back('\n');
        co_await client->send(message);
    }

    cocur::Scheduler<> &scheduler_;
    std::mutex mutex_;
    std::map<std::string_view, Client> users_;
};

inline UserHandle::~UserHandle() {
    if (room_) {
        room_->remove(name_);
        scheduler_.spawn(room_->userLeaves(std::string(name_)));
    }
}
