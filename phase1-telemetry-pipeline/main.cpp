#include "stm32f4xx.h"
#include <vector>
#include "I2CDriver.hpp"
#include "SpiMaster.hpp"

/*
    Bare Metal Code

*/

namespace{
    I2CDriver i2c;
    uint8_t sensor_address = 0x68;
    uint8_t sensor_register_address = 0x3B;
    uint8_t sensor_register_address_en_drdy = 0x38;
    uint8_t sensor_register_address_wake_up = 0x6B;
    uint8_t sensor_register_address_int_cfg = 0x37;
   // std::vector<uint8_t> data_buffer(100);
    size_t head = 0;
    size_t tail = 0;
    volatile bool drdy_triggered = false;
    volatile bool finished_reading = false;
    volatile bool dma_error = false;

    uint8_t data_buffer[14];
    volatile bool dma_busy = false;
    volatile bool sample_ready = false;
}

void enable_dma()
{
    DMA1_Stream0->CR &= ~DMA_SxCR_EN;

    while (DMA1_Stream0->CR & DMA_SxCR_EN) {}

        //INterrupt configurations
        
    //clear all interrupt flags
    DMA1->LIFCR = DMA_LIFCR_CTCIF0 |
                  DMA_LIFCR_CTEIF0 |
                  DMA_LIFCR_CDMEIF0 |
                  DMA_LIFCR_CFEIF0 |
                  DMA_LIFCR_CHTIF0;

    DMA1_Stream0->NDTR = 14;
    DMA1_Stream0->CR |= DMA_SxCR_EN;
}




void init_dma(){
    

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; //Enabble dma clock


    DMA1_Stream0->CR &= ~DMA_SxCR_EN;

    while (DMA1_Stream0->CR & DMA_SxCR_EN) {}


    //(uint32_t) &ADC1->DR
    DMA1_Stream0->PAR =  i2c.getDataAddress();

    //DMA1_Stream0->M0AR =  (uint32_t)data_buffer.data();
    DMA1_Stream0->M0AR =  reinterpret_cast<uint32_t>(data_buffer);


    //Reset stream 0 configuration
    DMA1_Stream0->CR =0;

    DMA1_Stream0->CR |= (1U << DMA_SxCR_CHSEL_Pos);  // Channel 0
    DMA1_Stream0->CR |= DMA_SxCR_MINC;               // Memory increment
  //  DMA1_Stream0->CR |= DMA_SxCR_CIRC;               // Circular mode
   // DMA1_Stream0->CR |= DMA_SxCR_PSIZE_0;            // Peripheral size 16-bit
  //  DMA1_Stream0->CR |= DMA_SxCR_MSIZE_0;            // Memory size 16-bit
    DMA1_Stream0->CR |= DMA_SxCR_PL_1;               // High priority
    //DMA1_Stream0->CR |= DMA_SxCR_HTIE;               // Half-transfer interrupt
    DMA1_Stream0->CR |= DMA_SxCR_TCIE;               // Transfer-complete interrupt
     DMA1_Stream0->CR |= DMA_SxCR_TEIE;                // Transfer error

    /*
    
    This gives you:
- Peripheral-to-memory transfer.
- 8-bit peripheral and memory transfers.
- Normal mode, not circular.
- One interrupt after 14 bytes.
    */



    //Enable interrup
    NVIC_EnableIRQ(DMA1_Stream0_IRQn);

}




//setup mcu for sensor external interrupt
void DRDY_EXTI_Init(){
    // 1. Clock GPIOA & SYSCFG
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Pin A0 as input, pull-down (or floating if MPU push-pull)
    GPIOA->MODER &= ~(3U << (0 * 2));
    GPIOA->PUPDR &= ~(3U << (0 * 2));
    GPIOA->PUPDR |=  (2U << (0 * 2)); // Pull-down

    // Route PA0 to EXTI0
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;

    // Trigger on Rising edge only
    EXTI->IMR  |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->FTSR &= ~EXTI_FTSR_TR0;

    // Clear stale flags
    EXTI->PR = EXTI_PR_PR0;
    NVIC_ClearPendingIRQ(EXTI0_IRQn);

    // Set NVIC Priorities: DMA must have HIGHER priority (lower number) than EXTI!
    NVIC_SetPriority(DMA1_Stream0_IRQn, 1);
    NVIC_SetPriority(EXTI0_IRQn, 2);

    // Enable EXTI0 in NVIC
    NVIC_EnableIRQ(EXTI0_IRQn);

}


