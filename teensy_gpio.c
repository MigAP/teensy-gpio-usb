#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(__linux__)
#include <linux/serial.h>
#include <linux/input.h>
#endif
#include "teensy_gpio.h"

// Flags
#define HOST_STANDALONE                             // main is added

#define HOST_BAUDRATE       B115200                 // Serial baudrate
#define HOST_READ_TIMEOUT   5                       // Tenth of second
#define HOST_NB_PING        500                     // Nb roundtrip communication
//#define HOST_PERIOD         10000                 // Period of serial exchange (us)
#define HOST_PERIOD         300000                  // Period of serial exchange (us)
#define HOST_DEV_SERIALNB    5488750                // Serial number of the teensy
#define HOST_DEV_SERIALLG     10                    // Max length of a serial number

#if defined(__linux__)
#define HOST_SERIAL_DEV_DIR   "/dev/serial/by-id/"
#elif defined(__APPLE__)
#define HOST_SERIAL_DEV_DIR   "/dev/"
#else
#define HOST_SERIAL_DEV_DIR   "/dev/"
#endif

// Globals
int  Host_fd = HOST_ERROR_FD;                // Serial port file descriptor
char Host_devname[PATH_MAX] =""; // Serial port devname used to get fd with open 

struct termios oldtio; // Backup of initial tty configuration

GPIOcomm_struct_t GPIO_comm; 
Hostcomm_struct_t Host_comm; 


//
//  Get the device name from the device serial number
//
char *Host_name_from_serial( uint32_t serial_nb ) {
  DIR           *d;
  struct dirent *dir;
  char          serial_nb_char[HOST_DEV_SERIALLG];
  static char   portname[PATH_MAX];
  
  // Convert serial number into string
  snprintf( serial_nb_char, HOST_DEV_SERIALLG, "%d", serial_nb );
  
  // Open directory where serial devices can be found
  d = opendir( HOST_SERIAL_DEV_DIR );
  
  // Look for a device name contining teensy serial number
  if ( d ) {
  
    // Scan each file in the directory
    while ( ( dir = readdir( d ) ) != NULL ) {
      if ( strstr( dir->d_name, serial_nb_char ) )  {
      
        // A match is a device name containing the serial number
        snprintf( portname, PATH_MAX, "%s%s", HOST_SERIAL_DEV_DIR, dir->d_name );
        return portname;
      }
    }
    closedir( d );
  }
  
  return NULL;
}


//
// Initialize serial port
//
int Host_init_port(uint32_t serial_nb)  {
    struct termios newtio;
    char    *portname;

    // Check if device plugged in
    portname = Host_name_from_serial( serial_nb );

    if ( !portname )
      return HOST_ERROR_DEV;

    // Open device
    Host_fd = open( portname, O_RDWR | O_NOCTTY | O_NONBLOCK );
    
    if ( Host_fd < 0 )  {
      perror( portname );
      return HOST_ERROR_DEV;
    }

    // Store the corresponding devname 
    strncpy( Host_devname, portname, PATH_MAX );


    // Initialize corresponding data structure
    Host_comm.magic     = GPIO_COMM_MAGIC; 
    Host_comm.writegpio = 0xF0; 

    // Save current port settings 
    tcgetattr( Host_fd, &oldtio);

    // Define new settings 
    bzero( &newtio, sizeof(newtio) );
    cfmakeraw( &newtio );

    newtio.c_cflag =      HOST_BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag =      IGNPAR;
    newtio.c_oflag =      0;
    newtio.c_lflag =      0;
    newtio.c_cc[VTIME] =  0;
    newtio.c_cc[VMIN] =   0;

    #if defined(__APPLE__)
    cfsetispeed( &newtio, HOST_BAUDRATE );
    cfsetospeed( &newtio, HOST_BAUDRATE );
    #endif

    // Apply the settings 
    tcflush( Host_fd, TCIFLUSH );
    tcsetattr( Host_fd, TCSANOW, &newtio );

    return 0;
}

