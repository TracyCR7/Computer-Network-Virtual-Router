// UDP�� ��������
//��Ŀ���ԵĲ��� ������ӣ�E:/Dev-Cpp/MinGW64/x86_64-w64-mingw32/lib/libws2_32.a
#include <stdio.h>
#include <WINSOCK2.H>
#include <stdlib.h>
#include <winsock2.h>
#include <time.h>
#include <math.h>
#include <iostream>
#include <string.h>
#include <set>
#include <map>
#include <vector> 
using namespace std;
#define BUF_SIZE    128
#define MAX_CONNECT_NUM 10
#define max_router_num 50
#define MAXcost 1000000000

map<string,string> nextRouter;

struct ConnectStatement {
	char routerName[30];//����·����������
	int ttl;//ʧЧʱ��
	int cost;//����
	char ipAddr[30]; //����·�ɵ�IP��ַ
	int portNum;//����·�ɵĶ˿ں�
	int rePortNum;//�Լ�������Ϣ�õĶ˿ں�
};

struct Routermsg {  //ÿ��·��ӵ�еĸ�·����Ϣ
	char selfname[30];  //�Լ���·������
	char destname[30];  //�Է���·������
	int ttl;
	int cost;
};

struct SpanningTreeData {
	char rootRouterName[30];   //��·��������
	int depth; //�������
	int ttl; //ʧЧʱ��
	int portTTL[MAX_CONNECT_NUM];
	int parentFrom;
} spanningTreeData;


char routerName[30];//·����������
int connectNum;//��������
int count=0;//ÿ��·����������ٸ�·����Ϣ

ConnectStatement connectStatement[MAX_CONNECT_NUM];
Routermsg  routermsg[MAX_CONNECT_NUM];

int boardcastMessage(char* message,int ignorePortNum = -1); //�㲥��Ϣ����
int sentMessage(int routerPortNum,char* message); //����Ϣ����
void dealMessage(char*message,int routerPortNum); //�յ���Ϣ������

void dealBoardcastMessage(char*message,int routerPort);
void dealHeartMessage(char*message,int routerPort);
void dealSpanningTreeMessage(char*message,int routerPort);
void dealSTRMessage(char*message,int routerPort);
void dealRouterMessage(char * message,int routerPort);

void * Reciever(void *);
void * Connecter(void *);
void * SpanningTree(void *);
void * Monitor(void *);
void * sendRouter(void *);
void * Router(void *);

char* Routermsg_to_char(Routermsg r);
Routermsg char_to_Routermsg(char* message);


void* RunDijkstra(void*);

