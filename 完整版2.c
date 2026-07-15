#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------- 常量定义 ------------------------- */
#define MAX_USERNAME 30
#define MAX_PASSWORD 30
#define MAX_NAME 50
#define MAX_PHONE 20
#define MAX_SHOP 50
#define MAX_ADDR 100
#define MAX_DISH 50
#define MAX_KEYWORD 50
#define NAME_LEN 32
#define MAX_NODE 20
#define INF 99999

/* ------------------------- 地图结构 ------------------------- */
struct map {
    int node_count;
    char nodes[MAX_NODE][NAME_LEN];
    int dist[MAX_NODE][MAX_NODE];
    int loaded;
};
static struct map g_map;

/* ------------------------- 1. 业务结构体（含ID） ------------------------- */
typedef struct DishNode {
    int id;
    char name[MAX_DISH];
    double price;
    struct DishNode *next;
} DishNode;

typedef struct MerchantNode {
    int id;
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char shopName[MAX_SHOP];
    char bossName[MAX_NAME];
    char bossPhone[MAX_PHONE];
    char location[NAME_LEN];
    DishNode *dishList;
    struct MerchantNode *next;
} MerchantNode;

typedef struct AddressNode {
    int id;
    char detail[MAX_ADDR];
    struct AddressNode *next;
} AddressNode;

typedef struct CustomerNode {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char name[MAX_NAME];
    char phone[MAX_PHONE];
    AddressNode *addrList;
    struct CustomerNode *next;
} CustomerNode;

typedef struct RiderNode {
    int id;
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char name[MAX_NAME];
    char phone[MAX_PHONE];
    char area[NAME_LEN];
    int busy;
    int task_count;
    struct RiderNode *next;
} RiderNode;

typedef struct OrderNode {
    int orderId;
    char customerName[MAX_USERNAME];
    char dishName[MAX_DISH];
    char address[MAX_ADDR];
    int status;          // 0待接单 1派送中 2已完成
    int rider_id;
    int merchant_id;     // 新增：所属商家ID
    char sp[NAME_LEN];
    char ep[NAME_LEN];
    char route[200];
    int distance;
    struct OrderNode *next;
} OrderNode;

/* ------------------------- 2. 全局链表头 ------------------------- */
MerchantNode *merchantHead = NULL;
CustomerNode *customerHead = NULL;
RiderNode *riderHead = NULL;
OrderNode *orderHead = NULL;
int globalOrderId = 1000;
int nextMerchantId = 1;
int nextRiderId = 1;

/* ------------------------- 3. 密码强度校验 ------------------------- */
int checkPasswordStrength(const char *pwd) {
    int hasLetter = 0, hasDigit = 0, hasSymbol = 0;
    for (int i = 0; pwd[i] != '\0'; i++) {
        if (isalpha((unsigned char)pwd[i])) hasLetter = 1;
        else if (isdigit((unsigned char)pwd[i])) hasDigit = 1;
        else hasSymbol = 1;
    }
    return hasLetter + hasDigit + hasSymbol;
}

/* ------------------------- 4. 辅助函数 ------------------------- */
void safeCopy(char *dest, const char *src, size_t size) {
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

/* ------------------------- 5. 地图函数 ------------------------- */
void map_init(struct map *m) {
    m->node_count = 0;
    m->loaded = 0;
    for (int i = 0; i < MAX_NODE; i++)
        for (int j = 0; j < MAX_NODE; j++)
            m->dist[i][j] = (i == j) ? 0 : INF;
}
int map_find_node(struct map *m, const char *name) {
    for (int i = 0; i < m->node_count; i++)
        if (strcmp(m->nodes[i], name) == 0) return i;
    return -1;
}
int map_add_node(struct map *m, const char *name) {
    if (m->node_count >= MAX_NODE) { printf("节点已满\n"); return -1; }
    if (map_find_node(m, name) >= 0) { printf("节点已存在\n"); return -1; }
    int id = m->node_count++;
    strcpy(m->nodes[id], name);
    m->dist[id][id] = 0;
    printf("已添加节点 %s (编号%d)\n", name, id);
    return id;
}
int map_del_node(struct map *m, const char *name) {
    int id = map_find_node(m, name);
    if (id < 0) { printf("节点不存在\n"); return -1; }
    int last = m->node_count - 1;
    if (id != last) {
        strcpy(m->nodes[id], m->nodes[last]);
        for (int i = 0; i < m->node_count; i++) {
            m->dist[id][i] = m->dist[last][i];
            m->dist[i][id] = m->dist[i][last];
        }
    }
    m->node_count--;
    printf("已删除节点 %s\n", name);
    return 0;
}
int map_add_edge(struct map *m, const char *a, const char *b, int d) {
    int ia = map_find_node(m, a), ib = map_find_node(m, b);
    if (ia < 0 || ib < 0) { printf("节点不存在\n"); return -1; }
    if (d <= 0) { printf("距离必须>0\n"); return -1; }
    m->dist[ia][ib] = m->dist[ib][ia] = d;
    printf("已添加路线 %s<->%s 距离%d\n", a, b, d);
    return 0;
}
int map_del_edge(struct map *m, const char *a, const char *b) {
    int ia = map_find_node(m, a), ib = map_find_node(m, b);
    if (ia < 0 || ib < 0) { printf("节点不存在\n"); return -1; }
    m->dist[ia][ib] = m->dist[ib][ia] = INF;
    printf("已删除路线 %s<->%s\n", a, b);
    return 0;
}
void map_print(struct map *m) {
    if (m->node_count == 0) { printf("地图为空\n"); return; }
    printf("\n--- 节点 ---\n");
    for (int i = 0; i < m->node_count; i++)
        printf("  %d: %s\n", i, m->nodes[i]);
    printf("\n--- 邻接矩阵 ---\n    ");
    for (int i = 0; i < m->node_count; i++) printf("%6s", m->nodes[i]);
    printf("\n");
    for (int i = 0; i < m->node_count; i++) {
        printf("%4s", m->nodes[i]);
        for (int j = 0; j < m->node_count; j++) {
            if (m->dist[i][j] >= INF) printf("%6s", "INF");
            else printf("%6d", m->dist[i][j]);
        }
        printf("\n");
    }
}
int map_dijkstra(struct map *m, const char *src, const char *dst,
                 char *path_out, int path_size, int *dist_out) {
    int s = map_find_node(m, src), t = map_find_node(m, dst);
    if (s < 0 || t < 0) return -1;
    int n = m->node_count;
    int visited[MAX_NODE] = {0}, d[MAX_NODE], prev[MAX_NODE];
    for (int i = 0; i < n; i++) { d[i] = m->dist[s][i]; prev[i] = -1; }
    for (int k = 0; k < n; k++) {
        int u = -1, min = INF;
        for (int i = 0; i < n; i++)
            if (!visited[i] && d[i] < min) { min = d[i]; u = i; }
        if (u < 0) break;
        visited[u] = 1;
        for (int v = 0; v < n; v++) {
            if (visited[v] || m->dist[u][v] >= INF) continue;
            if (d[u] + m->dist[u][v] < d[v]) {
                d[v] = d[u] + m->dist[u][v];
                prev[v] = u;
            }
        }
    }
    if (d[t] >= INF) return -1;
    char rev[200] = "";
    int cur = t;
    while (cur != -1) {
        char tmp[200];
        if (rev[0] == '\0') strcpy(tmp, m->nodes[cur]);
        else sprintf(tmp, "%s->%s", m->nodes[cur], rev);
        strcpy(rev, tmp);
        cur = prev[cur];
    }
    strncpy(path_out, rev, path_size - 1);
    path_out[path_size - 1] = '\0';
    *dist_out = d[t];
    return 0;
}
void map_save(struct map *m) {
    FILE *fp = fopen("map.txt", "w");
    if (!fp) return;
    for (int i = 0; i < m->node_count; i++)
        fprintf(fp, "NODE %s\n", m->nodes[i]);
    for (int i = 0; i < m->node_count; i++)
        for (int j = i + 1; j < m->node_count; j++)
            if (m->dist[i][j] < INF)
                fprintf(fp, "EDGE %s %s %d\n", m->nodes[i], m->nodes[j], m->dist[i][j]);
    fclose(fp);
}
void map_load(struct map *m) {
    FILE *fp = fopen("map.txt", "r");
    if (!fp) { m->loaded = 0; return; }
    map_init(m);
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char a[NAME_LEN], b[NAME_LEN]; int d;
        if (sscanf(line, "NODE %31s", a) == 1) map_add_node(m, a);
        else if (sscanf(line, "EDGE %31s %31s %d", a, b, &d) == 3) map_add_edge(m, a, b, d);
    }
    fclose(fp);
    m->loaded = 1;
}

