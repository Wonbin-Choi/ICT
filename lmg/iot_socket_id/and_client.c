#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <mysql/mysql.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
#define ARR_CNT 5

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);

char name[NAME_SIZE] = "[Default]";
char msg[BUF_SIZE];
char message_buffer[20];
int new_data_available = 0;
int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;

    if (argc != 4) {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "%s", argv[3]);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    // MySQL 데이터베이스 연결 설정
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

    if (mysql_real_connect(conn, "10.10.14.64", "lmg", "1234", "wild", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        exit(1);
    }

    // 메시지 전송 스레드 생성
    sprintf(msg, "[%s:PASSWD]", name);
    write(sock, msg, strlen(msg));
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);

    pthread_join(snd_thread, &thread_return);
    // pthread_join(rcv_thread, &thread_return);

    // 연결 종료
    mysql_close(conn);
    close(sock);
    return 0;
}
void *send_msg(void *arg) {
    int *sock = (int *)arg;
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return NULL;
    }

    if (mysql_real_connect(conn, "10.10.14.64", "lmg", "1234", "wild", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        return NULL;
    }

    MYSQL_RES *result;
    MYSQL_ROW row;

    while (1) {
        // 데이터베이스에서 새로운 메시지를 가져오는 쿼리 실행
        if (mysql_query(conn, "SELECT * FROM and_message_queue2") != 0) {
            fprintf(stderr, "mysql_query() failed\n");
            continue;
        }

        result = mysql_store_result(conn);
        if (result == NULL) {
            fprintf(stderr, "mysql_store_result() failed\n");
            continue;
        }

        // 가져온 메시지를 서버에 전송
    	while ((row = mysql_fetch_row(result)) != NULL) {
        	write(*sock, row[1], strlen(row[1])); // 메시지는 두 번째 컬럼에 저장되어 있을 것으로 가정
        	mysql_query(conn, "DELETE FROM and_message_queue2");
    }
        // 메시지 전송 후 테이블 비우기
       // mysql_query(conn, "DELETE FROM message_queue");

        mysql_free_result(result);
        sleep(1); // 1초마다 데이터베이스 확인
    }

    mysql_close(conn);
    return NULL;
}


void * recv_msg(void * arg)
{
        int * sock = (int *)arg;
        int i;
        char *pToken;
        char *pArray[ARR_CNT]={0};

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

                /*      pToken = strtok(name_msg,"[:]");
                        i = 0;
                        while(pToken != NULL)
                        {
                        pArray[i] =  pToken;
                        if(i++ >= ARR_CNT)
                        break;
                        pToken = strtok(NULL,"[:]");
                        }

                //              printf("id:%s, msg:%s,%s,%s,%s\n",pArray[0],pArray[1],pArray[],pArray[3],pArray[4]);
                printf("id:%s, msg:%s\n",pArray[0],pArray[1]);
                */
        }
}

void error_handling(char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

