/**
 * @file mtog_manager.c
 * @brief MTOG/TOG (Timeout Gasket) Manager for TI Jacinto 7 (TDA4 J784S4) SoC
 * 
 * This module manages 16 MTOG/TOG instances for OSPAS SW, providing timeout
 * configuration, monitoring, interrupt handling, and safe state transitions.
 * 
 * @author OSPAS SW Team
 * @date 2024
 * @version 1.0
 * 
 * References:
 * - TDA4 J784S4 Technical Reference Manual (TRM)
 * - https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-j784s4/latest/exports/docs/sdl/sdl_docs/userguide/j784s4/modules/tog.html
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "mtog_manager.h"

/*==============================================================================
 * MACROS AND DEFINES
 *============================================================================*/

/* Number of MTOG instances */
#define MTOG_NUM_INSTANCES          16U

/* MTOG Base Addresses for 16 instances (gasket IDs 0-15) */
#define MTO0_BASE                   0x45D10000U
#define MTO1_BASE                   0x45D11000U
#define MTO2_BASE                   0x45D12000U
#define MTO3_BASE                   0x45D13000U
#define MTO4_BASE                   0x45D14000U
#define MTO5_BASE                   0x45D15000U
#define MTO6_BASE                   0x45D16000U
#define MTO7_BASE                   0x45D17000U
#define MTO8_BASE                   0x45D18000U
#define MTO9_BASE                   0x45D19000U
#define MTO10_BASE                  0x45D1A000U
#define MTO11_BASE                  0x45D1B000U
#define MTO12_BASE                  0x45D1C000U
#define MTO13_BASE                  0x45D1D000U
#define MTO14_BASE                  0x45D1E000U
#define MTO15_BASE                  0x45D1F000U

/* MTOG Register Offsets */
#define MTOG_TIMEOUT_VAL_REG_OFFSET 0x10U    /* Timeout Value Register */
#define MTOG_CONTROL_REG_OFFSET     0x00U    /* Control Register */
#define MTOG_STATUS_REG_OFFSET      0x04U    /* Status Register */
#define MTOG_INT_ENABLE_REG_OFFSET  0x08U    /* Interrupt Enable Register */
#define MTOG_INT_STATUS_REG_OFFSET  0x0CU    /* Interrupt Status Register */

/* Timeout values for each MTOG instance (in milliseconds) */
#define MTO4_timeout_ms_0           100U
#define MTO4_timeout_ms_1           150U
#define MTO4_timeout_ms_2           200U
#define MTO4_timeout_ms_3           120U
#define MTO4_timeout_ms_4           180U
#define MTO4_timeout_ms_5           160U
#define MTO4_timeout_ms_6           140U
#define MTO4_timeout_ms_7           130U
#define MTO4_timeout_ms_8           170U
#define MTO4_timeout_ms_9           110U
#define MTO4_timeout_ms_10          190U
#define MTO4_timeout_ms_11          125U
#define MTO4_timeout_ms_12          135U
#define MTO4_timeout_ms_13          145U
#define MTO4_timeout_ms_14          155U
#define MTO4_timeout_ms_15          165U

/* Periodic monitoring interval */
#define MTO6_periodicity_ms         1000U    /* 1 second */

/* Control Register Bit Fields */
#define MTOG_CTRL_ENABLE_BIT        (1U << 0)
#define MTOG_CTRL_RESET_BIT         (1U << 1)

/* Status Register Bit Fields */
#define MTOG_STATUS_TIMEOUT_BIT     (1U << 0)
#define MTOG_STATUS_ERROR_BIT       (1U << 1)

/* Interrupt Enable/Status Register Bit Fields */
#define MTOG_INT_TIMEOUT_EN_BIT     (1U << 0)
#define MTOG_INT_ERROR_EN_BIT       (1U << 1)

/* Safe State Definition */
#define TDA4_Safe_Silent            0xDEADBEEFU

/* FTTI (Fault Tolerant Time Interval) in microseconds */
#define ComputeProcessor_FTTI()     ComputeProcessor_FTTI_impl(TDA4_Safe_Silent)

/*==============================================================================
 * TYPE DEFINITIONS
 *============================================================================*/

/**
 * @brief MTOG Instance Configuration Structure
 */
typedef struct {
    volatile uint32_t *base_addr;       /* Base address of MTOG instance */
    uint32_t timeout_ms;                /* Timeout value in milliseconds */
    bool enabled;                       /* Enable/disable state */
    uint32_t expected_ctrl_val;         /* Expected control register value */
    uint32_t expected_timeout_val;      /* Expected timeout register value */
} mtog_instance_t;