/* ------------------------- 6. 文件加载/保存（含地图） ------------------------- */
void loadData() {
    FILE *fp;
    // 商家
    fp = fopen("merchants.txt", "r");
    if (fp) {
        MerchantNode *tail = NULL;
        int id;
        char u[MAX_USERNAME], p[MAX_PASSWORD], shop[MAX_SHOP], boss[MAX_NAME], phone[MAX_PHONE], loc[NAME_LEN];
        while (fscanf(fp, "%d %s %s %s %s %s %s", &id, u, p, shop, boss, phone, loc) == 7) {
            MerchantNode *newM = (MerchantNode*)malloc(sizeof(MerchantNode));
            newM->id = id;
            safeCopy(newM->username, u, MAX_USERNAME);
            safeCopy(newM->password, p, MAX_PASSWORD);
            safeCopy(newM->shopName, shop, MAX_SHOP);
            safeCopy(newM->bossName, boss, MAX_NAME);
            safeCopy(newM->bossPhone, phone, MAX_PHONE);
            safeCopy(newM->location, loc, NAME_LEN);
            newM->dishList = NULL;
            newM->next = NULL;
            if (!merchantHead) merchantHead = newM;
            else tail->next = newM;
            tail = newM;
            if (id >= nextMerchantId) nextMerchantId = id + 1;
        }
        fclose(fp);
    }
    // 顾客
    fp = fopen("customers.txt", "r");
    if (fp) {
        CustomerNode *tail = NULL;
        char u[MAX_USERNAME], p[MAX_PASSWORD], name[MAX_NAME], phone[MAX_PHONE];
        while (fscanf(fp, "%s %s %s %s", u, p, name, phone) == 4) {
            CustomerNode *newC = (CustomerNode*)malloc(sizeof(CustomerNode));
            safeCopy(newC->username, u, MAX_USERNAME);
            safeCopy(newC->password, p, MAX_PASSWORD);
            safeCopy(newC->name, name, MAX_NAME);
            safeCopy(newC->phone, phone, MAX_PHONE);
            newC->addrList = NULL;
            newC->next = NULL;
            if (!customerHead) customerHead = newC;
            else tail->next = newC;
            tail = newC;
        }
        fclose(fp);
    }
    // 骑手
    fp = fopen("riders.txt", "r");
    if (fp) {
        RiderNode *tail = NULL;
        int id, busy, task;
        char u[MAX_USERNAME], p[MAX_PASSWORD], name[MAX_NAME], phone[MAX_PHONE], area[NAME_LEN];
        while (fscanf(fp, "%d %s %s %s %s %s %d %d", &id, u, p, name, phone, area, &busy, &task) == 8) {
            RiderNode *newR = (RiderNode*)malloc(sizeof(RiderNode));
            newR->id = id;
            safeCopy(newR->username, u, MAX_USERNAME);
            safeCopy(newR->password, p, MAX_PASSWORD);
            safeCopy(newR->name, name, MAX_NAME);
            safeCopy(newR->phone, phone, MAX_PHONE);
            safeCopy(newR->area, area, NAME_LEN);
            newR->busy = busy;
            newR->task_count = task;
            newR->next = NULL;
            if (!riderHead) riderHead = newR;
            else tail->next = newR;
            tail = newR;
            if (id >= nextRiderId) nextRiderId = id + 1;
        }
        fclose(fp);
    }
    // 订单（格式增加 merchant_id）
    fp = fopen("orders.txt", "r");
    if (fp) {
        OrderNode *tail = NULL;
        int oid, st, rid, mid, dist;
        char cn[MAX_USERNAME], dn[MAX_DISH], addr[MAX_ADDR], sp[NAME_LEN], ep[NAME_LEN], route[200];
        while (fscanf(fp, "%d %s %s %s %d %d %d %s %s %s %d",
                      &oid, cn, dn, addr, &st, &rid, &mid, sp, ep, route, &dist) == 11) {
            OrderNode *newO = (OrderNode*)malloc(sizeof(OrderNode));
            newO->orderId = oid;
            safeCopy(newO->customerName, cn, MAX_USERNAME);
            safeCopy(newO->dishName, dn, MAX_DISH);
            safeCopy(newO->address, addr, MAX_ADDR);
            newO->status = st;
            newO->rider_id = rid;
            newO->merchant_id = mid;
            safeCopy(newO->sp, sp, NAME_LEN);
            safeCopy(newO->ep, ep, NAME_LEN);
            safeCopy(newO->route, route, 200);
            newO->distance = dist;
            newO->next = NULL;
            if (!orderHead) orderHead = newO;
            else tail->next = newO;
            tail = newO;
            if (oid >= globalOrderId) globalOrderId = oid + 1;
        }
        fclose(fp);
    }
    // 菜品
    fp = fopen("dishes.txt", "r");
    if (fp) {
        char uname[MAX_USERNAME];
        int did;
        char dname[MAX_DISH];
        double price;
        while (fscanf(fp, "%s %d %s %lf", uname, &did, dname, &price) == 4) {
            MerchantNode *m = merchantHead;
            while (m) {
                if (strcmp(m->username, uname) == 0) {
                    DishNode *newD = (DishNode*)malloc(sizeof(DishNode));
                    newD->id = did;
                    safeCopy(newD->name, dname, MAX_DISH);
                    newD->price = price;
                    newD->next = m->dishList;
                    m->dishList = newD;
                    break;
                }
                m = m->next;
            }
        }
        fclose(fp);
    }
    map_load(&g_map);
}