/*void sensor_system_init(){

    i2c.init();
    NVIC_ClearPendingIRQ(EXTI0_IRQn);

    i2c.writeTo(sensor_address, sensor_register_address_wake_up, 0x80);

    i2c.writeTo(sensor_address, sensor_register_address_wake_up, 0x01);
    i2c.writeTo(sensor_address,sensor_register_address_int_cfg, 0x00);
    i2c.writeTo(sensor_address, sensor_register_address_en_drdy, 0x01);

     DRDY_EXTI_Init();

}*/

void sensor_system_init() {
    // 1. Initialize I2C hardware and clear stuck lines
    i2c.init();

    // 2. Hard delay before touching the sensor
    for (volatile int d = 0; d < 200'000; d++) __NOP();

    // 3. Reset MPU6050
    i2c.writeTo(sensor_address, sensor_register_address_wake_up, 0x80);
    
    // CRITICAL: MPU-6050 internal reset takes up to 100ms. 
    // At 16 MHz, 500,000 NOPs is ~100-150ms.
    for (volatile int d = 0; d < 500'000; d++) __NOP();

    // 4. Wake up & select X-axis gyro clock reference (0x01)
    i2c.writeTo(sensor_address, sensor_register_address_wake_up, 0x01);
    for (volatile int d = 0; d < 50'000; d++) __NOP();

    // 5. INT Config: Active-high, push-pull, 50us pulse (0x00)
    // Optional: Use 0x20 to enable Read-Clear latching if pulses are too narrow
    i2c.writeTo(sensor_address, sensor_register_address_int_cfg, 0x00);

    // 6. Clear any pending interrupt status by reading INT_STATUS (0x3A)
    uint8_t dummy_status = 0;
    i2c.readFrom(sensor_address, 0x3A, dummy_status);

    // 7. Enable Data Ready interrupt (0x01)
    i2c.writeTo(sensor_address, sensor_register_address_en_drdy, 0x01);

    // 8. ONLY NOW enable EXTI on the MCU side
    DRDY_EXTI_Init();
}


void init_gppio_a(){
    //We are using Port A
    //Enable GP_IO port Clock, 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    //COnfigure pin as analog mode
    GPIOA->MODER &= ~(3U << (0 * 2));
    GPIOA->MODER |= (3U << (0 * 2));

    GPIOA->PUPDR &= ~(3U << (0*2));

}




extern "C" {
/*
  //Sensor Interrupt handler (i2c)
    void EXTI0_IRQHandler(void){
                 
        if(EXTI->PR & EXTI_PR_PR0){
            EXTI->PR = EXTI_PR_PR0;

            if(dma_busy)
                return;
                dma_busy = true;

            I2CDriver::Status status;
            status = i2c.prepareToRead(sensor_address,sensor_register_address);

            if (status != I2CDriver::Status::Ok) {
                DMA1_Stream0->CR &= ~DMA_SxCR_EN;

                dma_busy = false;
                return;
            }

            enable_dma();
            //setupDMARead from i2c
            i2c.setupDMARead(sensor_address,sensor_register_address);
        }
    }

    //DMA Interrupt handler
    void DMA1_Stream0_IRQHandler(void) {

        if(DMA1->LISR & DMA_LISR_TCIF0){
            DMA1->LIFCR = DMA_LIFCR_CTCIF0;
            
            //Stop DMA read from i2c
            i2c.finishDMAReading();
            // Mark sensor_data ready for processing
            sample_ready = true;
            dma_busy = false;
        }

        if(DMA1->LISR & DMA_LISR_TEIF0){
            DMA1->LIFCR = DMA_LIFCR_CTEIF0;

                    // Abort DMA/I2C and recover from error
        // Do not mark sensor_data as valid
                    dma_busy = false;
        }

    }

*/

    void EXTI0_IRQHandler(void) {
        if (EXTI->PR & EXTI_PR_PR0) {
            EXTI->PR = EXTI_PR_PR0; // Clear EXTI pending bit

            // Just set a trigger flag if not currently busy
            if (!dma_busy) {
                drdy_triggered = true;
            }
        }
    }

    void DMA1_Stream0_IRQHandler(void) {
        if (DMA1->LISR & DMA_LISR_TCIF0) {
            DMA1->LIFCR = DMA_LIFCR_CTCIF0;
            sample_ready = true;
            dma_busy = false;
        }

        if (DMA1->LISR & DMA_LISR_TEIF0) {
            DMA1->LIFCR = DMA_LIFCR_CTEIF0;
            dma_error = true;
            dma_busy = false;
        }
    }
  
}




void init_adc(){

    //Enable ADC Clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    ADC->CCR &= ~(3U << 16);
    ADC->CCR |=  (1U << 16);

    //Choose adc channel
    ADC1->SQR1 &= ~ADC_SQR1_L; //0000 tell ac to do just one channel conversion

    
    ADC1->SQR3 &= ~ADC_SQR3_SQ1; //clear what first coversion was, sq1
    ADC1->SQR3 |= (0U << ADC_SQR3_SQ1_Pos); //Set first conversion (sq1) to channel 0

    // Configure resolution and sampling time
    ADC1->CR1 &= ~ADC_CR1_RES;

    ADC1->SMPR2 &= ~ADC_SMPR2_SMP0;
    ADC1->SMPR2 |= (3U << ADC_SMPR2_SMP0_Pos); //011: 56 cycles


}


void adc_read_via_dma(){


    ADC1->CR2 |= ADC_CR2_CONT; //setup continuos mode

    ADC1->CR2 |=ADC_CR2_DMA; //enable dma request link

    ADC1->CR2 |= ADC_CR2_DDS; //keep request link alive

}

void adc_start(){
    //Enable adc
    ADC1->CR2 |= ADC_CR2_ADON;

    // A simple assembly NOP loop or your HAL_Delay / DWT delay works here
    for (volatile int i = 0; i < 1000; i++) {
        __NOP(); 
    }

    ADC1->CR2 |= ADC_CR2_SWSTART;
}


void setUpToggle(){
    //Toggle on pin 5


    /*
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~ GPIO_MODER_MODE5 ;
    GPIOA->MODER |= 1U << GPIO_MODER_MODE5_Pos;

    GPIOA->OTYPER &= ~GPIO_OTYPER_OT5; 

    GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR5;
    */

        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // Safely clear ONLY pin 5 bits (bits 10 and 11 of MODER)
    GPIOA->MODER &= ~(3U << (5 * 2));
    // Set ONLY pin 5 to General Purpose Output (01)
    GPIOA->MODER |= (1U << (5 * 2));

    // Safely clear pin 5 output type (Push-Pull)
    GPIOA->OTYPER &= ~(1U << 5); 

    // Set pin 5 to Low Speed to completely eliminate electrical noise on the board
    GPIOA->OSPEEDR &= ~(3U << (5 * 2));

}



void systick_init(uint32_t system_clock_hz) {
    // Configure SysTick to trigger a reload every 1ms
    SysTick->LOAD = (system_clock_hz / 1000) - 1; 
    SysTick->VAL = 0; // Clear current value
    // Enable SysTick, use processor clock, no interrupt (we will poll it)
    SysTick->CTRL = (1U << 2) | (1U << 0); 
}

void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        // Wait until the COUNTFLAG bit becomes 1
        while ((SysTick->CTRL & (1U << 16)) == 0);
    }
}

volatile uint32_t reset_reason = 0;
int main(){

    setUpToggle();
    init_dma();
    sensor_system_init();
              


    //SpiMaster spi(SPI1);
   // spi.init(SpiMaster::Mode::MODE1, SpiMaster::Prescaler::Div64);

    
    while (true)
    {

        if (drdy_triggered && !dma_busy) {
            drdy_triggered = false;
            dma_busy = true;

            // 1. Prime DMA Stream
            enable_dma();

            // 2. Start transaction
            I2CDriver::Status st = i2c.startDmaRead(sensor_address, sensor_register_address);
            if (st != I2CDriver::Status::Ok) {
                // Abort DMA if start failed
                DMA1_Stream0->CR &= ~DMA_SxCR_EN;
                dma_busy = false;
            }
        }


        //turn on toggle on user led

         //// delay 
         // GPIOA->ODR ^= (1U << 5);

         if(dma_error){
            dma_error = false;
              i2c.finishDMAReading();
         }
            if (sample_ready)
            {

                GPIOA->ODR ^= (1U << 5);

                        i2c.finishDMAReading();
                
                sample_ready = false;
       //      GPIOA->ODR ^= (1U << 5);
            //new data 
          //  spi.write(data_buffer, head-tail);
        }
    }

}