int main() {

	WSADATA wsd;

	pthread_t pReciever[MAX_CONNECT_NUM],pConnecter,pSpanningTree,pMonitor,pRouter,psendRouter,pRunDijkstra;
	int pidNums[MAX_CONNECT_NUM];

	cout<<"��·���������֣�"<<endl;
	cin>>routerName;

	cout<<"��·����ֱ�����ӵ�·����������"<<endl;
	cin>>connectNum;
	if(connectNum>MAX_CONNECT_NUM) {
		cout<<"���������������"<<endl;
		system("PAUSE");
		return 0;
	}

	for(int i = 0; i<connectNum; i++) {
		cout<<"��·����ֱ�����ӵĵ�"<<i+1<<"̨·����:"<<endl;
		cout<<"���ӵ�·�ɵ�IP��ַ:";
		cin>>connectStatement[i].ipAddr;
		cout<<"���ӵ�·�ɵĶ˿ں�:";
		cin>>connectStatement[i].portNum;
		cout<<"�Լ�������Ϣ�õĶ˿ں�:";
		cin>>connectStatement[i].rePortNum;
		cout<<"���ۣ�";
		cin>>connectStatement[i].cost;
		connectStatement[i].ttl = 0;
		connectStatement[i].routerName[0] = '\0';
	}

	//��ʼ���׽��ֶ�̬�� ,����socket2.0��
	if(WSAStartup(MAKEWORD(2,2),&wsd)!=0) {
		printf("failed to load winsock!\n");
		return 1;
	}


	for(int i =0; i<connectNum; i++) {
		pidNums[i] = i;
		pthread_create(&pReciever[i],NULL,Reciever,(void *)&pidNums[i]);
	}


	pthread_create(&pConnecter,NULL,Connecter,(void *)NULL);
	pthread_create(&pSpanningTree,NULL,SpanningTree,(void *)NULL);
	pthread_create(&pMonitor,NULL,Monitor,(void *)NULL);
	pthread_create(&psendRouter,NULL,sendRouter,(void *)NULL);
	pthread_create(&pRouter,NULL,Router,(void *)NULL);
	pthread_create(&pRunDijkstra,NULL,RunDijkstra,(void *)NULL);
		
	string str;

	for(;;) {
		cin>>str;
		if(str == "boardcast") {
			char bMsg[60];
			strcpy(bMsg,"BRD ");
			cin>>&bMsg[4];
			cout<<"("<<routerName<<")";
			cout<<"���͹㲥��Ϣ��"<<&bMsg[4]<<endl;
			boardcastMessage(bMsg);
		} if(str == "message") {
			char bMsg[120];
			char destRoute[30];
			char msgText[30];
			cin>>destRoute;
			cin>>msgText;
			if(strcmp(routerName,destRoute)==0) {
				cout<<"("<<routerName<<")";
				cout<<"��"<<routerName<<"��"<<routerName<<"����Ϣ��"<<msgText<<endl; 
			} else {
				sprintf(bMsg,"MSG %s %s %d %s",destRoute,routerName,50,msgText);
				string destR,nextR;
				destR = string(destRoute);
				if(nextRouter.find(destR) == nextRouter.end()) {
					cout<<"("<<routerName<<")";
					cout<<"��"<<routerName<<"��"<<destR<<"����Ϣ����ʧ�ܣ��޷�����Ŀ��·����"<<endl;
				} else {
					nextR = nextRouter[destR];
					char nextRC[30];
					strcpy(nextRC,nextR.c_str());
					
					if(strcmp(nextRC,routerName)==0) {
						strcpy(nextRC,destR.c_str());
					}
					
					for(int i=0;i<connectNum;i++) {
						if(strcmp(connectStatement[i].routerName,nextRC) == 0) {
							sentMessage(i,bMsg);
							cout<<"("<<routerName<<")";
							cout<<"��"<<routerName<<"��"<<destR<<"����Ϣ,�Ѿ���"<<i<<"�˿�["<<connectStatement[i].routerName<<"]�ͳ�"<<endl;
							break;
						}
					}
				} 
			}
		} else {
			//cout<<str<<endl;
		}

	}

	for(int i =0; i<connectNum; i++) {
		pthread_join(pReciever[i],NULL);
	}
	pthread_join(pConnecter,NULL);


	WSACleanup();

	return 0;
}

