
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
            GPIOB->OTYPER 

            //High speed outputs
            GPIOB->OSPEEDR 

            //Alternate Function
            GPIOB->AFR[1]
            GPIOB->AFR[1]

            I2C1->CR1 = 
            I2C1->CR2 

            I2C1->CCR
            I2C1->TRISE
            I2C1 




        }

        void writeTo(){

        }

        void readFrom(){

        }


    private:


};



start 
select device 
check for acknowledge
write internal register 
check for ack
select device for reading
check for ack
read 

