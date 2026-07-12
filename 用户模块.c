/* =====================================================================
 *  用户模块.c —— 外卖系统 用户部分（单文件版）
 *
 *  功能：
 *    1. 用户名 + 密码 注册 / 登录  —— 账号保存在动态链表中
 *    2. 身份选择：0 商家 / 1 骑手 / 2 客户
 *    3. 登录后可修改用户名、密码
 *    4. 客户可：修改收货地址、查询商品
 *    5. 客户可提交订单 —— 订单自动写入商家与骑手可见的数据文件
 *
 *  数据组织：三张「动态单向链表」分别维护 用户 / 商品 / 订单
 *  持久化：data/users.txt  data/products.txt  data/orders.txt
 *          文件编码为 ANSI（中文 Windows 即 GBK），与控制台编码一致
 * ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

/* ============================ 常量 ============================ */
#define DATA_DIR        "data"
#define USER_FILE       DATA_DIR "/users.txt"
#define PRODUCT_FILE    DATA_DIR "/products.txt"
#define ORDER_FILE      DATA_DIR "/orders.txt"

#define MAX_NAME        32
#define MAX_PASS        32
#define MAX_ADDR        128
#define MAX_PROD_NAME   64
#define MAX_DESC        128

/* ============================ 结构体定义 ============================ */

/* 用户身份 */
typedef enum {
    IDENTITY_MERCHANT = 0,
    IDENTITY_RIDER    = 1,
    IDENTITY_CUSTOMER = 2
} Identity;

/* 订单状态 */
typedef enum {
    ORDER_PENDING    = 0,   /* 已下单，等待商家接单 */
    ORDER_ACCEPTED   = 1,   /* 商家已接单，等待骑手 */
    ORDER_DELIVERING = 2,   /* 配送中 */
    ORDER_COMPLETED  = 3,   /* 已完成 */
    ORDER_CANCELED   = 4
} OrderStatus;

/* 用户链表节点（账号数据） */
typedef struct UserNode {
    char    username[MAX_NAME];
    char    password[MAX_PASS];
    Identity identity;
    char    address[MAX_ADDR];          /* 仅客户使用：收货地址 */
    struct UserNode *next;
} UserNode;

/* 商品链表节点 */
typedef struct ProductNode {
    int     id;
    char    name[MAX_PROD_NAME];
    char    merchant[MAX_NAME];
    double  price;
    char    description[MAX_DESC];
    struct ProductNode *next;
} ProductNode;

/* 订单链表节点 */
typedef struct OrderNode {
    int         order_id;
    char        customer[MAX_NAME];
    char        merchant[MAX_NAME];
    char        rider[MAX_NAME];        /* 接单骑手，未接单时为空串 */
    int         product_id;
    char        product_name[MAX_PROD_NAME];
    double      unit_price;
    int         quantity;
    double      total_price;
    char        delivery_address[MAX_ADDR];
    OrderStatus status;
    struct OrderNode *next;
} OrderNode;

/* ============================ 全局动态链表 ============================ */
static UserNode    *g_user_list    = NULL;   /* 账号链表头 */
static ProductNode *g_product_list = NULL;   /* 商品链表头 */
static OrderNode   *g_order_list   = NULL;   /* 订单链表头 */
static int          g_next_order_id = 1000;  /* 订单号自增起点 */

/* ============================ 通用工具 ============================ */

/* 确保 data/ 目录存在 */
static void ensure_data_dir(void) {
#ifdef _WIN32
    _mkdir(DATA_DIR);
#else
    mkdir(DATA_DIR, 0755);
#endif
}

