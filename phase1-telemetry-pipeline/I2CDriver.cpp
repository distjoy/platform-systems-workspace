#include "stm32f4xx.h"
#include "I2CDriver.hpp"


// Pin configuration: PB8 (SCL), PB9 (SDA) - Alternate Function 4
    static constexpr uint8_t PIN_SCL = 8;
    static constexpr uint8_t PIN_SDA = 9;

void I2C_Bus_Clear(void) {
    // 1. Configure SCL and SDA as standard GPIO outputs (Open-Drain, Pull-Up)
    // 2. If SDA is Low, toggle SCL up to 9 times:
    for (int i = 0; i < 9; i++) {
        GPIOB->BSRR = GPIO_BSRR_BR8; // SCL Low
        for (volatile int d = 0; d < 100; d++);
        GPIOB->BSRR = GPIO_BSRR_BS8; // SCL High
        for (volatile int d = 0; d < 100; d++);
        
        // If slave released SDA, generate a STOP condition and exit
        if (GPIOB->IDR & GPIO_IDR_ID9) {
            break;
        }
    }
    // Reconfigure PB8/PB9 back to Alternate Function AF04 for I2C1
}

static inline void delay_cycles(uint32_t count) noexcept {
        while (count--) {
            __NOP();
        }
    }

/**
     * @brief Forces SCL/SDA into GPIO Open-Drain to tick clock cycles if SDA is stuck LOW.
     */
    static void recover_bus() noexcept {
        // Set PB8 and PB9 as General Purpose Output, Open-Drain, Pull-Up
        GPIOB->MODER &= ~((0x3 << (PIN_SCL * 2)) | (0x3 << (PIN_SDA * 2)));
        GPIOB->MODER |=  ((0x1 << (PIN_SCL * 2)) | (0x1 << (PIN_SDA * 2))); // Output mode (01)

        GPIOB->OTYPER |= (1 << PIN_SCL) | (1 << PIN_SDA);                   // Open-drain (1)
        GPIOB->PUPDR  &= ~((0x3 << (PIN_SCL * 2)) | (0x3 << (PIN_SDA * 2)));
        GPIOB->PUPDR  |=  ((0x1 << (PIN_SCL * 2)) | (0x1 << (PIN_SDA * 2))); // Internal pull-up (01)

        // Release SCL and SDA high
        GPIOB->BSRR = (1 << PIN_SCL) | (1 << PIN_SDA);
        delay_cycles(160);

        // If SDA is pulled low by the slave, clock SCL up to 9 times
        for (int i = 0; i < 9; ++i) {
            if ((GPIOB->IDR & (1 << PIN_SDA)) != 0) {
                break; // Target released SDA
            }
            // Pull SCL LOW
            GPIOB->BSRR = (1 << (PIN_SCL + 16));
            delay_cycles(160);
            // Float SCL HIGH
            GPIOB->BSRR = (1 << PIN_SCL);
            delay_cycles(160);
        }

        // Generate manual STOP condition (SDA: Low -> High while SCL is High)
        GPIOB->BSRR = (1 << (PIN_SDA + 16)); // SDA LOW
        delay_cycles(160);
        GPIOB->BSRR = (1 << PIN_SCL);        // SCL HIGH
        delay_cycles(160);
        GPIOB->BSRR = (1 << PIN_SDA);        // SDA HIGH
        delay_cycles(160);
}
void I2CDriver::init()
{

    //I2C_Bus_Clear();
    // 1. Enable GPIO and I2C Clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

recover_bus();
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    __asm("nop"); // Small delay to allow clock to stabilize

    // 2. Atomic Peripheral Reset via RCC (Clears any stubborn state machine hangs)
     RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    for (volatile int i = 0; i < 100; i++)
        ; // Short delay
         RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    // 3. Configure Pins (PB8 = SCL, PB9 = SDA)
    // Clear and set Alternate Function Mode (0x2)
    GPIOB->MODER &= ~((3U << GPIO_MODER_MODER8_Pos) | (3U << GPIO_MODER_MODER9_Pos));
    GPIOB->MODER |= ((2U << GPIO_MODER_MODER8_Pos) | (2U << GPIO_MODER_MODER9_Pos));

    // Open drain outputs
    GPIOB->OTYPER |= ((1U << GPIO_OTYPER_OT8_Pos) | (1U << GPIO_OTYPER_OT9_Pos));

    // High speed outputs
    GPIOB->OSPEEDR |= ((3U << GPIO_OSPEEDR_OSPEED8_Pos) | (3U << GPIO_OSPEEDR_OSPEED9_Pos));

    // Enable Internal Pull-up resistors (Essential if external ones are weak or missing)
    GPIOB->PUPDR &= ~((3U << GPIO_PUPDR_PUPD8_Pos) | (3U << GPIO_PUPDR_PUPD9_Pos));
    GPIOB->PUPDR |= ((1U << GPIO_PUPDR_PUPD8_Pos) | (1U << GPIO_PUPDR_PUPD9_Pos));

    // **FIXED**: Clear then Set Alternate Function 4 (I2C1) for pins 8 and 9
    GPIOB->AFR[1] &= ~((0xFU << GPIO_AFRH_AFSEL8_Pos) | (0xFU << GPIO_AFRH_AFSEL9_Pos));
    GPIOB->AFR[1] |= ((4U << GPIO_AFRH_AFSEL8_Pos) | (4U << GPIO_AFRH_AFSEL9_Pos));

    // 4. Configure I2C Parameters
    I2C1->CR1 &= ~I2C_CR1_PE; // Disable during config

    I2C1->CR2 = 16U; // Set PCLK1 frequency (16 MHz)
    I2C1->CCR = 80U; // Standard mode 100kHz setup
    I2C1->TRISE = 17U;

    I2C1->CR1 |= I2C_CR1_PE; // Enable peripheral
}

