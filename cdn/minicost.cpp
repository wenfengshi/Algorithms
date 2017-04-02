//
// Created by Wenfeng Shi on 17/4/2.
//

#include "minicost.h"
#include <stdio.h>
#include <stdlib.h>
#include <deque>
#include <string>
#include <map>
#include <ctime>
#include <iostream>

using namespace std;


#define MAXN (1000+2)       // 网络节点数 + S + D
#define MAXM 10000           // 5000条就行
#define INF 1000000           // 带宽消耗总需求是[0, 5000]
#define MAX_CONSUME 500     // 消费节点最大数



typedef struct edge
{
    struct edge *next;     // 下一个节点
    struct edge *pair;     // 配对边
    int to;
    int residual;   // 剩余容量
    int cost;       // 单位费用
    int flow;       // 边上的流
} edge;
edge g_edge[MAXM];      // 边集
edge *g_list[MAXN];     // 每个顶点的链表表头指针
int E_COUNT = -1;       // 边计数器, 从0开始
bool g_init_flag = true; // true表示要保存边的信息
int g_matrix[MAXM][4];    // from, to , cost, residual
int M_COUNT = -1;


typedef struct consume
{
    int netnode;
    int need;
}consume;
consume g_consume[MAX_CONSUME]; // 消费节点
map<int, int> g_consume_map;
int V_CONSUME;  // 消费节点数
int V_NUM;      // 顶点数
int SRV_COST;   // 服务器成本


// spfa && argument, 每次使用前初始化
int g_inq[MAXN];            // 每个顶点是否在队列中（1：在， 0: 不在）
int g_dist[MAXN];           // 当前最短路径
int g_path[MAXN];           // 最短路径中，当前节点的前驱节点
edge *g_path_edge[MAXN];    // 最短路径中，当前节点的前驱边
deque<int> Q;   // 队列中的节点为顶点序列号


int V_SRC;      // 源节点
int V_DST;      // 目的节点
int g_flow;     // 纪录总流量
string g_result;        // 保存路径结果
int g_flow_count = 0;   // 保存路径总数


static void dfs_path();
static int mcmf();
static void init_list_edge();
static bool spfa();
static int argument();
static int mcmf();
static void dfs_path();


char *cdn_minicost()
{

    int need_sum = 0;   // 总费用
    int min_cost = INF; // 记录全局当前最小费用
    int cdn_cost;
    int cost;
    int i;
    int n;
    clock_t start = clock();
    char buf[10];

    V_NUM += 2;
    V_DST = V_NUM - 2;
    V_SRC = V_NUM - 1;

    // 1. 添加汇聚节点
    for(i = 0; i < V_CONSUME; i++)
    {
        add_edge(g_consume[i].netnode, V_DST, 0, g_consume[i].need);
        need_sum += g_consume[i].need;
    }
    g_init_flag = false;

    // 2. 判断是否存在可行解
    for(i = 0; i < V_CONSUME; i++)
    {
        add_edge(V_SRC, g_consume[i].netnode, 0, INF);
    }
    cdn_cost = V_CONSUME * SRV_COST;
    min_cost = mcmf();
    min_cost += cdn_cost;

    if(g_flow != need_sum)
    {
        sprintf(buf, "%s", "NA");
        g_result = buf;
        return (char*)g_result.c_str();
    }
    else
    {
        dfs_path();
        n = sprintf(buf, "%d\n", g_flow_count);
        g_result.insert(0, buf, n);
        PRINT("[%d]%d\n%s\n", __LINE__, min_cost, g_result.c_str());
    }

    // 3. 迭代
    int cdn_nodes[] = {7, 13, 15, 22, 37, 38, 43};
    int cdn_num = 7;
    do
    {
        init_list_edge();

        // 选定CDN服务器位置，添加边
        cdn_cost = cdn_num * SRV_COST;
        for(i = 0; i < cdn_num; i++)
        {
            add_edge(V_SRC, cdn_nodes[i], 0, INF);
        }

        // 计算当前最大流、最小费用
        cost = mcmf();
        cost += cdn_cost;
        //PRINT("[%d]cost:%d", __LINE__, cost-cdn_cost);
        if(g_flow == need_sum && cost < min_cost)
        {
            min_cost = cost;
            // 保存路径信息
            dfs_path();
            n = sprintf(buf, "%d\n", g_flow_count);
            g_result.insert(0, buf, n);
            //PRINT("[%d]\n%s", __LINE__, g_result.c_str());
        }
        else
        {
            //PRINT("need_sum:%d, g_flow:%d, cost:%d\n", need_sum, g_flow, cost);
        }
    } while(0);
    // while(clock() - start < EXIT_SECOND * CLOCKS_PER_SEC);


    PRINT("\ng_flow:%d, need_sum:%d, g_flow_count: %d, min_cost: %d\n\n", g_flow, need_sum, g_flow_count, min_cost);

    return (char*)g_result.c_str();
}