/* 清屏：Windows 用 cls，其它用 clear */
static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* 从 stdin 读一行：去掉行尾换行；防御性跳过可能存在的 BOM */
static void read_line(char *buf, int n) {
    if (!fgets(buf, n, stdin)) { buf[0] = '\0'; return; }
    if ((unsigned char)buf[0] == 0xEF &&
        (unsigned char)buf[1] == 0xBB &&
        (unsigned char)buf[2] == 0xBF) {
        memmove(buf, buf + 3, strlen(buf + 3) + 1);
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
}

/* 把枚举值翻译成中文标签 */
static const char* id_str(Identity id) {
    switch (id) {
        case IDENTITY_MERCHANT: return "商家";
        case IDENTITY_RIDER:    return "骑手";
        case IDENTITY_CUSTOMER: return "客户";
        default:                return "未知";
    }
}

static const char* order_status_str(OrderStatus s) {
    switch (s) {
        case ORDER_PENDING:    return "待商家接单";
        case ORDER_ACCEPTED:   return "待骑手接单";
        case ORDER_DELIVERING: return "配送中";
        case ORDER_COMPLETED:  return "已完成";
        case ORDER_CANCELED:   return "已取消";
        default:               return "未知";
    }
}

/* =====================================================================
 *  模块一：用户账号（动态链表 g_user_list）
 *  链表操作统一使用「尾插法」：新节点追加到链表末尾
 * ===================================================================== */

/* 在指定身份下查找用户名是否已被注册 */
static int user_exists(const char *username, Identity id) {
    for (UserNode *p = g_user_list; p; p = p->next)
        if (p->identity == id && strcmp(p->username, username) == 0) return 1;
    return 0;
}

/* 释放整个用户链表 */
static void user_list_clear(void) {
    UserNode *p = g_user_list;
    while (p) { UserNode *t = p->next; free(p); p = t; }
    g_user_list = NULL;
}

/* 注册新用户：成功返回 1，失败（重名/空输入）返回 0 */
static int user_register(const char *username, const char *password, Identity id) {
    if (!username || !password || username[0] == '\0' || password[0] == '\0') return 0;
    if (user_exists(username, id)) return 0;
    UserNode *node = (UserNode *)malloc(sizeof(UserNode));
    if (!node) return 0;
    strncpy(node->username, username, MAX_NAME - 1); node->username[MAX_NAME - 1] = '\0';
    strncpy(node->password, password, MAX_PASS - 1); node->password[MAX_PASS - 1] = '\0';
    node->identity = id;
    node->address[0] = '\0';
    node->next = NULL;
    if (!g_user_list) g_user_list = node;
    else { UserNode *p = g_user_list; while (p->next) p = p->next; p->next = node; }
    return 1;
}

/* 登录校验：成功返回对应用户节点，失败返回 NULL */
static UserNode* user_login(const char *username, const char *password, Identity id) {
    for (UserNode *p = g_user_list; p; p = p->next)
        if (p->identity == id &&
            strcmp(p->username, username) == 0 &&
            strcmp(p->password, password) == 0) return p;
    return NULL;
}

/* 修改已登录用户的用户名（同名同身份下不可重名） */
static void user_modify_username(UserNode *u, const char *new_username) {
    if (!u || !new_username || new_username[0] == '\0') return;
    if (user_exists(new_username, u->identity) &&
        strcmp(new_username, u->username) != 0) {
        printf("用户名已被占用！\n");
        return;
    }
    strncpy(u->username, new_username, MAX_NAME - 1);
    u->username[MAX_NAME - 1] = '\0';
    printf("用户名已修改。\n");
}

/* 修改已登录用户的密码 */
static void user_modify_password(UserNode *u, const char *new_password) {
    if (!u || !new_password || new_password[0] == '\0') return;
    strncpy(u->password, new_password, MAX_PASS - 1);
    u->password[MAX_PASS - 1] = '\0';
    printf("密码已修改。\n");
}

/* 客户修改自己的收货地址 */
static void customer_modify_address(UserNode *u, const char *addr) {
    if (!u || u->identity != IDENTITY_CUSTOMER || !addr) return;
    strncpy(u->address, addr, MAX_ADDR - 1);
    u->address[MAX_ADDR - 1] = '\0';
    printf("收货地址已更新。\n");
}

/* users.txt 文件格式：每行一条记录
 *     username|password|identity|address         （identity: 0/1/2） */
static void user_save_to_file(void) {
    FILE *fp = fopen(USER_FILE, "w");
    if (!fp) { perror("user_save"); return; }
    for (UserNode *p = g_user_list; p; p = p->next)
        fprintf(fp, "%s|%s|%d|%s\n", p->username, p->password, (int)p->identity, p->address);
    fclose(fp);
}

static void user_load_from_file(void) {
    FILE *fp = fopen(USER_FILE, "r");
    if (!fp) return;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        char user[MAX_NAME] = {0}, pass[MAX_PASS] = {0}, addr[MAX_ADDR] = {0};
        int id = 2;
        char *tok = strtok(line,  "|"); if (!tok) continue; strncpy(user, tok, MAX_NAME - 1);
        tok     = strtok(NULL, "|");    if (!tok) continue; strncpy(pass, tok, MAX_PASS - 1);
        tok     = strtok(NULL, "|");    if (!tok) continue; id = atoi(tok);
        tok     = strtok(NULL, "|");    if (tok)           strncpy(addr, tok, MAX_ADDR - 1);
        user_register(user, pass, (Identity)id);
        /* user_register 会忽略 address，这里把 address 补回新节点 */
        if (g_user_list) {
            UserNode *p = g_user_list;
            while (p->next) p = p->next;
            strncpy(p->address, addr, MAX_ADDR - 1);
            p->address[MAX_ADDR - 1] = '\0';
        }
    }
    fclose(fp);
}

