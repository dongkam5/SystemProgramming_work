#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PWM 0
#define VALUE_MAX 30
#define DO 3816793
#define RE 3401360
#define MI 3030303
#define FA 2865329
#define SO 2551020
#define LA 2272727
#define TI 2024291
#define HD DO/2
#define HOME 10000
#define HIT 1111
#define ETC 1000

int val;
int on = 0;
int layer = 0;
int score= 100;
int pianoval[1000000];

int twincle[1000] = {DO, DO, SO, SO, LA, LA, SO, FA, FA, MI, MI, RE, RE, DO, \
            SO, SO, FA, FA, MI, MI, RE, SO, SO, FA, FA, MI, MI, RE, \
            DO, DO, SO, SO, LA, LA, SO, FA, FA, MI, MI, RE, RE, DO, HOME};

int bear[1000]= {DO, DO, DO, DO, DO, MI, SO, SO, MI, DO, SO, SO, MI, SO, SO, MI, DO, DO, DO, \
            SO, SO, MI, DO, SO, SO, SO, SO, SO, MI, DO, SO, SO, SO, \
            SO, SO, MI, DO, SO, SO, SO, LA, SO, HD, SO, HD, SO, MI, RE, DO, HOME};

int selecter = 0; 

int twinclecount = 42;
int bearcount = 49;
int count=0;

