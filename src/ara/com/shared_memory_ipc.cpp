#include "shared_memory_ipc.hpp"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace ara::com::runtime::internal {

namespace {

constexpr std::uint32_t kEventMagic = 0x45564E54U;
constexpr std::uint32_t kMethodMagic = 0x4D455448U;
constexpr std::uint32_t kOneWayMagic = 0x4F4E4557U;
constexpr std::size_t kPayloadCapacity = 4096U;

std::string SanitizeChannelName(std::string_view value) {
    std::string sanitized;
    sanitized.reserve(value.size() + 16U);
    sanitized.push_back('/');
    for (const char character : value) {
        if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9')) {
            sanitized.push_back(character);
        } else {
            sanitized.push_back('_');
        }
    }
    return sanitized;
}

std::string SemaphoreName(std::string_view channel_name) {
    return SanitizeChannelName(channel_name) + "_sem";
}

template <typename Layout> class NamedSharedMemoryRegion final {
public:
    explicit NamedSharedMemoryRegion(std::string_view channel_name) noexcept
        : name_(SanitizeChannelName(channel_name)) {
        Open();
    }

    ~NamedSharedMemoryRegion() noexcept {
        if (mapping_ != nullptr) {
            munmap(mapping_, sizeof(Layout));
        }
        if (fd_ != -1) {
            close(fd_);
        }
    }

    bool IsOpen() const noexcept {
        return mapping_ != nullptr;
    }

    Layout* Get() const noexcept {
        return mapping_;
    }

private:
    void Open() noexcept {
        fd_ = shm_open(name_.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd_ == -1) {
            return;
        }

        if (ftruncate(fd_, static_cast<off_t>(sizeof(Layout))) == -1) {
            close(fd_);
            fd_ = -1;
            return;
        }

        void* mapping = mmap(nullptr, sizeof(Layout), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (mapping == MAP_FAILED) {
            close(fd_);
            fd_ = -1;
            return;
        }

        mapping_ = static_cast<Layout*>(mapping);
    }

    std::string name_;
    int fd_{-1};
    Layout* mapping_{nullptr};
};

class NamedSemaphore final {
public:
    explicit NamedSemaphore(std::string_view channel_name) noexcept
        : name_(SemaphoreName(channel_name)) {
        semaphore_ = sem_open(name_.c_str(), O_CREAT, 0666, 1);
    }

    ~NamedSemaphore() noexcept {
        if (semaphore_ != SEM_FAILED) {
            sem_close(semaphore_);
        }
    }

    bool IsOpen() const noexcept {
        return semaphore_ != SEM_FAILED;
    }

    sem_t* Get() const noexcept {
        return semaphore_;
    }

private:
    std::string name_;
    sem_t* semaphore_{SEM_FAILED};
};

class SemaphoreGuard final {
public:
    explicit SemaphoreGuard(sem_t* semaphore) noexcept
        : semaphore_(semaphore) {
        if (semaphore_ != nullptr) {
            sem_wait(semaphore_);
        }
    }

    ~SemaphoreGuard() noexcept {
        if (semaphore_ != nullptr) {
            sem_post(semaphore_);
        }
    }

private:
    sem_t* semaphore_{nullptr};
};

struct EventLayout final {
    std::uint32_t magic{0U};
    std::uint64_t sequence{0U};
    std::uint32_t payload_size{0U};
    std::array<char, kPayloadCapacity> payload{};
};

struct MethodLayout final {
    std::uint32_t magic{0U};
    std::uint64_t request_sequence{0U};
    std::uint64_t response_sequence{0U};
    std::uint64_t correlation_id{0U};
    std::uint64_t response_correlation_id{0U};
    std::uint32_t request_size{0U};
    std::uint32_t response_size{0U};
    std::array<char, kPayloadCapacity> request_payload{};
    std::array<char, kPayloadCapacity> response_payload{};
};

struct OneWayLayout final {
    std::uint32_t magic{0U};
    std::uint64_t sequence{0U};
    std::uint32_t payload_size{0U};
    std::array<char, kPayloadCapacity> payload{};
};

ara::core::Result<void> ValidatePayloadSize(std::string_view payload) noexcept {
    if (payload.size() >= kPayloadCapacity) {
        return ara::core::Result<void>{MakeErrorCode(ComErrc::kMaxSamplesExceeded)};
    }
    return ara::core::Result<void>{};
}

template <typename Layout>
ara::core::Result<Layout*> OpenRegion(std::string_view channel_name,
                                      NamedSharedMemoryRegion<Layout>& region) noexcept {
    if (!region.IsOpen()) {
        return ara::core::Result<Layout*>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    return ara::core::Result<Layout*>{region.Get()};
}

} // namespace

ara::core::Result<void> SharedMemoryEventChannel::Publish(std::string_view channel_name,
                                                          std::string_view payload) noexcept {
    auto size_result = ValidatePayloadSize(payload);
    if (!size_result.HasValue()) {
        return size_result;
    }

    NamedSharedMemoryRegion<EventLayout> region(channel_name);
    auto region_result = OpenRegion(channel_name, region);
    if (!region_result.HasValue()) {
        return ara::core::Result<void>{region_result.Error()};
    }

    NamedSemaphore semaphore(channel_name);
    if (!semaphore.IsOpen()) {
        return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    SemaphoreGuard guard(semaphore.Get());
    auto* layout = region_result.Value();
    if (layout->magic != kEventMagic) {
        std::memset(layout, 0, sizeof(EventLayout));
        layout->magic = kEventMagic;
    }

    std::memcpy(layout->payload.data(), payload.data(), payload.size());
    layout->payload[payload.size()] = '\0';
    layout->payload_size = static_cast<std::uint32_t>(payload.size());
    ++layout->sequence;
    return ara::core::Result<void>{};
}

ara::core::Result<std::optional<ara::core::String>>
SharedMemoryEventChannel::ReadIfNew(std::string_view channel_name,
                                    std::uint64_t& last_seen_sequence) noexcept {
    NamedSharedMemoryRegion<EventLayout> region(channel_name);
    auto region_result = OpenRegion(channel_name, region);
    if (!region_result.HasValue()) {
        return ara::core::Result<std::optional<ara::core::String>>{region_result.Error()};
    }

    NamedSemaphore semaphore(channel_name);
    if (!semaphore.IsOpen()) {
        return ara::core::Result<std::optional<ara::core::String>>{
            MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    SemaphoreGuard guard(semaphore.Get());
    auto* layout = region_result.Value();
    if (layout->magic != kEventMagic || layout->sequence == 0U ||
        layout->sequence == last_seen_sequence) {
        return ara::core::Result<std::optional<ara::core::String>>{
            std::optional<ara::core::String>{}};
    }

    last_seen_sequence = layout->sequence;
    return ara::core::Result<std::optional<ara::core::String>>{std::optional<ara::core::String>{
        ara::core::String(std::string_view(layout->payload.data(), layout->payload_size))}};
}

ara::core::Result<ara::core::String>
SharedMemoryMethodChannel::Call(std::string_view channel_name,
                                std::string_view payload,
                                std::chrono::milliseconds timeout) noexcept {
    auto size_result = ValidatePayloadSize(payload);
    if (!size_result.HasValue()) {
        return ara::core::Result<ara::core::String>{size_result.Error()};
    }

    NamedSharedMemoryRegion<MethodLayout> region(channel_name);
    auto region_result = OpenRegion(channel_name, region);
    if (!region_result.HasValue()) {
        return ara::core::Result<ara::core::String>{region_result.Error()};
    }

    NamedSemaphore semaphore(channel_name);
    if (!semaphore.IsOpen()) {
        return ara::core::Result<ara::core::String>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::uint64_t correlation_id = 0U;
    std::uint64_t response_sequence = 0U;
    {
        SemaphoreGuard guard(semaphore.Get());
        auto* layout = region_result.Value();
        if (layout->magic != kMethodMagic) {
            std::memset(layout, 0, sizeof(MethodLayout));
            layout->magic = kMethodMagic;
        }

        correlation_id = layout->correlation_id + 1U;
        layout->correlation_id = correlation_id;
        std::memcpy(layout->request_payload.data(), payload.data(), payload.size());
        layout->request_payload[payload.size()] = '\0';
        layout->request_size = static_cast<std::uint32_t>(payload.size());
        ++layout->request_sequence;
        response_sequence = layout->response_sequence;
    }

    while (std::chrono::steady_clock::now() < deadline) {
        {
            SemaphoreGuard guard(semaphore.Get());
            auto* layout = region_result.Value();
            if (layout->magic == kMethodMagic && layout->response_sequence > response_sequence &&
                layout->response_correlation_id == correlation_id) {
                return ara::core::Result<ara::core::String>{ara::core::String(
                    std::string_view(layout->response_payload.data(), layout->response_size))};
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return ara::core::Result<ara::core::String>{MakeErrorCode(ComErrc::kCommunicationFailure)};
}

ara::core::Result<std::optional<ChannelMessage>>
SharedMemoryMethodChannel::TakeRequest(std::string_view channel_name,
                                       std::uint64_t& last_seen_sequence) noexcept {
    NamedSharedMemoryRegion<MethodLayout> region(channel_name);
    auto region_result = OpenRegion(channel_name, region);
    if (!region_result.HasValue()) {
        return ara::core::Result<std::optional<ChannelMessage>>{region_result.Error()};
    }

    NamedSemaphore semaphore(channel_name);
    if (!semaphore.IsOpen()) {
        return ara::core::Result<std::optional<ChannelMessage>>{
            MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    SemaphoreGuard guard(semaphore.Get());
    auto* layout = region_result.Value();
    if (layout->magic != kMethodMagic || layout->request_sequence == 0U ||
        layout->request_sequence == last_seen_sequence) {
        return ara::core::Result<std::optional<ChannelMessage>>{std::optional<ChannelMessage>{}};
    }

    last_seen_sequence = layout->request_sequence;
    return ara::core::Result<std::optional<ChannelMessage>>{
        std::optional<ChannelMessage>{ChannelMessage{
            layout->correlation_id,
            ara::core::String(
                std::string_view(layout->request_payload.data(), layout->request_size)),
        }}};
}

ara::core::Result<void> SharedMemoryMethodChannel::SendResponse(std::string_view channel_name,
                                                                std::uint64_t correlation_id,
                                                                std::string_view payload) noexcept {
    auto size_result = ValidatePayloadSize(payload);
    if (!size_result.HasValue()) {
        return size_result;
    }

    NamedSharedMemoryRegion<MethodLayout> region(channel_name);
    auto region_result = OpenRegion(channel_name, region);
    if (!region_result.HasValue()) {
        return ara::core::Result<void>{region_result.Error()};
    }

    NamedSemaphore semaphore(channel_name);
    if (!semaphore.IsOpen()) {
        return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    SemaphoreGuard guard(semaphore.Get());
    auto* layout = region_result.Value();
    if (layout->magic != kMethodMagic) {
        std::memset(layout, 0, sizeof(MethodLayout));
        layout->magic = kMethodMagic;
    }

    std::memcpy(layout->response_payload.data(), payload.data(), payload.size());
    layout->response_payload[payload.size()] = '\0';
    layout->response_size = static_cast<std::uint32_t>(payload.size());
    layout->response_correlation_id = correlation_id;
    ++layout->response_sequence;
    return ara::core::Result<void>{};
}

ara::core::Result<void> SharedMemoryFireAndForgetChannel::Send(std::string_view channel_name,
                                                               std::string_view payload) noexcept {
    auto size_result = ValidatePayloadSize(payload);
    if (!size_result.HasValue()) {
        return size_result;
    }

    NamedSharedMemoryRegion<OneWayLayout> region(channel_name);
    auto region_result = OpenRegion(channel_name, region);
    if (!region_result.HasValue()) {
        return ara::core::Result<void>{region_result.Error()};
    }

    NamedSemaphore semaphore(channel_name);
    if (!semaphore.IsOpen()) {
        return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    SemaphoreGuard guard(semaphore.Get());
    auto* layout = region_result.Value();
    if (layout->magic != kOneWayMagic) {
        std::memset(layout, 0, sizeof(OneWayLayout));
        layout->magic = kOneWayMagic;
    }

    std::memcpy(layout->payload.data(), payload.data(), payload.size());
    layout->payload[payload.size()] = '\0';
    layout->payload_size = static_cast<std::uint32_t>(payload.size());
    ++layout->sequence;
    return ara::core::Result<void>{};
}

ara::core::Result<std::optional<ara::core::String>>
SharedMemoryFireAndForgetChannel::Take(std::string_view channel_name,
                                       std::uint64_t& last_seen_sequence) noexcept {
    NamedSharedMemoryRegion<OneWayLayout> region(channel_name);
    auto region_result = OpenRegion(channel_name, region);
    if (!region_result.HasValue()) {
        return ara::core::Result<std::optional<ara::core::String>>{region_result.Error()};
    }

    NamedSemaphore semaphore(channel_name);
    if (!semaphore.IsOpen()) {
        return ara::core::Result<std::optional<ara::core::String>>{
            MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    SemaphoreGuard guard(semaphore.Get());
    auto* layout = region_result.Value();
    if (layout->magic != kOneWayMagic || layout->sequence == 0U ||
        layout->sequence == last_seen_sequence) {
        return ara::core::Result<std::optional<ara::core::String>>{
            std::optional<ara::core::String>{}};
    }

    last_seen_sequence = layout->sequence;
    return ara::core::Result<std::optional<ara::core::String>>{std::optional<ara::core::String>{
        ara::core::String(std::string_view(layout->payload.data(), layout->payload_size))}};
}

} // namespace ara::com::runtime::internal