/* =====================================================================
 *  模块二：商品（动态链表 g_product_list）
 * ===================================================================== */

static void product_list_clear(void) {
    ProductNode *p = g_product_list;
    while (p) { ProductNode *t = p->next; free(p); p = t; }
    g_product_list = NULL;
}

/* 在商品链表末尾追加一个商品节点 */
static void product_add(int id, const char *name, const char *merchant,
                        double price, const char *desc) {
    if (!name || !merchant) return;
    ProductNode *node = (ProductNode *)malloc(sizeof(ProductNode));
    if (!node) return;
    node->id = id;
    strncpy(node->name, name, MAX_PROD_NAME - 1);       node->name[MAX_PROD_NAME - 1] = '\0';
    strncpy(node->merchant, merchant, MAX_NAME - 1);    node->merchant[MAX_NAME - 1] = '\0';
    node->price = price;
    strncpy(node->description, desc ? desc : "", MAX_DESC - 1);
    node->description[MAX_DESC - 1] = '\0';
    node->next = NULL;
    if (!g_product_list) g_product_list = node;
    else { ProductNode *p = g_product_list; while (p->next) p = p->next; p->next = node; }
}

/* 按商品 ID 查找 */
static ProductNode* product_find_by_id(int id) {
    for (ProductNode *p = g_product_list; p; p = p->next)
        if (p->id == id) return p;
    return NULL;
}

/* 打印所有商品 */
static void product_show_all(void) {
    if (!g_product_list) { printf("  (暂无商品)\n"); return; }
    printf("  %-4s %-16s %-16s %-8s %s\n", "ID", "名称", "商家", "价格", "描述");
    printf("  ----------------------------------------------------------------\n");
    for (ProductNode *p = g_product_list; p; p = p->next)
        printf("  %-4d %-16s %-16s %.2f   %s\n",
               p->id, p->name, p->merchant, p->price, p->description);
}

/* 首次启动时种入一批示例商品，便于客户查询/下单 */
static void product_seed_sample(void) {
    product_add(1001, "黄焖鸡米饭", "merchant_zhang", 18.0, "秘制酱汁，配料丰富");
    product_add(1002, "兰州拉面",   "merchant_zhang", 16.0, "牛肉拉面");
    product_add(1003, "麻辣香锅",   "merchant_li",    28.5, "可自选配菜");
    product_add(1004, "宫保鸡丁",   "merchant_li",    20.0, "经典川菜");
    product_add(1005, "珍珠奶茶",   "merchant_wang",   8.0, "大杯/少糖");
}

/* products.txt 文件格式：每行一个商品
 *     id|name|merchant|price|description */
static void product_save_to_file(void) {
    FILE *fp = fopen(PRODUCT_FILE, "w");
    if (!fp) { perror("product_save"); return; }
    for (ProductNode *p = g_product_list; p; p = p->next)
        fprintf(fp, "%d|%s|%s|%.2f|%s\n",
                p->id, p->name, p->merchant, p->price, p->description);
    fclose(fp);
}

