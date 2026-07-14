#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // 引入此头文件以使用 isdigit() 和 isalpha()

// ================= 1. 结构体定义 =================
typedef struct DishNode {
    int id;
    char name[50];
    double price;
    struct DishNode *next;
} DishNode;

typedef struct MerchantNode {
    char username[30];
    char password[30];
    DishNode *dishList;
    struct MerchantNode *next;
} MerchantNode;

typedef struct AddressNode {
    int id;
    char detail[100];
    struct AddressNode *next;
} AddressNode;

typedef struct CustomerNode {
    char username[30];
    char password[30];
    AddressNode *addrList;
    struct CustomerNode *next;
} CustomerNode;

typedef struct OrderNode {
    int orderId;
    char customerName[30];
    char dishName[50];
    char address[100];
    int status; // 0:待接单, 1:已接单
    struct OrderNode *next;
} OrderNode;

typedef struct RiderNode {
    char username[30];
    char password[30];
    struct RiderNode *next;
} RiderNode;

// ================= 2. 全局链表头指针 =================
MerchantNode *merchantHead = NULL;
CustomerNode *customerHead = NULL;
RiderNode *riderHead = NULL;
OrderNode *orderHead = NULL;

// 【算法：自增ID生成】
// 使用全局变量作为计数器，确保每个新订单都有一个唯一的ID
int globalOrderId = 1000;

// ================= 3. 密码强度校验函数 =================
// 【算法：密码强度校验算法】
// 规则：必须包含字母、数字、符号中的至少两种
// 逻辑：遍历密码字符串，通过计数判断包含的字符种类
int checkPasswordStrength(const char *pwd) {
    int hasLetter = 0, hasDigit = 0, hasSymbol = 0;
    for (int i = 0; pwd[i] != '\0'; i++) {
        if (isalpha((unsigned char)pwd[i])) {
            hasLetter = 1;
        } else if (isdigit((unsigned char)pwd[i])) {
            hasDigit = 1;
        } else {
            // 既不是字母也不是数字，则视为符号
            hasSymbol = 1;
        }
    }
    // 返回满足的种类总数
    return hasLetter + hasDigit + hasSymbol;
}