I2CDriver::Status I2CDriver::writeTo(uint8_t device,
                                     uint8_t reg,
                                     uint8_t value)
{

    if (!waitBusIdle())
        return Status::Busy;

    auto status = start();
    if (status != Status::Ok)
        return handleFailure(status);

    status = sendAddress(device, false, true);
    if (status != Status::Ok)
        return handleFailure(status);

    status = writeByte(reg);
    if (status != Status::Ok)
        return handleFailure(status);

    status = writeByte(value);
    if (status != Status::Ok)
        return handleFailure(status);

    status = waitFor(I2C_SR1_BTF);
    if (status != Status::Ok)
        return handleFailure(status);

    stop();
    return Status::Ok;
}

uint32_t I2CDriver::getDataAddress()
{
    return (uint32_t)&(I2C1->DR);
}

I2CDriver::Status I2CDriver::prepareToRead(uint8_t device, uint8_t store)
{
    Status status = Status::Ok;
    status = start();
    if (status != Status::Ok)
    {
        handleFailure(status);
        return status;
    }

    // send address
    status = sendAddress(device, false, true);
    if (status != Status::Ok)
        return handleFailure(status);

    // Write register address
    status = writeByte(store);
    if (status != Status::Ok)
        return handleFailure(status);

    // Restart coms

    status = start();
    if (status != Status::Ok)
    {
        handleFailure(status);
        return status;
    }

    // send address now for reading
    status = sendAddress(device, true, false);
    if (status != Status::Ok)
        return handleFailure(status);
    return Status::Ok;
}

I2CDriver::Status I2CDriver::setupDMARead(uint8_t device, uint8_t store)
{
    // Tell I2C that DMA handles the final byte
    I2C1->CR1 |= I2C_CR1_ACK;
    I2C1->CR2 |= I2C_CR2_LAST;

    // Enable I2C DMA requests
    I2C1->CR2 |= I2C_CR2_DMAEN;

    clearAddressFlag();
    return Status::Ok;
}