static void product_load_from_file(void) {
    FILE *fp = fopen(PRODUCT_FILE, "r");
    if (!fp) return;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        int id; double price;
        char name[MAX_PROD_NAME] = {0}, merchant[MAX_NAME] = {0}, desc[MAX_DESC] = {0};
        char *tok = strtok(line, "|");  if (!tok) continue; id = atoi(tok);
        tok     = strtok(NULL, "|");    if (!tok) continue; strncpy(name, tok, MAX_PROD_NAME - 1);
        tok     = strtok(NULL, "|");    if (!tok) continue; strncpy(merchant, tok, MAX_NAME - 1);
        tok     = strtok(NULL, "|");    if (!tok) continue; price = atof(tok);
        tok     = strtok(NULL, "|");    if (tok)           strncpy(desc, tok, MAX_DESC - 1);
        product_add(id, name, merchant, price, desc);
    }
    fclose(fp);
}

/* =====================================================================
 *  模块三：订单（动态链表 g_order_list）
 *  客户下单：写入本进程链表 + 立即落盘到 data/orders.txt
 *           商家 / 骑手模块读取同一文件即可看到订单
 * ===================================================================== */

static void order_list_clear(void) {
    OrderNode *p = g_order_list;
    while (p) { OrderNode *t = p->next; free(p); p = t; }
    g_order_list = NULL;
}

/* 客户下单：把订单节点追加到链表尾部，返回订单号 */
static int order_create(const char *customer, const char *merchant,
                        int product_id, const char *product_name,
                        double price, int quantity, const char *address) {
    if (!customer || !merchant || !product_name || quantity <= 0 || !address) return 0;
    OrderNode *node = (OrderNode *)malloc(sizeof(OrderNode));
    if (!node) return 0;
    node->order_id = g_next_order_id++;
    strncpy(node->customer, customer, MAX_NAME - 1);       node->customer[MAX_NAME - 1] = '\0';
    strncpy(node->merchant, merchant, MAX_NAME - 1);       node->merchant[MAX_NAME - 1] = '\0';
    node->rider[0] = '\0';
    node->product_id = product_id;
    strncpy(node->product_name, product_name, MAX_PROD_NAME - 1);
    node->product_name[MAX_PROD_NAME - 1] = '\0';
    node->unit_price = price;
    node->quantity = quantity;
    node->total_price = price * quantity;
    strncpy(node->delivery_address, address, MAX_ADDR - 1);
    node->delivery_address[MAX_ADDR - 1] = '\0';
    node->status = ORDER_PENDING;
    node->next = NULL;
    if (!g_order_list) g_order_list = node;
    else { OrderNode *p = g_order_list; while (p->next) p = p->next; p->next = node; }
    return node->order_id;
}

/* orders.txt 文件格式：每行一个订单
 *   order_id|customer|merchant|rider|product_id|product_name|
 *   unit_price|quantity|total_price|address|status
 *   状态 status: 0=待商家接单 1=待骑手接单 2=配送中 3=已完成 4=已取消
 *   商家 / 骑手模块按 merchant / rider 字段过滤即可 */
static void order_save_to_file(void) {
    FILE *fp = fopen(ORDER_FILE, "w");
    if (!fp) { perror("order_save"); return; }
    for (OrderNode *p = g_order_list; p; p = p->next)
        fprintf(fp, "%d|%s|%s|%s|%d|%s|%.2f|%d|%.2f|%s|%d\n",
                p->order_id, p->customer, p->merchant, p->rider,
                p->product_id, p->product_name, p->unit_price, p->quantity,
                p->total_price, p->delivery_address, (int)p->status);
    fclose(fp);
}

