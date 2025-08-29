# MTOG/TOG Manager for TI Jacinto 7 (TDA4 J784S4) SoC

## Overview

This repository contains the MTOG/TOG (Timeout Gasket) Manager implementation for OSPAS SW targeting the Texas Instruments Jacinto 7 (TDA4 J784S4) SoC. The implementation manages 16 Timeout Gasket modules with safety-critical timeout monitoring and error handling capabilities.

## Features

- **16 MTOG Instance Management**: Supports all 16 timeout gasket instances (IDs 0-15)
- **Configurable Timeouts**: Individual timeout periods for each gasket instance
- **Safety-Critical Design**: Immediate safe state transitions on timeout or error conditions
- **Runtime Control**: Enable/disable functionality for individual gasket instances
- **Periodic Monitoring**: Continuous validation of configuration registers
- **Interrupt Handling**: Dedicated handlers for timeout and error conditions
- **Self-Test Capability**: Initialization tests to verify timeout functionality
- **Configuration Verification**: Readback verification of all programmed values

## Files

- `mtog_manager.c` - Main implementation file
- `mtog_manager.h` - Public API header file
- `Makefile` - Build configuration
- `README.md` - This documentation

## Architecture

### MTOG Instances
The implementation manages 16 MTOG instances with base addresses:
- MTO0_BASE through MTO15_BASE (0x45D10000 - 0x45D1F000)
- Each instance has a 4KB memory space with standard register layout

### Register Map
- **Control Register (0x00)**: Enable/disable and reset control
- **Status Register (0x04)**: Timeout and error status flags
- **Interrupt Enable Register (0x08)**: Interrupt enable configuration
- **Interrupt Status Register (0x0C)**: Interrupt status and clearing
- **Timeout Value Register (0x10)**: Configurable timeout period

### Safety Features
- **FTTI Compliance**: All safe state transitions occur within Fault Tolerant Time Interval
- **Configuration Monitoring**: Periodic verification of register values
- **Self-Test**: Mandatory timeout functionality testing during initialization
- **Error Detection**: Comprehensive timeout and transaction error detection

## API Reference

### Initialization Functions
```c
bool mtog_manager_init(void);
bool mtog_test_timeout_functionality(void);
```

### Runtime Control Functions
```c
bool mtog_enable_instance(uint32_t gasket_id);
bool mtog_disable_instance(uint32_t gasket_id);
```

### Monitoring Functions
```c
bool mtog_periodic_monitor(void);
uint32_t mtog_get_instance_status(uint32_t gasket_id);
bool mtog_is_initialized(void);
```

### Interrupt Handlers
```c
void mtog_timeout_interrupt_handler(uint32_t gasket_id);
void mtog_error_interrupt_handler(uint32_t gasket_id);
```

### Test Functions
```c
void mtog_inject_timeout_event(uint32_t gasket_id);
void mtog_inject_error_event(uint32_t gasket_id);
```

## Configuration

### Timeout Values
Each MTOG instance has a pre-configured timeout value (in milliseconds):
- MTO4_timeout_ms_0 through MTO4_timeout_ms_15
- Range: 100ms to 200ms (configurable per requirements)

### Monitoring Period
- **MTO6_periodicity_ms**: 1000ms (1 second) periodic monitoring interval

### Safety State
- **TDA4_Safe_Silent**: Safe state identifier (0xDEADBEEF)

## Build Instructions

### Standard Build
```bash
make all
```

### Static Analysis
```bash
make check    # Requires cppcheck
make lint     # Requires splint
```

### Test Execution
```bash
make test
```

### Clean Build
```bash
make clean      # Remove build artifacts
make distclean  # Remove all generated files
```

## Integration Guidelines

### Interrupt Registration
The interrupt handlers must be registered with the system interrupt controller:
```c
// Example interrupt registration (platform-specific)
register_interrupt_handler(MTOG0_TIMEOUT_IRQ, mtog_timeout_interrupt_handler, 0);
register_interrupt_handler(MTOG0_ERROR_IRQ, mtog_error_interrupt_handler, 0);
// ... repeat for all 16 instances
```

### System Integration
1. **Initialization Phase**:
   - Call `mtog_manager_init()` during system startup
   - Call `mtog_test_timeout_functionality()` for safety validation
   - Enable required MTOG instances using `mtog_enable_instance()`

2. **Runtime Phase**:
   - Call `mtog_periodic_monitor()` every 1000ms
   - Handle interrupts through registered handlers
   - Use enable/disable functions for dynamic control

3. **Safe State Integration**:
   - Implement `ComputeProcessor_FTTI_impl()` for platform-specific safe state handling
   - Ensure safe state transition completes within FTTI requirements

### Platform Adaptation
- **Timer Implementation**: Replace `get_system_time_ms()` with platform-specific timer
- **Memory Access**: Verify memory mapping for target hardware
- **Interrupt Handling**: Adapt interrupt registration to target RTOS/OS
- **Safe State**: Implement platform-specific safe state procedures

## Safety Considerations

### ISO 26262 Compliance
- **ASIL Rating**: Designed for ASIL-D safety requirements
- **Fault Detection**: Comprehensive timeout and error detection
- **Fault Response**: Immediate safe state transitions
- **Diagnostic Coverage**: High diagnostic coverage through monitoring

### Testing Requirements
- **Unit Testing**: Individual function validation
- **Integration Testing**: Full system integration validation
- **Safety Testing**: Fault injection and error response validation
- **Performance Testing**: Timing and FTTI compliance validation

## References

- [TDA4 J784S4 Technical Reference Manual](https://www.ti.com/lit/ug/spruil1/spruil1.pdf)
- [TI SDL Documentation](https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-j784s4/latest/exports/docs/sdl/sdl_docs/userguide/j784s4/modules/tog.html)
- ISO 26262 Functional Safety Standard

## Version History

- **v1.0** - Initial implementation with full MTOG/TOG management
  - 16 instance support
  - Safety-critical design
  - Complete API implementation
  - Test harness included

## License

This code is provided for OSPAS SW integration and is subject to project-specific licensing terms.

## Contact

For technical questions or integration support, contact the OSPAS SW development team.