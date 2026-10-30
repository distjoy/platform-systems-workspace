#include "stm32f4xx.h"
#include <vector>
#include "I2CDriver.hpp"
#include "SpiMaster.hpp"

/*
    Bare Metal Code

*/

namespace{
    I2CDriver i2c;
    uint8_t sensor_address;
    uint8_t sensor_register_address;
    std::vector<uint8_t> data_buffer(100);
    size_t head = 0;
    size_t tail = 0;
}




void init_dma(){
    

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN; //Enabble dma clock

    DMA2_Stream0->CR &= ~DMA_SxCR_EN; //disable dma 2 stream 0

    while(DMA2_Stream0->CR & DMA_SxCR_EN){} //DO NOTHING while stream 0 is active


    //(uint32_t) &ADC1->DR
    DMA2_Stream0->PAR =  i2c.getDataAddress();

    DMA2_Stream0->M0AR =  (uint32_t)data_buffer.data();

    DMA2_Stream0->NDTR = data_buffer.size();


    //Reset stream 0 configuration
    DMA2_Stream0->CR =0;

    DMA2_Stream0->CR |= (0U << DMA_SxCR_CHSEL_Pos);  // Channel 0
    DMA2_Stream0->CR |= DMA_SxCR_MINC;               // Memory increment
    DMA2_Stream0->CR |= DMA_SxCR_CIRC;               // Circular mode
    DMA2_Stream0->CR |= DMA_SxCR_PSIZE_0;            // Peripheral size 16-bit
    DMA2_Stream0->CR |= DMA_SxCR_MSIZE_0;            // Memory size 16-bit
    DMA2_Stream0->CR |= DMA_SxCR_PL_1;               // High priority
    DMA2_Stream0->CR |= DMA_SxCR_HTIE;               // Half-transfer interrupt
    DMA2_Stream0->CR |= DMA_SxCR_TCIE;               // Transfer-complete interrupt


    //INterrupt configurations
        
    //clear all interrupt flags

    DMA2->LIFCR = DMA_LIFCR_CFEIF0  |
                  DMA_LIFCR_CDMEIF0 |
                  DMA_LIFCR_CTEIF0  |
                  DMA_LIFCR_CHTIF0  |
                  DMA_LIFCR_CTCIF0;

    //Enable interrup
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}


//setup mcu for sensor external interrupt
void DRDY_EXTI_Init(){

    //Enable GPIO for external interupt
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER &= ~ (0xFU << 0);

    //Enable syscfg
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;

    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->FTSR  &=  ~EXTI_FTSR_TR0; //Disable falling edge
    EXTI->PR = EXTI_PR_PR0;

    NVIC_EnableIRQ(EXTI0_IRQn);

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


//DMA Interrupt handler
void DMA2_Stream0_IRQHandler(void) {

    if(DMA1->LISR & DMA_LISR_TCIF0){
        //Stop DMA read from i2c
        i2c.finishDMAReading();
    }

    if(DMA1->LISR & DMA_LISR_TEIF0){

    }

}


//Sensor Interrupt handler (i2c)
void EXTIO_IRQHandler(void){
    if(EXTI->PR & EXTI_PR_PR0){

        //setupDMARead from i2c
        i2c.setupDMARead(sensor_address,sensor_register_address);
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

int main(){


    i2c.init();
    init_dma();

    SpiMaster spi(SPI1);
    spi.init(SpiMaster::Mode::MODE1, SpiMaster::Prescaler::Div64);


    while (true)
    {
        size_t local_tail = head;
        if(head != local_tail){
            //new data 
            spi.write(&data_buffer.at(tail), head-tail);
        }
    }

}