void saveAll() {
    FILE *fp;
    // 商家
    fp = fopen("merchants.txt", "w");
    if (fp) {
        MerchantNode *m = merchantHead;
        while (m) {
            fprintf(fp, "%d %s %s %s %s %s %s\n", m->id, m->username, m->password,
                    m->shopName, m->bossName, m->bossPhone, m->location);
            m = m->next;
        }
        fclose(fp);
    }
    // 顾客
    fp = fopen("customers.txt", "w");
    if (fp) {
        CustomerNode *c = customerHead;
        while (c) {
            fprintf(fp, "%s %s %s %s\n", c->username, c->password, c->name, c->phone);
            c = c->next;
        }
        fclose(fp);
    }
    // 骑手
    fp = fopen("riders.txt", "w");
    if (fp) {
        RiderNode *r = riderHead;
        while (r) {
            fprintf(fp, "%d %s %s %s %s %s %d %d\n", r->id, r->username, r->password,
                    r->name, r->phone, r->area, r->busy, r->task_count);
            r = r->next;
        }
        fclose(fp);
    }
    // 订单
    fp = fopen("orders.txt", "w");
    if (fp) {
        OrderNode *o = orderHead;
        while (o) {
            fprintf(fp, "%d %s %s %s %d %d %d %s %s %s %d\n",
                    o->orderId, o->customerName, o->dishName, o->address,
                    o->status, o->rider_id, o->merchant_id, o->sp, o->ep, o->route, o->distance);
            o = o->next;
        }
        fclose(fp);
    }
    // 菜品
    fp = fopen("dishes.txt", "w");
    if (fp) {
        MerchantNode *m = merchantHead;
        while (m) {
            DishNode *d = m->dishList;
            while (d) {
                fprintf(fp, "%s %d %s %.2f\n", m->username, d->id, d->name, d->price);
                d = d->next;
            }
            m = m->next;
        }
        fclose(fp);
    }
    map_save(&g_map);
}

