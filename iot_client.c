#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h> // 시간 관련 헤더 파일 추가

#define BUF_SIZE 100
#define NAME_SIZE 20

pthread_mutex_t mutex; // Mutex 선언

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

char name[NAME_SIZE]="[Default]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void * thread_return;

    if(argc != 4) {
        printf("Usage : %s <IP> <port> <name>\n",argv[0]);
        exit(1);
    }

    sprintf(name, "%s",argv[3]);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    sprintf(msg,"[%s:PASSWD]",name);
    write(sock, msg, strlen(msg));
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);

    pthread_join(snd_thread, &thread_return);
    close(sock);
    return 0;
}

void * send_msg(void * arg)
{
    int *sock = (int *)arg;
    char servo_msg[BUF_SIZE];
    int str_len;
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char *server = "localhost";
    char *user = "cwb";
    char *password = "1234";
    char *database = "wild";

    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    while(1) {
        pthread_mutex_lock(&mutex); // Mutex 락

        // 현재 시간을 가져와서 문자열로 변환
        time_t now = time(NULL);
	time_t one_minute_ago = now - 2;
        struct tm *tm_info = localtime(&one_minute_ago);
        char current_time[20];
        strftime(current_time, sizeof(current_time), "%Y-%m-%d %H:%M:%S", tm_info);

        // 데이터베이스에서 새로운 데이터 확인
        char query[100];
        sprintf(query, "SELECT * FROM wild_animal WHERE capture_datetime > '%s'", current_time);
        if (mysql_query(conn, query)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
            exit(1);
        }
        res = mysql_use_result(conn);
        if ((row = mysql_fetch_row(res)) != NULL) {
            // 데이터베이스에서 새로운 데이터가 있을 때 서보모터 제어 명령 전송
            sprintf(servo_msg, "[%s]SERVO\n", "KSH_BT");
	    printf(servo_msg);
            write(*sock, servo_msg, strlen(servo_msg));
        }
        mysql_free_result(res);

        pthread_mutex_unlock(&mutex); // Mutex 언락

        // 일정 시간 간격으로 반복 확인
        sleep(2);
    }
}

void * recv_msg(void * arg)
{
    int * sock = (int *)arg;
    char name_msg[NAME_SIZE + BUF_SIZE +1];
    int str_len;
    while(1) {
        memset(name_msg,0x0,sizeof(name_msg));
        str_len = read(*sock, name_msg, NAME_SIZE + BUF_SIZE );
        if(str_len <= 0)
        {
            *sock = -1;
            return NULL;
        }
        name_msg[str_len] = 0;
        fputs(name_msg, stdout);
    }
}

void error_handling(char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