static void init_list_edge()
{
    int i;

    memset(g_list, 0, MAXN * sizeof(g_list[0]));
    memset(g_edge, 0, MAXM * sizeof(g_edge[0]));
    E_COUNT = -1;

    for(i = 0; i <= M_COUNT; i++)
    {
        add_edge(g_matrix[i][0], g_matrix[i][1], g_matrix[i][2], g_matrix[i][3]);
    }
}


void set_num(int vNum, int linkNum, int consumerNum, int srvcost)
{
    V_NUM = vNum;
    V_CONSUME = consumerNum;
    SRV_COST = srvcost;
}

void set_consume(int index, int netnode, int need)
{
    g_consume[index] = {netnode, need};
    g_consume_map[netnode] = index;
    //PRINT("netnode:%d, index:%d\n", netnode, index);
}

//插入邻接表, cost 表示单位费用; res 表示容量
void add_edge(int from, int to, int cost, int res)
{
    //PRINT("%d %d %d %d\n", from , to, res, cost);
    if(g_init_flag)
    {
        M_COUNT++;
        g_matrix[M_COUNT][0] = from;
        g_matrix[M_COUNT][1] = to;
        g_matrix[M_COUNT][2] = cost;
        g_matrix[M_COUNT][3] = res;

    }
    edge e1 = {g_list[from], 0, to, res, cost, 0};
    edge e2 = {g_list[to], 0, from, 0, -cost, 0};

    g_edge[++E_COUNT] = e1;
    g_list[from] = &g_edge[E_COUNT];
    g_edge[++E_COUNT] = e2;
    g_list[to] = &g_edge[E_COUNT];

    g_list[from]->pair = g_list[to];
    g_list[to]->pair = g_list[from];
}


// true 表示找到了最短路径
static bool spfa()
{
    int i;
    int cur, ver;
    edge *tmp;

    for(i = 0; i < V_NUM; i++)
    {
        g_dist[i] = INF;
        g_path[i] = -1;
        g_inq[i] = 0;  // 顶点不在队列中
    }
    Q.clear();
    memset(g_path_edge, 0, MAXN * sizeof(g_path_edge[0]));

    Q.push_back(V_SRC);
    g_dist[V_SRC] = 0;
    g_path[V_SRC] = -1; //V_SRC; // -1
    g_inq[V_SRC] = 1;

    while(!Q.empty())
    {
        cur = Q.front();
        Q.pop_front();
        g_inq[cur] = 0;
        for(tmp = g_list[cur]; tmp != NULL; tmp = tmp->next)
        {
            ver = tmp->to;
            // 松弛
            if(tmp->residual > 0 && g_dist[ver] > g_dist[cur] + tmp->cost)
            {
                g_dist[ver] = g_dist[cur] + tmp->cost;
                g_path[ver] = cur;
                g_path_edge[ver] = tmp; // 注意！！！
                if(g_inq[ver] == 0) // 加入队列
                {
                    Q.push_back(ver);
                    g_inq[ver] = 1;
                }
            }
        }
    }

    return g_dist[V_DST] != INF;
}

//增广路算法，寻找增广路并调整流量, 返回调整的费用
static int argument()
{
    int i;
    int delta = INF;     // 可改进量
    int flow_cost = 0;   // 费用
    edge *e;

    // 获取改进量min
    for (i = V_DST; g_path[i] != -1; i = g_path[i])
    {
        e = g_path_edge[i];
        if( e->residual < delta)
            delta = e->residual;
    }
    // 调整流量
    for(i = V_DST; g_path[i] != -1; i = g_path[i])
    {
        e = g_path_edge[i];
        e->residual -= delta;
        e->flow += delta;                 // path
        e->pair->residual += delta;       // 注意！！！
        e->pair->flow -= delta;           // path
        flow_cost += e->cost * delta;     // 计算费用！！！
    }
    // 纪录最大流
    if(delta != INF)
    {
        g_flow += delta;
        //printf("%d\n", g_flow);
    }

    return flow_cost;
}