static void order_load_from_file(void) {
    FILE *fp = fopen(ORDER_FILE, "r");
    if (!fp) return;
    char line[1024];
    int max_id = 999;
    while (fgets(line, sizeof(line), fp)) {
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        int oid, pid, qty, status; double price, total;
        char cust[MAX_NAME] = {0}, merch[MAX_NAME] = {0}, rider[MAX_NAME] = {0};
        char pname[MAX_PROD_NAME] = {0}, addr[MAX_ADDR] = {0};
        char *tok = strtok(line,  "|"); if (!tok) continue; oid = atoi(tok);
        tok     = strtok(NULL, "|");    if (!tok) continue; strncpy(cust, tok, MAX_NAME - 1);
        tok     = strtok(NULL, "|");    if (!tok) continue; strncpy(merch, tok, MAX_NAME - 1);
        tok     = strtok(NULL, "|");    if (!tok) continue; strncpy(rider, tok, MAX_NAME - 1);
        tok     = strtok(NULL, "|");    if (!tok) continue; pid = atoi(tok);
        tok     = strtok(NULL, "|");    if (!tok) continue; strncpy(pname, tok, MAX_PROD_NAME - 1);
        tok     = strtok(NULL, "|");    if (!tok) continue; price = atof(tok);
        tok     = strtok(NULL, "|");    if (!tok) continue; qty = atoi(tok);
        tok     = strtok(NULL, "|");    if (!tok) continue; total = atof(tok);
        tok     = strtok(NULL, "|");    if (!tok) continue; strncpy(addr, tok, MAX_ADDR - 1);
        tok     = strtok(NULL, "|");    if (!tok) continue; status = atoi(tok);

        OrderNode *node = (OrderNode *)malloc(sizeof(OrderNode));
        if (!node) break;
        node->order_id = oid;
        strncpy(node->customer, cust, MAX_NAME - 1);     node->customer[MAX_NAME - 1] = '\0';
        strncpy(node->merchant, merch, MAX_NAME - 1);    node->merchant[MAX_NAME - 1] = '\0';
        strncpy(node->rider, rider, MAX_NAME - 1);       node->rider[MAX_NAME - 1] = '\0';
        node->product_id = pid;
        strncpy(node->product_name, pname, MAX_PROD_NAME - 1);
        node->product_name[MAX_PROD_NAME - 1] = '\0';
        node->unit_price = price;
        node->quantity = qty;
        node->total_price = total;
        strncpy(node->delivery_address, addr, MAX_ADDR - 1);
        node->delivery_address[MAX_ADDR - 1] = '\0';
        node->status = (OrderStatus)status;
        node->next = NULL;
        if (!g_order_list) g_order_list = node;
        else { OrderNode *p = g_order_list; while (p->next) p = p->next; p->next = node; }
        if (oid > max_id) max_id = oid;
    }
    fclose(fp);
    g_next_order_id = max_id + 1;
}

/* =====================================================================
 *  模块四：客户业务（登录后的菜单与各功能）
 * ===================================================================== */

/* 浏览全部商品 */
static void customer_browse_products(void) {
    printf("\n--- 全部商品 ---\n");
    product_show_all();
}

/* 按关键字搜索商品（匹配商品名） */
static void customer_search_product(void) {
    char kw[64];
    printf("输入关键字: ");
    read_line(kw, sizeof(kw));
    if (kw[0] == '\0') return;
    printf("\n--- 搜索结果 ---\n");
    int found = 0;
    for (ProductNode *p = g_product_list; p; p = p->next) {
        if (strstr(p->name, kw)) {
            if (!found) {
                printf("  %-4s %-16s %-16s %-8s %s\n",
                       "ID", "名称", "商家", "价格", "描述");
                printf("  ----------------------------------------------------------------\n");
            }
            printf("  %-4d %-16s %-16s %.2f   %s\n",
                   p->id, p->name, p->merchant, p->price, p->description);
            found = 1;
        }
    }
    if (!found) printf("  没有匹配的商品。\n");
}

/* 客户下单：选商品 → 选数量 → 选地址 → 写入订单链表 + 落盘 */
static void customer_place_order(UserNode *u) {
    customer_browse_products();
    char buf[64];
    printf("\n请输入要下单的商品ID: ");
    read_line(buf, sizeof(buf));
    int pid = atoi(buf);
    ProductNode *p = product_find_by_id(pid);
    if (!p) { printf("商品不存在。\n"); return; }

    printf("请输入购买数量: ");
    read_line(buf, sizeof(buf));
    int qty = atoi(buf);
    if (qty <= 0) { printf("数量无效。\n"); return; }

    char addr[MAX_ADDR];
    if (u->address[0] != '\0') {
        printf("默认收货地址: %s\n", u->address);
        printf("直接回车使用默认地址，或输入新地址: ");
        read_line(addr, sizeof(addr));
        if (addr[0] == '\0') strncpy(addr, u->address, MAX_ADDR - 1);
    } else {
        printf("请输入收货地址: ");
        read_line(addr, sizeof(addr));
    }
    if (addr[0] == '\0') { printf("地址不能为空。\n"); return; }

    int oid = order_create(u->username, p->merchant, p->id, p->name,
                           p->price, qty, addr);
    if (oid > 0) {
        printf("\n下单成功！订单号: %d\n", oid);
        printf("商品: %s x %d  合计: %.2f\n", p->name, qty, p->price * qty);
        printf("订单已自动推送给商家 [%s] 与接单骑手。\n", p->merchant);
        customer_modify_address(u, addr);
    } else {
        printf("下单失败。\n");
    }
}

