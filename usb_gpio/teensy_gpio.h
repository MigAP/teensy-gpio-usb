#ifndef __TEENSY_GPIO_H
#define __TEENSY_GPIO_H

// Defines
#define HOST_ERROR_FD         -1        // Non existant file descriptor
#define HOST_ERROR_DEV        -2        // Non existant serial device
#define HOST_ERROR_MAX_DEV    -3        // Maximum devices limit 
#define HOST_ERROR_WRITE_SER  -4        // Write error on serial
#define HOST_ERROR_BAD_PK_SZ  -5        // Bad incoming packet size error
#define HOST_ERROR_MAGIC      -6        // Bad magic number received

#define GPIO_COMM_MAGIC         0x43305735        // Magic number: "teensy35" in leet speech
#define GPIO_USB_UART_SPEED     115200            // Baudrate of the teeensy USB serial link
#define GPIO_ERROR_MAGIC        -1                // Magic number error code (Teensy)

//
// Structures
//

// Teensy->host communication data structure
typedef struct {
  uint32_t      magic;                        // Magic number
  uint8_t      readgpio;  // Read values of port  C in teensy
  } GPIOcomm_struct_t;

// Host->teensy communication data structure
typedef struct {
  uint32_t      magic;                        // Magic number
  uint8_t      writegpio; // Output value of port D in teensy
} Hostcomm_struct_t;

//
// Prototypes
//
char *Host_name_from_serial(  uint32_t );
int   Host_get_fd(            uint32_t );
int   Host_init_port(         uint32_t );
void  Host_release_port(      uint32_t );
int   Host_comm_update(       uint32_t,
		              uint8_t, 
                              GPIOcomm_struct_t** );


#endif
