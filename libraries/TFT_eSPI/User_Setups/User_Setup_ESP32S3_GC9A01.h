#define GC9A01_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// Pines SPI
#define TFT_MOSI 17
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  23

// Opcionalmente puedes controlar el brillo del backlight con PWM
// Si lo conectas directo a 3.3V, ignora esto
//#define TFT_BL    21
//#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY  27000000 // 27 MHz es seguro
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000