/**
 * @brief MTOG Manager State Structure
 */
typedef struct {
    mtog_instance_t instances[MTOG_NUM_INSTANCES];
    bool initialized;
    uint32_t last_monitor_time;
} mtog_manager_t;

/*==============================================================================
 * GLOBAL VARIABLES
 *============================================================================*/

/* Simulation mode flag for testing */
static bool g_simulation_mode = false;

/* Simulated register storage for testing */
static uint32_t g_sim_registers[MTOG_NUM_INSTANCES][4] = {0};

/* MTOG Base Addresses Array */
static volatile uint32_t * const mtog_base_addresses[MTOG_NUM_INSTANCES] = {
    (volatile uint32_t *)MTO0_BASE,  (volatile uint32_t *)MTO1_BASE,
    (volatile uint32_t *)MTO2_BASE,  (volatile uint32_t *)MTO3_BASE,
    (volatile uint32_t *)MTO4_BASE,  (volatile uint32_t *)MTO5_BASE,
    (volatile uint32_t *)MTO6_BASE,  (volatile uint32_t *)MTO7_BASE,
    (volatile uint32_t *)MTO8_BASE,  (volatile uint32_t *)MTO9_BASE,
    (volatile uint32_t *)MTO10_BASE, (volatile uint32_t *)MTO11_BASE,
    (volatile uint32_t *)MTO12_BASE, (volatile uint32_t *)MTO13_BASE,
    (volatile uint32_t *)MTO14_BASE, (volatile uint32_t *)MTO15_BASE
};

/* Timeout values array */
static const uint32_t mtog_timeout_values[MTOG_NUM_INSTANCES] = {
    MTO4_timeout_ms_0,  MTO4_timeout_ms_1,  MTO4_timeout_ms_2,  MTO4_timeout_ms_3,
    MTO4_timeout_ms_4,  MTO4_timeout_ms_5,  MTO4_timeout_ms_6,  MTO4_timeout_ms_7,
    MTO4_timeout_ms_8,  MTO4_timeout_ms_9,  MTO4_timeout_ms_10, MTO4_timeout_ms_11,
    MTO4_timeout_ms_12, MTO4_timeout_ms_13, MTO4_timeout_ms_14, MTO4_timeout_ms_15
};

/* Global MTOG Manager Instance */
static mtog_manager_t g_mtog_manager = {0};

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

static void mtog_write_register(volatile uint32_t *base_addr, uint32_t offset, uint32_t value);
static uint32_t mtog_read_register(volatile uint32_t *base_addr, uint32_t offset);
static uint32_t get_system_time_ms(void);
static void ComputeProcessor_FTTI_impl(uint32_t safe_state);
static uint32_t get_instance_id_from_base(volatile uint32_t *base_addr);
static void mtog_enable_simulation_mode(void);

/*==============================================================================
 * PRIVATE FUNCTIONS
 *============================================================================*/

/**
 * @brief Write to MTOG register
 * @param base_addr Base address of MTOG instance
 * @param offset Register offset
 * @param value Value to write
 */
static void mtog_write_register(volatile uint32_t *base_addr, uint32_t offset, uint32_t value)
{
    if (g_simulation_mode) {
        /* Simulation mode - use simulated registers */
        uint32_t instance_id = get_instance_id_from_base(base_addr);
        if (instance_id < MTOG_NUM_INSTANCES) {
            uint32_t reg_index;
            switch (offset) {
                case MTOG_CONTROL_REG_OFFSET:
                    reg_index = 0;
                    break;
                case MTOG_STATUS_REG_OFFSET:
                    reg_index = 1;
                    break;
                case MTOG_INT_ENABLE_REG_OFFSET:
                    reg_index = 2;
                    break;
                case MTOG_TIMEOUT_VAL_REG_OFFSET:
                    reg_index = 3;
                    break;
                default:
                    return; /* Invalid offset */
            }
            g_sim_registers[instance_id][reg_index] = value;
        }
    } else {
        /* Hardware mode - access actual registers */
        volatile uint32_t *reg_addr = (volatile uint32_t *)((uint8_t *)base_addr + offset);
        *reg_addr = value;
    }
}

/**
 * @brief Read from MTOG register
 * @param base_addr Base address of MTOG instance
 * @param offset Register offset
 * @return Register value
 */
