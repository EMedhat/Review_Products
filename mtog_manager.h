/**
 * @file mtog_manager.h
 * @brief MTOG/TOG (Timeout Gasket) Manager Header for TI Jacinto 7 (TDA4 J784S4) SoC
 * 
 * This header file contains the public API declarations for the MTOG/TOG manager
 * module used in OSPAS SW for safety-critical timeout management.
 * 
 * @author OSPAS SW Team
 * @date 2024
 * @version 1.0
 */

#ifndef MTOG_MANAGER_H
#define MTOG_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * INCLUDES
 *============================================================================*/

#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * PUBLIC MACROS AND DEFINES
 *============================================================================*/

/** Number of MTOG instances supported */
#define MTOG_NUM_INSTANCES          16U

/** Maximum timeout value in milliseconds */
#define MTOG_MAX_TIMEOUT_MS         1000U

/** Minimum timeout value in milliseconds */
#define MTOG_MIN_TIMEOUT_MS         10U

/*==============================================================================
 * PUBLIC FUNCTION DECLARATIONS
 *============================================================================*/

/**
 * @brief Initialize MTOG Manager
 * 
 * Initializes all 16 MTOG instances with their respective timeout values,
 * performs configuration readback verification, and sets up interrupt handling.
 * 
 * @return true if initialization successful, false otherwise
 * @note If initialization fails, system transitions to safe state
 */
bool mtog_manager_init(void);

/**
 * @brief Test MTOG timeout functionality during initialization
 * 
 * Tests each gasket by setting timeout to zero and verifying timeout detection.
 * This is a mandatory initialization test per safety requirements.
 * 
 * @return true if all tests pass, false otherwise
 * @note If any test fails, system transitions to safe state
 */
bool mtog_test_timeout_functionality(void);

/**
 * @brief Enable MTOG instance at runtime
 * 
 * Enables the specified MTOG instance by setting the enable bit in the
 * control register. This allows runtime control of timeout monitoring.
 * 
 * @param gasket_id Gasket ID (0-15)
 * @return true if successful, false if invalid ID or not initialized
 */
bool mtog_enable_instance(uint32_t gasket_id);

/**
 * @brief Disable MTOG instance at runtime
 * 
 * Disables the specified MTOG instance by clearing the enable bit in the
 * control register. This allows runtime control of timeout monitoring.
 * 
 * @param gasket_id Gasket ID (0-15)
 * @return true if successful, false if invalid ID or not initialized
 */
bool mtog_disable_instance(uint32_t gasket_id);

/**
 * @brief Periodic monitoring of MTOG configuration registers
 * 
 * Monitors all MTOG configuration MMRs for correct values by reading and
 * comparing with expected values. Should be called periodically every
 * MTO6_periodicity_ms (1000ms).
 * 
 * @return true if all configurations are correct, false otherwise
 * @note If any mismatch is detected, system transitions to safe state
 */
bool mtog_periodic_monitor(void);

/**
 * @brief MTOG interrupt handler for timeout conditions
 * 
 * Handles transaction timeout interrupts. When called, immediately
 * transitions system to safe state within FTTI requirements.
 * 
 * @param gasket_id Gasket ID that generated the interrupt (0-15)
 * @note This function should be registered with the interrupt controller
 */
void mtog_timeout_interrupt_handler(uint32_t gasket_id);

/**
 * @brief MTOG interrupt handler for error conditions
 * 
 * Handles unexpected response error interrupts. When called, immediately
 * transitions system to safe state within FTTI requirements.
 * 
 * @param gasket_id Gasket ID that generated the interrupt (0-15)
 * @note This function should be registered with the interrupt controller
 */
void mtog_error_interrupt_handler(uint32_t gasket_id);

/**
 * @brief Get MTOG instance status
 * 
 * Reads and returns the status register value for the specified MTOG instance.
 * Useful for diagnostics and monitoring.
 * 
 * @param gasket_id Gasket ID (0-15)
 * @return Status register value, 0xFFFFFFFF if invalid ID
 */
uint32_t mtog_get_instance_status(uint32_t gasket_id);

/**
 * @brief Check if MTOG manager is initialized
 * 
 * @return true if initialized, false otherwise
 */
bool mtog_is_initialized(void);

/*==============================================================================
 * TEST FUNCTIONS (FOR DEVELOPMENT/TESTING ONLY)
 *============================================================================*/

/**
 * @brief Simulate timeout event injection for testing
 * 
 * @param gasket_id Gasket ID to inject timeout (0-15)
 * @note This function is for testing purposes only
 */
void mtog_inject_timeout_event(uint32_t gasket_id);

/**
 * @brief Simulate error event injection for testing
 * 
 * @param gasket_id Gasket ID to inject error (0-15)
 * @note This function is for testing purposes only
 */
void mtog_inject_error_event(uint32_t gasket_id);

#ifdef __cplusplus
}
#endif

#endif /* MTOG_MANAGER_H */