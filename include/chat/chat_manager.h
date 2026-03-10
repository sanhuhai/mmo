#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <mutex>
#include <memory>

// Forward declare protobuf type
namespace mmo {
class ChatMessage;
}

namespace mmo {
namespace chat {

// Chat channel enum
enum ChatChannel {
    CHANNEL_WORLD = 0,
    CHANNEL_PRIVATE = 1,
    CHANNEL_GUILD = 2,
    CHANNEL_TEAM = 3,
    CHANNEL_SYSTEM = 4
};

// Chat message structure
struct ChatMessageStruct {
    std::uint64_t sender_id = 0;
    std::string sender_name;
    int channel = 0;
    std::string content;
    std::int64_t timestamp = 0;
    std::uint64_t target_id = 0;
    
    void FromProto(const ::mmo::ChatMessage& proto);
    void ToProto(::mmo::ChatMessage* proto) const;
};

// Chat history
class ChatHistory {
public:
    static constexpr size_t MAX_HISTORY_SIZE = 100;
    
    void AddMessage(const ChatMessageStruct& msg);
    std::vector<ChatMessageStruct> GetRecentMessages(size_t count) const;
    void Clear();
    
private:
    mutable std::mutex mutex_;
    std::vector<ChatMessageStruct> messages_;
};

// Message callback type
using MessageCallback = std::function<void(const ChatMessageStruct&)>;

// Chat manager - singleton
class ChatManager {
public:
    static ChatManager& Instance();
    
    // Init/Shutdown
    bool Initialize();
    void Shutdown();
    
    // Send message
    bool SendChat(std::uint64_t sender_id, const std::string& sender_name, 
                  int channel, const std::string& content, std::uint64_t target_id = 0);
    
    bool SendSystemMessage(int channel, const std::string& content);
    
    void BroadcastMessage(const ChatMessageStruct& msg);
    
    bool SendPrivateMessage(std::uint64_t sender_id, const std::string& sender_name,
                           std::uint64_t target_id, const std::string& content);
    
    // Get chat history
    std::vector<ChatMessageStruct> GetChatHistory(int channel, int count);
    
    // Player session management
    void SetPlayerSession(std::uint64_t player_id, std::uint32_t session_id);
    void RemovePlayerSession(std::uint64_t player_id);
    std::uint32_t GetPlayerSession(std::uint64_t player_id);
    
    // Mute management
    bool IsMuted(std::uint64_t player_id);
    void MutePlayer(std::uint64_t player_id, std::int64_t duration_seconds);
    void UnmutePlayer(std::uint64_t player_id);
    
    // Message callback registration
    void RegisterMessageCallback(int channel, MessageCallback callback);
    void UnregisterMessageCallback(int channel);
    
    // Client interface
    void SetClientMessageHandler(std::function<void(std::uint64_t, const std::vector<uint8_t>&)> handler);
    void SendToClient(std::uint64_t player_id, const std::vector<uint8_t>& data);
    
private:
    ChatManager() = default;
    ~ChatManager() = default;
    ChatManager(const ChatManager&) = delete;
    ChatManager& operator=(const ChatManager&) = delete;
    
    bool initialized_ = false;
    std::mutex mutex_;
    
    // Channel histories (use unique_ptr because ChatHistory is non-copyable/non-movable)
    std::map<int, std::unique_ptr<ChatHistory>> channel_histories_;
    
    // Player sessions player_id -> session_id
    std::map<std::uint64_t, std::uint32_t> player_sessions_;
    
    // Muted players player_id -> unmute timestamp
    std::map<std::uint64_t, std::int64_t> muted_players_;
    
    // Message callbacks
    std::map<int, MessageCallback> message_callbacks_;
    
    // Client message handler
    std::function<void(std::uint64_t, const std::vector<uint8_t>&)> client_handler_;
};

} // namespace chat
} // namespace mmo