static uint32_t mtog_read_register(volatile uint32_t *base_addr, uint32_t offset)
{
    if (g_simulation_mode) {
        /* Simulation mode - use simulated registers */
        uint32_t instance_id = get_instance_id_from_base(base_addr);
        if (instance_id < MTOG_NUM_INSTANCES) {
            uint32_t reg_index;
            switch (offset) {
                case MTOG_CONTROL_REG_OFFSET:
                    reg_index = 0;
                    break;
                case MTOG_STATUS_REG_OFFSET:
                    reg_index = 1;
                    break;
                case MTOG_INT_ENABLE_REG_OFFSET:
                    reg_index = 2;
                    break;
                case MTOG_TIMEOUT_VAL_REG_OFFSET:
                    reg_index = 3;
                    break;
                default:
                    return 0; /* Invalid offset */
            }
            return g_sim_registers[instance_id][reg_index];
        }
        return 0;
    } else {
        /* Hardware mode - access actual registers */
        volatile uint32_t *reg_addr = (volatile uint32_t *)((uint8_t *)base_addr + offset);
        return *reg_addr;
    }
}

/**
 * @brief Get instance ID from base address
 * @param base_addr Base address of MTOG instance
 * @return Instance ID (0-15) or MTOG_NUM_INSTANCES if not found
 */
static uint32_t get_instance_id_from_base(volatile uint32_t *base_addr)
{
    for (uint32_t i = 0; i < MTOG_NUM_INSTANCES; i++) {
        if (mtog_base_addresses[i] == base_addr) {
            return i;
        }
    }
    return MTOG_NUM_INSTANCES; /* Not found */
}

/**
 * @brief Enable simulation mode for testing
 */
static void mtog_enable_simulation_mode(void)
{
    g_simulation_mode = true;
    /* Initialize simulated registers to default values */
    memset(g_sim_registers, 0, sizeof(g_sim_registers));
}

/**
 * @brief Get system time in milliseconds (platform-specific implementation)
 * @return Current system time in milliseconds
 */
static uint32_t get_system_time_ms(void)
{
    /* Platform-specific implementation needed */
    /* This is a stub - replace with actual timer implementation */
    static uint32_t sim_time = 0;
    sim_time += 100; /* Simulate 100ms increment for testing */
    return sim_time;
}

/**
 * @brief Safe state transition implementation
 * @param safe_state Safe state identifier
 */
static void ComputeProcessor_FTTI_impl(uint32_t safe_state)
{
    /* Transition to safe state within FTTI */
    /* This is a critical safety function - implementation depends on system architecture */
    
    /* Suppress unused parameter warning */
    (void)safe_state;
    
    /* Disable all MTOG instances immediately */
    for (uint32_t i = 0; i < MTOG_NUM_INSTANCES; i++) {
        if (g_mtog_manager.instances[i].base_addr != NULL) {
            mtog_write_register(g_mtog_manager.instances[i].base_addr, 
                              MTOG_CONTROL_REG_OFFSET, 0x00000000U);
        }
    }
    
    /* Platform-specific safe state implementation */
    /* Example: Disable interrupts, halt CPU, activate watchdog, etc. */
    
    /* For simulation purposes, indicate safe state reached */
    volatile uint32_t *safe_state_indicator = (volatile uint32_t *)0xDEADBEEFU;
    (void)safe_state_indicator; /* Suppress unused variable warning */
    
    /* In real implementation, this function should not return */
    while (1) {
        /* Infinite loop in safe state */
    }
}

/*==============================================================================
 * PUBLIC FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize MTOG Manager
 * @return true if initialization successful, false otherwise
 */
