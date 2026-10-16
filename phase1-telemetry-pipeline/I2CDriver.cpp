#include "stm32f4xx.h"


class I2CDriver {
    public:

        enum Status {

        };

        void init(){
            //we are using PB 8 and 9, these are the pins
            // used by I2C on stm32, GPIOB <--> I2C1

            //enable gpio and i2c clocks
            RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
            RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

            //Clear and set mode, alternate function
            GPIOB->MODER &= ~((3U << 16) | (3U <<18));
            GPIOB->MODER |= ((2U << 16) | (2U << 18));

            //Open drain
            GPIOB->OTYPER |= ((1U << 8) | (1U <<9));

            //High speed outputs
            GPIOB->OSPEEDR |= ((3U << GPIO_OSPEEDR_OSPEED8_Pos) | (3U <<GPIO_OSPEEDR_OSPEED9_Pos));

            //Alternate Function
            GPIOB->AFR[1] &=((0xFU << GPIO_AFRH_AFSEL9_Pos) | (0xFU << GPIO_AFRH_AFSEL9_Pos) );
            GPIOB->AFR[1] &=((4U << GPIO_AFRH_AFSEL9_Pos) | (4U << GPIO_AFRH_AFSEL9_Pos) );



            

            I2C1->CR1 &= ~I2C_CR1_PE;

            //Sets pclk1
            I2C1->CR2 = 16U;

            I2C1->CCR = 80U;
            I2C1->TRISE = 17U;
            
            I2C1->CR1 |= I2C_CR1_PE; 




        }

        void writeTo(){

        }

        void readFrom(){

        }


    private:


};



/** start 
select device 
check for acknowledge
write internal register 
check for ack
select device for reading
check for ack
read **/