/* ------------------------- 7. 查找函数 ------------------------- */
MerchantNode* findMerchantByPhone(const char *phone) {
    MerchantNode *curr = merchantHead;
    while (curr) {
        if (strcmp(curr->bossPhone, phone) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}
MerchantNode* findMerchantById(int id) {
    MerchantNode *curr = merchantHead;
    while (curr) {
        if (curr->id == id) return curr;
        curr = curr->next;
    }
    return NULL;
}
CustomerNode* findCustomerByPhone(const char *phone) {
    CustomerNode *curr = customerHead;
    while (curr) {
        if (strcmp(curr->phone, phone) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}
RiderNode* findRiderByPhone(const char *phone) {
    RiderNode *curr = riderHead;
    while (curr) {
        if (strcmp(curr->phone, phone) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}
RiderNode* findRiderById(int id) {
    RiderNode *curr = riderHead;
    while (curr) {
        if (curr->id == id) return curr;
        curr = curr->next;
    }
    return NULL;
}
int findUserByUsername(const char *username) {
    MerchantNode *m = merchantHead;
    while (m) { if (strcmp(m->username, username) == 0) return 1; m = m->next; }
    CustomerNode *c = customerHead;
    while (c) { if (strcmp(c->username, username) == 0) return 1; c = c->next; }
    RiderNode *r = riderHead;
    while (r) { if (strcmp(r->username, username) == 0) return 1; r = r->next; }
    return 0;
}

/* ------------------------- 8. 系统管理（增删改查） ------------------------- */
void addMerchant(const char *username, const char *password, const char *shop,
                 const char *boss, const char *phone, const char *location) {
    if (findMerchantByPhone(phone)) { printf("电话已存在\n"); return; }
    if (findUserByUsername(username)) { printf("用户名已存在\n"); return; }
    MerchantNode *newM = (MerchantNode*)malloc(sizeof(MerchantNode));
    newM->id = nextMerchantId++;
    safeCopy(newM->username, username, MAX_USERNAME);
    safeCopy(newM->password, password, MAX_PASSWORD);
    safeCopy(newM->shopName, shop, MAX_SHOP);
    safeCopy(newM->bossName, boss, MAX_NAME);
    safeCopy(newM->bossPhone, phone, MAX_PHONE);
    safeCopy(newM->location, location, NAME_LEN);
    newM->dishList = NULL;
    newM->next = merchantHead;
    merchantHead = newM;
    saveAll();
    printf("商家添加成功 (ID:%d)\n", newM->id);
}
void deleteMerchant(const char *phone) {
    MerchantNode *prev = NULL, *curr = merchantHead;
    while (curr && strcmp(curr->bossPhone, phone) != 0) { prev = curr; curr = curr->next; }
    if (!curr) { printf("未找到\n"); return; }
    if (prev) prev->next = curr->next;
    else merchantHead = curr->next;
    DishNode *d = curr->dishList;
    while (d) { DishNode *tmp = d; d = d->next; free(tmp); }
    free(curr);
    saveAll();
    printf("商家已删除\n");
}
void updateMerchant(const char *oldPhone, const char *newUsername, const char *newPassword,
                    const char *newShop, const char *newBoss, const char *newPhone, const char *newLocation) {
    MerchantNode *target = findMerchantByPhone(oldPhone);
    if (!target) { printf("未找到\n"); return; }
    if (strcmp(oldPhone, newPhone) != 0 && findMerchantByPhone(newPhone)) { printf("新电话被占用\n"); return; }
    if (strcmp(target->username, newUsername) != 0 && findUserByUsername(newUsername)) { printf("用户名被占用\n"); return; }
    safeCopy(target->username, newUsername, MAX_USERNAME);
    safeCopy(target->password, newPassword, MAX_PASSWORD);
    safeCopy(target->shopName, newShop, MAX_SHOP);
    safeCopy(target->bossName, newBoss, MAX_NAME);
    safeCopy(target->bossPhone, newPhone, MAX_PHONE);
    safeCopy(target->location, newLocation, NAME_LEN);
    saveAll();
    printf("商家信息已更新，旧信息已从文件中清除\n");
}
void printAllMerchants() {
    MerchantNode *curr = merchantHead;
    if (!curr) { printf("无商家\n"); return; }
    int i = 1;
    while (curr) {
        printf("%d. ID:%d 用户名:%s 店名:%s 老板:%s 电话:%s 位置:%s\n",
               i++, curr->id, curr->username, curr->shopName, curr->bossName, curr->bossPhone, curr->location);
        curr = curr->next;
    }
}

void addCustomer(const char *username, const char *password, const char *name, const char *phone) {
    if (findCustomerByPhone(phone)) { printf("手机号已存在\n"); return; }
    if (findUserByUsername(username)) { printf("用户名已存在\n"); return; }
    CustomerNode *newC = (CustomerNode*)malloc(sizeof(CustomerNode));
    safeCopy(newC->username, username, MAX_USERNAME);
    safeCopy(newC->password, password, MAX_PASSWORD);
    safeCopy(newC->name, name, MAX_NAME);
    safeCopy(newC->phone, phone, MAX_PHONE);
    newC->addrList = NULL;
    newC->next = customerHead;
    customerHead = newC;
    saveAll();
    printf("顾客添加成功\n");
}
void deleteCustomer(const char *phone) {
    CustomerNode *prev = NULL, *curr = customerHead;
    while (curr && strcmp(curr->phone, phone) != 0) { prev = curr; curr = curr->next; }
    if (!curr) { printf("未找到\n"); return; }
    if (prev) prev->next = curr->next;
    else customerHead = curr->next;
    AddressNode *a = curr->addrList;
    while (a) { AddressNode *tmp = a; a = a->next; free(tmp); }
    free(curr);
    saveAll();
    printf("顾客已删除\n");
}
void updateCustomer(const char *oldPhone, const char *newUsername, const char *newPassword,
                    const char *newName, const char *newPhone) {
    CustomerNode *target = findCustomerByPhone(oldPhone);
    if (!target) { printf("未找到\n"); return; }
    if (strcmp(oldPhone, newPhone) != 0 && findCustomerByPhone(newPhone)) { printf("新手机号被占用\n"); return; }
    if (strcmp(target->username, newUsername) != 0 && findUserByUsername(newUsername)) { printf("用户名被占用\n"); return; }
    safeCopy(target->username, newUsername, MAX_USERNAME);
    safeCopy(target->password, newPassword, MAX_PASSWORD);
    safeCopy(target->name, newName, MAX_NAME);
    safeCopy(target->phone, newPhone, MAX_PHONE);
    saveAll();
    printf("顾客信息已更新，旧信息已从文件中清除\n");
}
void printAllCustomers() {
    CustomerNode *curr = customerHead;
    if (!curr) { printf("无顾客\n"); return; }
    int i = 1;
    while (curr) {
        printf("%d. 用户名:%s 姓名:%s 手机:%s\n", i++, curr->username, curr->name, curr->phone);
        curr = curr->next;
    }
}

void addRider(const char *username, const char *password, const char *name, const char *phone,
              const char *area, int busy, int task) {
    if (findRiderByPhone(phone)) { printf("手机号已存在\n"); return; }
    if (findUserByUsername(username)) { printf("用户名已存在\n"); return; }
    RiderNode *newR = (RiderNode*)malloc(sizeof(RiderNode));
    newR->id = nextRiderId++;
    safeCopy(newR->username, username, MAX_USERNAME);
    safeCopy(newR->password, password, MAX_PASSWORD);
    safeCopy(newR->name, name, MAX_NAME);
    safeCopy(newR->phone, phone, MAX_PHONE);
    safeCopy(newR->area, area, NAME_LEN);
    newR->busy = busy;
    newR->task_count = task;
    newR->next = riderHead;
    riderHead = newR;
    saveAll();
    printf("骑手添加成功 (ID:%d)\n", newR->id);
}
void deleteRider(const char *phone) {
    RiderNode *prev = NULL, *curr = riderHead;
    while (curr && strcmp(curr->phone, phone) != 0) { prev = curr; curr = curr->next; }
    if (!curr) { printf("未找到\n"); return; }
    if (prev) prev->next = curr->next;
    else riderHead = curr->next;
    free(curr);
    saveAll();
    printf("骑手已删除\n");
}
void updateRider(const char *oldPhone, const char *newUsername, const char *newPassword,
                 const char *newName, const char *newPhone, const char *newArea, int newBusy, int newTask) {
    RiderNode *target = findRiderByPhone(oldPhone);
    if (!target) { printf("未找到\n"); return; }
    if (strcmp(oldPhone, newPhone) != 0 && findRiderByPhone(newPhone)) { printf("新手机号被占用\n"); return; }
    if (strcmp(target->username, newUsername) != 0 && findUserByUsername(newUsername)) { printf("用户名被占用\n"); return; }
    safeCopy(target->username, newUsername, MAX_USERNAME);
    safeCopy(target->password, newPassword, MAX_PASSWORD);
    safeCopy(target->name, newName, MAX_NAME);
    safeCopy(target->phone, newPhone, MAX_PHONE);
    safeCopy(target->area, newArea, NAME_LEN);
    target->busy = newBusy;
    target->task_count = newTask;
    saveAll();
    printf("骑手信息已更新，旧信息已从文件中清除\n");
}
void printAllRiders() {
    RiderNode *curr = riderHead;
    if (!curr) { printf("无骑手\n"); return; }
    int i = 1;
    while (curr) {
        printf("%d. ID:%d 用户名:%s 姓名:%s 手机:%s 区域:%s 状态:%s 任务数:%d\n",
               i++, curr->id, curr->username, curr->name, curr->phone, curr->area,
               curr->busy ? "忙" : "闲", curr->task_count);
        curr = curr->next;
    }
}

/* ------------------------- 9. 订单派单 ------------------------- */
int dispatch_order(OrderNode *order) {
    if (!riderHead) return -1;
    RiderNode *pick = NULL;
    for (RiderNode *r = riderHead; r; r = r->next)
        if (r->busy == 0 && strcmp(r->area, order->ep) == 0)
            if (!pick || r->task_count < pick->task_count) pick = r;
    if (!pick)
        for (RiderNode *r = riderHead; r; r = r->next)
            if (r->busy == 0)
                if (!pick || r->task_count < pick->task_count) pick = r;
    if (!pick) return -1;
    pick->busy = 1;
    pick->task_count++;
    order->rider_id = pick->id;
    return pick->id;
}

/* ------------------------- 10. 注册功能 ------------------------- */
void registerMenu() {
    system("cls");
    int role;
    char uname[MAX_USERNAME], pwd[MAX_PASSWORD];
    char shop[MAX_SHOP], boss[MAX_NAME], phone[MAX_PHONE], loc[NAME_LEN];
    char name[MAX_NAME];

    printf("\n===== 用户注册 =====\n");
    printf("选择注册身份 (1.顾客 2.商家 3.骑手 0.返回): ");
    scanf("%d", &role);
    if (role == 0) return;

    printf("请输入用户名: "); scanf("%s", uname);
    printf("请输入密码(需包含字母/数字/符号中的至少两种): "); scanf("%s", pwd);
    if (checkPasswordStrength(pwd) < 2) {
        printf("密码强度不足！必须包含字母、数字、符号中的至少两种。\n");
        system("pause");
        return;
    }
    if (findUserByUsername(uname)) {
        printf("用户名已存在！\n");
        system("pause");
        return;
    }

    if (role == 1) {
        printf("请输入姓名: "); scanf("%s", name);
        printf("请输入手机号: "); scanf("%s", phone);
        addCustomer(uname, pwd, name, phone);
    } else if (role == 2) {
        printf("请输入店名: "); scanf("%s", shop);
        printf("请输入老板姓名: "); scanf("%s", boss);
        printf("请输入老板电话: "); scanf("%s", phone);
        printf("请输入商家位置（地图节点名）: "); scanf("%s", loc);
        addMerchant(uname, pwd, shop, boss, phone, loc);
    } else if (role == 3) {
        printf("请输入姓名: "); scanf("%s", name);
        printf("请输入手机号: "); scanf("%s", phone);
        printf("请输入所属区域（地图节点名）: "); scanf("%s", loc);
        addRider(uname, pwd, name, phone, loc, 0, 0);
    }
    printf("注册成功！\n");
    system("pause");
}

/* ------------------------- 11. 角色菜单 ------------------------- */

/* 商家菜单 */
void merchantMenu(MerchantNode *merchant) {
    int choice;
    while (1) {
        system("cls");
        printf("\n===== 商家后台 (%s) ID:%d =====\n", merchant->username, merchant->id);
        printf("1. 添加菜品\n2. 修改菜品\n3. 查看我的菜品\n4. 修改个人信息\n");
        printf("5. 查看订单配送信息\n0. 退出登录\n");
        printf("请选择: ");
        scanf("%d", &choice);
        getchar();

        if (choice == 1) {
            DishNode *newDish = (DishNode*)malloc(sizeof(DishNode));
            printf("输入菜品ID: "); scanf("%d", &newDish->id);
            printf("输入菜品名称: "); scanf("%s", newDish->name);
            printf("输入菜品价格: "); scanf("%lf", &newDish->price);
            newDish->next = merchant->dishList;
            merchant->dishList = newDish;
            saveAll();
            printf("菜品添加成功！\n");
            system("pause");
        } else if (choice == 2) {
            int id;
            printf("输入要修改的菜品ID: "); scanf("%d", &id);
            DishNode *p = merchant->dishList;
            while (p && p->id != id) p = p->next;
            if (p) {
                printf("输入新名称: "); scanf("%s", p->name);
                printf("输入新价格: "); scanf("%lf", &p->price);
                saveAll();
                printf("修改成功！\n");
            } else printf("未找到该菜品！\n");
            system("pause");
        } else if (choice == 3) {
            DishNode *p = merchant->dishList;
            if (!p) { printf("暂无菜品。\n"); system("pause"); continue; }
            printf("ID\t名称\t\t价格\n");
            while (p) {
                printf("%d\t%s\t\t%.2f\n", p->id, p->name, p->price);
                p = p->next;
            }
            system("pause");
        } else if (choice == 4) {
            char newU[MAX_USERNAME], newP[MAX_PASSWORD], newShop[MAX_SHOP], newBoss[MAX_NAME], newPhone[MAX_PHONE], newLoc[NAME_LEN];
            printf("输入新用户名 (当前: %s): ", merchant->username);
            scanf("%s", newU);
            printf("输入新密码 (需强度): "); scanf("%s", newP);
            if (checkPasswordStrength(newP) < 2) {
                printf("密码强度不足！\n");
                system("pause");
                continue;
            }
            printf("输入新店名 (当前: %s): ", merchant->shopName);
            scanf("%s", newShop);
            printf("输入新老板姓名 (当前: %s): ", merchant->bossName);
            scanf("%s", newBoss);
            printf("输入新老板电话 (当前: %s): ", merchant->bossPhone);
            scanf("%s", newPhone);
            printf("输入新位置 (当前: %s): ", merchant->location);
            scanf("%s", newLoc);
            updateMerchant(merchant->bossPhone, newU, newP, newShop, newBoss, newPhone, newLoc);
            printf("信息已更新，请重新登录。\n");
            system("pause");
            break;
        } else if (choice == 5) {
            OrderNode *p = orderHead;
            int found = 0;
            printf("\n===== 本店订单配送信息 =====\n");
            printf("订单号\t顾客\t菜品\t地址\t状态\t骑手ID\t路径\t距离(km)\n");
            while (p) {
                if (p->merchant_id == merchant->id) {
                    const char *statusStr = (p->status == 0) ? "待接单" : (p->status == 1) ? "派送中" : "已完成";
                    printf("%d\t%s\t%s\t%s\t%s\t%d\t%s\t%d\n",
                           p->orderId, p->customerName, p->dishName, p->address,
                           statusStr, p->rider_id, p->route, p->distance);
                    found = 1;
                }
                p = p->next;
            }
            if (!found) printf("暂无订单\n");
            system("pause");
        } else if (choice == 0) break;
        else { printf("无效选项！\n"); system("pause"); }
    }
}

/* 骑手菜单 */
void riderMenu(RiderNode *rider) {
    int choice;
    while (1) {
        system("cls");
        printf("\n===== 骑手中心 (%s) ID:%d =====\n", rider->username, rider->id);
        printf("1. 查看我的订单\n2. 完成订单（标记送达）\n3. 修改个人信息\n0. 退出登录\n");
        printf("请选择: ");
        scanf("%d", &choice);
        if (choice == 1) {
            OrderNode *p = orderHead;
            int found = 0;
            printf("订单号\t顾客\t菜品\t地址\t状态\n");
            while (p) {
                if (p->rider_id == rider->id) {
                    const char *st = (p->status == 0) ? "待接单" : (p->status == 1) ? "派送中" : "已完成";
                    printf("%d\t%s\t%s\t%s\t%s\n", p->orderId, p->customerName, p->dishName, p->address, st);
                    found = 1;
                }
                p = p->next;
            }
            if (!found) printf("暂无订单\n");
            system("pause");
        } else if (choice == 2) {
            int oid;
            printf("输入要完成的订单号: "); scanf("%d", &oid);
            OrderNode *o = orderHead;
            while (o && o->orderId != oid) o = o->next;
            if (!o) { printf("订单不存在\n"); system("pause"); continue; }
            if (o->rider_id != rider->id) { printf("这不是您的订单\n"); system("pause"); continue; }
            if (o->status == 2) { printf("订单已完成\n"); system("pause"); continue; }
            o->status = 2;
            if (rider->task_count > 0) rider->task_count--;
            if (rider->task_count <= 0) { rider->busy = 0; rider->task_count = 0; }
            saveAll();
            printf("订单 %d 已完成！\n", oid);
            system("pause");
        } else if (choice == 3) {
            char newU[MAX_USERNAME], newP[MAX_PASSWORD], newName[MAX_NAME], newPhone[MAX_PHONE], newArea[NAME_LEN];
            printf("输入新用户名 (当前: %s): ", rider->username);
            scanf("%s", newU);
            printf("输入新密码 (需强度): "); scanf("%s", newP);
            if (checkPasswordStrength(newP) < 2) {
                printf("密码强度不足！\n");
                system("pause");
                continue;
            }
            printf("输入新姓名 (当前: %s): ", rider->name);
            scanf("%s", newName);
            printf("输入新手机号 (当前: %s): ", rider->phone);
            scanf("%s", newPhone);
            printf("输入新区域 (当前: %s): ", rider->area);
            scanf("%s", newArea);
            updateRider(rider->phone, newU, newP, newName, newPhone, newArea, rider->busy, rider->task_count);
            printf("信息已更新，请重新登录。\n");
            system("pause");
            break;
        } else if (choice == 0) break;
        else { printf("无效选项！\n"); system("pause"); }
    }
}

/* 顾客菜单（浏览商品先选商家） */
void customerMenu(CustomerNode *customer) {
    int choice;
    while (1) {
        system("cls");
        printf("\n===== 顾客中心 (%s) =====\n", customer->username);
        printf("1. 浏览商家商品\n2. 搜索商品\n3. 管理收货地址\n4. 提交订单（使用地图路径）\n");
        printf("5. 修改个人信息\n0. 退出登录\n");
        printf("请选择: ");
        scanf("%d", &choice);
        getchar();

        if (choice == 1) {
            printf("--- 商家列表 ---\n");
            MerchantNode *m = merchantHead;
            if (!m) { printf("暂无商家\n"); system("pause"); continue; }
            int idx = 1;
            while (m) {
                printf("%d. ID:%d 店名:%s 位置:%s\n", idx++, m->id, m->shopName, m->location);
                m = m->next;
            }
            int mid;
            printf("请选择商家ID: ");
            scanf("%d", &mid);
            MerchantNode *selected = findMerchantById(mid);
            if (!selected) { printf("商家不存在\n"); system("pause"); continue; }
            printf("\n【商家: %s (位置:%s)】\n", selected->shopName, selected->location);
            DishNode *d = selected->dishList;
            if (!d) { printf("该商家暂无菜品\n"); system("pause"); continue; }
            printf("菜品列表:\n");
            while (d) {
                printf("  - %s (￥%.2f)\n", d->name, d->price);
                d = d->next;
            }
            system("pause");
        } else if (choice == 2) {
            char keyword[MAX_KEYWORD];
            printf("输入搜索关键字: "); scanf("%s", keyword);
            MerchantNode *m = merchantHead;
            while (m) {
                DishNode *d = m->dishList;
                while (d) {
                    if (strstr(d->name, keyword)) {
                        printf("找到: [%s] %s (￥%.2f)\n", m->shopName, d->name, d->price);
                    }
                    d = d->next;
                }
                m = m->next;
            }
            system("pause");
        } else if (choice == 3) {
            int addrChoice;
            printf("1. 添加地址 2. 查看地址: ");
            scanf("%d", &addrChoice);
            if (addrChoice == 1) {
                AddressNode *newAddr = (AddressNode*)malloc(sizeof(AddressNode));
                newAddr->id = 1;
                AddressNode *temp = customer->addrList;
                while (temp && temp->next) { temp = temp->next; newAddr->id++; }
                printf("输入地址（地图节点名）: "); scanf("%s", newAddr->detail);
                newAddr->next = NULL;
                if (!customer->addrList) customer->addrList = newAddr;
                else temp->next = newAddr;
                printf("地址添加成功！\n");
                system("pause");
            } else if (addrChoice == 2) {
                AddressNode *p = customer->addrList;
                if (!p) printf("暂无地址。\n");
                else {
                    printf("您的地址列表（地图节点名）:\n");
                    while (p) {
                        printf("  ID:%d %s\n", p->id, p->detail);
                        p = p->next;
                    }
                }
                system("pause");
            }
        } else if (choice == 4) {
            if (!merchantHead) { printf("暂无商家\n"); system("pause"); continue; }
            printf("--- 商家列表 ---\n");
            MerchantNode *m = merchantHead;
            while (m) {
                printf("ID:%d %s (位置:%s)\n", m->id, m->shopName, m->location);
                m = m->next;
            }
            int mid;
            printf("选择商家ID: "); scanf("%d", &mid);
            MerchantNode *merch = findMerchantById(mid);
            if (!merch) { printf("商家不存在\n"); system("pause"); continue; }

            printf("--- %s 菜单 ---\n", merch->shopName);
            DishNode *d = merch->dishList;
            int idx = 1;
            while (d) {
                printf("%d. %s (￥%.2f)\n", idx++, d->name, d->price);
                d = d->next;
            }
            int dishIdx;
            printf("选择菜品编号: "); scanf("%d", &dishIdx);
            d = merch->dishList;
            for (int i = 1; i < dishIdx && d; i++) d = d->next;
            if (!d) { printf("无效选择\n"); system("pause"); continue; }

            if (!customer->addrList) { printf("请先添加收货地址\n"); system("pause"); continue; }
            printf("--- 您的地址 ---\n");
            AddressNode *a = customer->addrList;
            while (a) {
                printf("ID:%d %s\n", a->id, a->detail);
                a = a->next;
            }
            int addrId;
            printf("选择地址ID: "); scanf("%d", &addrId);
            a = customer->addrList;
            while (a && a->id != addrId) a = a->next;
            if (!a) { printf("地址不存在\n"); system("pause"); continue; }

            OrderNode *newO = (OrderNode*)malloc(sizeof(OrderNode));
            newO->orderId = globalOrderId++;
            safeCopy(newO->customerName, customer->username, MAX_USERNAME);
            safeCopy(newO->dishName, d->name, MAX_DISH);
            safeCopy(newO->address, a->detail, MAX_ADDR);
            newO->status = 0;
            newO->rider_id = -1;
            newO->merchant_id = merch->id;
            safeCopy(newO->sp, merch->location, NAME_LEN);
            safeCopy(newO->ep, a->detail, NAME_LEN);

            char path[200]; int dist;
            if (map_dijkstra(&g_map, newO->sp, newO->ep, path, sizeof(path), &dist) == 0) {
                safeCopy(newO->route, path, 200);
                newO->distance = dist;
                printf("配送路径: %s  距离: %d km\n", path, dist);
            } else {
                safeCopy(newO->route, "无路径", 200);
                newO->distance = -1;
                printf("地图无路径，请管理员添加路线后再下单\n");
                free(newO);
                system("pause");
                continue;
            }

            newO->next = orderHead;
            orderHead = newO;

            int rid = dispatch_order(newO);
            if (rid >= 0) {
                newO->status = 1;
                printf("已派单给骑手ID %d\n", rid);
            } else {
                printf("暂无可用骑手，订单待接单\n");
            }
            saveAll();
            printf("下单成功！订单号: %d\n", newO->orderId);
            system("pause");
        } else if (choice == 5) {
            char newU[MAX_USERNAME], newP[MAX_PASSWORD], newName[MAX_NAME], newPhone[MAX_PHONE];
            printf("输入新用户名 (当前: %s): ", customer->username);
            scanf("%s", newU);
            printf("输入新密码 (需强度): "); scanf("%s", newP);
            if (checkPasswordStrength(newP) < 2) {
                printf("密码强度不足！\n");
                system("pause");
                continue;
            }
            printf("输入新姓名 (当前: %s): ", customer->name);
            scanf("%s", newName);
            printf("输入新手机号 (当前: %s): ", customer->phone);
            scanf("%s", newPhone);
            updateCustomer(customer->phone, newU, newP, newName, newPhone);
            printf("信息已更新，请重新登录。\n");
            system("pause");
            break;
        } else if (choice == 0) break;
        else { printf("无效选项！\n"); system("pause"); }
    }
}

/* 管理员菜单 */
void adminMenu() {
    int choice, sub;
    char username[MAX_USERNAME], password[MAX_PASSWORD];
    char shop[MAX_SHOP], boss[MAX_NAME], phone[MAX_PHONE], loc[NAME_LEN];
    char name[MAX_NAME], newU[MAX_USERNAME], newP[MAX_PASSWORD], newName[MAX_NAME];
    char newShop[MAX_SHOP], newBoss[MAX_NAME], newPhone[MAX_PHONE], newLoc[NAME_LEN];
    char oldPhone[MAX_PHONE];
    char node1[NAME_LEN], node2[NAME_LEN];
    int dist;

    while (1) {
        system("cls");
        printf("\n========== 系统管理 ==========\n");
        printf("1. 查看所有商家\n");
        printf("2. 添加商家\n");
        printf("3. 修改商家信息\n");
        printf("4. 删除商家\n");
        printf("5. 查看所有顾客\n");
        printf("6. 添加顾客\n");
        printf("7. 修改顾客信息\n");
        printf("8. 删除顾客\n");
        printf("9. 查看所有骑手\n");
        printf("10. 添加骑手\n");
        printf("11. 修改骑手信息\n");
        printf("12. 删除骑手\n");
        printf("13. 地图管理\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        scanf("%d", &choice);
        if (choice == 0) break;

        switch (choice) {
            case 1: printAllMerchants(); system("pause"); break;
            case 2:
                printf("用户名: "); scanf("%s", username);
                printf("密码: "); scanf("%s", password);
                if (checkPasswordStrength(password) < 2) { printf("密码强度不足\n"); system("pause"); break; }
                printf("店名: "); scanf("%s", shop);
                printf("老板姓名: "); scanf("%s", boss);
                printf("老板电话: "); scanf("%s", phone);
                printf("位置（地图节点）: "); scanf("%s", loc);
                addMerchant(username, password, shop, boss, phone, loc);
                system("pause"); break;
            case 3:
                printf("要修改的商家当前老板电话: "); scanf("%s", oldPhone);
                printf("新用户名: "); scanf("%s", newU);
                printf("新密码: "); scanf("%s", newP);
                if (checkPasswordStrength(newP) < 2) { printf("密码强度不足\n"); system("pause"); break; }
                printf("新店名: "); scanf("%s", newShop);
                printf("新老板姓名: "); scanf("%s", newBoss);
                printf("新老板电话: "); scanf("%s", newPhone);
                printf("新位置: "); scanf("%s", newLoc);
                updateMerchant(oldPhone, newU, newP, newShop, newBoss, newPhone, newLoc);
                system("pause"); break;
            case 4:
                printf("要删除的老板电话: "); scanf("%s", phone);
                deleteMerchant(phone);
                system("pause"); break;
            case 5: printAllCustomers(); system("pause"); break;
            case 6:
                printf("用户名: "); scanf("%s", username);
                printf("密码: "); scanf("%s", password);
                if (checkPasswordStrength(password) < 2) { printf("密码强度不足\n"); system("pause"); break; }
                printf("姓名: "); scanf("%s", name);
                printf("手机号: "); scanf("%s", phone);
                addCustomer(username, password, name, phone);
                system("pause"); break;
            case 7:
                printf("要修改的顾客当前手机号: "); scanf("%s", oldPhone);
                printf("新用户名: "); scanf("%s", newU);
                printf("新密码: "); scanf("%s", newP);
                if (checkPasswordStrength(newP) < 2) { printf("密码强度不足\n"); system("pause"); break; }
                printf("新姓名: "); scanf("%s", newName);
                printf("新手机号: "); scanf("%s", newPhone);
                updateCustomer(oldPhone, newU, newP, newName, newPhone);
                system("pause"); break;
            case 8:
                printf("要删除的手机号: "); scanf("%s", phone);
                deleteCustomer(phone);
                system("pause"); break;
            case 9: printAllRiders(); system("pause"); break;
            case 10:
                printf("用户名: "); scanf("%s", username);
                printf("密码: "); scanf("%s", password);
                if (checkPasswordStrength(password) < 2) { printf("密码强度不足\n"); system("pause"); break; }
                printf("姓名: "); scanf("%s", name);
                printf("手机号: "); scanf("%s", phone);
                printf("区域（地图节点）: "); scanf("%s", loc);
                addRider(username, password, name, phone, loc, 0, 0);
                system("pause"); break;
            case 11:
                printf("要修改的骑手当前手机号: "); scanf("%s", oldPhone);
                printf("新用户名: "); scanf("%s", newU);
                printf("新密码: "); scanf("%s", newP);
                if (checkPasswordStrength(newP) < 2) { printf("密码强度不足\n"); system("pause"); break; }
                printf("新姓名: "); scanf("%s", newName);
                printf("新手机号: "); scanf("%s", newPhone);
                printf("新区域: "); scanf("%s", newLoc);
                updateRider(oldPhone, newU, newP, newName, newPhone, newLoc, 0, 0);
                system("pause"); break;
            case 12:
                printf("要删除的手机号: "); scanf("%s", phone);
                deleteRider(phone);
                system("pause"); break;
            case 13:
                while (1) {
                    system("cls");
                    printf("\n===== 地图管理 =====\n");
                    printf("1. 查看地图\n2. 添加节点\n3. 删除节点\n4. 添加路线\n5. 删除路线\n6. 测最短路径\n7. 保存地图\n8. 重新加载地图\n0. 返回\n");
                    printf("请选择: ");
                    scanf("%d", &sub);
                    if (sub == 0) break;
                    switch (sub) {
                        case 1: map_print(&g_map); system("pause"); break;
                        case 2: printf("节点名: "); scanf("%s", node1); map_add_node(&g_map, node1); system("pause"); break;
                        case 3: printf("节点名: "); scanf("%s", node1); map_del_node(&g_map, node1); system("pause"); break;
                        case 4: printf("起点 终点 距离: "); scanf("%s %s %d", node1, node2, &dist); map_add_edge(&g_map, node1, node2, dist); system("pause"); break;
                        case 5: printf("起点 终点: "); scanf("%s %s", node1, node2); map_del_edge(&g_map, node1, node2); system("pause"); break;
                        case 6: {
                            char path[200]; int d;
                            printf("起点 终点: "); scanf("%s %s", node1, node2);
                            if (map_dijkstra(&g_map, node1, node2, path, sizeof(path), &d) == 0)
                                printf("最短路径: %s  距离: %d km\n", path, d);
                            else printf("无法到达或节点不存在\n");
                            system("pause");
                            break;
                        }
                        case 7: map_save(&g_map); printf("已保存\n"); system("pause"); break;
                        case 8: map_init(&g_map); map_load(&g_map); printf("已重新加载\n"); system("pause"); break;
                        default: printf("无效选项\n"); system("pause");
                    }
                }
                break;
            default: printf("无效选项\n"); system("pause");
        }
    }
}

/* ------------------------- 12. 管理员登录 ------------------------- */
#define ADMIN_FILE "admin.txt"
void loadAdmin() {
    FILE *fp = fopen(ADMIN_FILE, "r");
    if (!fp) {
        fp = fopen(ADMIN_FILE, "w");
        if (fp) { fprintf(fp, "admin 123456\n"); fclose(fp); }
        return;
    }
    fclose(fp);
}
int checkAdmin(const char *username, const char *password) {
    FILE *fp = fopen(ADMIN_FILE, "r");
    if (!fp) return 0;
    char u[30], p[30];
    int ok = 0;
    if (fscanf(fp, "%s %s", u, p) == 2) {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0) ok = 1;
    }
    fclose(fp);
    return ok;
}

/* ------------------------- 13. 主程序 ------------------------- */
int main() {
    loadData();
    loadAdmin();
    int role;
    char uname[MAX_USERNAME], pwd[MAX_PASSWORD];

    while (1) {
        system("cls");
        printf("\n========== 欢迎使用校园外卖系统 ==========\n");
        printf("1. 顾客登录\n2. 商家登录\n3. 骑手登录\n");
        printf("4. 注册新账号\n5. 管理员登录\n0. 退出系统\n");
        printf("请输入数字选项: ");
        scanf("%d", &role);
        getchar();

        if (role == 0) { printf("感谢使用，再见！\n"); break; }
        if (role == 4) { registerMenu(); continue; }

        if (role == 5) {
            printf("请输入管理员用户名: "); scanf("%s", uname);
            printf("请输入管理员密码: "); scanf("%s", pwd);
            if (checkAdmin(uname, pwd)) {
                adminMenu();
            } else {
                printf("管理员用户名或密码错误！\n");
                system("pause");
            }
            continue;
        }

        printf("请输入用户名: "); scanf("%s", uname);
        printf("请输入密码: "); scanf("%s", pwd);

        int loginSuccess = 0;
        if (role == 1) {
            CustomerNode *p = customerHead;
            while (p) {
                if (strcmp(p->username, uname) == 0 && strcmp(p->password, pwd) == 0) {
                    customerMenu(p);
                    loginSuccess = 1;
                    break;
                }
                p = p->next;
            }
        } else if (role == 2) {
            MerchantNode *p = merchantHead;
            while (p) {
                if (strcmp(p->username, uname) == 0 && strcmp(p->password, pwd) == 0) {
                    merchantMenu(p);
                    loginSuccess = 1;
                    break;
                }
                p = p->next;
            }
        } else if (role == 3) {
            RiderNode *p = riderHead;
            while (p) {
                if (strcmp(p->username, uname) == 0 && strcmp(p->password, pwd) == 0) {
                    riderMenu(p);
                    loginSuccess = 1;
                    break;
                }
                p = p->next;
            }
        }
        if (!loginSuccess && role >= 1 && role <= 3) {
            printf("用户名或密码错误！\n");
            system("pause");
        }
    }
    saveAll();
    return 0;
}
