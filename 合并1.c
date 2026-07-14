#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>   // for isdigit, isalpha

// ================= 全局常量定义 =================
#define MAX_USERNAME 30
#define MAX_PASSWORD 30
#define MAX_NAME 50
#define MAX_PHONE 20
#define MAX_SHOP 50
#define MAX_ADDR 100
#define MAX_DISH 50
#define MAX_KEYWORD 50

// ================= 1. 结构体定义 =================

// 商家结点（含登录信息、个人资料、菜品链表）
typedef struct DishNode {
    int id;
    char name[MAX_DISH];
    double price;
    struct DishNode *next;
} DishNode;

typedef struct MerchantNode {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char shopName[MAX_SHOP];
    char bossName[MAX_NAME];
    char bossPhone[MAX_PHONE];   // 作为唯一标识
    DishNode *dishList;
    struct MerchantNode *next;
} MerchantNode;

// 顾客结点（含登录信息、个人资料、地址链表）
typedef struct AddressNode {
    int id;
    char detail[MAX_ADDR];
    struct AddressNode *next;
} AddressNode;

typedef struct CustomerNode {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char name[MAX_NAME];
    char phone[MAX_PHONE];      // 作为唯一标识
    AddressNode *addrList;
    struct CustomerNode *next;
} CustomerNode;

// 骑手结点（含登录信息、个人资料）
typedef struct RiderNode {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char name[MAX_NAME];
    char phone[MAX_PHONE];      // 作为唯一标识
    struct RiderNode *next;
} RiderNode;

// 订单结点
typedef struct OrderNode {
    int orderId;
    char customerName[MAX_USERNAME];
    char dishName[MAX_DISH];
    char address[MAX_ADDR];
    int status; // 0:待接单, 1:已接单
    struct OrderNode *next;
} OrderNode;
// ================= 新增：管理员账号管理 =================
#define ADMIN_FILE "admin.txt"

// 加载管理员账号（若文件不存在则创建默认）
void loadAdmin() {
    FILE *fp = fopen(ADMIN_FILE, "r");
    if (!fp) {
        // 文件不存在，创建默认管理员
        fp = fopen(ADMIN_FILE, "w");
        if (fp) {
            fprintf(fp, "admin 123456\n");  // 默认账号
            fclose(fp);
        }
        return;
    }
    fclose(fp);
}

// 验证管理员登录
int checkAdmin(const char *username, const char *password) {
    FILE *fp = fopen(ADMIN_FILE, "r");
    if (!fp) return 0;
    char u[30], p[30];
    int ok = 0;
    if (fscanf(fp, "%s %s", u, p) == 2) {
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0)
            ok = 1;
    }
    fclose(fp);
    return ok;
}

// 修改管理员密码（可选功能）
void updateAdminPassword(const char *newPassword) {
    FILE *fp = fopen(ADMIN_FILE, "w");
    if (fp) {
        // 保持用户名不变（假设为 admin），只改密码
        fprintf(fp, "admin %s\n", newPassword);
        fclose(fp);
    }
}

// ================= 2. 全局链表头指针 =================
MerchantNode *merchantHead = NULL;
CustomerNode *customerHead = NULL;
RiderNode *riderHead = NULL;
OrderNode *orderHead = NULL;
int globalOrderId = 1000;

// ================= 3. 密码强度校验函数 =================
int checkPasswordStrength(const char *pwd) {
    int hasLetter = 0, hasDigit = 0, hasSymbol = 0;
    for (int i = 0; pwd[i] != '\0'; i++) {
        if (isalpha((unsigned char)pwd[i])) hasLetter = 1;
        else if (isdigit((unsigned char)pwd[i])) hasDigit = 1;
        else hasSymbol = 1;
    }
    return hasLetter + hasDigit + hasSymbol;
}