// 返回总费用
static int mcmf()
{
    int flow_cost = 0;

    // 初始化
    g_flow = 0;

    while(spfa())
    {
        flow_cost += argument();
    }

    return flow_cost;
}

// 找流量路径, 结果存放在g_result中
static void dfs_path()
{
    edge *p;
    int min = INF;
    int i;
    deque<edge*> S;     // 栈

    memset(g_inq, 0, sizeof(g_inq)); //初始化为0
    p = g_list[V_SRC];
    g_inq[V_SRC] = 1;
    g_flow_count = 0;

    g_result.clear();
    g_result = "";

    while(1)
    {
        // 寻找满足条件的边
        while(p->flow <= 0 || g_inq[p->to] == 1)    // 无流量或在队列中
        {
            p = p->next;
            if(p == NULL) break;
        }
        if(p == NULL)
        {
            if(!S.empty())
            {
                p = S.back();
                S.pop_back();
                continue;   // 重新开始循环
            }
            break;          // 路径查找结束
        }

        S.push_back(p);     // 压入栈
        // g_inq[p->to] = 1;   // 在队列中
        if(min > p->flow)
        {
            min = p->flow;
        }
        if(p->to == V_DST)
        {
            // 1. 保存路径信息
            //add_print_flow(min, p->pair->to);
            for(i = 0; i < S.size(); i++)
            {
                g_result += to_string(S[i]->to) + " ";
                //PRINT("%d ", S[i]->to);
            }
            g_result += to_string(g_consume_map[p->pair->to]) + " " + to_string(min) + "\n";
            g_flow_count++;
            //PRINT("netnode: %d, consume: %d, flow: %d\n", netnode, g_consume_map[netnode], flow);

            // 2. 调整flow, 并更新min
            int min_new = INF;
            for(i = 0; i < S.size(); i++)
            {
                S[i]->flow -= min;
                if(S[i]->flow > 0 && min_new > S[i]->flow) {
                    min_new = S[i]->flow;
                }
            }

            min = min_new;
            while(!S.empty())
            {
                p = S.back();
                S.pop_back();
                g_inq[p->to] = 0;   // 不在队列中
                if(p->flow != 0)
                {
                    break;
                }
            }
        }
        else
        {
            g_inq[p->to] = 1;   // 在队列中
            p = g_list[p->to];
        }
    }
}



//int main()
//{
//    init();
//    int cost;
//    //g_flow = 0;
//
//    //return 0;
//    cost = mcmf();
//    dfs_path();
//    printf("cost: %d\n", cost);
//    printf("flow: %d\n", g_flow);
//    //debug_print();
//
//    printf("======\n%d\n", g_flow_count);
//    printf("%s", g_result.c_str());
//    return 0;
//}


//int g_consume[501]; // 保存消费节点对应的
/*void init()
{
    int s, d, r, c;
    int i;
    int link_num, con_num;
    scanf(" %d %d %d", &V_NUM, &E_NUM, &V_CONSUME);
    scanf(" %d ", &SRV_COST);
    //scanf(" %d %d ", &V_SRC, &V_DST);  // 总顶点数

    // 起、末、容、单位费用
    for(i = 0; i < E_NUM; i++)
    {
        scanf(" %d %d %d %d", &s, &d, &r, &c);
        if(r == -1)
        {
            r = INF;
        }
        add_edge(s, d, c, r);
        //printf("%d %d %d %d\n", s, d, r, c);
    }
    // 消费节点、相连的网络节点、带宽消耗
    for(i = 0; i < V_CONSUME; i++)
    {
        //xscanf(" %d %d %d ", );
    }
}*/

/*
void debug_print()
{
    int i;
    for(i = 0; i < V_NUM; i++)
    {
        edge *p = g_list[i];
        printf("[%d]: ", i);
        while(p != NULL)
        {
            printf("->%d(%d)", p->to, p->flow);
            p = p->next;
        }
        printf("\n");
    }
}*/

/*
inline void add_print_flow(int flow, int netnode)
{
    int i;

    for(i = 0; i < S.size(); i++)
    {
        g_result += to_string(S[i]->to) + " ";
        //PRINT("%d ", S[i]->to);
    }
    g_result += to_string(g_consume_map[netnode]) + " " + to_string(flow) + "\n";
    g_flow_count++;
    //PRINT("netnode: %d, consume: %d, flow: %d\n", netnode, g_consume_map[netnode], flow);
}
*/