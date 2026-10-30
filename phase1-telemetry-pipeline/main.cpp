#include "stm32f4xx.h"
#include <vector>


/*
    Bare Metal Code

*/

int main(){

    std::vector<uint16_t> data_buffer(100);
    init_gppio_a();
    init_adc();
    init_dma(data_buffer);

    adc_start();


    //init I2C1
    //read from
    //store in buffer
    //



}


void init_dma(std::vector<uint16_t> buffer, uint32_t paddr){
    

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN; //Enabble dma clock

    DMA2_Stream0->CR &= ~DMA_SxCR_EN; //disable dma 2 stream 0

    while(DMA2_Stream0->CR & DMA_SxCR_EN){} //DO NOTHING while stream 0 is active


    //(uint32_t) &ADC1->DR
    DMA2_Stream0->PAR =  paddr ;

    DMA2_Stream0->M0AR =  (uint32_t)buffer.data();

    DMA2_Stream0->NDTR = buffer.size();


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


void init_gppio_a(){
    //We are using Port A
    //Enable GP_IO port Clock, 
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    //COnfigure pin as analog mode
    GPIOA->MODER &= ~(3U << (0 * 2));
    GPIOA->MODER |= (3U << (0 * 2));

    GPIOA->PUPDR &= ~(3U << (0*2));

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


