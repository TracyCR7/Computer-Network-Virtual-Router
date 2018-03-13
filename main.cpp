// UDP包 服务器端
//项目属性的参数 内需添加：E:/Dev-Cpp/MinGW64/x86_64-w64-mingw32/lib/libws2_32.a
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
	char routerName[30];//对面路由器的名字
	int ttl;//失效时间
	int cost;//代价
	char ipAddr[30]; //对面路由的IP地址
	int portNum;//对面路由的端口号
	int rePortNum;//自己接收消息用的端口号
};

struct Routermsg {  //每个路由拥有的各路由信息
	char selfname[30];  //自己的路由名字
	char destname[30];  //对方的路由名字
	int ttl;
	int cost;
};

struct SpanningTreeData {
	char rootRouterName[30];   //根路由器名字
	int depth; //树的深度
	int ttl; //失效时间
	int portTTL[MAX_CONNECT_NUM];
	int parentFrom;
} spanningTreeData;


char routerName[30];//路由器的名字
int connectNum;//连接数量
int count=0;//每个路由器保存多少个路由信息

ConnectStatement connectStatement[MAX_CONNECT_NUM];
Routermsg  routermsg[MAX_CONNECT_NUM];

int boardcastMessage(char* message,int ignorePortNum = -1); //广播信息函数
int sentMessage(int routerPortNum,char* message); //发信息函数
void dealMessage(char*message,int routerPortNum); //收到信息处理函数

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

	cout<<"此路由器的名字："<<endl;
	cin>>routerName;

	cout<<"此路由器直接连接的路由器数量："<<endl;
	cin>>connectNum;
	if(connectNum>MAX_CONNECT_NUM) {
		cout<<"超出容许的连接数"<<endl;
		system("PAUSE");
		return 0;
	}

	for(int i = 0; i<connectNum; i++) {
		cout<<"此路由器直接连接的第"<<i+1<<"台路由器:"<<endl;
		cout<<"连接的路由的IP地址:";
		cin>>connectStatement[i].ipAddr;
		cout<<"连接的路由的端口号:";
		cin>>connectStatement[i].portNum;
		cout<<"自己接收消息用的端口号:";
		cin>>connectStatement[i].rePortNum;
		cout<<"代价：";
		cin>>connectStatement[i].cost;
		connectStatement[i].ttl = 0;
		connectStatement[i].routerName[0] = '\0';
	}

	//初始化套接字动态库 ,导入socket2.0库
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
			cout<<"发送广播消息："<<&bMsg[4]<<endl;
			boardcastMessage(bMsg);
		} if(str == "message") {
			char bMsg[120];
			char destRoute[30];
			char msgText[30];
			cin>>destRoute;
			cin>>msgText;
			if(strcmp(routerName,destRoute)==0) {
				cout<<"("<<routerName<<")";
				cout<<"从"<<routerName<<"到"<<routerName<<"的消息："<<msgText<<endl; 
			} else {
				sprintf(bMsg,"MSG %s %s %d %s",destRoute,routerName,50,msgText);
				string destR,nextR;
				destR = string(destRoute);
				if(nextRouter.find(destR) == nextRouter.end()) {
					cout<<"("<<routerName<<")";
					cout<<"从"<<routerName<<"到"<<destR<<"的消息发送失败：无法访问目标路由器"<<endl;
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
							cout<<"从"<<routerName<<"到"<<destR<<"的消息,已经向"<<i<<"端口["<<connectStatement[i].routerName<<"]送出"<<endl;
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
		printf("创建接收消息的套接字失败 !\n",WSAGetLastError());
		return (void*) NULL;
	}

	SOCKADDR_IN addrSrv;
	SOCKADDR_IN addrClient;
	char        buf[BUF_SIZE];
	int         len = sizeof(SOCKADDR);

	// 设置服务器地址 :
	ZeroMemory(buf,BUF_SIZE);
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(portNum);

	// 绑定套接字 :
	noReturn = bind(socketSrv,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	if(SOCKET_ERROR == noReturn) {
		printf("绑定套接字失败 !\n");
		return (void*) NULL;
	}

	//printf("就绪，等待客户端信息。\n");

	for(int index=1 ; ; index++) { //持续保持在线
		// 从客户端接收数据  :
		noReturn = recvfrom(socketSrv,buf,BUF_SIZE,0,(SOCKADDR*)&addrClient,&len);
		if(SOCKET_ERROR == noReturn) {
			printf("接收信息失败!\n");
			continue;
		}
		// 打印来自客户端发送来的数据  :
		//printf("从客户端来的第 %d 条信息：%s\n",index,buf);
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
	/*生成树所用的线程 
	使用如下结构体 
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
		监控用的线程
		最后不需要，所以直接返回 
	*/ 
	return (void*) NULL;
	
	for(;;) {

		cout<<routerName<<"连接状态"<<endl;

		cout<<"端口号\t路由名字\t代价\tTTL\t生成树端口"<<endl;
		for(int i=0; i<connectNum; i++) {

			if(connectStatement[i].ttl > 0) {
				cout<<i<<'\t'<<connectStatement[i].routerName<<'\t'<<connectStatement[i].cost<<'\t'<<connectStatement[i].ttl<<'\t'<<(spanningTreeData.portTTL[i]>0)<<endl;
			} else {
				cout<<i<<endl;
			}

		}
		cout<<"生成树根路由"<<spanningTreeData.rootRouterName<<" 父结点所连端口"<<spanningTreeData.parentFrom<<" 深度"<<spanningTreeData.depth<<endl;
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
		cout<<"源路由"<<'\t'<<"目的路由"<<'\t'<<"ttl"<<'\t'<<"代价"<<endl;
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

//完整的一个发送信息流程
int sentMessage(int routerPortNum,char* message) {

	char ipAddr[30]; //IP地址
	int rPortNum;//端口号
	int repeatTime;//发包数量

	sprintf(ipAddr,"%s",connectStatement[routerPortNum].ipAddr);
	rPortNum = connectStatement[routerPortNum].portNum;

	repeatTime = 1;

	char        buf[BUF_SIZE];  // 要发送给服务器端的数据
	SOCKADDR_IN servAddr;       // 服务器套接字地址
	SOCKET      sockClient = socket(AF_INET,SOCK_DGRAM,0);  //创建客户端套接字
	int         nRet;

	servAddr.sin_family=AF_INET; //地址家族，一般都是“AF_xxx”的形式。通常大多用的是都是AF_INET,代表TCP/IP协议族。
	servAddr.sin_addr.S_un.S_addr=inet_addr(ipAddr); //存储IP地址
	servAddr.sin_port=htons(rPortNum); // 存储端口号

	// 向服务器发送数据
	for(int i=1 ; i<=repeatTime ; i++) {
		ZeroMemory(buf,BUF_SIZE);   //用0填充一块内存区域
		sprintf(buf,"%s",message);  //将填充的内容换为字符串
		int nServAddLen = sizeof(servAddr);
		if(sendto(sockClient,buf,BUF_SIZE,0,(sockaddr *)&servAddr,nServAddLen) == SOCKET_ERROR) {
			//出错时函数返回SOCKET_ERROR
			printf("错误代码:%d\n",WSAGetLastError());
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
		cout<<"从"<<routerPort<<"端口收到从"<<rRoute<<"到"<<routerName<<"的消息："<<msgText<<endl; 
	} else {
		string destR,nextR;
		destR = string(destRoute);
		if(nextRouter.find(destR) == nextRouter.end()) {
			cout<<"("<<routerName<<")";
			cout<<"从"<<routerPort<<"端口收到从"<<rRoute<<"到"<<destR<<"的消息发送失败：无法访问目标路由器"<<endl;
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
					cout<<"从"<<routerPort<<"端口收到从"<<rRoute<<"到"<<destR
					<<"的消息,已经向"<<i<<"端口["<<connectStatement[i].routerName<<"]送出"<<endl;
					break;
				}
			}
		} 
	}
}

void dealMessage(char*message,int routerPortNum) {
	if(message[0] == 'H' && message[1] == 'R' && message[2] == 'T') {
		//心跳包处理
		dealHeartMessage(message,routerPortNum);
	} else if(message[0] == 'S' && message[1] == 'P' && message[2] == 'T') {
		//生成树请求
		dealSpanningTreeMessage(message,routerPortNum);
	} else if(message[0] == 'S' && message[1] == 'P' && message[2] == 'R') {
		//生成树接受回复
		dealSTRMessage(message,routerPortNum);
	} else if(message[0] == 'B' && message[1] == 'R' && message[2] == 'D') {
		//生成树接受回复
		dealBoardcastMessage(message,routerPortNum);
	} else if(message[0] == 'R' && message[1] == 'O' && message[2] == 'U') {
		dealRouterMessage(message,routerPortNum);
	} else if(message[0] == 'M' && message[1] == 'S' && message[2] == 'G') {
		dealMessageText(message,routerPortNum);
	} else {
		//cout<<"收到未知消息类型："<<message<<endl;
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
	printf("收到广播消息：%s\n",&message[4]);
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
		sentMessage(routerPort,SPRMsg); //回复接受其作为生成树父结点

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
 //定义图 的邻接矩阵表示法结构
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
	//输入图的顶点
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
    /*cout<<"输出图顶点set集合："<<endl;
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
	
/*	cout<<"输出路由对应序号 :"<<endl;
	iter = temp.begin();
	while(iter != temp.end()){
		cout<<*iter<<" "<<R2N[*iter]<<endl;
		iter++;
	}*/ 
	
	
	VertexNum = temp.size();
	//输入相应的的邻接矩阵
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
 * Dijkstra最短路径。
 * 即，统计图(G)中"顶点vs"到其它各个顶点的最短路径。
 *
 * 参数说明：
 *        G -- 图
 *       vs -- 起始顶点(start vertex)。即计算"顶点vs"到其它顶点的最短路径。
 *     prev -- 前驱顶点数组。即，prev[i]的值是"顶点vs"到"顶点i"的最短路径所经历的全部顶点中，位于"顶点i"之前的那个顶点。
 *     dist -- 长度数组。即，dist[i]是"顶点vs"到"顶点i"的最短路径的长度。
 */

int prev[max_router_num];
int dist[max_router_num];
void Dijkstra(int v)
{
	/*cout<<"G的邻接矩阵："<<endl;
	for(int in = 0 ; in < VertexNum ; in++){
		for(int j = 0 ; j < VertexNum ; j++){
			cout<<G.edg[in][j]<<" ";
		}
		cout<<endl;
	} */
 
    bool s[VertexNum];    // 判断是否已存入该点到S集合中
    for(int i = 0; i < VertexNum; ++i)
    {
        dist[i] = G.edg[v][i];
        s[i] = 0;     // 初始都未用过该点
        if(dist[i] == 500000)
            prev[i] = -1;
        else
            prev[i] = v;
    }
    dist[v] = 0;
    s[v] = 1;
 
    // 依次将未放入S集合的结点中，取dist[]最小值的结点，放入结合S中
    // 一旦S包含了所有V中顶点，dist就记录了从源点到所有其他顶点之间的最短路径长度

    for(int i = 1; i < VertexNum; ++i)
    {
        int tmp = 500000;
        int u = v;
        // 找出当前未使用的点j的dist[j]最小值
        for(int j = 0; j < VertexNum; ++j)
            if((!s[j]) && dist[j]<tmp)
            {
                u = j;              // u保存当前邻接点中距离最小的点的号码
                tmp = dist[j];
            }
        s[u] = 1;    // 表示u点已存入S集合中
        // 更新dist
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