I2CDriver::Status I2CDriver::startDmaRead(uint8_t device, uint8_t reg)
{
    // 1. Wait for bus to be free
    if (!waitBusIdle()) return Status::Busy;

    // 2. Write register pointer
    auto status = start();
    if (status != Status::Ok) return handleFailure(status);

    status = sendAddress(device, false, true); // Write mode, clear ADDR
    if (status != Status::Ok) return handleFailure(status);

    status = writeByte(reg);
    if (status != Status::Ok) return handleFailure(status);

    // 3. Repeated Start for Read
    status = start();
    if (status != Status::Ok) return handleFailure(status);

    // 4. Configure I2C peripheral for DMA reception
    I2C1->CR1 |= I2C_CR1_ACK;
    I2C1->CR2 |= (I2C_CR2_DMAEN | I2C_CR2_LAST);

    // 5. Send read address - DO NOT clear ADDR yet
    status = sendAddress(device, true, false);
    if (status != Status::Ok) return handleFailure(status);

    // 6. Clear ADDR flag to release SCL and let DMA pull all 14 bytes
    clearAddressFlag();

    return Status::Ok;
}

/*

*/
void I2CDriver::finishDMAReading()
{

    /*
    */
    stop();
    I2C1->CR2 &= ~(I2C_CR2_DMAEN | I2C_CR2_LAST);
    I2C1->CR1 &= ~I2C_CR1_ACK;
}



I2CDriver::Status I2CDriver::readFrom(uint8_t device,
                                      uint8_t store, uint8_t &byteData)
{
    Status status;

    status = prepareToRead(device, store);

    if (status != Status::Ok)
    {
        handleFailure(status);
        return status;
    }
    // prepare to read
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();

    I2C1->CR1 &= ~I2C_CR1_ACK;
    clearAddressFlag();
    stop();

    __set_PRIMASK(primask);

    // actually read the data
    status = waitFor(I2C_SR1_RXNE);
    if (status != Status::Ok)
        return handleFailure(status);

    byteData = static_cast<uint8_t>(I2C1->DR);

    I2C1->CR1 |= I2C_CR1_ACK;

    return Status::Ok;
}

I2CDriver::Status I2CDriver::waitFor(uint32_t flag)
{
    uint32_t timeout = TIMEOUT;

    while (timeout > 0U)
    {
        uint32_t sr1 = I2C1->SR1;
        if (sr1 & flag)
            return Status::Ok;

        if (sr1 & I2C_SR1_AF)
            return Status::Nack;

        if (sr1 & I2C_SR1_BERR)
            return Status::BusError;

        if (sr1 & I2C_SR1_ARLO)
            return Status::ArbitrationLost;

        if (sr1 & I2C_SR1_OVR)
            return Status::Overrun;

        if (--timeout == 0U)
            return Status::Timeout;
    }
    return Status::Ok;
}

bool I2CDriver::waitBusIdle()
{
    uint32_t timeout = TIMEOUT;

    while (I2C1->SR2 & I2C_SR2_BUSY)
    {
        if (--timeout == 0U)
            return false;
    }
    return true;
}

void I2CDriver::clearAddressFlag()
{
    volatile uint32_t dummy = I2C1->SR1;
    dummy = I2C1->SR2;
    (void)dummy;
}

I2CDriver::Status I2CDriver::sendAddress(uint8_t addr, bool read,
                                         bool clearAddr)
{
    I2C1->DR = (addr << 1) | (read ? 1U : 0U);

    auto status = waitFor(I2C_SR1_ADDR);
    if (status != Status::Ok)
        return status;

    if (clearAddr)
        clearAddressFlag();
    return Status::Ok;
}

I2CDriver::Status I2CDriver::start()
{
    I2C1->CR1 |= I2C_CR1_START;
    return waitFor(I2C_SR1_SB);
}

void I2CDriver::stop()
{

    I2C1->CR1 |= I2C_CR1_STOP;
}

I2CDriver::Status I2CDriver::writeByte(uint8_t &byte)
{
    Status status = waitFor(I2C_SR1_TXE);
    if (Status::Ok != status)
        return status;

    I2C1->DR = byte;
    return waitFor(I2C_SR1_BTF);
}

I2CDriver::Status I2CDriver::handleFailure(Status failure)
{
    if (I2C1->SR2 & I2C_SR2_MSL)
    {
        stop();
    }

    // Clear all error flags
    I2C1->SR1 = ~(
        I2C_SR1_AF |
        I2C_SR1_ARLO |
        I2C_SR1_BERR |
        I2C_SR1_OVR);

    I2C1->CR1 |= I2C_CR1_ACK;

    return failure;
}

/** start
select device
check for acknowledge
write internal register
check for ack
select device for reading
check for ack
read **/