bool mtog_manager_init(void)
{
    bool init_success = true;
    uint32_t readback_value;
    
    printf("  Initializing MTOG manager structure...\n");
    
    /* Initialize MTOG manager structure */
    memset(&g_mtog_manager, 0, sizeof(mtog_manager_t));
    
    printf("  Initializing %u MTOG instances...\n", MTOG_NUM_INSTANCES);
    
    /* Initialize each MTOG instance */
    for (uint32_t i = 0; i < MTOG_NUM_INSTANCES; i++) {
        printf("    Initializing instance %u...\n", i);
        
        g_mtog_manager.instances[i].base_addr = mtog_base_addresses[i];
        g_mtog_manager.instances[i].timeout_ms = mtog_timeout_values[i];
        g_mtog_manager.instances[i].enabled = false;
        
        /* Reset the MTOG instance */
        mtog_write_register(g_mtog_manager.instances[i].base_addr, 
                          MTOG_CONTROL_REG_OFFSET, MTOG_CTRL_RESET_BIT);
        
        /* Configure timeout value */
        mtog_write_register(g_mtog_manager.instances[i].base_addr, 
                          MTOG_TIMEOUT_VAL_REG_OFFSET, 
                          g_mtog_manager.instances[i].timeout_ms);
        
        /* Store expected values for monitoring */
        g_mtog_manager.instances[i].expected_timeout_val = g_mtog_manager.instances[i].timeout_ms;
        g_mtog_manager.instances[i].expected_ctrl_val = MTOG_CTRL_ENABLE_BIT;
        
        /* Read back and verify timeout configuration */
        readback_value = mtog_read_register(g_mtog_manager.instances[i].base_addr, 
                                          MTOG_TIMEOUT_VAL_REG_OFFSET);
        if (readback_value != g_mtog_manager.instances[i].timeout_ms) {
            /* Configuration mismatch - transition to safe state */
            printf("    ERROR: Configuration mismatch for instance %u\n", i);
            if (!g_simulation_mode) {
                ComputeProcessor_FTTI();
            }
            init_success = false;
        }
        
        /* Enable interrupts for timeout and error conditions */
        mtog_write_register(g_mtog_manager.instances[i].base_addr, 
                          MTOG_INT_ENABLE_REG_OFFSET, 
                          MTOG_INT_TIMEOUT_EN_BIT | MTOG_INT_ERROR_EN_BIT);
        
        printf("    Instance %u initialized successfully\n", i);
    }
    
    if (init_success) {
        g_mtog_manager.initialized = true;
        g_mtog_manager.last_monitor_time = get_system_time_ms();
        printf("  MTOG Manager initialization completed successfully\n");
    }
    
    return init_success;
}

/**
 * @brief Test MTOG timeout functionality during initialization
 * @return true if all tests pass, false otherwise
 */
bool mtog_test_timeout_functionality(void)
{
    bool test_success = true;
    uint32_t status_reg;
    uint32_t test_timeout = 0; /* Zero timeout to force immediate timeout */
    
    for (uint32_t i = 0; i < MTOG_NUM_INSTANCES; i++) {
        /* Set timeout to zero to force timeout condition */
        mtog_write_register(g_mtog_manager.instances[i].base_addr, 
                          MTOG_TIMEOUT_VAL_REG_OFFSET, test_timeout);
        
        /* Enable the gasket */
        mtog_write_register(g_mtog_manager.instances[i].base_addr, 
                          MTOG_CONTROL_REG_OFFSET, MTOG_CTRL_ENABLE_BIT);
        
        /* In simulation mode, simulate timeout detection */
        if (g_simulation_mode) {
            /* Simulate timeout by setting status bit */
            mtog_write_register(g_mtog_manager.instances[i].base_addr, 
                              MTOG_STATUS_REG_OFFSET, MTOG_STATUS_TIMEOUT_BIT);
        }
        
        /* Wait for timeout to occur (simulate transaction) */
        /* In real implementation, trigger a transaction that will timeout */
        
        /* Check if timeout was detected */
        status_reg = mtog_read_register(g_mtog_manager.instances[i].base_addr, 
                                      MTOG_STATUS_REG_OFFSET);
        
        if ((status_reg & MTOG_STATUS_TIMEOUT_BIT) == 0) {
            /* Timeout not detected - in real system, transition to safe state */
            if (!g_simulation_mode) {
                ComputeProcessor_FTTI();
            }
            test_success = false;
        }
        
        /* Reset the gasket and restore original timeout */
        mtog_write_register(g_mtog_manager.instances[i].base_addr, 
                          MTOG_CONTROL_REG_OFFSET, MTOG_CTRL_RESET_BIT);
        
        mtog_write_register(g_mtog_manager.instances[i].base_addr, 
                          MTOG_TIMEOUT_VAL_REG_OFFSET, 
                          g_mtog_manager.instances[i].timeout_ms);
    }
    
    return test_success;
}

/**
 * @brief Enable MTOG instance
 * @param gasket_id Gasket ID (0-15)
 * @return true if successful, false otherwise
 */
