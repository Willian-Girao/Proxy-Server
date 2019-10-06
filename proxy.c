#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <limits.h>

#define QUEUE_LIMITE 1
#define LEN 700000
#define CHUNKSIZE 1024
#define MAX_L 4096

typedef int bool;
#define true 1
#define false 0

void replace (char *, char *, char *);
struct sockaddr_in servidor;
struct sockaddr_in clin_serv_par;
struct sockaddr_in sock_cli;
struct hostent *host_ent;
struct in_addr **lista_addr;

struct entry_s {
	char *key;
	char *value;
	struct entry_s *next;
};

typedef struct entry_s entry_t;

struct hashtable_s {
	int size;
	struct entry_s **table;
};

typedef struct hashtable_s hashtable_t;

/* Create a new hashtable. */
hashtable_t *ht_create( int size ) {

	hashtable_t *hashtable = NULL;
	int i;

	if( size < 1 ) return NULL;

	/* Allocate the table itself. */
	if( ( hashtable = malloc( sizeof( hashtable_t ) ) ) == NULL ) {
		return NULL;
	}

	/* Allocate pointers to the head nodes. */
	if( ( hashtable->table = malloc( sizeof( entry_t * ) * size ) ) == NULL ) {
		return NULL;
	}
	for( i = 0; i < size; i++ ) {
		hashtable->table[i] = NULL;
	}

	hashtable->size = size;

	return hashtable;
}

/* Hash a string for a particular hash table. */
int ht_hash( hashtable_t *hashtable, char *key ) {

	unsigned long int hashval;
	int i = 0;

	/* Convert our string to an integer */
	while( hashval < ULONG_MAX && i < strlen( key ) ) {
		hashval = hashval << 8;
		hashval += key[ i ];
		i++;
	}

	return hashval % hashtable->size;
}

/* Create a key-value pair. */
entry_t *ht_newpair( char *key, char *value ) {
	entry_t *newpair;

	if( ( newpair = malloc( sizeof( entry_t ) ) ) == NULL ) {
		return NULL;
	}

	if( ( newpair->key = strdup( key ) ) == NULL ) {
		return NULL;
	}

	if( ( newpair->value = strdup( value ) ) == NULL ) {
		return NULL;
	}

	newpair->next = NULL;

	return newpair;
}

/* Insert a key-value pair into a hash table. */
void ht_set( hashtable_t *hashtable, char *key, char *value ) {
	int bin = 0;
	entry_t *newpair = NULL;
	entry_t *next = NULL;
	entry_t *last = NULL;

	bin = ht_hash( hashtable, key );

	next = hashtable->table[ bin ];

	while( next != NULL && next->key != NULL && strcmp( key, next->key ) > 0 ) {
		last = next;
		next = next->next;
	}

	/* There's already a pair.  Let's replace that string. */
	if( next != NULL && next->key != NULL && strcmp( key, next->key ) == 0 ) {

		free( next->value );
		next->value = strdup( value );

		/* Nope, could't find it.  Time to grow a pair. */
	} else {
		newpair = ht_newpair( key, value );

		/* We're at the start of the linked list in this bin. */
		if( next == hashtable->table[ bin ] ) {
			newpair->next = next;
			hashtable->table[ bin ] = newpair;

			/* We're at the end of the linked list in this bin. */
		} else if ( next == NULL ) {
			last->next = newpair;

			/* We're in the middle of the list. */
		} else  {
			newpair->next = next;
			last->next = newpair;
		}
	}
}

/* Retrieve a key-value pair from a hash table. */
bool ht_get(hashtable_t *hashtable, char *key) {
	int bin = 0;
	entry_t *pair;

	bin = ht_hash( hashtable, key );

	/* Step through the bin, looking for our value. */
	pair = hashtable->table[ bin ];
	while( pair != NULL && pair->key != NULL && strcmp( key, pair->key ) > 0 ) {
		pair = pair->next;
	}

	/* Did we actually find anything? */
	if( pair == NULL || pair->key == NULL || strcmp( key, pair->key ) != 0 ) {
		return false;

	} else {
		return true;
	}

}

