#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <omp.h>
#include <unistd.h>

omp_lock_t lock;
int client_sock[3];
int cli_num;

void send_msg(char* msg, int len){
    omp_set_lock(&lock);
    for(int i = 0; i <= cli_num; i++){
       send(client_sock[i], msg, len, 0);         
    };
    
    omp_unset_lock(&lock);
}

void recv_cli(int sock_cli){
    int len;
    char msg[10000];
    
    memset(msg, 0, sizeof(msg));

    while(1){
        len = recv(sock_cli, msg, sizeof(msg), 0);
        if(len == 0){
            break;
        }
        send_msg(msg, len);
    }
    omp_set_lock(&lock);
    for(int i = 0; i <= cli_num; i++){
        if(sock_cli == client_sock[i]){
            while(i <= cli_num - 1){
                client_sock[i] = client_sock[i+1];
                i++;
            }
            break;
        }
    }
    cli_num--;
    omp_unset_lock(&lock);
    close(sock_cli);  
}

int main(){
    int sock_ser, sock_cli, cli_len;
    int num = 0;
    struct sockaddr_in serv, cli;
    
    omp_init_lock(&lock);

    sock_ser = socket(AF_INET, SOCK_STREAM, 0);
    
    if(sock_ser == -1){
        perror("server socket error");
        exit(1);
    }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(30030);
    serv.sin_addr.s_addr = inet_addr("0.0.0.0");
    
    if(bind(sock_ser, (struct sockaddr *) &serv, sizeof(serv)) == -1){
        perror("bind error");
        exit(1);
    }
    if(listen(sock_ser, 5) == -1){
        perror("listen error");
        exit(1);
    }

    #pragma omp parallel num_threads(3)
    {
        #pragma omp for
        for(int i = 0; i < 3; i++){
            cli_len = sizeof(cli);
            sock_cli = accept(sock_ser, (struct sockaddr *) &cli, (socklen_t *) &cli_len);
            
            if(sock_cli == -1){
                perror("accept error");
                exit(1);
            }

            omp_set_lock(&lock);
            cli_num = num++; 
            client_sock[cli_num] = sock_cli;
            omp_unset_lock(&lock);
            
            printf("IP : %s client connected, cli num : %d\n", inet_ntoa(cli.sin_addr), cli_num);
            recv_cli(client_sock[cli_num]);
        }
    }
    close(sock_ser);
    return 0;
}