int randidx = -1;
int melody[7] = {DO, RE, MI, FA, SO, LA, TI};
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void *client()
{
    int sock;
    struct sockaddr_in serv_addr;
    int vaa;
    int str_len;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("192.168.0.18");
    serv_addr.sin_port = htons(5000);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    while (1)
    {
        // initialize recieved message
        vaa = 0;

        // recieve message
        str_len = read(sock, (char *)&vaa, sizeof(int));

        if (str_len == -1)
            error_handling("read() error");

        //Menu layer
        if (layer == 0)
        {
            if (vaa == DO)
            {
                printf("selected piano\n");
                layer = 1;
                val = layer;
                on = 1;
            }
            else if (vaa == RE)
            {
                printf("selected beat test\n");
                layer = 2;
                val = layer;
                on = 1;
            }
            else if (vaa == MI)
            {
                printf("selected absolute melody test\n");
                layer = 3;
                val = layer;
                on = 1;
            }
            else if (vaa == FA)
            {
                printf("selected twincle player\n");
                layer = 4;
                val = layer;
                on = 1;
            }
            else if (vaa == SO)
            {
                printf("selected twincle player\n");
                layer = 5;
                val = layer;
                on = 1;
            }
            else if (vaa == LA)
            {
                printf("selected exit\n");
                layer = 6;
                val = layer;
                on = 1;

                selecter++;
                usleep(100000);

                if (selecter >= 10) {
                    printf("now exited\n");
                    exit(0);
                }


                //exit(0);
            }
            else if (vaa == HOME)
            {
                layer = 0;
                val = layer;
                on = 1;
            }
            else
            {
                printf("please correct button\n");
            }
        }

        // Piano layer.
        else if (layer == 1)
        { // 각 값을 받으면 전달

            // if recieved message is melody number, set send message value to current recieved message;
            if (vaa == DO || vaa == RE || vaa == MI || vaa == FA || vaa == SO || vaa == LA || vaa == TI) 
            {
                val = vaa;
                on = 1;
            }

            // if recieved message is home button, layer setted 0. Then send layer data to output program.
            else if (vaa == HOME)
            {
                layer = 0;

                val = layer;
                on = 1;
            }

            // even if AFK data is recieved, it also send.
            else
            {
                val = vaa;
                on = 1;
            }

            // each sended melody or data must be write in array. 
            pianoval[count] = vaa;
            count++;     //also counting how many array is saved.
        }

        //Recoder layer
        else if (layer == 2)
        { 
            //until i <= count; send data in array.
            //last 2 letter would be home and 0 data.

            for (int i =0; i <= count-2; i++) {
                val = pianoval[i];
                on = 1;
                usleep(100000);
            }

            //after speak all record data, reset count
            count = 0;

            //then move back to menu process.
            layer = 0;
            val = layer;
            on = 1;
        }
        
        //Melody test layer
        else if (layer == 3)
        {
            usleep(1000 * 1000);

            // Random index data selected. set send message to selected data. 
            int randidx = rand() % 7;
            int rand = melody[randidx];
            int temp = 1000; 
            val = rand;
            on = 1;

            // check with printf fuction
            printf("RAND 값-%d  \n", rand);

            // set last test's recieved data in temp integer variable.
            temp = vaa;

            // get start time for checking late
            time_t start = time(NULL);

            // Answering loop syntax
            while (1)
            {
                // get current time for checking late
                time_t end = time(NULL);

                // in while syntax, read function read data from recieved massge;
                str_len = read(sock, (char *)&vaa, sizeof(int));

                // when last data and current data is overlapped, set current data to AFK data
                if (temp == vaa) {
                    vaa = 1000;            // set it to AFK for ignore current overlapped data
                    on = 1;
                }
                // if both data isn't overlapped, set temp to AFK data for recieving next data
                else {
                    temp = 1000;          // when overlapped data ended, AFK data will be recieved for reset temp.
                }
                
                // if recieved message is home button, layer setted 0. Then send layer data to output program.
                if (vaa == HOME)
                {
                layer = 0;
                val = layer;
                on = 1;

                score = 100; // initialize score data when test ended.
                break;
                }

                // checking function for recieved message;
                if (vaa != 1000) {
                    printf("누른값-%d\n",vaa);
                }
                
                // when current recieved message isn't melody and late 5 sec, score descrease 1 point;
                if ((vaa < 200000) && ((int)(end - start) > 5))
                {
                    score -= 1;
                    val = score;
                    on = 1;

                    printf("score: %d",score);

                    // when each problem ended with success or fail, break answering loop.
                    break;
                }
                // when current recieved message is melody data,
                else if (str_len != -1 && vaa>200000 )
                {
                    // if it is correct, add 2 to score
                    if (rand == vaa)
                    {
                        score+=2;
                        val = score ;
                        on = 1;
                        printf("score: %d ",score);
                    }
                    //if it is wrong, substract 1 to score;
                    else
                    {
                        score-=1;
                        val = score;
                        on = 1;
                        printf("score: %d",score);
                    }

                    // when each problem ended with success or fail, break answering loop.
                    break;
                }
                //printf("time : %d ", (int)(end - start));
                usleep(100);

                vaa=1000;

                if (score > 105) {
                    layer = 0;
                    val = layer;
                    score = 100;
                    break;
                }
            }
    
        }
        else if (layer == 4) {

                printf("selected tincle\n");

                // Set melody data to message until all sound spoken
                for (int i =0; i <= twinclecount; i++) {

                        // Send 1 sec for some melodies. 2분의 1박자
                        if (i == 6 || i == 13 || i == 20 || i == 27 || i == 34 || i == 41)
                        {
                            for (int j = 0; j < 10; j++) {
                                val = twincle[i];
                                on = 1;

                                usleep(100000);
                                } 
                            }
                        // Send 0.5 sec for other melodies. 4분의 1박자.
                        else {
                            for (int j = 0; j < 5; j++) {
                                val = twincle[i];
                                on = 1;

                                usleep(100000);
                                }
                        }
                    
                    //send term signal. 박자간 간격
                    val = ETC;
                    on = 1;
                    usleep(10000);

                    }            
                }
                layer = 0;
                //on = 1;

        }
            else if (layer == 5) {

            printf("selected bear\n");
                
                // Set melody data to message until all sound spoken
                for (int i = 0; i <= bearcount; i++) {

                    // Send 1 sec for some melodies. 2분의 1박자
                    if (i == 18 || i == 25 || i == 32 || i == 41 || i == 48)
                    {
                        val = bear[i];
                        printf("%d\n", val);
                        on = 1;
                        usleep(1000000);
                        
                    }
                    // Send 2.5 sec for some melodies. 16분의 1박자
                    else if (i == 1 || i == 2 || i == 6 || i == 7 || i == 10 || i == 11 || i == 13 || i == 14 || i == 37 || i == 38 || i == 39 || i == 40) {
                        val = bear[i];
                        printf("%d\n", val);
                        on = 1;
                        usleep(250000);
                          
                    }
                    // Send 0.5 sec for other melodies. 4분의 1박자.
                    else {
                        val = bear[i];
                        printf("%d\n", val);
                        on = 1;
                        usleep(500000);
                        }   
                    }

                    //send term signal. 박자간 간격
                    val = ETC;
                    on = 1;
                    usleep(10000);
                }

                layer = 0;


            }

        if (vaa != 1000) {
        printf("Receive message from Server : %d\n", vaa);
        }


    }
    close(sock);
    return (0);
}

void *server()
{
    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    int vaa;
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    // Output program connect to main program with socket
    // With already defined port number : 6000

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(6000);

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

        // Until layer variable is in 0~6,and On varaible is 1 (activated);
        if ((layer >= 0 && layer <= 6) && on == 1)
        {
            //Send current val(send message) to socket
            write(clnt_sock, (char *)&val, sizeof(int));
            //Then reset activate permission to 0 (disactivated);
            on = 0;
        }
    
        usleep(100000);
    }
    close(clnt_sock);
    close(serv_sock);
}

int main()
{
    pthread_t p_thread[2];
    int thr_id;
    int status;

    char p1[] = "thread_1";
    char p2[] = "thread_2";

    // Create (input to main) Thread
    thr_id = pthread_create(&p_thread[0], NULL, client, (void *)p1);
    if (thr_id < 0)
    {
        perror("thread create error : ");
        exit(0);
    }
    // Create (main to output) Thread
    thr_id = pthread_create(&p_thread[1], NULL, server, (void *)p2);
    if (thr_id < 0)
    {
        perror("thread create error : ");
        exit(0);
    }
    pthread_join(p_thread[0], (void **)&status);
    pthread_join(p_thread[1], (void **)&status);

    return 0;
}