// ================= 4. 通用辅助函数（字符串安全拷贝） =================
void safeCopy(char *dest, const char *src, size_t size) {
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

// ================= 5. 文件读取与初始化 =================
void loadData() {
    FILE *fp;
    // ----- 加载商家 -----
    fp = fopen("merchants.txt", "r");
    if (fp) {
        MerchantNode *tail = NULL;
        char u[MAX_USERNAME], p[MAX_PASSWORD], shop[MAX_SHOP], boss[MAX_NAME], phone[MAX_PHONE];
        while (fscanf(fp, "%s %s %s %s %s", u, p, shop, boss, phone) == 5) {
            MerchantNode *newM = (MerchantNode*)malloc(sizeof(MerchantNode));
            safeCopy(newM->username, u, MAX_USERNAME);
            safeCopy(newM->password, p, MAX_PASSWORD);
            safeCopy(newM->shopName, shop, MAX_SHOP);
            safeCopy(newM->bossName, boss, MAX_NAME);
            safeCopy(newM->bossPhone, phone, MAX_PHONE);
            newM->dishList = NULL;
            newM->next = NULL;
            if (!merchantHead) merchantHead = newM;
            else tail->next = newM;
            tail = newM;
        }
        fclose(fp);
    }
    // ----- 加载顾客 -----
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
    // ----- 加载骑手 -----
    fp = fopen("riders.txt", "r");
    if (fp) {
        RiderNode *tail = NULL;
        char u[MAX_USERNAME], p[MAX_PASSWORD], name[MAX_NAME], phone[MAX_PHONE];
        while (fscanf(fp, "%s %s %s %s", u, p, name, phone) == 4) {
            RiderNode *newR = (RiderNode*)malloc(sizeof(RiderNode));
            safeCopy(newR->username, u, MAX_USERNAME);
            safeCopy(newR->password, p, MAX_PASSWORD);
            safeCopy(newR->name, name, MAX_NAME);
            safeCopy(newR->phone, phone, MAX_PHONE);
            newR->next = NULL;
            if (!riderHead) riderHead = newR;
            else tail->next = newR;
            tail = newR;
        }
        fclose(fp);
    }
    // ----- 加载订单 -----
    fp = fopen("orders.txt", "r");
    if (fp) {
        OrderNode *tail = NULL;
        int oid, st;
        char cn[MAX_USERNAME], dn[MAX_DISH], addr[MAX_ADDR];
        while (fscanf(fp, "%d %s %s %s %d", &oid, cn, dn, addr, &st) == 5) {
            OrderNode *newO = (OrderNode*)malloc(sizeof(OrderNode));
            newO->orderId = oid;
            safeCopy(newO->customerName, cn, MAX_USERNAME);
            safeCopy(newO->dishName, dn, MAX_DISH);
            safeCopy(newO->address, addr, MAX_ADDR);
            newO->status = st;
            newO->next = NULL;
            if (!orderHead) orderHead = newO;
            else tail->next = newO;
            tail = newO;
            if (oid >= globalOrderId) globalOrderId = oid + 1;
        }
        fclose(fp);
    }
}

// ================= 6. 保存数据到文件 =================
void saveAll() {
    FILE *fp;
    // 保存商家
    fp = fopen("merchants.txt", "w");
    if (fp) {
        MerchantNode *m = merchantHead;
        while (m) {
            fprintf(fp, "%s %s %s %s %s\n", m->username, m->password, m->shopName, m->bossName, m->bossPhone);
            m = m->next;
        }
        fclose(fp);
    }
    // 保存顾客
    fp = fopen("customers.txt", "w");
    if (fp) {
        CustomerNode *c = customerHead;
        while (c) {
            fprintf(fp, "%s %s %s %s\n", c->username, c->password, c->name, c->phone);
            c = c->next;
        }
        fclose(fp);
    }
    // 保存骑手
    fp = fopen("riders.txt", "w");
    if (fp) {
        RiderNode *r = riderHead;
        while (r) {
            fprintf(fp, "%s %s %s %s\n", r->username, r->password, r->name, r->phone);
            r = r->next;
        }
        fclose(fp);
    }
    // 保存订单（注意：订单信息可能已变更，但这里只保存内存中的）
    fp = fopen("orders.txt", "w");
    if (fp) {
        OrderNode *o = orderHead;
        while (o) {
            fprintf(fp, "%d %s %s %s %d\n", o->orderId, o->customerName, o->dishName, o->address, o->status);
            o = o->next;
        }
        fclose(fp);
    }
    // 菜品信息（商家菜品）单独保存？我们依赖商家链表中的 dishList，但为了持久化，我们遍历所有商家并写入 dishes.txt
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
}

// ================= 7. 查找函数（按手机号/电话） =================
MerchantNode* findMerchantByPhone(const char *phone) {
    MerchantNode *curr = merchantHead;
    while (curr) {
        if (strcmp(curr->bossPhone, phone) == 0) return curr;
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

// 查找用户名是否已存在（注册时用）
int findUserByUsername(const char *username) {
    MerchantNode *m = merchantHead;
    while (m) { if (strcmp(m->username, username) == 0) return 1; m = m->next; }
    CustomerNode *c = customerHead;
    while (c) { if (strcmp(c->username, username) == 0) return 1; c = c->next; }
    RiderNode *r = riderHead;
    while (r) { if (strcmp(r->username, username) == 0) return 1; r = r->next; }
    return 0;
}

// ================= 8. 系统管理功能（增删改查） =================

// 8.1 商家管理
void addMerchant(const char *username, const char *password, const char *shop, const char *boss, const char *phone) {
    if (findMerchantByPhone(phone)) {
        printf("老板电话 %s 已存在，添加失败。\n", phone);
        return;
    }
    if (findUserByUsername(username)) {
        printf("用户名 %s 已被使用，添加失败。\n", username);
        return;
    }
    MerchantNode *newM = (MerchantNode*)malloc(sizeof(MerchantNode));
    safeCopy(newM->username, username, MAX_USERNAME);
    safeCopy(newM->password, password, MAX_PASSWORD);
    safeCopy(newM->shopName, shop, MAX_SHOP);
    safeCopy(newM->bossName, boss, MAX_NAME);
    safeCopy(newM->bossPhone, phone, MAX_PHONE);
    newM->dishList = NULL;
    newM->next = merchantHead;
    merchantHead = newM;
    saveAll();
    printf("商家添加成功：%s %s %s\n", shop, boss, phone);
}

void deleteMerchant(const char *phone) {
    MerchantNode *prev = NULL, *curr = merchantHead;
    while (curr && strcmp(curr->bossPhone, phone) != 0) {
        prev = curr;
        curr = curr->next;
    }
    if (!curr) {
        printf("未找到老板电话 %s 的商家。\n", phone);
        return;
    }
    if (prev) prev->next = curr->next;
    else merchantHead = curr->next;
    // 释放菜品链表
    DishNode *d = curr->dishList;
    while (d) { DishNode *tmp = d; d = d->next; free(tmp); }
    free(curr);
    saveAll();
    printf("商家删除成功（老板电话 %s）\n", phone);
}

void updateMerchant(const char *oldPhone, const char *newUsername, const char *newPassword,
                    const char *newShop, const char *newBoss, const char *newPhone) {
    MerchantNode *target = findMerchantByPhone(oldPhone);
    if (!target) {
        printf("未找到老板电话 %s 的商家。\n", oldPhone);
        return;
    }
    // 检查新电话是否被其他商家占用
    if (strcmp(oldPhone, newPhone) != 0) {
        if (findMerchantByPhone(newPhone)) {
            printf("新电话 %s 已被其他商家使用。\n", newPhone);
            return;
        }
    }
    // 检查新用户名是否被其他用户占用（除自身外）
    if (strcmp(target->username, newUsername) != 0) {
        if (findUserByUsername(newUsername)) {
            printf("用户名 %s 已被其他用户使用。\n", newUsername);
            return;
        }
    }
    safeCopy(target->username, newUsername, MAX_USERNAME);
    safeCopy(target->password, newPassword, MAX_PASSWORD);
    safeCopy(target->shopName, newShop, MAX_SHOP);
    safeCopy(target->bossName, newBoss, MAX_NAME);
    safeCopy(target->bossPhone, newPhone, MAX_PHONE);
    saveAll();
    printf("商家信息修改成功。\n");
}

void printAllMerchants() {
    MerchantNode *curr = merchantHead;
    if (!curr) { printf("（无商家数据）\n"); return; }
    int i = 1;
    while (curr) {
        printf("%d. 用户名: %s, 店名: %s, 老板: %s, 电话: %s\n",
               i++, curr->username, curr->shopName, curr->bossName, curr->bossPhone);
        curr = curr->next;
    }
}

// 8.2 顾客管理
void addCustomer(const char *username, const char *password, const char *name, const char *phone) {
    if (findCustomerByPhone(phone)) {
        printf("手机号 %s 已存在，添加失败。\n", phone);
        return;
    }
    if (findUserByUsername(username)) {
        printf("用户名 %s 已被使用，添加失败。\n", username);
        return;
    }
    CustomerNode *newC = (CustomerNode*)malloc(sizeof(CustomerNode));
    safeCopy(newC->username, username, MAX_USERNAME);
    safeCopy(newC->password, password, MAX_PASSWORD);
    safeCopy(newC->name, name, MAX_NAME);
    safeCopy(newC->phone, phone, MAX_PHONE);
    newC->addrList = NULL;
    newC->next = customerHead;
    customerHead = newC;
    saveAll();
    printf("顾客添加成功：%s %s\n", name, phone);
}

void deleteCustomer(const char *phone) {
    CustomerNode *prev = NULL, *curr = customerHead;
    while (curr && strcmp(curr->phone, phone) != 0) {
        prev = curr;
        curr = curr->next;
    }
    if (!curr) {
        printf("未找到手机号 %s 的顾客。\n", phone);
        return;
    }
    if (prev) prev->next = curr->next;
    else customerHead = curr->next;
    // 释放地址链表
    AddressNode *a = curr->addrList;
    while (a) { AddressNode *tmp = a; a = a->next; free(tmp); }
    free(curr);
    saveAll();
    printf("顾客删除成功（手机号 %s）\n", phone);
}

void updateCustomer(const char *oldPhone, const char *newUsername, const char *newPassword,
                    const char *newName, const char *newPhone) {
    CustomerNode *target = findCustomerByPhone(oldPhone);
    if (!target) {
        printf("未找到手机号 %s 的顾客。\n", oldPhone);
        return;
    }
    if (strcmp(oldPhone, newPhone) != 0 && findCustomerByPhone(newPhone)) {
        printf("新手机号 %s 已被其他顾客使用。\n", newPhone);
        return;
    }
    if (strcmp(target->username, newUsername) != 0 && findUserByUsername(newUsername)) {
        printf("用户名 %s 已被其他用户使用。\n", newUsername);
        return;
    }
    safeCopy(target->username, newUsername, MAX_USERNAME);
    safeCopy(target->password, newPassword, MAX_PASSWORD);
    safeCopy(target->name, newName, MAX_NAME);
    safeCopy(target->phone, newPhone, MAX_PHONE);
    saveAll();
    printf("顾客信息修改成功。\n");
}

void printAllCustomers() {
    CustomerNode *curr = customerHead;
    if (!curr) { printf("（无顾客数据）\n"); return; }
    int i = 1;
    while (curr) {
        printf("%d. 用户名: %s, 姓名: %s, 手机: %s\n",
               i++, curr->username, curr->name, curr->phone);
        curr = curr->next;
    }
}

// 8.3 骑手管理
void addRider(const char *username, const char *password, const char *name, const char *phone) {
    if (findRiderByPhone(phone)) {
        printf("手机号 %s 已存在，添加失败。\n", phone);
        return;
    }
    if (findUserByUsername(username)) {
        printf("用户名 %s 已被使用，添加失败。\n", username);
        return;
    }
    RiderNode *newR = (RiderNode*)malloc(sizeof(RiderNode));
    safeCopy(newR->username, username, MAX_USERNAME);
    safeCopy(newR->password, password, MAX_PASSWORD);
    safeCopy(newR->name, name, MAX_NAME);
    safeCopy(newR->phone, phone, MAX_PHONE);
    newR->next = riderHead;
    riderHead = newR;
    saveAll();
    printf("骑手添加成功：%s %s\n", name, phone);
}

void deleteRider(const char *phone) {
    RiderNode *prev = NULL, *curr = riderHead;
    while (curr && strcmp(curr->phone, phone) != 0) {
        prev = curr;
        curr = curr->next;
    }
    if (!curr) {
        printf("未找到手机号 %s 的骑手。\n", phone);
        return;
    }
    if (prev) prev->next = curr->next;
    else riderHead = curr->next;
    free(curr);
    saveAll();
    printf("骑手删除成功（手机号 %s）\n", phone);
}

void updateRider(const char *oldPhone, const char *newUsername, const char *newPassword,
                 const char *newName, const char *newPhone) {
    RiderNode *target = findRiderByPhone(oldPhone);
    if (!target) {
        printf("未找到手机号 %s 的骑手。\n", oldPhone);
        return;
    }
    if (strcmp(oldPhone, newPhone) != 0 && findRiderByPhone(newPhone)) {
        printf("新手机号 %s 已被其他骑手使用。\n", newPhone);
        return;
    }
    if (strcmp(target->username, newUsername) != 0 && findUserByUsername(newUsername)) {
        printf("用户名 %s 已被其他用户使用。\n", newUsername);
        return;
    }
    safeCopy(target->username, newUsername, MAX_USERNAME);
    safeCopy(target->password, newPassword, MAX_PASSWORD);
    safeCopy(target->name, newName, MAX_NAME);
    safeCopy(target->phone, newPhone, MAX_PHONE);
    saveAll();
    printf("骑手信息修改成功。\n");
}

void printAllRiders() {
    RiderNode *curr = riderHead;
    if (!curr) { printf("（无骑手数据）\n"); return; }
    int i = 1;
    while (curr) {
        printf("%d. 用户名: %s, 姓名: %s, 手机: %s\n",
               i++, curr->username, curr->name, curr->phone);
        curr = curr->next;
    }
}

// ================= 9. 注册功能 =================
void registerMenu() {
    system("cls");
    int role;
    char uname[MAX_USERNAME], pwd[MAX_PASSWORD];
    char shop[MAX_SHOP], boss[MAX_NAME], phone[MAX_PHONE];
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

    if (role == 1) { // 顾客
        printf("请输入姓名: "); scanf("%s", name);
        printf("请输入手机号: "); scanf("%s", phone);
        addCustomer(uname, pwd, name, phone);
    } else if (role == 2) { // 商家
        printf("请输入店名: "); scanf("%s", shop);
        printf("请输入老板姓名: "); scanf("%s", boss);
        printf("请输入老板电话: "); scanf("%s", phone);
        addMerchant(uname, pwd, shop, boss, phone);
    } else if (role == 3) { // 骑手
        printf("请输入姓名: "); scanf("%s", name);
        printf("请输入手机号: "); scanf("%s", phone);
        addRider(uname, pwd, name, phone);
    }
    printf("注册成功！\n");
    system("pause");
}

// ================= 10. 登录及角色功能 =================

// 10.1 商家菜单
void merchantMenu(MerchantNode *merchant) {
    int choice;
    while (1) {
        system("cls");
        printf("\n===== 商家后台 (%s) =====\n", merchant->username);
        printf("1. 添加菜品\n2. 修改菜品\n3. 查看我的菜品\n4. 修改个人信息\n0. 退出登录\n");
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
            saveAll(); // 保存到 dishes.txt
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
            // 修改个人信息（用户名、密码、店名、老板名、电话）
            char newU[MAX_USERNAME], newP[MAX_PASSWORD], newShop[MAX_SHOP], newBoss[MAX_NAME], newPhone[MAX_PHONE];
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
            // 调用更新函数（注意：更新时不能与别人冲突）
            updateMerchant(merchant->bossPhone, newU, newP, newShop, newBoss, newPhone);
            // 由于更新函数会保存并可能改变链表，需要重新获取当前商家指针（实际上更新后指针仍有效，但内容已变）
            // 为了安全，我们可以重新赋值 merchant 指针（但可能改变位置），这里简单处理：让用户重新登录
            printf("信息已更新，请重新登录。\n");
            system("pause");
            break; // 退出商家菜单，回到登录状态
        } else if (choice == 0) break;
        else { printf("无效选项！\n"); system("pause"); }
    }
}

// 10.2 骑手菜单
void riderMenu(RiderNode *rider) {
    int choice;
    while (1) {
        system("cls");
        printf("\n===== 骑手中心 (%s) =====\n", rider->username);
        printf("1. 查看已接单信息\n2. 修改个人信息\n0. 退出登录\n");
        printf("请选择: ");
        scanf("%d", &choice);
        if (choice == 1) {
            OrderNode *p = orderHead;
            int found = 0;
            printf("订单号\t顾客\t\t菜品\t\t地址\t\t状态\n");
            while (p) {
                if (p->status == 1) {
                    printf("%d\t%s\t\t%s\t\t%s\t已接单\n", p->orderId, p->customerName, p->dishName, p->address);
                    found = 1;
                }
                p = p->next;
            }
            if (!found) printf("当前没有已接单的订单。\n");
            system("pause");
        } else if (choice == 2) {
            char newU[MAX_USERNAME], newP[MAX_PASSWORD], newName[MAX_NAME], newPhone[MAX_PHONE];
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
            updateRider(rider->phone, newU, newP, newName, newPhone);
            printf("信息已更新，请重新登录。\n");
            system("pause");
            break;
        } else if (choice == 0) break;
        else printf("无效选项！\n");
        system("pause");
    }
}

// 10.3 顾客菜单
void customerMenu(CustomerNode *customer) {
    int choice;
    while (1) {
        system("cls");
        printf("\n===== 顾客中心 (%s) =====\n", customer->username);
        printf("1. 浏览商家商品\n2. 搜索商品\n3. 管理收货地址\n4. 提交订单\n");
        printf("5. 修改订单地址\n6. 修改个人信息\n0. 退出登录\n");
        printf("请选择: ");
        scanf("%d", &choice);
        getchar();

        if (choice == 1) {
            MerchantNode *m = merchantHead;
            while (m) {
                printf("【商家: %s】\n", m->shopName);
                DishNode *d = m->dishList;
                while (d) {
                    printf("  - %s (￥%.2f)\n", d->name, d->price);
                    d = d->next;
                }
                m = m->next;
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
                printf("输入地址详情: "); scanf("%s", newAddr->detail);
                newAddr->next = NULL;
                if (!customer->addrList) customer->addrList = newAddr;
                else temp->next = newAddr;
                printf("地址添加成功！\n");
                system("pause");
            } else if (addrChoice == 2) {
                AddressNode *p = customer->addrList;
                if (!p) printf("暂无地址。\n");
                while (p) {
                    printf("地址%d: %s\n", p->id, p->detail);
                    p = p->next;
                }
                system("pause");
            }
        } else if (choice == 4) {
            char dish[MAX_DISH], addr[MAX_ADDR];
            printf("输入要买的菜品名称: "); scanf("%s", dish);
            printf("输入收货地址: "); scanf("%s", addr);
            OrderNode *newOrder = (OrderNode*)malloc(sizeof(OrderNode));
            newOrder->orderId = globalOrderId++;
            safeCopy(newOrder->customerName, customer->username, MAX_USERNAME);
            safeCopy(newOrder->dishName, dish, MAX_DISH);
            safeCopy(newOrder->address, addr, MAX_ADDR);
            newOrder->status = 0; // 待接单
            newOrder->next = orderHead;
            orderHead = newOrder;
            saveAll();
            printf("订单提交成功！订单号: %d\n", newOrder->orderId);
            printf("(系统模拟: 商家已接单)\n");
            newOrder->status = 1; // 模拟接单
            saveAll();
            system("pause");
        } else if (choice == 5) {
            int oid; char newAddr[MAX_ADDR];
            printf("输入要修改的订单号: "); scanf("%d", &oid);
            OrderNode *p = orderHead;
            while (p && p->orderId != oid) p = p->next;
            if (p && strcmp(p->customerName, customer->username) == 0) {
                printf("输入新地址: "); scanf("%s", newAddr);
                safeCopy(p->address, newAddr, MAX_ADDR);
                saveAll();
                printf("地址修改成功！\n");
            } else printf("未找到您的该订单！\n");
            system("pause");
        } else if (choice == 6) {
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

// ================= 11. 系统管理菜单（集成 delivery.c 功能） =================
void adminMenu() {
    int choice, sub;
    char username[MAX_USERNAME], password[MAX_PASSWORD];
    char shop[MAX_SHOP], boss[MAX_NAME], phone[MAX_PHONE];
    char name[MAX_NAME], newU[MAX_USERNAME], newP[MAX_PASSWORD];
    char newShop[MAX_SHOP], newBoss[MAX_NAME], newPhone[MAX_PHONE];
    char oldPhone[MAX_PHONE];

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
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        scanf("%d", &choice);
        if (choice == 0) break;

        switch (choice) {
            case 1: printAllMerchants(); system("pause"); break;
            case 2:
                printf("请输入用户名: "); scanf("%s", username);
                printf("请输入密码: "); scanf("%s", password);
                if (checkPasswordStrength(password) < 2) { printf("密码强度不足！\n"); system("pause"); break; }
                printf("请输入店名: "); scanf("%s", shop);
                printf("请输入老板姓名: "); scanf("%s", boss);
                printf("请输入老板电话: "); scanf("%s", phone);
                addMerchant(username, password, shop, boss, phone);
                system("pause"); break;
            case 3:
                printf("请输入要修改的商家的当前老板电话: "); scanf("%s", oldPhone);
                printf("请输入新用户名: "); scanf("%s", newU);
                printf("请输入新密码: "); scanf("%s", newP);
                if (checkPasswordStrength(newP) < 2) { printf("密码强度不足！\n"); system("pause"); break; }
                printf("请输入新店名: "); scanf("%s", newShop);
                printf("请输入新老板姓名: "); scanf("%s", newBoss);
                printf("请输入新老板电话: "); scanf("%s", newPhone);
                updateMerchant(oldPhone, newU, newP, newShop, newBoss, newPhone);
                system("pause"); break;
            case 4:
                printf("请输入要删除的老板电话: "); scanf("%s", phone);
                deleteMerchant(phone);
                system("pause"); break;
            case 5: printAllCustomers(); system("pause"); break;
            case 6:
                printf("请输入用户名: "); scanf("%s", username);
                printf("请输入密码: "); scanf("%s", password);
                if (checkPasswordStrength(password) < 2) { printf("密码强度不足！\n"); system("pause"); break; }
                printf("请输入姓名: "); scanf("%s", name);
                printf("请输入手机号: "); scanf("%s", phone);
                addCustomer(username, password, name, phone);
                system("pause"); break;
            case 7:
                printf("请输入要修改的顾客的当前手机号: "); scanf("%s", oldPhone);
                printf("请输入新用户名: "); scanf("%s", newU);
                printf("请输入新密码: "); scanf("%s", newP);
                if (checkPasswordStrength(newP) < 2) { printf("密码强度不足！\n"); system("pause"); break; }
                printf("请输入新姓名: "); scanf("%s", newU); // 注意变量名，这里用newName
                printf("请输入新手机号: "); scanf("%s", newPhone);
                updateCustomer(oldPhone, newU, newP, newU, newPhone); // 参数顺序：oldPhone, username, password, name, phone
                system("pause"); break;
            case 8:
                printf("请输入要删除的手机号: "); scanf("%s", phone);
                deleteCustomer(phone);
                system("pause"); break;
            case 9: printAllRiders(); system("pause"); break;
            case 10:
                printf("请输入用户名: "); scanf("%s", username);
                printf("请输入密码: "); scanf("%s", password);
                if (checkPasswordStrength(password) < 2) { printf("密码强度不足！\n"); system("pause"); break; }
                printf("请输入姓名: "); scanf("%s", name);
                printf("请输入手机号: "); scanf("%s", phone);
                addRider(username, password, name, phone);
                system("pause"); break;
            case 11:
                printf("请输入要修改的骑手的当前手机号: "); scanf("%s", oldPhone);
                printf("请输入新用户名: "); scanf("%s", newU);
                printf("请输入新密码: "); scanf("%s", newP);
                if (checkPasswordStrength(newP) < 2) { printf("密码强度不足！\n"); system("pause"); break; }
                printf("请输入新姓名: "); scanf("%s", newU); // 注意变量
                printf("请输入新手机号: "); scanf("%s", newPhone);
                updateRider(oldPhone, newU, newP, newU, newPhone);
                system("pause"); break;
            case 12:
                printf("请输入要删除的手机号: "); scanf("%s", phone);
                deleteRider(phone);
                system("pause"); break;
            default: printf("无效选项！\n"); system("pause");
        }
    }
}

// ================= 12. 主程序 =================
int main() {
    loadData();
    loadAdmin();  // 确保管理员账号存在
    int role;
    char uname[MAX_USERNAME], pwd[MAX_PASSWORD];

    while (1) {
        system("cls");
        printf("\n========== 欢迎使用外卖系统 ==========\n");
        printf("1. 顾客登录\n2. 商家登录\n3. 骑手登录\n");
        printf("4. 注册新账号\n5. 管理员登录\n0. 退出系统\n");
        printf("请输入数字选项: ");
        scanf("%d", &role);
        getchar();

        if (role == 0) { printf("感谢使用，再见！\n"); break; }
        if (role == 4) { registerMenu(); continue; }

        if (role == 5) { // 管理员登录
            printf("请输入管理员用户名: "); scanf("%s", uname);
            printf("请输入管理员密码: "); scanf("%s", pwd);
            if (checkAdmin(uname, pwd)) {
                adminMenu();   // 进入系统管理
            } else {
                printf("管理员用户名或密码错误！\n");
                system("pause");
            }
            continue;
        }

        // 普通用户登录（1,2,3）
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