// ================= 4. 文件读取与初始化 =================
void loadData() {
    FILE *fp;
    // 加载商家数据（使用尾插法保持顺序）
    fp = fopen("merchants.txt", "r");
    if (fp) {
        MerchantNode *tail = NULL;
        char u[30], p[30];
        while (fscanf(fp, "%s %s", u, p) == 2) {
            MerchantNode *newM = (MerchantNode*)malloc(sizeof(MerchantNode));
            strcpy(newM->username, u);
            strcpy(newM->password, p);
            newM->dishList = NULL;
            newM->next = NULL;
            if (!merchantHead) merchantHead = newM;
            else tail->next = newM;
            tail = newM;
        }
        fclose(fp);
    }
    // 加载顾客数据（使用尾插法保持顺序）
    fp = fopen("customers.txt", "r");
    if (fp) {
        CustomerNode *tail = NULL;
        char u[30], p[30];
        while (fscanf(fp, "%s %s", u, p) == 2) {
            CustomerNode *newC = (CustomerNode*)malloc(sizeof(CustomerNode));
            strcpy(newC->username, u);
            strcpy(newC->password, p);
            newC->addrList = NULL;
            newC->next = NULL;
            if (!customerHead) customerHead = newC;
            else tail->next = newC;
            tail = newC;
        }
        fclose(fp);
    }
    // 加载骑手数据（使用尾插法保持顺序）
    fp = fopen("riders.txt", "r");
    if (fp) {
        RiderNode *tail = NULL;
        char u[30], p[30];
        while (fscanf(fp, "%s %s", u, p) == 2) {
            RiderNode *newR = (RiderNode*)malloc(sizeof(RiderNode));
            strcpy(newR->username, u);
            strcpy(newR->password, p);
            newR->next = NULL;
            if (!riderHead) riderHead = newR;
            else tail->next = newR;
            tail = newR;
        }
        fclose(fp);
    }
    // 加载订单数据
    fp = fopen("orders.txt", "r");
    if (fp) {
        OrderNode *tail = NULL;
        int oid, st;
        char cn[30], dn[50], addr[100];
        while (fscanf(fp, "%d %s %s %s %d", &oid, cn, dn, addr, &st) == 5) {
            OrderNode *newO = (OrderNode*)malloc(sizeof(OrderNode));
            newO->orderId = oid;
            strcpy(newO->customerName, cn);
            strcpy(newO->dishName, dn);
            strcpy(newO->address, addr);
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

// ================= 5. 注册功能 =================
void registerMenu() {
    system("cls"); // 进入注册界面清屏
    int role;
    char uname[30], pwd[30];
    printf("\n===== 用户注册 =====\n");
    printf("选择注册身份 (1.顾客 2.商家 3.骑手 0.返回): ");
    scanf("%d", &role);
    getchar();
    if (role == 0) return;
    printf("请输入用户名: ");
    scanf("%s", uname);
    printf("请输入密码(需包含字母/数字/符号中的至少两种): ");
    scanf("%s", pwd);

    // 调用密码强度校验算法
    if (checkPasswordStrength(pwd) < 2) {
        printf("密码强度不足！必须包含字母、数字、符号中的至少两种。\n");
        system("pause");
        return;
    }

    if (role == 1) {
        // 【算法：线性查找】
        // 在注册前遍历链表，检查用户名是否已存在
        CustomerNode *p = customerHead;
        while (p) {
            if (strcmp(p->username, uname) == 0) {
                printf("顾客已存在！\n");
                system("pause");
                return;
            }
            p = p->next;
        }

        FILE *fp = fopen("customers.txt", "a");
        fprintf(fp, "%s %s\n", uname, pwd);
        fclose(fp);

        CustomerNode *newC = (CustomerNode*)malloc(sizeof(CustomerNode));
        strcpy(newC->username, uname);
        strcpy(newC->password, pwd);
        newC->addrList = NULL;
        newC->next = NULL;

        if (!customerHead) customerHead = newC;
        else {
            // 【算法：尾插法】
            // 遍历到链表末尾，将新用户节点添加到最后
            p = customerHead;
            while (p->next) p = p->next;
            p->next = newC;
        }
        printf("顾客注册成功！\n");
        system("pause");
    } else if (role == 2) {
        // 【算法：线性查找】
        MerchantNode *p = merchantHead;
        while (p) {
            if (strcmp(p->username, uname) == 0) {
                printf("商家已存在！\n");
                system("pause");
                return;
            }
            p = p->next;
        }

        FILE *fp = fopen("merchants.txt", "a");
        fprintf(fp, "%s %s\n", uname, pwd);
        fclose(fp);

        MerchantNode *newM = (MerchantNode*)malloc(sizeof(MerchantNode));
        strcpy(newM->username, uname);
        strcpy(newM->password, pwd);
        newM->dishList = NULL;
        newM->next = NULL;

        if (!merchantHead) merchantHead = newM;
        else {
            // 【算法：尾插法】
            p = merchantHead;
            while (p->next) p = p->next;
            p->next = newM;
        }
        printf("商家注册成功！\n");
        system("pause");
    } else if (role == 3) {
        // 【算法：线性查找】
        RiderNode *p = riderHead;
        while (p) {
            if (strcmp(p->username, uname) == 0) {
                printf("骑手已存在！\n");
                system("pause");
                return;
            }
            p = p->next;
        }

        FILE *fp = fopen("riders.txt", "a");
        fprintf(fp, "%s %s\n", uname, pwd);
        fclose(fp);

        RiderNode *newR = (RiderNode*)malloc(sizeof(RiderNode));
        strcpy(newR->username, uname);
        strcpy(newR->password, pwd);
        newR->next = NULL;

        if (!riderHead) riderHead = newR;
        else {
            // 【算法：尾插法】
            p = riderHead;
            while (p->next) p = p->next;
            p->next = newR;
        }
        printf("骑手注册成功！\n");
        system("pause");
    }
}

// ================= 6. 商家功能 =================
void merchantMenu(MerchantNode *merchant) {
    int choice;
    while (1) {
        system("cls");
        printf("\n===== 商家后台 (%s) =====\n", merchant->username);
        printf("1. 添加菜品\n2. 修改菜品\n3. 查看我的菜品\n0. 退出登录\n");
        printf("请选择: ");
        scanf("%d", &choice);
        getchar();
        if (choice == 1) {
            DishNode *newDish = (DishNode*)malloc(sizeof(DishNode));
            printf("输入菜品ID: ");
            scanf("%d", &newDish->id);
            printf("输入菜品名称: ");
            scanf("%s", newDish->name);
            printf("输入菜品价格: ");
            scanf("%lf", &newDish->price);

            // 【算法：头插法】
            // 将新菜品节点直接插入到商家菜品链表的头部，效率高
            newDish->next = merchant->dishList;
            merchant->dishList = newDish;

            FILE *fp = fopen("dishes.txt", "a");
            fprintf(fp, "%s %d %s %.2f\n", merchant->username, newDish->id, newDish->name, newDish->price);
            fclose(fp);
            printf("菜品添加成功！\n");
            system("pause");
        } else if (choice == 2) {
            int id;
            char name[50];
            double price;
            printf("输入要修改的菜品ID: ");
            scanf("%d", &id);

            // 【算法：线性查找】
            // 遍历商家的菜品链表，根据ID查找特定菜品
            DishNode *p = merchant->dishList;
            while (p != NULL && p->id != id) p = p->next;

            if (p) {
                printf("输入新名称: ");
                scanf("%s", name);
                printf("输入新价格: ");
                scanf("%lf", &price);
                strcpy(p->name, name);
                p->price = price;
                printf("修改成功！\n");
                system("pause");
            } else {
                printf("未找到该菜品！\n");
                system("pause");
            }
        } else if (choice == 3) {
            DishNode *p = merchant->dishList;
            if (!p) {
                printf("暂无菜品。\n");
                system("pause");
                continue;
            }
            printf("ID\t名称\t\t价格\n");
            while (p) {
                printf("%d\t%s\t\t%.2f\n", p->id, p->name, p->price);
                p = p->next;
            }
            system("pause");
        } else if (choice == 0) break;
        else {
            printf("无效选项！\n");
            system("pause");
        }
    }
}

// ================= 7. 骑手功能 =================
void riderMenu(RiderNode *rider) {
    int choice;
    while (1) {
        system("cls");
        printf("\n===== 骑手中心 (%s) =====\n", rider->username);
        printf("1. 查看已接单信息\n0. 退出登录\n");
        printf("请选择: ");
        scanf("%d", &choice);
        getchar();
        if (choice == 1) {
            // 【算法：线性查找】
            // 遍历所有订单，筛选出状态为“已接单”的订单
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
        } else if (choice == 0) break;
        else {
            printf("无效选项！\n");
            system("pause");
        }
    }
}

// ================= 8. 顾客功能 =================
void customerMenu(CustomerNode *customer) {
    int choice;
    while (1) {
        system("cls");
        printf("\n===== 顾客中心 (%s) =====\n", customer->username);
        printf("1. 浏览商家商品\n2. 搜索商品\n3. 管理收货地址\n4. 提交订单\n");
        printf("5. 修改订单地址\n6. 修改用户名和密码\n0. 退出登录\n");
        printf("请选择: ");
        scanf("%d", &choice);
        getchar();
        if (choice == 1) {
            MerchantNode *m = merchantHead;
            while (m) {
                printf("【商家: %s】\n", m->username);
                DishNode *d = m->dishList;
                while (d) {
                    printf(" - %s (￥%.2f)\n", d->name, d->price);
                    d = d->next;
                }
                m = m->next;
            }
            system("pause");
        } else if (choice == 2) {
            char keyword[50];
            printf("输入搜索关键字: ");
            scanf("%s", keyword);
            MerchantNode *m = merchantHead;
            while (m) {
                DishNode *d = m->dishList;
                while (d) {
                    // 【算法：字符串匹配】
                    // 使用strstr函数在菜品名称中查找是否包含用户输入的关键词
                    if (strstr(d->name, keyword)) {
                        printf("找到: [%s] %s (￥%.2f)\n", m->username, d->name, d->price);
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
                while (temp && temp->next) {
                    temp = temp->next;
                    newAddr->id++;
                }
                printf("输入地址详情: ");
                scanf("%s", newAddr->detail);
                newAddr->next = NULL;
                if (!customer->addrList) customer->addrList = newAddr;
                else temp->next = newAddr;
                printf("地址添加成功！\n");
                system("pause");
            } else if (addrChoice == 2) {
                AddressNode *p = customer->addrList;
                while (p) {
                    printf("地址%d: %s\n", p->id, p->detail);
                    p = p->next;
                }
                system("pause");
            }
        } else if (choice == 4) {
            char dish[50], addr[100];
            printf("输入要买的菜品名称: ");
            scanf("%s", dish);
            printf("输入收货地址: ");
            scanf("%s", addr);
            OrderNode *newOrder = (OrderNode*)malloc(sizeof(OrderNode));

            // 使用自增ID算法生成唯一订单号
            newOrder->orderId = globalOrderId++;

            strcpy(newOrder->customerName, customer->username);
            strcpy(newOrder->dishName, dish);
            strcpy(newOrder->address, addr);
            newOrder->status = 0;

            // 【算法：头插法】
            // 将新订单节点插入到订单链表的头部
            newOrder->next = orderHead;
            orderHead = newOrder;

            FILE *fp = fopen("orders.txt", "a");
            fprintf(fp, "%d %s %s %s %d\n", newOrder->orderId, newOrder->customerName, newOrder->dishName, newOrder->address, newOrder->status);
            fclose(fp);
            printf("订单提交成功！订单号: %d\n", newOrder->orderId);
            printf("(系统模拟: 商家已接单)\n");
            newOrder->status = 1;
            system("pause");
        } else if (choice == 5) {
            int oid;
            char newAddr[100];
            printf("输入要修改的订单号: ");
            scanf("%d", &oid);

            // 【算法：线性查找】
            // 遍历订单链表，根据订单号和用户名查找特定订单
            OrderNode *p = orderHead;
            while (p && p->orderId != oid) p = p->next;

            if (p && strcmp(p->customerName, customer->username) == 0) {
                printf("输入新地址: ");
                scanf("%s", newAddr);
                strcpy(p->address, newAddr);
                printf("地址修改成功！\n");
                system("pause");
            } else {
                printf("未找到您的该订单！\n");
                system("pause");
            }
        } else if (choice == 6) {
            char newPwd[30];
            printf("输入新用户名: ");
            scanf("%s", customer->username);
            printf("输入新密码(需包含字母/数字/符号中的至少两种): ");
            scanf("%s", newPwd);
            // 修改密码时也要校验
            if (checkPasswordStrength(newPwd) < 2) {
                printf("密码强度不足！必须包含字母、数字、符号中的至少两种。\n");
                system("pause");
            } else {
                strcpy(customer->password, newPwd);
                printf("修改成功！\n");
                system("pause");
            }
        } else if (choice == 0) break;
        else {
            printf("无效选项！\n");
            system("pause");
        }
    }
}

// ================= 9. 登录与主程序 =================
int main() {
    loadData();
    int role;
    char uname[30], pwd[30];
    while (1) {
        system("cls");
        printf("\n========== 欢迎使用外卖系统 ==========\n");
        printf("1. 顾客登录\n2. 商家登录\n3. 骑手登录\n4. 注册新账号\n0. 退出系统\n");
        printf("请输入数字选项: ");
        scanf("%d", &role);
        getchar();
        if (role == 0)
        if (role == 4) { registerMenu(); continue; }

        printf("请输入用户名: "); scanf("%s", uname);
        printf("请输入密码: "); scanf("%s", pwd);

        int loginSuccess = 0;
        if (role == 1) {
            CustomerNode *p = customerHead;
            while (p) {
                if (strcmp(p->username, uname) == 0 && strcmp(p->password, pwd) == 0) {
                    customerMenu(p); loginSuccess = 1; break;
                }
                p = p->next;
            }
        } else if (role == 2) {
            MerchantNode *p = merchantHead;
            while (p) {
                if (strcmp(p->username, uname) == 0 && strcmp(p->password, pwd) == 0) {
                    merchantMenu(p); loginSuccess = 1; break;
                }
                p = p->next;
            }
        } else if (role == 3) {
            RiderNode *p = riderHead;
            while (p) {
                if (strcmp(p->username, uname) == 0 && strcmp(p->password, pwd) == 0) {
                    riderMenu(p); loginSuccess = 1; break;
                }
                p = p->next;
            }
        }
        if (!loginSuccess && role >= 1 && role <= 3) {
            printf("用户名或密码错误！\n");
            system("pause");
        }
    }
    return 0;
}
