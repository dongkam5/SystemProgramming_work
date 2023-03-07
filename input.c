#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define VALUE_MAX 256
#define DIRECTION_MAX 45

#define PIN 16      // Btn1_IN
#define PIN2 20     // Btn2_IN
#define PIN3 21     // BTN3_IN
#define PIN4 4      // Btn4_IN
#define PIN5 22     // Btn5_IN
#define PIN6 12     // Btn6_IN
#define PIN7 13     // Btn7_IN
#define POUT 23     // Btn1_OUT
#define POUT2 24    // Btn2_OUT
#define POUT3 25    // Btn3_OUT
#define POUT4 17    // Btn4_OUT
#define POUT5 27    // Btn5_OUT
#define POUT6 26    // Btn6_OUT
#define POUT7 19    // Btn7_OUT
#define BEATIN 18   // BEATIN
#define HOME_IN 9   // HOME_IN
#define HOME_OUT 11 // HOME_OUT
#define DO 3816793
#define RE 3401360
#define MI 3030303
#define FA 2865329
#define SO 2551020
#define LA 2272727
#define TI 2024291
#define HOME 10000
#define HIT 1111
#define ETC 1000

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
        fprintf(stderr, "Failed to open gpio message for writing!\n");
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

int main()
{
    if (-1 == GPIOExport(POUT) || -1 == GPIOExport(POUT2) || -1 == GPIOExport(POUT3) || -1 == GPIOExport(POUT4) || -1 == GPIOExport(POUT5) || -1 == GPIOExport(POUT6) || -1 == GPIOExport(POUT7) || -1 == GPIOExport(PIN) || -1 == GPIOExport(PIN2) || -1 == GPIOExport(PIN3) || -1 == GPIOExport(PIN4) || -1 == GPIOExport(PIN5) || -1 == GPIOExport(PIN6) || -1 == GPIOExport(PIN7) || -1 == GPIOExport(HOME_IN) || -1 == GPIOExport(HOME_OUT) || -1 == GPIOExport(BEATIN))
        return (1);

    if (-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(POUT2, OUT) || -1 == GPIODirection(POUT3, OUT) || -1 == GPIODirection(POUT4, OUT) || -1 == GPIODirection(POUT5, OUT) || -1 == GPIODirection(POUT6, OUT) || -1 == GPIODirection(POUT7, OUT) || -1 == GPIODirection(HOME_OUT, OUT) || -1 == GPIODirection(PIN, IN) || -1 == GPIODirection(PIN2, IN) || -1 == GPIODirection(PIN3, IN) || -1 == GPIODirection(PIN4, IN) || -1 == GPIODirection(PIN5, IN) || -1 == GPIODirection(PIN6, IN) || -1 == GPIODirection(PIN7, IN) || -1 == GPIODirection(HOME_IN, IN) || -1 == GPIODirection(BEATIN, IN))
        return (2);

    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    int msg;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    if (clnt_sock < 0)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("accept() error");
    }

    while (1)
    {
        if (-1 == GPIOWrite(POUT, 1) || -1 == GPIOWrite(POUT2, 1) || -1 == GPIOWrite(POUT3, 1) || -1 == GPIOWrite(POUT4, 1) || -1 == GPIOWrite(POUT5, 1) || -1 == GPIOWrite(POUT6, 1) || -1 == GPIOWrite(POUT7, 1) || -1 == GPIOWrite(HOME_OUT, 1))
            return (3);
        if (!GPIORead(PIN))
        {
            msg = DO;
            write(clnt_sock, (char *)&msg, sizeof(int));
        }
        else if (!GPIORead(PIN2))
        {
            msg = RE;
            write(clnt_sock, (char *)&msg, sizeof(int));
        }
        else if (!GPIORead(PIN3))
        {

            msg = MI;
            write(clnt_sock, (char *)&msg, sizeof(int));
        }
        else if (!GPIORead(PIN4))
        {

            msg = FA;
            write(clnt_sock, (char *)&msg, sizeof(int));
        }
        else if (!GPIORead(PIN5))
        {

            msg = SO;
            write(clnt_sock, (char *)&msg, sizeof(int));
        }
        else if (!GPIORead(PIN6))
        {

            msg = LA;
            write(clnt_sock, (char *)&msg, sizeof(int));
        }
        else if (!GPIORead(PIN7))
        {
            msg = TI;
            write(clnt_sock, (char *)&msg, sizeof(int));
        }
        else if (!GPIORead(HOME_IN))
        {
            msg = HOME;
            write(clnt_sock, (char *)&msg, sizeof(int));
        }
        // else if (!GPIORead(BEATIN))
        // {
        //     msg = HIT;
        //     write(clnt_sock, (char *)&msg, sizeof(int));
        // }
        else
        {
            msg = ETC;
            write(clnt_sock, (char *)&msg, sizeof(int));
        }
        usleep(100000);
    }
    close(clnt_sock);
    close(serv_sock);
    if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(POUT2) || -1 == GPIOUnexport(POUT3) || -1 == GPIOUnexport(POUT4) || -1 == GPIOUnexport(POUT5) || -1 == GPIOUnexport(POUT6) || -1 == GPIOUnexport(POUT7) || -1 == GPIOUnexport(HOME_OUT) || -1 == GPIOUnexport(PIN) || -1 == GPIOUnexport(PIN2) || -1 == GPIOUnexport(PIN3) || -1 == GPIOUnexport(PIN4) || -1 == GPIOUnexport(PIN5) || -1 == GPIOUnexport(PIN6) || -1 == GPIOUnexport(PIN7) || -1 == GPIOUnexport(HOME_IN) || -1 == GPIOUnexport(BEATIN))
        return (4);

    return 0;
}