void * Reciever(void * routerPortNumber) {

	int ri = *((int*)routerPortNumber);

	int portNum = connectStatement[ri].rePortNum;
		
	char retMessage[1000];
	int     noReturn;

	SOCKET socketSrv = socket(AF_INET,SOCK_DGRAM,0);
	if(socketSrv == INVALID_SOCKET) {
		printf("����������Ϣ���׽���ʧ�� !\n",WSAGetLastError());
		return (void*) NULL;
	}

	SOCKADDR_IN addrSrv;
	SOCKADDR_IN addrClient;
	char        buf[BUF_SIZE];
	int         len = sizeof(SOCKADDR);

	// ���÷�������ַ :
	ZeroMemory(buf,BUF_SIZE);
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(portNum);

	// ���׽��� :
	noReturn = bind(socketSrv,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	if(SOCKET_ERROR == noReturn) {
		printf("���׽���ʧ�� !\n");
		return (void*) NULL;
	}

	//printf("�������ȴ��ͻ�����Ϣ��\n");

	for(int index=1 ; ; index++) { //������������
		// �ӿͻ��˽�������  :
		noReturn = recvfrom(socketSrv,buf,BUF_SIZE,0,(SOCKADDR*)&addrClient,&len);
		if(SOCKET_ERROR == noReturn) {
			printf("������Ϣʧ��!\n");
			continue;
		}
		// ��ӡ���Կͻ��˷�����������  :
		//printf("�ӿͻ������ĵ� %d ����Ϣ��%s\n",index,buf);
		dealMessage(buf,ri);

	}

	closesocket(socketSrv);

	return (void*) NULL;
}

void * Connecter(void *) {

	char message[40];
	sprintf(message,"HRT %s",routerName);

	for(;;) {
		for(int i=0; i<connectNum; i++) {
			sentMessage(i,message);

			if(connectStatement[i].ttl > 0) {
				connectStatement[i].ttl --;
			}

		}

		Sleep(1000);

	}

	return (void*) NULL;
}

void * SpanningTree(void *) {
	/*���������õ��߳� 
	ʹ�����½ṹ�� 
	struct SpanningTreeData {
		char rootRouterName[30];
		int depth;
		int ttl;
		bool isSTPort[MAX_CONNECT_NUM];
	} spanningTreeData; */

	char message[90];
	spanningTreeData.ttl = 0;

	for(;;) {
		spanningTreeData.ttl --;
		if(spanningTreeData.ttl <= 0) {
			strcpy(spanningTreeData.rootRouterName,routerName);
			spanningTreeData.depth = 0;
			for(int i =0; i<connectNum; i++) {
				spanningTreeData.portTTL[i] = 0;
			}
			spanningTreeData.parentFrom = 0;
			spanningTreeData.ttl = 1;
			for(int i =0; i<connectNum; i++) {
				sprintf(message,"SPT %s %d",spanningTreeData.rootRouterName,spanningTreeData.depth);
				sentMessage(i,message);
			}

		}
		for(int i =0; i<connectNum; i++) {
			if(spanningTreeData.portTTL>0) {
				spanningTreeData.portTTL[i] --;
			}
		}

		Sleep(1000);
	}

}

void * Monitor(void *) {
	/*
		����õ��߳�
		�����Ҫ������ֱ�ӷ��� 
	*/ 
	return (void*) NULL;
	
	for(;;) {

		cout<<routerName<<"����״̬"<<endl;

		cout<<"�˿ں�\t·������\t����\tTTL\t�������˿�"<<endl;
		for(int i=0; i<connectNum; i++) {

			if(connectStatement[i].ttl > 0) {
				cout<<i<<'\t'<<connectStatement[i].routerName<<'\t'<<connectStatement[i].cost<<'\t'<<connectStatement[i].ttl<<'\t'<<(spanningTreeData.portTTL[i]>0)<<endl;
			} else {
				cout<<i<<endl;
			}

		}
		cout<<"��������·��"<<spanningTreeData.rootRouterName<<" ����������˿�"<<spanningTreeData.parentFrom<<" ���"<<spanningTreeData.depth<<endl;
		Sleep(10000);

	}

	return (void*) NULL;
}

void * sendRouter(void *) {

	char message[100];
	for(;;) {

		/*for(int i=0 ; i<connectNum ; i++) {
			strcpy(routermsg[i].selfname,routerName);
			strcpy(routermsg[i].destname,connectStatement[i].routerName);
			routermsg[i].ttl=0;
			routermsg[i].cost=connectStatement[i].cost;
		}
		count+=connectNum;*/

		for(int i=0 ; i<connectNum  ; i++) {
			if(connectStatement[i].ttl>0)  {
				sprintf(message,"ROU %s %s %d",routerName,connectStatement[i].routerName,connectStatement[i].cost);
				strcpy(routermsg[count].selfname,routerName);
				strcpy(routermsg[count].destname,connectStatement[i].routerName);
				routermsg[count].ttl=2;
				routermsg[count].cost=connectStatement[i].cost;
				count++;
				boardcastMessage(message);
			}
		}

		for(int i=0 ; i<count ; i++) {
			if(routermsg[i].ttl>0) {
				routermsg[i].ttl--;
			}
		}
		int j=0;
		for(int i=0 ; i<count ; i++) {
			if(routermsg[i].ttl>0) {
				routermsg[j]=routermsg[i];
				j++;
			}
		}
		count=j;
		Sleep(10000);
	}

}

void * Router(void *) {
	return (void*) NULL;
	for(;;) {
		cout<<"Դ·��"<<'\t'<<"Ŀ��·��"<<'\t'<<"ttl"<<'\t'<<"����"<<endl;
		for(int i=0 ; i<count ; i++) {
			if(routermsg[i].ttl>0) {
				cout<<routermsg[i].selfname<<'\t'<<routermsg[i].destname<<'\t'<<routermsg[i].ttl<<'\t'<<routermsg[i].cost<<endl;
			} else {
				//cout<<i<<endl;
			}
		}
//		(*RunDijkstra)(NULL);
		Sleep(10000);
	}
	return (void*) NULL;
}

int boardcastMessage(char* message,int ignorePortNum) {
	for(int i =0 ; i<connectNum; i++) {
		if(spanningTreeData.portTTL[i]>0 && i != ignorePortNum) {
			sentMessage(i,message);
		}
	}
	return 0;
}

//������һ��������Ϣ����
int sentMessage(int routerPortNum,char* message) {

	char ipAddr[30]; //IP��ַ
	int rPortNum;//�˿ں�
	int repeatTime;//��������

	sprintf(ipAddr,"%s",connectStatement[routerPortNum].ipAddr);
	rPortNum = connectStatement[routerPortNum].portNum;

	repeatTime = 1;

	char        buf[BUF_SIZE];  // Ҫ���͸��������˵�����
	SOCKADDR_IN servAddr;       // �������׽��ֵ�ַ
	SOCKET      sockClient = socket(AF_INET,SOCK_DGRAM,0);  //�����ͻ����׽���
	int         nRet;

	servAddr.sin_family=AF_INET; //��ַ���壬һ�㶼�ǡ�AF_xxx������ʽ��ͨ������õ��Ƕ���AF_INET,����TCP/IPЭ���塣
	servAddr.sin_addr.S_un.S_addr=inet_addr(ipAddr); //�洢IP��ַ
	servAddr.sin_port=htons(rPortNum); // �洢�˿ں�

	// ���������������
	for(int i=1 ; i<=repeatTime ; i++) {
		ZeroMemory(buf,BUF_SIZE);   //��0���һ���ڴ�����
		sprintf(buf,"%s",message);  //���������ݻ�Ϊ�ַ���
		int nServAddLen = sizeof(servAddr);
		if(sendto(sockClient,buf,BUF_SIZE,0,(sockaddr *)&servAddr,nServAddLen) == SOCKET_ERROR) {
			//����ʱ��������SOCKET_ERROR
			printf("�������:%d\n",WSAGetLastError());
			continue;
		}
	}

	return 0;
}

void dealMessageText(char* message,int routerPort) {
	char destRoute[30],rRoute[30],msgText[120];
	int ttl;
	char newMessage[190];
	sscanf(&message[4],"%s %s %d %s",destRoute,rRoute,&ttl,msgText); 
	
	if(ttl<=0) {
		return;
	}
	
	sprintf(newMessage,"MSG %s %s %d %s",destRoute,rRoute,ttl - 1,msgText);
	
	if(strcmp(routerName,destRoute)==0) {
		cout<<"("<<routerName<<")";
		cout<<"��"<<routerPort<<"�˿��յ���"<<rRoute<<"��"<<routerName<<"����Ϣ��"<<msgText<<endl; 
	} else {
		string destR,nextR;
		destR = string(destRoute);
		if(nextRouter.find(destR) == nextRouter.end()) {
			cout<<"("<<routerName<<")";
			cout<<"��"<<routerPort<<"�˿��յ���"<<rRoute<<"��"<<destR<<"����Ϣ����ʧ�ܣ��޷�����Ŀ��·����"<<endl;
		} else {
			nextR = nextRouter[destR];
			char nextRC[30];
			strcpy(nextRC,nextR.c_str());
			
			if(strcmp(nextRC,routerName)==0) {
				strcpy(nextRC,destR.c_str());
			}
			
			for(int i=0;i<connectNum;i++) {
				if(strcmp(connectStatement[i].routerName,nextRC)==0) {
					sentMessage(i,newMessage);
					cout<<"("<<routerName<<")";
					cout<<"��"<<routerPort<<"�˿��յ���"<<rRoute<<"��"<<destR
					<<"����Ϣ,�Ѿ���"<<i<<"�˿�["<<connectStatement[i].routerName<<"]�ͳ�"<<endl;
					break;
				}
			}
		} 
	}
}

void dealMessage(char*message,int routerPortNum) {
	if(message[0] == 'H' && message[1] == 'R' && message[2] == 'T') {
		//����������
		dealHeartMessage(message,routerPortNum);
	} else if(message[0] == 'S' && message[1] == 'P' && message[2] == 'T') {
		//����������
		dealSpanningTreeMessage(message,routerPortNum);
	} else if(message[0] == 'S' && message[1] == 'P' && message[2] == 'R') {
		//���������ܻظ�
		dealSTRMessage(message,routerPortNum);
	} else if(message[0] == 'B' && message[1] == 'R' && message[2] == 'D') {
		//���������ܻظ�
		dealBoardcastMessage(message,routerPortNum);
	} else if(message[0] == 'R' && message[1] == 'O' && message[2] == 'U') {
		dealRouterMessage(message,routerPortNum);
	} else if(message[0] == 'M' && message[1] == 'S' && message[2] == 'G') {
		dealMessageText(message,routerPortNum);
	} else {
		//cout<<"�յ�δ֪��Ϣ���ͣ�"<<message<<endl;
	}
}
void dealRouterMessage(char* message,int routerPort) {
	char a[100],b[100];
	int c;
	sscanf(&message[4],"%s %s %d",a,b,&c);
	int i=0;
	for(i=0 ; i<count ; i++) {
		if(strcmp(routermsg[i].selfname,a)==0 && strcmp(routermsg[i].destname,b)==0) {
			break;
		}
	}

	strcpy(routermsg[i].selfname,a);
	strcpy(routermsg[i].destname,b);
	routermsg[i].ttl=3;
	routermsg[i].cost=c;
	if(i==count)
		count++;

	boardcastMessage(message,routerPort);
}

void dealBoardcastMessage(char*message,int routerPort) {
	cout<<"("<<routerName<<")";
	printf("�յ��㲥��Ϣ��%s\n",&message[4]);
	boardcastMessage(message,routerPort);
}

void dealHeartMessage(char*message,int routerPort) {
	sscanf(&message[4],"%s",connectStatement[routerPort].routerName);
	connectStatement[routerPort].ttl = 3;
}

void dealSpanningTreeMessage(char*message,int routerPort) {

	/*struct SpanningTreeData {
		char rootRouterName[30];
		int depth;
		int ttl;
		bool isSTPort[MAX_CONNECT_NUM];
	} spanningTreeData; */

	char rName[30];
	int depth;
	bool change = false;

	sscanf(&message[4],"%s %d",rName,&depth);

	if(strcmp(rName,spanningTreeData.rootRouterName)<0) {
		change = true;
	} else if(strcmp(rName,spanningTreeData.rootRouterName)==0) {
		if(spanningTreeData.depth > depth+1) {
			change = true;
		} else if(spanningTreeData.depth == depth+1 && spanningTreeData.parentFrom == routerPort) {
			change = true;
		}
	}

	if(change) {
		char SPRMsg[6] = "SPR ";
		strcpy(spanningTreeData.rootRouterName,rName);
		spanningTreeData.ttl = 3;
		spanningTreeData.depth = depth+1;
		spanningTreeData.parentFrom = routerPort;
		spanningTreeData.portTTL[routerPort] = 3;
		sentMessage(routerPort,SPRMsg); //�ظ���������Ϊ�����������

		for(int i =0; i<connectNum; i++) {
			if(i==routerPort) {
				continue;
			}
			sprintf(message,"SPT %s %d",spanningTreeData.rootRouterName,spanningTreeData.depth);
			sentMessage(i,message);
		}
	}
}

void dealSTRMessage(char*message,int routerPort) {
	spanningTreeData.portTTL[routerPort] = 3;
}


#define MaxSize 60
set<string> temp;
typedef string VertexType;
 //����ͼ ���ڽӾ����ʾ���ṹ
typedef struct Graph {
	VertexType ver[MaxSize+1];
	int edg[MaxSize][MaxSize];
}Graph;
Graph G;
int VertexNum;
map<string,int> R2N;
string to_String(char* a){
	return string(a);
	/*string ans;
	int i = 0;
	while(a[i] != '\0'){
		ans += a[i]; 
		i++; 
	}
	ans += '\0'; 
	return ans;*/
} 
void CreateGraph()
{
	int i = 0;
	int j = 0;
	VertexType Ver;
	//����ͼ�Ķ���
	for(int in = 0 ; in < count ; in++) {
		if(routermsg[in].ttl>0){
				temp.insert(to_String(routermsg[in].selfname));
				temp.insert(to_String(routermsg[in].destname));
		}
	}
	set<string>::iterator iter = temp.begin();  
	 while(iter != temp.end())  
    {  
		G.ver[i++] = *iter;
		iter++;	
    }  		
    /*cout<<"���ͼ����set���ϣ�"<<endl;
    iter = temp.begin();  
	 while(iter != temp.end())  
    {  
		cout<<*iter<<endl;
		iter++;	
    }*/ 
	//g.ver[i] = '\0';
	
	iter = temp.begin();
	int num = 0;
	while(iter != temp.end()){
		R2N[*iter] = num;
		iter++;num++;
	}
	
/*	cout<<"���·�ɶ�Ӧ��� :"<<endl;
	iter = temp.begin();
	while(iter != temp.end()){
		cout<<*iter<<" "<<R2N[*iter]<<endl;
		iter++;
	}*/ 
	
	
	VertexNum = temp.size();
	//������Ӧ�ĵ��ڽӾ���
	for( i=0; i<VertexNum; i++ )
		for(j=0; j<VertexNum; j++ )
			 if(i != j) 
			 	G.edg[i][j] = 500000;	
			 else G.edg[i][j] = 0;
			 
	for(int in = 0 ; in < count ; in++) {
		if(routermsg[in].ttl>0){
			G.edg[R2N[to_String(routermsg[in].selfname)]][R2N[to_String(routermsg[in].destname)]] = routermsg[in].cost;
		}
	}
	

}


/*
 * Dijkstra���·����
 * ����ͳ��ͼ(G)��"����vs"������������������·����
 *
 * ����˵����
 *        G -- ͼ
 *       vs -- ��ʼ����(start vertex)��������"����vs"��������������·����
 *     prev -- ǰ���������顣����prev[i]��ֵ��"����vs"��"����i"�����·����������ȫ�������У�λ��"����i"֮ǰ���Ǹ����㡣
 *     dist -- �������顣����dist[i]��"����vs"��"����i"�����·���ĳ��ȡ�
 */

int prev[max_router_num];
int dist[max_router_num];
void Dijkstra(int v)
{
	/*cout<<"G���ڽӾ���"<<endl;
	for(int in = 0 ; in < VertexNum ; in++){
		for(int j = 0 ; j < VertexNum ; j++){
			cout<<G.edg[in][j]<<" ";
		}
		cout<<endl;
	} */
 
    bool s[VertexNum];    // �ж��Ƿ��Ѵ���õ㵽S������
    for(int i = 0; i < VertexNum; ++i)
    {
        dist[i] = G.edg[v][i];
        s[i] = 0;     // ��ʼ��δ�ù��õ�
        if(dist[i] == 500000)
            prev[i] = -1;
        else
            prev[i] = v;
    }
    dist[v] = 0;
    s[v] = 1;
 
    // ���ν�δ����S���ϵĽ���У�ȡdist[]��Сֵ�Ľ�㣬������S��
    // һ��S����������V�ж��㣬dist�ͼ�¼�˴�Դ�㵽������������֮������·������

    for(int i = 1; i < VertexNum; ++i)
    {
        int tmp = 500000;
        int u = v;
        // �ҳ���ǰδʹ�õĵ�j��dist[j]��Сֵ
        for(int j = 0; j < VertexNum; ++j)
            if((!s[j]) && dist[j]<tmp)
            {
                u = j;              // u���浱ǰ�ڽӵ��о�����С�ĵ�ĺ���
                tmp = dist[j];
            }
        s[u] = 1;    // ��ʾu���Ѵ���S������
        // ����dist
        for(int j = 0 ; j < VertexNum; ++j)
            if((!s[j]) && G.edg[u][j]<500000)
            {
                int newdist = dist[u] + G.edg[u][j];
                if(newdist < dist[j])
                {
                    dist[j] = newdist;
                    prev[j] = u;
                }
            }
    }
    
    /*cout<<endl;
	for(int i = 0 ; i < VertexNum ; i++){
		cout<<"dist["<<i<<"] = "<<dist[i]<<endl;
	} 
	for(int i = 0 ; i < VertexNum ; i++){
		cout<<"prev["<<i<<"] = "<<prev[i]<<endl;
	} */
	
}

string N2R(int node){
	set<string>::iterator iter = temp.begin();  
	for(int k = 0 ; k < node ; k++){
		iter++;
	}
	return *iter;
}
void Goto(){
	for(int i = 0 ; i < VertexNum ; i++){
		string destination = N2R(i);
		if(destination == to_String(routerName)) continue;
		int tempprev = prev[i];
		while(prev[tempprev] != R2N[to_String(routerName)]){
			tempprev = prev[prev[i]];
		}
		nextRouter[destination] = N2R(tempprev);
	}
	
/*	for(int i = 0 ; i < VertexNum ; i++){
		cout<<"nextRouter["<<N2R(i)<<"] = <"<<nextRouter[N2R(i)]<<">"<<endl; 
	}*/
}
void * RunDijkstra(void *){
	while(1){
		
		temp.clear();
		R2N.clear();
		nextRouter.clear();
		
		CreateGraph();
		Dijkstra(R2N[to_String(routerName)]);	
	//	cout<<"11111111111111111111"<<endl; 
		Goto();
		Sleep(10000); 
	}
}
