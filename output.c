#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <pthread.h>
// Define some device parameters
#define I2C_ADDR   0x27 // I2C device address

// Define some device constants
#define LCD_CHR  1 // Mode - Sending data
#define LCD_CMD  0 // Mode - Sending command

#define LINE1  0x80 // 1st line
#define LINE2  0xC0 // 2nd line

#define LCD_BACKLIGHT   0x08  // On
// LCD_BACKLIGHT = 0x00  # Off

#define ENABLE  0b00000100 // Enable bit
void *soundthread();
void *LCDthread();
void lcd_init(void);
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);




void lcdLoc(int line); //move cursor
void ClrLcd(void); // clr LCD return home
void typeln(const char *s);

int fd;  // seen by all subroutines
#define PWM 0
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define LED_OUT 17
#define VALUE_MAX 256
#define DIRECTION_MAX 45

#define DO 3816793
#define RE 3401360
#define MI 3030303
#define FA 2865329
#define SO 2551020
#define LA 2272727
#define TI 2024291
#define HOME 10000
#define HIT 1111
int sock;
struct sockaddr_in serv_addr;
int msg;
int str_len;
int layer;
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

static int GPIOExport(int pin)
{
#define BUFFER_MAX 3
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}

static int GPIOUnexport(int pin)
{
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open unexport for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}

static int GPIODirection(int pin, int dir)
{
    static const char s_directions_str[] = "in\0out";

    char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

    fd = open(path, O_WRONLY);

    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio direction for writing!\n");
        return (-1);
    }
    if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3))
    {
        fprintf(stderr, "Failed to set direction!\n");
        close(fd);
        return (-1);
    }
    close(fd);
    return (0);
}

static int GPIORead(int pin)
{
    char path[VALUE_MAX];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio value for reading!\n");
        return (-1);
    }

    if (-1 == read(fd, value_str, 3))
    {
        fprintf(stderr, "Failed to read value!\n");
        close(fd);
        return (-1);
    }

    close(fd);

    return (atoi(value_str));
}

static int GPIOWrite(int pin, int value)
{
    static const char s_values_str[] = "01";
    char path[VALUE_MAX];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return (-1);
    }

    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
    {
        fprintf(stderr, "Failed to write value!\n");
        close(fd);
        return (-1);
    }
    close(fd);
    return (0);
}

static int PWMExport(int pwmnum)
{
#define BUFFER_MAX 3
    char buffer[BUFFER_MAX];
    int bytes_written;
    int fd;

    fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open in unexport!\n");
        return -1;
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, bytes_written);
    close(fd);

    sleep(1);
    fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open in export!\n");
        return -1;
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, bytes_written);
    close(fd);
    sleep(1);
    return 0;
}

static int PWMEnable(int pwmnum)
{
    static const char s_unenable_str[] = "0";
    static const char s_enable_str[] = "1";
    char path[DIRECTION_MAX];
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum); // agian to check
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open in enable!\n");
        return -1;
    }

    write(fd, s_unenable_str, strlen(s_unenable_str));
    close(fd);

    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open in enable!\n");
        return -1;
    }

    write(fd, s_enable_str, strlen(s_enable_str));
    close(fd);
    return 0;
}