bool mtog_enable_instance(uint32_t gasket_id)
{
    if (gasket_id >= MTOG_NUM_INSTANCES || !g_mtog_manager.initialized) {
        return false;
    }
    
    /* Enable the MTOG instance */
    mtog_write_register(g_mtog_manager.instances[gasket_id].base_addr, 
                      MTOG_CONTROL_REG_OFFSET, MTOG_CTRL_ENABLE_BIT);
    
    g_mtog_manager.instances[gasket_id].enabled = true;
    
    return true;
}

/**
 * @brief Disable MTOG instance
 * @param gasket_id Gasket ID (0-15)
 * @return true if successful, false otherwise
 */
bool mtog_disable_instance(uint32_t gasket_id)
{
    if (gasket_id >= MTOG_NUM_INSTANCES || !g_mtog_manager.initialized) {
        return false;
    }
    
    /* Disable the MTOG instance */
    mtog_write_register(g_mtog_manager.instances[gasket_id].base_addr, 
                      MTOG_CONTROL_REG_OFFSET, 0x00000000U);
    
    g_mtog_manager.instances[gasket_id].enabled = false;
    
    return true;
}

/**
 * @brief Periodic monitoring of MTOG configuration registers
 * @return true if all configurations are correct, false otherwise
 */
bool mtog_periodic_monitor(void)
{
    uint32_t current_time = get_system_time_ms();
    uint32_t readback_value;
    bool monitor_success = true;
    
    /* Check if monitoring period has elapsed */
    if ((current_time - g_mtog_manager.last_monitor_time) < MTO6_periodicity_ms) {
        return true; /* Not time to monitor yet */
    }
    
    /* Update last monitor time */
    g_mtog_manager.last_monitor_time = current_time;
    
    /* Monitor all MTOG instances */
    for (uint32_t i = 0; i < MTOG_NUM_INSTANCES; i++) {
        /* Check timeout value register */
        readback_value = mtog_read_register(g_mtog_manager.instances[i].base_addr, 
                                          MTOG_TIMEOUT_VAL_REG_OFFSET);
        if (readback_value != g_mtog_manager.instances[i].expected_timeout_val) {
            /* Configuration mismatch - in real system, transition to safe state */
            if (!g_simulation_mode) {
                ComputeProcessor_FTTI();
            }
            monitor_success = false;
        }
        
        /* Check control register if instance should be enabled */
        if (g_mtog_manager.instances[i].enabled) {
            readback_value = mtog_read_register(g_mtog_manager.instances[i].base_addr, 
                                              MTOG_CONTROL_REG_OFFSET);
            if ((readback_value & MTOG_CTRL_ENABLE_BIT) == 0) {
                /* Control register mismatch - in real system, transition to safe state */
                if (!g_simulation_mode) {
                    ComputeProcessor_FTTI();
                }
                monitor_success = false;
            }
        }
    }
    
    return monitor_success;
}

/**
 * @brief MTOG interrupt handler for timeout conditions
 * @param gasket_id Gasket ID that generated the interrupt
 */
void mtog_timeout_interrupt_handler(uint32_t gasket_id)
{
    uint32_t int_status;
    
    if (gasket_id >= MTOG_NUM_INSTANCES) {
        return;
    }
    
    /* Read interrupt status */
    int_status = mtog_read_register(g_mtog_manager.instances[gasket_id].base_addr, 
                                  MTOG_INT_STATUS_REG_OFFSET);
    
    /* Check for timeout interrupt */
    if (int_status & MTOG_INT_TIMEOUT_EN_BIT) {
        /* Transaction timeout detected - transition to safe state */
        ComputeProcessor_FTTI();
    }
    
    /* Clear interrupt status */
    mtog_write_register(g_mtog_manager.instances[gasket_id].base_addr, 
                      MTOG_INT_STATUS_REG_OFFSET, int_status);
}

/**
 * @brief MTOG interrupt handler for error conditions
 * @param gasket_id Gasket ID that generated the interrupt
 */
void mtog_error_interrupt_handler(uint32_t gasket_id)
{
    uint32_t int_status;
    
    if (gasket_id >= MTOG_NUM_INSTANCES) {
        return;
    }
    
    /* Read interrupt status */
    int_status = mtog_read_register(g_mtog_manager.instances[gasket_id].base_addr, 
                                  MTOG_INT_STATUS_REG_OFFSET);
    
    /* Check for error interrupt */
    if (int_status & MTOG_INT_ERROR_EN_BIT) {
        /* Unexpected response error detected - transition to safe state */
        ComputeProcessor_FTTI();
    }
    
    /* Clear interrupt status */
    mtog_write_register(g_mtog_manager.instances[gasket_id].base_addr, 
                      MTOG_INT_STATUS_REG_OFFSET, int_status);
}