//
//  Release serial port
//
void Host_release_port(uint32_t serial_nb)  {
  
  if ( Host_fd != HOST_ERROR_FD ) {
    // Restore initial settings if needed
    tcsetattr( Host_fd, TCSANOW, &oldtio );
    close( Host_fd );
    
    // Clear fd and corresponding devname
    Host_fd = HOST_ERROR_FD;
    strncpy( Host_devname, "", PATH_MAX );
  }

}


//
// Manage communication with the teensy connected to portname
//
int Host_comm_update( uint32_t      serial_nb,
                      uint8_t       writegpio,
                      GPIOcomm_struct_t **comm ) {


  int                 ret, res = 0;
  uint8_t             *pt_in;
  struct timespec     start, cur;
  unsigned long long  elapsed_us;
 
  // Check if fd index is valid
  if ( Host_fd == HOST_ERROR_FD )
    return HOST_ERROR_FD;
 
  // Update output data structue
  Host_comm.writegpio = writegpio;
  
  // Send output structure
  res = write( Host_fd, &Host_comm, sizeof( Host_comm ) );
  if ( res < 0 )  {
    perror( "write Host_comm" );
    return HOST_ERROR_WRITE_SER;
  }
  
  // Flush output buffer
  fsync(Host_fd);

  // Wait for response

  // Get current time
  clock_gettime( CLOCK_MONOTONIC, &start );

  // Reset byte counter and magic number
  res = 0;
  GPIO_comm.magic = 0;
  pt_in = (uint8_t*)(&GPIO_comm);

  do  {
    ret = read( Host_fd, &pt_in[res], 1 );

    // Data received
    if ( ret > 0 )  {
      res += ret;
    }

    // Read error
    if ( ret < 0 )
      break;

    // Compute time elapsed
    clock_gettime( CLOCK_MONOTONIC, &cur );
    elapsed_us =  ( cur.tv_sec * 1e6 + cur.tv_nsec / 1e3 ) -
                  ( start.tv_sec * 1e6 + start.tv_nsec / 1e3 );

    // Timeout
    if ( elapsed_us / 100000 > HOST_READ_TIMEOUT ){
      break;
    }
    

  } while ( res < sizeof( GPIO_comm ) );

  // Check response size
  if ( res != sizeof( GPIO_comm ) )  {
    fprintf( stderr, "Packet with bad size received.\n" );

    // Flush input buffer
    while ( ( ret = read( Host_fd, pt_in, 1 ) ) )
      if ( ret <= 0 )
        break;
        
    return HOST_ERROR_BAD_PK_SZ;
  }

  // Check magic number
  if ( GPIO_comm.magic !=  GPIO_COMM_MAGIC )  {
    fprintf( stderr, "Invalid magic number.\n" );
    return HOST_ERROR_MAGIC;
  }
  
  // Return pointer to GPIO_comm structure
  *comm = &GPIO_comm;
  
  // Print rountrip duration
  #ifdef HOST_STANDALONE
  fprintf( stderr, "Delay: %llu us\n", elapsed_us );
  #endif

  return 0;

}


#ifdef HOST_STANDALONE

int main(int argc, char* argv[]) {

  int i, ret; 
  uint8_t writemsg = 0; 
  GPIOcomm_struct_t *comm; 


  // Initialize serial port
  if ( Host_init_port( HOST_DEV_SERIALNB ) )  {
    fprintf( stderr, "Error initializing serial port.\n" );
    exit( -1 );
  }

  // Testing roundtrip serial link duration
  for ( i = 0; i < HOST_NB_PING; i++ )  {
  
    // Serial exchange with teensy
    if ( ( ret = Host_comm_update(  HOST_DEV_SERIALNB,
                                    ~(writemsg),
				    &comm ) ) )  {
      fprintf( stderr, "Error %d in Host_comm_update.\n", ret );
      break;
    }
    
    // Display telemetry
    fprintf(stderr, "#:%d\t Read: %2x\n",i,comm->readgpio); 
              
    // Wait loop period
    usleep( HOST_PERIOD );

    writemsg++;
  }

  // Restoring serial port initial configuration
  Host_release_port( HOST_DEV_SERIALNB );

  return 0;
}
#endif
