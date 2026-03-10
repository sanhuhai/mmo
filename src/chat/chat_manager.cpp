#include "chat/chat_manager.h"
#include "chat.pb.h"
#include <algorithm>
#include <chrono>

namespace mmo {
namespace chat {

void ChatMessageStruct::FromProto(const ::mmo::ChatMessage& proto) {
    sender_id = proto.sender_id();
    sender_name = proto.sender_name();
    channel = static_cast<int>(proto.channel());
    content = proto.content();
    timestamp = proto.timestamp();
    target_id = proto.target_id();
}

void ChatMessageStruct::ToProto(::mmo::ChatMessage* proto) const {
    proto->set_sender_id(sender_id);
    proto->set_sender_name(sender_name);
    proto->set_channel(static_cast<::mmo::ChatChannel>(channel));
    proto->set_content(content);
    proto->set_timestamp(timestamp);
    proto->set_target_id(target_id);
}

void ChatHistory::AddMessage(const ChatMessageStruct& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push_back(msg);
    if (messages_.size() > MAX_HISTORY_SIZE) {
        messages_.erase(messages_.begin());
    }
}

std::vector<ChatMessageStruct> ChatHistory::GetRecentMessages(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count >= messages_.size()) {
        return messages_;
    }
    return std::vector<ChatMessageStruct>(messages_.end() - count, messages_.end());
}

void ChatHistory::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
}

ChatManager& ChatManager::Instance() {
    static ChatManager instance;
    return instance;
}

bool ChatManager::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return true;
    }
    channel_histories_[CHANNEL_WORLD] = std::make_unique<ChatHistory>();
    channel_histories_[CHANNEL_PRIVATE] = std::make_unique<ChatHistory>();
    channel_histories_[CHANNEL_GUILD] = std::make_unique<ChatHistory>();
    channel_histories_[CHANNEL_TEAM] = std::make_unique<ChatHistory>();
    channel_histories_[CHANNEL_SYSTEM] = std::make_unique<ChatHistory>();
    initialized_ = true;
    return true;
}

void ChatManager::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        return;
    }
    channel_histories_.clear();
    player_sessions_.clear();
    muted_players_.clear();
    message_callbacks_.clear();
    client_handler_ = nullptr;
    initialized_ = false;
}

bool ChatManager::SendChat(std::uint64_t sender_id, const std::string& sender_name,
                          int channel, const std::string& content, std::uint64_t target_id) {
    if (IsMuted(sender_id)) {
        return false;
    }
    ChatMessageStruct msg;
    msg.sender_id = sender_id;
    msg.sender_name = sender_name;
    msg.channel = channel;
    msg.content = content;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    msg.target_id = target_id;
    BroadcastMessage(msg);
    return true;
}

bool ChatManager::SendSystemMessage(int channel, const std::string& content) {
    ChatMessageStruct msg;
    msg.sender_id = 0;
    msg.sender_name = "System";
    msg.channel = channel;
    msg.content = content;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    msg.target_id = 0;
    BroadcastMessage(msg);
    return true;
}

void ChatManager::BroadcastMessage(const ChatMessageStruct& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = channel_histories_.find(msg.channel);
    if (it != channel_histories_.end() && it->second) {
        it->second->AddMessage(msg);
    }
    auto cb_it = message_callbacks_.find(msg.channel);
    if (cb_it != message_callbacks_.end() && cb_it->second) {
        cb_it->second(msg);
    }
}

bool ChatManager::SendPrivateMessage(std::uint64_t sender_id, const std::string& sender_name,
                                    std::uint64_t target_id, const std::string& content) {
    if (IsMuted(sender_id)) {
        return false;
    }
    ChatMessageStruct msg;
    msg.sender_id = sender_id;
    msg.sender_name = sender_name;
    msg.channel = CHANNEL_PRIVATE;
    msg.content = content;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    msg.target_id = target_id;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = channel_histories_.find(CHANNEL_PRIVATE);
        if (it != channel_histories_.end() && it->second) {
            it->second->AddMessage(msg);
        }
    }
    auto cb_it = message_callbacks_.find(CHANNEL_PRIVATE);
    if (cb_it != message_callbacks_.end() && cb_it->second) {
        cb_it->second(msg);
    }
    return true;
}

std::vector<ChatMessageStruct> ChatManager::GetChatHistory(int channel, int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = channel_histories_.find(channel);
    if (it != channel_histories_.end() && it->second) {
        return it->second->GetRecentMessages(static_cast<size_t>(count));
    }
    return std::vector<ChatMessageStruct>();
}

void ChatManager::SetPlayerSession(std::uint64_t player_id, std::uint32_t session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    player_sessions_[player_id] = session_id;
}

void ChatManager::RemovePlayerSession(std::uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    player_sessions_.erase(player_id);
}

std::uint32_t ChatManager::GetPlayerSession(std::uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = player_sessions_.find(player_id);
    if (it != player_sessions_.end()) {
        return it->second;
    }
    return 0;
}

bool ChatManager::IsMuted(std::uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = muted_players_.find(player_id);
    if (it != muted_players_.end()) {
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (it->second > now) {
            return true;
        } else {
            muted_players_.erase(it);
            return false;
        }
    }
    return false;
}

void ChatManager::MutePlayer(std::uint64_t player_id, std::int64_t duration_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    muted_players_[player_id] = now + duration_seconds;
}

void ChatManager::UnmutePlayer(std::uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    muted_players_.erase(player_id);
}

void ChatManager::RegisterMessageCallback(int channel, MessageCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    message_callbacks_[channel] = callback;
}

void ChatManager::UnregisterMessageCallback(int channel) {
    std::lock_guard<std::mutex> lock(mutex_);
    message_callbacks_.erase(channel);
}

void ChatManager::SetClientMessageHandler(std::function<void(std::uint64_t, const std::vector<uint8_t>&)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    client_handler_ = handler;
}

void ChatManager::SendToClient(std::uint64_t player_id, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (client_handler_) {
        client_handler_(player_id, data);
    }
}

} // namespace chat
} // namespace mmo