/* 列出当前客户的所有订单 */
static void customer_view_my_orders(UserNode *u) {
    printf("\n--- 我的订单 ---\n");
    printf("  %-6s %-12s %-4s %-6s %-10s %s\n",
           "订单号", "商品", "数量", "金额", "状态", "收货地址");
    printf("  ----------------------------------------------------------------\n");
    int found = 0;
    for (OrderNode *o = g_order_list; o; o = o->next) {
        if (strcmp(o->customer, u->username) == 0) {
            printf("  %-6d %-12s %-4d %.2f   %-10s %s\n",
                   o->order_id, o->product_name, o->quantity,
                   o->total_price, order_status_str(o->status), o->delivery_address);
            found = 1;
        }
    }
    if (!found) printf("  (暂无订单)\n");
}

/* 客户修改自己的收货地址 */
static void customer_change_address(UserNode *u) {
    char buf[MAX_ADDR];
    printf("请输入新的收货地址: ");
    read_line(buf, sizeof(buf));
    if (buf[0] == '\0') { printf("地址不能为空。\n"); return; }
    customer_modify_address(u, buf);
}

/* 客户主菜单循环
 * 菜单本身不清屏；进入具体功能前先清屏，确保看到的是干净的功能界面 */
static void customer_run(UserNode *u) {
    char buf[64];
    while (1) {
        printf("\n========== 客户中心 (当前用户: %s) ==========\n", u->username);
        printf("  1. 修改收货地址\n");
        printf("  2. 浏览全部商品\n");
        printf("  3. 搜索商品\n");
        printf("  4. 提交订单\n");
        printf("  5. 查看我的订单\n");
        printf("  6. 修改用户名\n");
        printf("  7. 修改密码\n");
        printf("  0. 退出登录\n");
        printf("请选择: ");
        read_line(buf, sizeof(buf));
        int c = atoi(buf);
        switch (c) {
            case 1: clear_screen(); customer_change_address(u); break;
            case 2: clear_screen(); customer_browse_products(); break;
            case 3: clear_screen(); customer_search_product();  break;
            case 4: clear_screen(); customer_place_order(u);     break;
            case 5: clear_screen(); customer_view_my_orders(u);  break;
            case 6: {
                clear_screen();
                char nu[MAX_NAME];
                printf("新用户名: "); read_line(nu, sizeof(nu));
                user_modify_username(u, nu);
                break;
            }
            case 7: {
                clear_screen();
                char np[MAX_PASS];
                printf("新密码: "); read_line(np, sizeof(np));
                user_modify_password(u, np);
                break;
            }
            case 0: return;
            default: printf("无效选项。\n");
        }
    }
}

/* =====================================================================
 *  模块五：身份选择 与 登录/注册入口
 * ===================================================================== */

/* 身份选择菜单：返回 0=商家 1=骑手 2=客户，q 退出返回 -1 */
static int select_identity(void) {
    char buf[16];
    while (1) {
        printf("\n========== 外卖系统 · 请选择身份 ==========\n");
        printf("  0 - 商家\n");
        printf("  1 - 骑手\n");
        printf("  2 - 客户\n");
        printf("  q - 退出\n");
        printf("请输入: ");
        read_line(buf, sizeof(buf));
        if (buf[0] == 'q' || buf[0] == 'Q') return -1;
        int id = atoi(buf);
        if (id >= 0 && id <= 2) return id;
        printf("输入无效，请重新选择。\n");
    }
}

/* 单个身份的登录 / 注册入口
 * 菜单本身不清屏；进入登录 / 注册操作前先清屏 */