void readFileAndInsertToHash(char file_name[25], hashtable_t *hashtable){
	char termo[40];
	char ch;
	int tamanho,i;
	FILE *fp;

	/* open file */
	fp = fopen(file_name, "r"); // read mode

	if(fp == NULL)
	{
		perror("Error enquanto abria o arquivo!\n");
		exit(EXIT_FAILURE);
	}else{
		printf("Conseguiu abrir o arquivo!\n");
	}

	while( ( ch = fgetc(fp) ) != EOF ) {
		if(ch == '\n'){
			/* insert word of file in a hash table */
			ht_set(hashtable, (char*)termo, (char *)termo);
			for (i = 0; i < 40; ++i)
				termo[i] = '\0';
		}else{
			tamanho=strlen(termo);
			termo[tamanho+1] = '\0';
			termo[tamanho]   = ch;
		}
	}

	/* close file */
	fclose(fp);
}

/* remove spaces between strings */
char *trim(char *str)
{
	char *end;
	// Trim leading space
	while(isspace(*str)) str++;

	if(*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace(*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return str;
}

bool verifyHTMLIsOffensive(char serverResponse[], hashtable_t *hashtable){
	char htmlContent[LEN];
	/* copy serverResponse to htmlContent */
	strcpy(htmlContent, serverResponse);
	bool isOffensive = false;
	char * token;
	
	/* get the first word into htmlContent */

	token = strtok (htmlContent, " ,.-><");
	/* walk through other tokens */
	
	while(token != NULL)
	{
		token = trim(token);
		if(ht_get(hashtable, token)){
			return true;
		}
		token = strtok(NULL, " ,.-><");
	}
	
	return isOffensive;
}

void socket_erro2 (int x) {		// Recebe como paramentro a descricao do socket criado e verifica se a criacao da descricao foi bem sucesidada.
	if (x == -1) {
		perror ("Socket");
		exit (1);
	}
	else {
		printf("Socket criado com sucesso.\n");
	}
}


void socket_inicialization(char* ip_bin, int servPort) {														// Preenche estrutura de descricao do socket.
	sock_cli.sin_family = AF_INET;												// IPv4.
	sock_cli.sin_port = htons(servPort);										// htons() converte formato da porta Host Byte Order para Network Byte Oreder.
	sock_cli.sin_addr.s_addr = inet_addr (ip_bin);							// FUNCAO inet_addr VAI DAR PROBLEMA POIS -1 EH UM IP VALIDO. PESQUISAR DEPOIS E USAR FUNCAO inet_aton.
	memset(sock_cli.sin_zero, 0x0, 8);											// Preenche com zeros campo sin_zero para igualar tamanho de sockaddr_in ao de sockaddr.
}

void replace(char * o_string, char * s_string, char * r_string) {
	//a buffer variable to do all replace things
	char buffer[MAX_L];
	//to store the pointer returned from strstr
	char * ch;
	//first exit condition
	if(!(ch = strstr(o_string, s_string)))
		return;

	//copy all the content to buffer before the first occurrence of the search string
	strncpy(buffer, o_string, ch-o_string);

	//prepare the buffer for appending by adding a null to the end of it
	buffer[ch-o_string] = 0;

	//append using sprintf function
	sprintf(buffer+(ch - o_string), "%s%s", r_string, ch + strlen(s_string));

	//empty o_string for copying
	o_string[0] = 0;
	strcpy(o_string, buffer);
	//pass recursively to replace other occurrences
	return replace(o_string, s_string, r_string);
}

int changeServerPort(char *buf)
{
	char *path  = NULL;
	if(strtok(buf, ":"))
	{
		path = strtok(NULL, ":");
		if(path)
			path = strdup(path);
	}
	if(atoi(path) != 443)
		return 80;
	else
		return 443;
}

char *getPathOfGetRequest(char *buf)
{
	char *path  = NULL;
	replace(buf, "http://","");
	printf("buf--->%s\n", buf);
	if(strtok(buf, " /"))
	{
		path = strtok(NULL, " /");
		if(path)
			path = strdup(path);
	}
	return path;
}
char *getPathOfGetRequest2(char *buf)
{
	char *path  = NULL;
	if(strtok(buf, "/"))
	{
		path = strtok(NULL, "/");
		if(path)
			path = strdup(path);
	}
	return path;
}

char *getUrn(char *url){
	char *path  = NULL;
	replace(url,"http://","");
	if(strtok(url, " "))
	{
		path = strtok(NULL, " ");
		if(path)
			path = strdup(path);
	}
	return path;
}
char *getUri(char *url){
	char *path  = NULL;
	replace(url,"http://","");
	if(strtok(url, "/"))
	{
		path = strtok(NULL, "/");
		if(path)
			path = strdup(path);
	}
	return path;
}

char * getLocation(char acumulaResposta[LEN]){
	char *path  = NULL;
	char headerContent[LEN];
	int i;
	strcpy(headerContent, acumulaResposta);

	if(strstr(headerContent, "Location:") != NULL){
		if(strtok(strstr(headerContent,"Location:"), " "))
		{	
			//for (i = 0; i < 6; ++i)
			//{
				path = strtok(NULL, " ");
				if(path)
					path = strdup(path);
			//}
		}
	}
	else
		return NULL;
	replace(path, "Date:", "");

	return trim(path);
}

int main(int argc, char **argv){

	int socket_proxy;
	int socket_servidor_remoto;
	int cliente_accept;
	int tamanho = sizeof(clin_serv_par);
	int slen;
	int n;
	int end_bin_form;
	int proxyPort;
	int serverPort;
	int count;
	bool bypass = false;
	char* ip_bin;
	char serverResponse[CHUNKSIZE];
	char messageBrowser[CHUNKSIZE];
	char serverRequest[CHUNKSIZE];
	char* mensagem;
	char* mensagem2;
	char * location;
	char * url2;
	char * urnUrl;
	char* urn;
	char erroPalavraOfensiva[] = "<html>403 Forbidden</html>";
	int htmlofensivo;
	char acumulaResposta[LEN];
						
	/* create a hash table */
	hashtable_t *hashtable = ht_create(65536);

	/* get proxy port in second argument of terminal*/
	proxyPort = atoi(argv[1]);
	
	/* get file in first argument of terminal to execute the procedure*/
	readFileAndInsertToHash("bannedwordlist.txt", hashtable);

	socket_proxy = socket (AF_INET, SOCK_STREAM, 0);

	if (socket_proxy == -1) {
		perror ("Socket");
		exit(1);
	} else {
		printf("Socket criado com sucesso.\n");
	}

	servidor.sin_family = AF_INET;
	servidor.sin_port = htons (proxyPort);

	memset (servidor.sin_zero, 0, 8);

	if (bind (socket_proxy, (struct sockaddr*)&servidor, sizeof(servidor)) == -1) {
		perror ("Bind error");
		exit (1);
	}
	else if (bind (socket_proxy, (struct sockaddr*)&servidor, sizeof(servidor)) == 0) {
		printf("Sucesso....\n");
	}

	listen (socket_proxy, 3);

	/*----------------------------------INICIO BROWSER - PROXY---------------------------------------------------------*/
	while(1){
		if(!(bypass)) {
			if ((cliente_accept = accept (socket_proxy, (struct sockaddr*)&clin_serv_par, &tamanho)) == -1) {
				perror ("Accept error");
				//exit (1);
			}
			memset(messageBrowser, 0, CHUNKSIZE);

			n = read (cliente_accept, messageBrowser, LEN-1);
		}
		if((n <= 0) && !(bypass)) {
			perror("read error");
			printf("No data from browser\n");
		}
		else {
//			printf("Mensagem browser----------->%s\n", messageBrowser);
			/*----------------------------------END BROWSER - PROXY---------------------------------------------------------*/
			if(!(bypass) ){
				char* url = messageBrowser;
				urnUrl = (char *) malloc(1 + strlen(messageBrowser) );
				strcpy(urnUrl, messageBrowser);
				char * portaUrl = (char *) malloc(1 + strlen(messageBrowser) );
				strcpy(portaUrl, messageBrowser);
				serverPort = changeServerPort(portaUrl);
				url2 = getPathOfGetRequest(url);
				if(serverPort == 443) replace(url2,":443","");
				//printf("Url2------->%s\n", url2);	
			}
			else {
				strcpy(urnUrl,url2);
				//printf("url2 --->%s\n", url2);
				char* urlPass;
				urlPass = (char *) malloc(1 + strlen(url2));
				strcpy(urlPass, url2);
				url2 = getPathOfGetRequest2(urlPass);
				replace(urnUrl,"http://","");
			}
			if (url2 == NULL) {
				perror ("Mensagem browser is NULL");
				//exit (1);
			}else{
				host_ent = gethostbyname(url2);
				if(!(bypass)) urn = getUrn(urnUrl);
				else urn = getUri(urnUrl);
				replace(urn, url2, "");
				
				/*---------------------------------INICIO PROXY - SERVIDOR------------------------------------------------*/
				if (host_ent == NULL) {
					herror ("ERROR - IP nao obtido");
					//exit (1);
				}
				else{
					//CRIACAO SOCKET CLIENTE( PROXY PARA SERVIDOR WEB)
					socket_servidor_remoto = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					socket_erro2(socket_servidor_remoto);
					lista_addr = (struct in_addr**)host_ent->h_addr_list;
					if(lista_addr == NULL){
						perror("LISTA DE IP VAZIA!");
					}else{
						ip_bin = inet_ntoa(*lista_addr[0]);
						if(serverPort < 80) serverPort = 80;
						socket_inicialization(ip_bin, serverPort);
						end_bin_form = inet_pton (AF_INET,ip_bin,(void *)(&(sock_cli.sin_addr.s_addr)));

						int conexao_servidor = connect (socket_servidor_remoto, (struct sockaddr*)&sock_cli, sizeof(struct sockaddr));

						//mensagem = "GET / HTTP/1.O\r\n\r\n";

						if(serverPort == 443) {
							mensagem = "CONNECT ";
							replace(urn,":443","/");
						}
						else mensagem = "GET ";
						if(bypass) mensagem = "GET /";
						char * novamensagem0 = (char *) malloc(1 + strlen(mensagem)+ strlen(urn) );
						strcpy(novamensagem0, mensagem); // urn ja tem GET /... HTTP/1.0
						strcat(novamensagem0, urn); // estamos add o \r\n

						mensagem2 = " HTTP/1.0\r\nHost:";
						char * novamensagem1 = (char *) malloc(1 + strlen(novamensagem0)+ strlen(mensagem2));
						strcpy(novamensagem1, novamensagem0); // add Host , ficando GET /.... HTTP/1.0\r\n Host:
						strcat(novamensagem1, mensagem2);

						char * novamensagem = (char *) malloc(1 + strlen(novamensagem1)+ strlen(url2) );
						strcpy(novamensagem, novamensagem1); // add url2, ficando GET /... HTTP/1.0\r\n Host:www.xxx.com
						strcat(novamensagem, url2);

						char * temp = "\r\n\r\n";
						char * novamensagem2 = (char *) malloc(1 + strlen(novamensagem)+ strlen(temp) );
						strcpy(novamensagem2, novamensagem); // add \r\n\r\n no final
						strcat(novamensagem2, temp);
						
						memset(serverRequest, 0, CHUNKSIZE);
						strcpy (serverRequest, novamensagem2);
						
						if( send(socket_servidor_remoto , serverRequest , CHUNKSIZE , 0) < 0)
						{
							puts("Send failed");
							//return 1;
						}

						printf("Server Request send to remote server----->%s\n", serverRequest);
						memset(serverResponse, 0x0, CHUNKSIZE);
						memset(acumulaResposta, 0, sizeof(acumulaResposta));
						while(n=recv(socket_servidor_remoto , serverResponse , sizeof(serverResponse) , 0) > 0){
							replace(serverResponse,"GET",""); // something strange here, the reply contains GET
							strcat(acumulaResposta, serverResponse); // acumulate each chunk received in acumulaResposta variable
							htmlofensivo = verifyHTMLIsOffensive(serverResponse, hashtable);
							if(htmlofensivo) {//html contains ofensive contents, it will send Forbiden reply to browser
								write (cliente_accept, erroPalavraOfensiva, sizeof(erroPalavraOfensiva));
								close(cliente_accept);
								break;
							}
							memset(serverResponse, 0, sizeof(serverResponse));
						}
						
						if(!(htmlofensivo))
						{	
							url2 = getLocation(acumulaResposta);
							if(url2 == NULL){
								bypass = false;
								//printf("Reply to browser:%s\n",acumulaResposta );
								send(cliente_accept, acumulaResposta, sizeof(acumulaResposta),0);		
								close(cliente_accept);
							}
							else{ // activate bypass mode to redirect website using Location information
								bypass = true;
							}
						}
						 
						close(socket_servidor_remoto);
					}
				}
			}
		}
	}

	close(socket_proxy);
}

