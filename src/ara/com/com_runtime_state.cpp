#include "com_runtime_state.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ara::com::runtime::internal {

namespace {

constexpr std::uint32_t kRegistryMagic = 0x434F4D52U;
constexpr std::size_t kMaxRegistryEntries = 64U;
constexpr std::size_t kMaxInstanceSpecifierLength = 256U;
constexpr std::size_t kMaxInstanceIdentifierLength = 128U;
constexpr std::size_t kMaxServiceIdentifierLength = 128U;
constexpr std::size_t kMaxEndpointLength = 256U;

std::string Sanitize(std::string_view value) {
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

template <typename T, typename Callable>
ara::core::Result<T> ExecuteNoexcept(Callable&& callable) noexcept {
    try {
        return callable();
    } catch (...) {
        return ara::core::Result<T>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }
}

template <typename Callable>
ara::core::Result<void> ExecuteNoexceptVoid(Callable&& callable) noexcept {
    try {
        return callable();
    } catch (...) {
        return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }
}

struct RegistryInstanceEntry final {
    bool in_use{false};
    std::uint32_t binding_type{0U};
    std::array<char, kMaxInstanceSpecifierLength> instance_specifier{};
    std::array<char, kMaxInstanceIdentifierLength> instance_identifier{};
    std::array<char, kMaxEndpointLength> endpoint{};
};

struct RegistryServiceEntry final {
    bool in_use{false};
    std::uint32_t binding_type{0U};
    std::array<char, kMaxServiceIdentifierLength> service_id{};
    std::array<char, kMaxInstanceIdentifierLength> instance_identifier{};
    std::array<char, kMaxEndpointLength> endpoint{};
};

struct RegistryLayout final {
    std::uint32_t magic{0U};
    std::array<RegistryInstanceEntry, kMaxRegistryEntries> instances{};
    std::array<RegistryServiceEntry, kMaxRegistryEntries> services{};
};

template <std::size_t N>
bool CopyString(std::array<char, N>& destination, std::string_view source) {
    if (source.size() >= destination.size()) {
        return false;
    }
    std::fill(destination.begin(), destination.end(), '\0');
    std::memcpy(destination.data(), source.data(), source.size());
    return true;
}
template <typename T>
std::string_view ViewString(const T& source) {
    return std::string_view(source.data(), ::strnlen(source.data(), source.size()));
}

class RegistrySharedMemory final {
public:
    RegistrySharedMemory() noexcept {
        fd_ = shm_open(Sanitize("openaa_ara_com_registry").c_str(), O_CREAT | O_RDWR, 0666);
        if (fd_ == -1) {
            return;
        }

        if (ftruncate(fd_, static_cast<off_t>(sizeof(RegistryLayout))) == -1) {
            close(fd_);
            fd_ = -1;
            return;
        }

        void* mapping =
            mmap(nullptr, sizeof(RegistryLayout), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (mapping == MAP_FAILED) {
            close(fd_);
            fd_ = -1;
            return;
        }

        layout_ = static_cast<RegistryLayout*>(mapping);
    }

    RegistrySharedMemory(const RegistrySharedMemory&) = delete;
    RegistrySharedMemory(RegistrySharedMemory&&) = delete;
    RegistrySharedMemory& operator=(const RegistrySharedMemory&) = delete;
    RegistrySharedMemory& operator=(RegistrySharedMemory&&) = delete;

    ~RegistrySharedMemory() noexcept {
        if (layout_ != nullptr) {
            munmap(layout_, sizeof(RegistryLayout));
        }
        if (fd_ != -1) {
            close(fd_);
        }
    }

    bool IsOpen() const noexcept {
        return layout_ != nullptr;
    }

    RegistryLayout* Get() const noexcept {
        return layout_;
    }

private:
    int fd_{-1};
    RegistryLayout* layout_{nullptr};
};

class RegistrySemaphore final {
public:
    RegistrySemaphore() noexcept {
        semaphore_ = sem_open(Sanitize("openaa_ara_com_registry_sem").c_str(), O_CREAT, 0666, 1);
    }

    RegistrySemaphore(const RegistrySemaphore&) = delete;
    RegistrySemaphore(RegistrySemaphore&&) = delete;
    RegistrySemaphore& operator=(const RegistrySemaphore&) = delete;
    RegistrySemaphore& operator=(RegistrySemaphore&&) = delete;

    ~RegistrySemaphore() noexcept {
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

    SemaphoreGuard(const SemaphoreGuard&) = delete;
    SemaphoreGuard(SemaphoreGuard&&) = delete;
    SemaphoreGuard& operator=(const SemaphoreGuard&) = delete;
    SemaphoreGuard& operator=(SemaphoreGuard&&) = delete;

    ~SemaphoreGuard() noexcept {
        if (semaphore_ != nullptr) {
            sem_post(semaphore_);
        }
    }

private:
    sem_t* semaphore_{nullptr};
};

ara::core::Result<RegistryLayout*> OpenRegistry() noexcept {
    static RegistrySharedMemory persistent_region;
    static RegistrySemaphore persistent_semaphore;
    if (!persistent_region.IsOpen() || !persistent_semaphore.IsOpen()) {
        return ara::core::Result<RegistryLayout*>{MakeErrorCode(ComErrc::kCommunicationFailure)};
    }

    SemaphoreGuard guard(persistent_semaphore.Get());
    auto* layout = persistent_region.Get();
    if (layout->magic != kRegistryMagic) {
        std::memset(layout, 0, sizeof(RegistryLayout));
        layout->magic = kRegistryMagic;
    }

    return ara::core::Result<RegistryLayout*>{layout};
}

RegistrySemaphore& GetRegistrySemaphore() {
    static RegistrySemaphore semaphore;
    return semaphore;
}

template <typename Entry> BindingMetadata ToBindingMetadata(const Entry& entry) {
    return BindingMetadata{
        static_cast<BindingType>(entry.binding_type),
        ara::core::String(ViewString(entry.endpoint)),
    };
}

} // namespace

class IpcBindingRuntime final : public IBindingRuntime {
public:
    ara::core::Result<void> OfferService(const ServiceRecord& record) noexcept override {
        return ExecuteNoexceptVoid([&record]() -> ara::core::Result<void> {
            auto registry_result = OpenRegistry();
            if (!registry_result.HasValue()) {
                return ara::core::Result<void>{registry_result.Error()};
            }

            auto& semaphore = GetRegistrySemaphore();
            if (!semaphore.IsOpen()) {
                return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
            }

            SemaphoreGuard guard(semaphore.Get());
            auto* layout = registry_result.Value();
            for (auto& entry : layout->services) {
                if (entry.in_use && ViewString(entry.service_id) == record.service_id.toString() &&
                    ViewString(entry.instance_identifier) ==
                        record.instance_identifier.toString()) {
                    entry.binding_type = static_cast<std::uint32_t>(record.metadata.binding_type);
                    if (!CopyString(entry.endpoint, record.metadata.endpoint.View())) {
                        return ara::core::Result<void>{
                            MakeErrorCode(ComErrc::kCommunicationFailure)};
                    }
                    return ara::core::Result<void>{};
                }
            }

            for (auto& entry : layout->services) {
                if (entry.in_use) {
                    continue;
                }

                if (!CopyString(entry.service_id, record.service_id.toString()) ||
                    !CopyString(entry.instance_identifier, record.instance_identifier.toString()) ||
                    !CopyString(entry.endpoint, record.metadata.endpoint.View())) {
                    return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
                }

                entry.binding_type = static_cast<std::uint32_t>(record.metadata.binding_type);
                entry.in_use = true;
                return ara::core::Result<void>{};
            }

            return ara::core::Result<void>{MakeErrorCode(ComErrc::kMaxSamplesExceeded)};
        });
    }

    ara::core::Result<void>
    StopOfferService(const ara::com::ServiceIdentifierType& service_id,
                     const ara::com::InstanceIdentifier& instance_identifier) noexcept override {
        return ExecuteNoexceptVoid(
            [&service_id, &instance_identifier]() -> ara::core::Result<void> {
                auto registry_result = OpenRegistry();
                if (!registry_result.HasValue()) {
                    return ara::core::Result<void>{registry_result.Error()};
                }

                auto& semaphore = GetRegistrySemaphore();
                if (!semaphore.IsOpen()) {
                    return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
                }

                SemaphoreGuard guard(semaphore.Get());
                auto* layout = registry_result.Value();
                for (auto& entry : layout->services) {
                    if (entry.in_use && ViewString(entry.service_id) == service_id.toString() &&
                        ViewString(entry.instance_identifier) == instance_identifier.toString()) {
                        entry = RegistryServiceEntry{};
                        return ara::core::Result<void>{};
                    }
                }

                return ara::core::Result<void>{MakeErrorCode(ComErrc::kServiceNotOffered)};
            });
    }

    ara::core::Result<ara::core::Vector<BindingMetadata>>
    FindServices(const ara::com::ServiceIdentifierType& service_id,
                 const ara::com::InstanceIdentifier& instance_identifier) noexcept override {
        return ExecuteNoexcept<ara::core::Vector<BindingMetadata>>(
            [&service_id,
             &instance_identifier]() -> ara::core::Result<ara::core::Vector<BindingMetadata>> {
                auto registry_result = OpenRegistry();
                if (!registry_result.HasValue()) {
                    return ara::core::Result<ara::core::Vector<BindingMetadata>>{
                        registry_result.Error()};
                }

                auto& semaphore = GetRegistrySemaphore();
                if (!semaphore.IsOpen()) {
                    return ara::core::Result<ara::core::Vector<BindingMetadata>>{
                        MakeErrorCode(ComErrc::kCommunicationFailure)};
                }

                ara::core::Vector<BindingMetadata> matches;
                SemaphoreGuard guard(semaphore.Get());
                auto* layout = registry_result.Value();
                for (const auto& entry : layout->services) {
                    if (!entry.in_use) {
                        continue;
                    }
                    if (ViewString(entry.service_id) == service_id.toString() &&
                        ViewString(entry.instance_identifier) == instance_identifier.toString()) {
                        matches.push_back(ToBindingMetadata(entry));
                    }
                }

                return ara::core::Result<ara::core::Vector<BindingMetadata>>{std::move(matches)};
            });
    }
};

ComRuntimeState& ComRuntimeState::Instance() noexcept {
    static ComRuntimeState instance;
    return instance;
}

ara::core::Result<void>
ComRuntimeState::RegisterInstanceMapping(const ara::core::InstanceSpecifier& instance_specifier,
                                         const ara::com::InstanceIdentifier& instance_identifier,
                                         const BindingMetadata& metadata) noexcept {
    return ExecuteNoexceptVoid([&instance_specifier,
                                &instance_identifier,
                                &metadata]() -> ara::core::Result<void> {
        auto registry_result = OpenRegistry();
        if (!registry_result.HasValue()) {
            return ara::core::Result<void>{registry_result.Error()};
        }

        auto& semaphore = GetRegistrySemaphore();
        if (!semaphore.IsOpen()) {
            return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
        }

        SemaphoreGuard guard(semaphore.Get());
        auto* layout = registry_result.Value();
        for (auto& entry : layout->instances) {
            if (entry.in_use && ViewString(entry.instance_specifier) == instance_specifier.View()) {
                entry.binding_type = static_cast<std::uint32_t>(metadata.binding_type);
                if (!CopyString(entry.instance_identifier, instance_identifier.toString()) ||
                    !CopyString(entry.endpoint, metadata.endpoint.View())) {
                    return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
                }
                return ara::core::Result<void>{};
            }
        }

        for (auto& entry : layout->instances) {
            if (entry.in_use) {
                continue;
            }
            if (!CopyString(entry.instance_specifier, instance_specifier.View()) ||
                !CopyString(entry.instance_identifier, instance_identifier.toString()) ||
                !CopyString(entry.endpoint, metadata.endpoint.View())) {
                return ara::core::Result<void>{MakeErrorCode(ComErrc::kCommunicationFailure)};
            }
            entry.binding_type = static_cast<std::uint32_t>(metadata.binding_type);
            entry.in_use = true;
            return ara::core::Result<void>{};
        }

        return ara::core::Result<void>{MakeErrorCode(ComErrc::kMaxSamplesExceeded)};
    });
}

ara::core::Result<ara::com::InstanceIdentifierContainer> ComRuntimeState::ResolveInstanceIDs(
    const ara::core::InstanceSpecifier& instance_specifier) noexcept {
    return ExecuteNoexcept<ara::com::InstanceIdentifierContainer>(
        [&instance_specifier]() -> ara::core::Result<ara::com::InstanceIdentifierContainer> {
            auto registry_result = OpenRegistry();
            if (!registry_result.HasValue()) {
                return ara::core::Result<ara::com::InstanceIdentifierContainer>{
                    registry_result.Error()};
            }

            auto& semaphore = GetRegistrySemaphore();
            if (!semaphore.IsOpen()) {
                return ara::core::Result<ara::com::InstanceIdentifierContainer>{
                    MakeErrorCode(ComErrc::kCommunicationFailure)};
            }

            ara::com::InstanceIdentifierContainer result;
            SemaphoreGuard guard(semaphore.Get());
            auto* layout = registry_result.Value();
            for (const auto& entry : layout->instances) {
                if (!entry.in_use) {
                    continue;
                }
                if (ViewString(entry.instance_specifier) == instance_specifier.View()) {
                    result.push_back(ara::com::InstanceIdentifier::Create(
                        ViewString(entry.instance_identifier)));
                }
            }

            if (result.empty()) {
                return ara::core::Result<ara::com::InstanceIdentifierContainer>{
                    MakeErrorCode(ComErrc::kInstanceIDNotResolvable)};
            }

            return ara::core::Result<ara::com::InstanceIdentifierContainer>{std::move(result)};
        });
}

ara::core::Result<void>
ComRuntimeState::OfferService(const ara::com::ServiceIdentifierType& service_id,
                              const ara::com::InstanceIdentifier& instance_identifier,
                              const BindingMetadata& metadata) noexcept {
    ServiceRecord record{service_id, instance_identifier, metadata};
    return GetOrCreateBindingRuntime(metadata.binding_type).OfferService(record);
}

ara::core::Result<void> ComRuntimeState::StopOfferService(
    const ara::com::ServiceIdentifierType& service_id,
    const ara::com::InstanceIdentifier& instance_identifier) noexcept {
    auto& ipc_runtime = GetOrCreateBindingRuntime(BindingType::kIpc);
    return ipc_runtime.StopOfferService(service_id, instance_identifier);
}

ara::core::Result<ara::core::Vector<BindingMetadata>>
ComRuntimeState::FindServices(const ara::com::ServiceIdentifierType& service_id,
                              const ara::com::InstanceIdentifier& instance_identifier) {
    auto& ipc_runtime = GetOrCreateBindingRuntime(BindingType::kIpc);
    return ipc_runtime.FindServices(service_id, instance_identifier);
}

IBindingRuntime& ComRuntimeState::GetOrCreateBindingRuntime(BindingType binding_type) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto iter = binding_runtimes_.find(binding_type);
    if (iter == binding_runtimes_.end()) {
        iter = binding_runtimes_.emplace(binding_type, std::make_unique<IpcBindingRuntime>()).first;
    }

    return *iter->second;
}

} // namespace ara::com::runtime::internal