static UserNode* entry_for_identity(Identity id) {
    char buf[16];
    while (1) {
        printf("\n[%s] 1.登录  2.注册  0.返回上层: ", id_str(id));
        read_line(buf, sizeof(buf));
        if (buf[0] == '0') return NULL;
        if (buf[0] == '1') {
            clear_screen();
            char user[MAX_NAME], pass[MAX_PASS];
            printf("用户名: "); read_line(user, sizeof(user));
            printf("密码:   "); read_line(pass, sizeof(pass));
            UserNode *u = user_login(user, pass, id);
            if (u) {
                printf("登录成功！欢迎 %s (身份: %s)\n", u->username, id_str(u->identity));
                return u;
            }
            printf("登录失败：用户名或密码错误。\n");
        } else if (buf[0] == '2') {
            clear_screen();
            char user[MAX_NAME], pass[MAX_PASS];
            printf("用户名: "); read_line(user, sizeof(user));
            printf("密码:   "); read_line(pass, sizeof(pass));
            if (user_register(user, pass, id))
                printf("注册成功！请使用新账号登录。\n");
            else
                printf("注册失败：用户名已存在或输入不合法。\n");
        } else {
            printf("无效选项。\n");
        }
    }
}

/* 商家 / 骑手登录后：展示与其相关的订单信息，并询问是否修改密码
 * （商家接单 / 骑手抢单的实际业务由其他模块实现） */
static void merchant_or_rider_view(UserNode *u) {
    printf("\n[%s %s] 已登录。\n", id_str(u->identity), u->username);
    if (u->identity == IDENTITY_MERCHANT) {
        printf("\n待处理订单:\n");
        int any = 0;
        for (OrderNode *o = g_order_list; o; o = o->next) {
            if (strcmp(o->merchant, u->username) == 0) {
                printf("  #%d  %s x%d  状态:%s  地址:%s\n",
                       o->order_id, o->product_name, o->quantity,
                       order_status_str(o->status), o->delivery_address);
                any = 1;
            }
        }
        if (!any) printf("  (暂无)\n");
    } else {
        printf("\n可接订单:\n");
        int any = 0;
        for (OrderNode *o = g_order_list; o; o = o->next) {
            if (o->status == ORDER_ACCEPTED && o->rider[0] == '\0') {
                printf("  #%d  %s x%d  商家:%s  地址:%s\n",
                       o->order_id, o->product_name, o->quantity,
                       o->merchant, o->delivery_address);
                any = 1;
            }
        }
        if (!any) printf("  (暂无)\n");
    }
    {
        char buf[16];
        clear_screen();
        printf("\n是否修改密码? (y/n): ");
        read_line(buf, sizeof(buf));
        if (buf[0] == 'y' || buf[0] == 'Y') {
            char np[MAX_PASS];
            printf("新密码: "); read_line(np, sizeof(np));
            user_modify_password(u, np);
        }
    }
}

/* =====================================================================
 *  主程序入口
 * ===================================================================== */
int main(void) {
    ensure_data_dir();

    /* 启动时把已有数据装载进三个动态链表 */
    user_load_from_file();
    product_load_from_file();
    if (!g_product_list) {              /* 首次启动种入示例商品并落盘 */
        product_seed_sample();
        product_save_to_file();
    }
    order_load_from_file();

    /* 显示欢迎界面 */
    clear_screen();
    printf("=========================================\n");
    printf("     欢迎使用 外卖点餐系统\n");
    printf("=========================================\n");
    printf("数据文件: %s/{users,products,orders}.txt\n", DATA_DIR);

    /* 主循环：选择身份 -> 登录/注册 -> 进入对应功能 */
    while (1) {
        int id = select_identity();
        if (id < 0) break;

        UserNode *u = entry_for_identity((Identity)id);
        if (!u) continue;

        switch (u->identity) {
            case IDENTITY_CUSTOMER:
                customer_run(u);
                break;
            case IDENTITY_MERCHANT:
            case IDENTITY_RIDER:
                merchant_or_rider_view(u);
                break;
        }
    }

    /* 程序退出前：所有链表数据落盘 */
    user_save_to_file();
    product_save_to_file();
    order_save_to_file();

    /* 释放三张动态链表 */
    user_list_clear();
    product_list_clear();
    order_list_clear();

    printf("再见！\n");
    return 0;
}