static int PWMWritePeriod(int pwmnum, int value)
{
    char s_values_str[VALUE_MAX];
    char path[VALUE_MAX];
    int fd, byte;

    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum); // check again
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open in period!\n");
        return -1;
    }

    byte = snprintf(s_values_str, VALUE_MAX, "%d", value); // check again

    if (-1 == write(fd, s_values_str, byte))
    {
        //fprintf(stderr, "Failed to write value in period!\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int PWMWriteDutyCycle(int pwmnum, int value)
{
    char path[VALUE_MAX];
    char s_values_str[VALUE_MAX];
    int fd, byte;

    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum); // check this again
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open in duty_cycle!\n");
        return -1;
    }

    byte = snprintf(s_values_str, VALUE_MAX, "%d", value); // check this again

    if (-1 == write(fd, s_values_str, byte))
    {
        fprintf(stderr, "Failed to write value! in duty_cycle\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

void *speak(int period)
{
    PWMWritePeriod(0, period);
    PWMWriteDutyCycle(PWM, 1000000);
    usleep(100000);
}





// clr lcd go home loc 0x80
void ClrLcd(void)   {
  lcd_byte(0x01, LCD_CMD);
  lcd_byte(0x02, LCD_CMD);
}

// go to location on LCD
void lcdLoc(int line)   {
  lcd_byte(line, LCD_CMD);
}




// this allows use of any size string
void typeln(const char *s)   {

  while ( *s ) lcd_byte(*(s++), LCD_CHR);

}

void lcd_byte(int bits, int mode)   {

  //Send byte to data pins
  // bits = the data
  // mode = 1 for data, 0 for command
  int bits_high;
  int bits_low;
  // uses the two half byte writes to LCD
  bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT ;
  bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT ;

  // High bits
  wiringPiI2CReadReg8(fd, bits_high);
  lcd_toggle_enable(bits_high);

  // Low bits
  wiringPiI2CReadReg8(fd, bits_low);
  lcd_toggle_enable(bits_low);
}

void lcd_toggle_enable(int bits)   {
  // Toggle enable pin on LCD display
  delayMicroseconds(500);
  wiringPiI2CReadReg8(fd, (bits | ENABLE));
  delayMicroseconds(500);
  wiringPiI2CReadReg8(fd, (bits & ~ENABLE));
  delayMicroseconds(500);
}


void lcd_init()   {
  // Initialise display
  lcd_byte(0x33, LCD_CMD); // Initialise
  lcd_byte(0x32, LCD_CMD); // Initialise
  lcd_byte(0x06, LCD_CMD); // Cursor move direction
  lcd_byte(0x0C, LCD_CMD); // 0x0F On, Blink Off
  lcd_byte(0x28, LCD_CMD); // Data length, number of lines, font size
  lcd_byte(0x01, LCD_CMD); // Clear display
  delayMicroseconds(500);
}
int main()
{  if (wiringPiSetup () == -1) exit (1);

  fd = wiringPiI2CSetup(I2C_ADDR);

  //printf("fd = %d ", fd);

  lcd_init(); // setup LCD
     pthread_t p_thread[2];
    int thr_id;
    int status;
    char p1[]="thread_1";
    char p2[]="thread_2";
    thr_id= pthread_create(&p_thread[0],NULL, LCDthread,(void*)p1);
    if (thr_id<0){
        perror("thread error");
        exit(0);
    }
     thr_id= pthread_create(&p_thread[1],NULL, soundthread,(void*)p2);
    if (thr_id<0){
        perror("thread error");
        exit(0);
    }
    pthread_join(p_thread[1], (void**)&status);
    pthread_join(p_thread[0], (void**)&status);

    return 0;
   
}
void *LCDthread(){
        char buffer[16];
    strcpy(buffer,"SCORE : 0      ");
    while(1){
    if (layer==0){

    lcdLoc(LINE1);
    typeln("1. PIANO      ");
    lcdLoc(LINE2);
    typeln("2. PLAY RECORD            ");
    delay(1000);
    ClrLcd();
    lcdLoc(LINE1);
    typeln("3. MELODY TEST             ");
    lcdLoc(LINE2);
    typeln("4. PLAY TWINKLE  ");
    delay(1000);
    lcdLoc(LINE1);
     typeln("5. PLAY BEAR       ");
    lcdLoc(LINE2);
    typeln("                      ");
    delay(1000);
    }

    else if (layer==1){
    lcdLoc(LINE1);
    typeln("1. PIANO         ");
    lcdLoc(LINE2);
    typeln("PRESS THE BUTTON");
    }
    else if (layer==2){
    lcdLoc(LINE1);
    typeln("2.PLAY RECORD          ");
    lcdLoc(LINE2);
    typeln(" PLAYING~!        ");
    }
    else if (layer==3){
    lcdLoc(LINE1);
    typeln("3. MELODY TEST         ");
    
    if (msg > 10 && msg < 1000) {
      //  printf("점수-%d",msg-100);
        snprintf(buffer, 16, "SCORE : %d       " ,msg-100);
    }
    lcdLoc(LINE2);
    typeln(buffer);
    } 
     else if (layer==4){
    lcdLoc(LINE1);
    typeln("4.PLAY SONG          ");
    lcdLoc(LINE2);
    typeln(" TWINKLE!            ");
    }
    else if (layer==5){
    lcdLoc(LINE1);
    typeln("4.PLAY SONG          ");
    lcdLoc(LINE2);
    typeln(" LITTLE BEAR!        ");
    }
    
    }
    
}

void *soundthread(){
    PWMExport(0);
    PWMWritePeriod(0, 10000);
    PWMWriteDutyCycle(0, 0);
    PWMEnable(0);
    // Enable GPIO pins
 
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("192.168.0.2");
    serv_addr.sin_port = htons(6000);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect(11) error");
    while (1)
    {
        int prev_msg = -1;
        str_len = read(sock, (char *)&msg, sizeof(int));
        if (str_len == -1)
            error_handling("read() error");
        if (msg > 1000 && prev_msg == msg)
        {
            PWMWritePeriod(0, msg);
            PWMWriteDutyCycle(PWM, 100000);
            usleep(100000);
            continue;
        }
        else {
            PWMWritePeriod(0, 1000);
            PWMWriteDutyCycle(PWM, 0);

        }
        if (msg == 0 || msg == 1 || msg == 2 || msg == 3 || msg == 4 ||msg==5 ) 
        {
            layer = msg;
        }
        else
        {
            if (layer == 0)
            {
                continue;
            }
            else if (layer == 1)
            {
                if (msg == DO)
                {
                    speak(DO);
                    printf("도");
                    prev_msg = msg;
                }
                else if (msg == RE)
                {
                    speak(RE);
                                        printf("도1");

                    prev_msg = msg;
                }
                else if (msg == MI)
                {
                    speak(MI);
                                        printf("도2");

                    prev_msg = msg;
                }
                else if (msg == FA)
                {
                    speak(FA);
                                        printf("도");

                    prev_msg = msg;
                }
                else if (msg == SO)
                {
                    speak(SO);
                    prev_msg = msg;
                }
                else if (msg == LA)
                {
                    speak(LA);
                    prev_msg = msg;
                }
                else if (msg == TI)
                {
                    speak(TI);
                    prev_msg = msg;
                }
                else if (msg == HOME)
                {
                    layer = 0;
                    prev_msg = -1;
                }
                else
                {
                   // printf("i don't know\n");
                    prev_msg = -1;
                }
            }
            else if (layer == 2 || layer ==4 || layer==5)
            {
                if (msg == DO)
                {
                    speak(DO);
                    prev_msg = msg;
                }
                else if (msg == RE)
                {
                    speak(RE);
                    prev_msg = msg;
                }
                else if (msg == MI)
                {
                    speak(MI);
                    prev_msg = msg;
                }
                else if (msg == FA)
                {
                    speak(FA);
                    prev_msg = msg;
                }
                else if (msg == SO)
                {
                    speak(SO);
                    prev_msg = msg;
                }
                else if (msg == LA)
                {
                    speak(LA);
                    prev_msg = msg;
                }
                else if (msg == TI)
                {
                    speak(TI);
                    prev_msg = msg;
                }
                 else if (msg== DO/2){
                  speak(DO/2);
                    prev_msg = msg;
                }
                else if (msg == HOME)
                {
                    layer = 0;
                    prev_msg = -1;
                }
               
                else
                {
                  //  printf("i don't know\n");
                    prev_msg = -1;
                }
               
            }
            else if (layer == 3)
            {

                if (msg == DO)
                {
                    speak(DO);
                    prev_msg = msg;
                }
                else if (msg == RE)
                {
                    speak(RE);
                    prev_msg = msg;
                }
                else if (msg == MI)
                {
                    speak(MI);
                    prev_msg = msg;
                }
                else if (msg == FA)
                {
                    speak(FA);
                    prev_msg = msg;
                }
                else if (msg == SO)
                {
                    speak(SO);
                    prev_msg = msg;
                }
                else if (msg == LA)
                {
                    speak(LA);
                    prev_msg = msg;
                }

                else if (msg == TI)
                {
                    speak(TI);
                    prev_msg = msg;
                }
                else if (msg == HOME)
                {
                    layer = 0;
                    prev_msg = -1;
                }
                else
                {
                    printf("i don't know\n");
                    prev_msg = -1;
                }
            }
           
        }
    }
    // printf("Receive message from Server : %s\n", msg);
    close(sock);
    // Disable GPIO pins


}