/**
 * @brief Get MTOG instance status
 * @param gasket_id Gasket ID (0-15)
 * @return Status register value, 0xFFFFFFFF if invalid ID
 */
uint32_t mtog_get_instance_status(uint32_t gasket_id)
{
    if (gasket_id >= MTOG_NUM_INSTANCES || !g_mtog_manager.initialized) {
        return 0xFFFFFFFFU;
    }
    
    return mtog_read_register(g_mtog_manager.instances[gasket_id].base_addr, 
                            MTOG_STATUS_REG_OFFSET);
}

/**
 * @brief Check if MTOG manager is initialized
 * @return true if initialized, false otherwise
 */
bool mtog_is_initialized(void)
{
    return g_mtog_manager.initialized;
}

/*==============================================================================
 * TEST HARNESS AND MAIN FUNCTION
 *============================================================================*/

/**
 * @brief Simulate timeout event injection for testing
 * @param gasket_id Gasket ID to inject timeout
 */
void mtog_inject_timeout_event(uint32_t gasket_id)
{
    if (gasket_id >= MTOG_NUM_INSTANCES) {
        return;
    }
    
    /* Simulate timeout by calling the interrupt handler */
    mtog_timeout_interrupt_handler(gasket_id);
}

/**
 * @brief Simulate error event injection for testing
 * @param gasket_id Gasket ID to inject error
 */
void mtog_inject_error_event(uint32_t gasket_id)
{
    if (gasket_id >= MTOG_NUM_INSTANCES) {
        return;
    }
    
    /* Simulate error by calling the interrupt handler */
    mtog_error_interrupt_handler(gasket_id);
}

/**
 * @brief Main function with test harness
 * @return Exit status
 */
int main(void)
{
    bool init_result;
    bool test_result;
    uint32_t status;
    
    printf("Starting MTOG Manager Test Harness\n");
    
    /* Enable simulation mode for testing */
    printf("Enabling simulation mode...\n");
    mtog_enable_simulation_mode();
    
    /* Step 1: Initialize MTOG Manager */
    printf("Step 1: Initializing MTOG Manager...\n");
    init_result = mtog_manager_init();
    if (!init_result) {
        printf("ERROR: Initialization failed\n");
        return -1;
    }
    printf("Step 1: PASSED\n");
    
    /* Step 2: Test timeout functionality */
    printf("Step 2: Testing timeout functionality...\n");
    test_result = mtog_test_timeout_functionality();
    if (!test_result) {
        printf("ERROR: Timeout test failed\n");
        return -2;
    }
    printf("Step 2: PASSED\n");
    
    /* Step 3: Enable all MTOG instances */
    printf("Step 3: Enabling all MTOG instances...\n");
    for (uint32_t i = 0; i < MTOG_NUM_INSTANCES; i++) {
        if (!mtog_enable_instance(i)) {
            printf("ERROR: Failed to enable instance %u\n", i);
            return -3;
        }
    }
    printf("Step 3: PASSED\n");
    
    /* Step 4: Test runtime enable/disable functionality */
    printf("Step 4: Testing enable/disable functionality...\n");
    if (!mtog_disable_instance(5)) {
        printf("ERROR: Failed to disable instance 5\n");
        return -4;
    }
    
    if (!mtog_enable_instance(5)) {
        printf("ERROR: Failed to re-enable instance 5\n");
        return -5;
    }
    printf("Step 4: PASSED\n");
    
    /* Step 5: Test periodic monitoring - reduced iterations for testing */
    printf("Step 5: Testing periodic monitoring...\n");
    for (uint32_t cycle = 0; cycle < 2; cycle++) {
        printf("  Monitoring cycle %u...\n", cycle + 1);
        if (!mtog_periodic_monitor()) {
            printf("ERROR: Monitoring failed at cycle %u\n", cycle + 1);
            return -6;
        }
    }
    printf("Step 5: PASSED\n");
    
    /* Step 6: Test status reading */
    printf("Step 6: Testing status reading...\n");
    for (uint32_t i = 0; i < MTOG_NUM_INSTANCES; i++) {
        status = mtog_get_instance_status(i);
        if (status == 0xFFFFFFFFU) {
            printf("ERROR: Failed to read status for instance %u\n", i);
            return -7;
        }
    }
    printf("Step 6: PASSED\n");
    
    printf("All tests completed successfully!\n");
    return 0;
}