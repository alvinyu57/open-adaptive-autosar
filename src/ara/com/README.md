# `ara/com`

Communication APIs and service binding runtime for Adaptive AUTOSAR.

## Responsibilities

The `ara/com` package provides service-oriented communication contracts and manifest-driven runtime support for service providers and consumers.

## Main Building Blocks

- **`include/ara/com/types.h`**: Core communication types (ServiceIdentifierType, InstanceIdentifier)
- **`include/ara/com/com_error_domain.h`**: Communication error codes and exception handling
- **`include/ara/com/runtime.h`**: Service registration, discovery, and binding management

## Software Architecture Requirements

### Role in the system

- `ara/com` shall provide reusable service contracts consumable by applications on both client and server sides
- `ara/com` shall support manifest-driven service registration and discovery
- `ara/com` shall remain independent from transport protocols (enabling SOME/IP, native IPC, REST, etc.)
- `ara/com` shall integrate with `ara/core` for error handling and runtime context

### Service Registration

- **Service Offering**: Applications register provided services with identifiers and instance mappings
- **Instance Mapping**: Instance specifiers map logical identifiers to concrete transport bindings
- **Service Discovery**: Runtime resolves available service instances and their binding metadata

### Error Handling

- **Error Domain**: Defines communication-specific error codes (ServiceNotAvailable, CommunicationFailure, etc.)
- **Exception Support**: ComException carries detailed error information for exceptional conditions
- **Result Type**: All async operations return `ara::core::Result<T>` for caller flexibility

### Binding Metadata

- **Transport Independence**: Metadata structure enables different binding technologies without API changes
- **Dynamic Resolution**: Service discovery includes binding metadata for runtime endpoint selection
- **Future Extensions**: Architecture supports SOME/IP, native IPC, REST, gRPC, and custom bindings

## Current Limitations

- The current implementation is an in-process reference and does not support inter-process communication
- Service registration is local and not distributed across machines
- Binding enforcement is minimal; metadata is primarily for application-level routing
- Full SOME/IP binding is deferred to future evolution

## Design Principles

- **Simplicity First**: API contracts favor clarity and teachability over feature completeness
- **Manifest-Driven**: Service topology is declarative (via ARXML manifests) rather than imperative code
- **Standards Aligned**: APIs follow AUTOSAR PRS 00048 service interface specifications
- **Test Coverage**: Core functionality is unit-tested and validated in smoke tests
