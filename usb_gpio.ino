#include "teensy_gpio.h"

// Globals
GPIOcomm_struct_t GPIO_comm = {
                                  GPIO_COMM_MAGIC,
                                  {},
                                  };
Hostcomm_struct_t   Host_comm =   {
                                  GPIO_COMM_MAGIC,
                                  {},
                                  };
                                  
uint8_t outpins[] = {2,14,7,8,6,20,21,5};     // Port D 
uint8_t inpins[]  = {15,22,23,9,10,13,11,12}; // Port C

char msg[200]; // Debug message 

#define GPIO_NB 8 // Number of controlled GPIOs 

//
// Manage communication with the host
//
int GPIO_comm_update( void ) {
  static uint8_t      *ptin   = (uint8_t*)(&Host_comm),
                      *ptout  = (uint8_t*)(&GPIO_comm);
  static int          ret;
  static int          in_cnt = 0;
  int                 i; 
  
  ret = 0;
  
  // Read all incoming bytes available until incoming structure is complete
  while(  ( Serial.available( ) > 0 ) && 
          ( in_cnt < (int)sizeof( Host_comm ) ) ){
    ptin[in_cnt++] = Serial.read( );
  }
  
  // Check if a complete incoming packet is available
  if ( in_cnt == (int)sizeof( Host_comm ) ) {
  
    // Clear incoming bytes counter
    in_cnt = 0;
    
    
    // Check the validity of the magic number
    if ( Host_comm.magic != GPIO_COMM_MAGIC ) {
      
      // Flush input buffer
      while ( Serial.available( ) )
        Serial.read( );
    
      ret = GPIO_ERROR_MAGIC;
    }
    else {
    
      // Valid packet received
      
      // Update GPIO values
      GPIOD_PDOR &= ~(0xFF);              // mask for the first 8 bits
      GPIOD_PDOR |= Host_comm.writegpio;  // update only the first 8 bits 
      
            
      // Update output data structure values
      GPIO_comm.readgpio = GPIOC_PDIR; // First 8bits of the 32bit GPIOC_PDIR register 
      
      // Send data structure to host
      Serial.write( ptout, sizeof( GPIO_comm ) );

      // Force immediate transmission
      Serial.send_now( );
      
    }
  }

  return ret;
}

//
//  Arduino setup function
//
void setup() {
  int i; 
  // Initialize UART1 serial link
  Serial.begin( GPIO_USB_UART_SPEED );

  for(i=0; i < GPIO_NB; i++){
    pinMode(outpins[i], OUTPUT); 
    pinMode(inpins[i], INPUT); 
  }
  
}


//
//  Arduino main loop
//
void loop( ) {
  // Bidirectional serial exchange with host
  GPIO_comm_update(); 
}
