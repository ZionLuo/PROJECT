#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==================== 商家结构体及函数 ====================
typedef struct Merchant {
    char shopName[50];
    char bossName[50];
    char bossPhone[20];   // 作为唯一标识
} Merchant;

typedef struct MerchantNode {
    Merchant data;
    struct MerchantNode* next;
} MerchantNode;

// 商家函数声明
MerchantNode* loadMerchants(const char* filename);
void saveMerchants(MerchantNode* head, const char* filename);
void freeMerchants(MerchantNode* head);
MerchantNode* findMerchant(MerchantNode* head, const char* phone);
int addMerchant(MerchantNode** head, const char* shop, const char* boss, const char* phone, const char* filename);
int deleteMerchant(MerchantNode** head, const char* phone, const char* filename);
int updateMerchant(MerchantNode** head, const char* oldPhone, const char* newShop, const char* newBoss, const char* newPhone, const char* filename);
void printAllMerchants(MerchantNode* head);

// ==================== 通用人员（顾客/骑手）结构体及函数 ====================
typedef struct Person {
    char name[50];
    char phone[20];   // 唯一标识
} Person;

typedef struct PersonNode {
    Person data;
    struct PersonNode* next;
} PersonNode;

// 通用人员函数声明（通过 filename 区分顾客或骑手）
PersonNode* loadPersons(const char* filename);
void savePersons(PersonNode* head, const char* filename);
void freePersons(PersonNode* head);
PersonNode* findPerson(PersonNode* head, const char* phone);
int addPerson(PersonNode** head, const char* name, const char* phone, const char* filename);
int deletePerson(PersonNode** head, const char* phone, const char* filename);
int updatePerson(PersonNode** head, const char* oldPhone, const char* newName, const char* newPhone, const char* filename);
void printAllPersons(PersonNode* head);

// ==================== 商家函数实现 ====================
MerchantNode* loadMerchants(const char* filename) {
    MerchantNode* head = NULL;
    MerchantNode* tail = NULL;
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) return NULL;

    Merchant temp;
    while (fscanf(fp, "%49s %49s %19s", temp.shopName, temp.bossName, temp.bossPhone) == 3) {
        MerchantNode* node = (MerchantNode*)malloc(sizeof(MerchantNode));
        node->data = temp;
        node->next = NULL;
        if (head == NULL) head = tail = node;
        else {
            tail->next = node;
            tail = node;
        }
    }
    fclose(fp);
    return head;
}

void saveMerchants(MerchantNode* head, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("无法打开文件 %s 写入\n", filename);
        return;
    }
    MerchantNode* curr = head;
    while (curr != NULL) {
        fprintf(fp, "%s %s %s\n", curr->data.shopName, curr->data.bossName, curr->data.bossPhone);
        curr = curr->next;
    }
    fclose(fp);
}

void freeMerchants(MerchantNode* head) {
    MerchantNode* curr = head;
    while (curr != NULL) {
        MerchantNode* next = curr->next;
        free(curr);
        curr = next;
    }
}

MerchantNode* findMerchant(MerchantNode* head, const char* phone) {
    MerchantNode* curr = head;
    while (curr != NULL) {
        if (strcmp(curr->data.bossPhone, phone) == 0)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

int addMerchant(MerchantNode** head, const char* shop, const char* boss, const char* phone, const char* filename) {
    if (findMerchant(*head, phone) != NULL) {
        printf("老板电话 %s 已存在，添加失败。\n", phone);
        return 0;
    }
    MerchantNode* node = (MerchantNode*)malloc(sizeof(MerchantNode));
    strcpy(node->data.shopName, shop);
    strcpy(node->data.bossName, boss);
    strcpy(node->data.bossPhone, phone);
    node->next = *head;
    *head = node;
    saveMerchants(*head, filename);
    printf("商家添加成功：%s %s %s\n", shop, boss, phone);
    return 1;
}

int deleteMerchant(MerchantNode** head, const char* phone, const char* filename) {
    if (*head == NULL) {
        printf("链表为空。\n");
        return 0;
    }
    MerchantNode *prev = NULL, *curr = *head;
    while (curr != NULL && strcmp(curr->data.bossPhone, phone) != 0) {
        prev = curr;
        curr = curr->next;
    }
    if (curr == NULL) {
        printf("未找到老板电话 %s 的商家。\n", phone);
        return 0;
    }
    if (prev == NULL) *head = curr->next;
    else prev->next = curr->next;
    free(curr);
    saveMerchants(*head, filename);
    printf("商家删除成功（老板电话 %s）\n", phone);
    return 1;
}

int updateMerchant(MerchantNode** head, const char* oldPhone, const char* newShop, const char* newBoss, const char* newPhone, const char* filename) {
    MerchantNode* target = findMerchant(*head, oldPhone);
    if (target == NULL) {
        printf("未找到老板电话 %s 的商家。\n", oldPhone);
        return 0;
    }
    if (strcmp(oldPhone, newPhone) != 0) {
        MerchantNode* exist = findMerchant(*head, newPhone);
        if (exist != NULL && exist != target) {
            printf("新电话 %s 已被其他商家使用，修改失败。\n", newPhone);
            return 0;
        }
    }
    strcpy(target->data.shopName, newShop);
    strcpy(target->data.bossName, newBoss);
    strcpy(target->data.bossPhone, newPhone);
    saveMerchants(*head, filename);
    printf("商家信息修改成功：%s %s %s\n", newShop, newBoss, newPhone);
    return 1;
}

void printAllMerchants(MerchantNode* head) {
    if (head == NULL) {
        printf("（无商家数据）\n");
        return;
    }
    int i = 1;
    MerchantNode* curr = head;
    while (curr != NULL) {
        printf("%d. 店名: %s, 老板: %s, 电话: %s\n", i++, curr->data.shopName, curr->data.bossName, curr->data.bossPhone);
        curr = curr->next;
    }
}

// ==================== 通用人员函数实现 ====================
PersonNode* loadPersons(const char* filename) {
    PersonNode* head = NULL;
    PersonNode* tail = NULL;
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) return NULL;

    Person temp;
    while (fscanf(fp, "%49s %19s", temp.name, temp.phone) == 2) {
        PersonNode* node = (PersonNode*)malloc(sizeof(PersonNode));
        node->data = temp;
        node->next = NULL;
        if (head == NULL) head = tail = node;
        else {
            tail->next = node;
            tail = node;
        }
    }
    fclose(fp);
    return head;
}

void savePersons(PersonNode* head, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("无法打开文件 %s 写入\n", filename);
        return;
    }
    PersonNode* curr = head;
    while (curr != NULL) {
        fprintf(fp, "%s %s\n", curr->data.name, curr->data.phone);
        curr = curr->next;
    }
    fclose(fp);
}

void freePersons(PersonNode* head) {
    PersonNode* curr = head;
    while (curr != NULL) {
        PersonNode* next = curr->next;
        free(curr);
        curr = next;
    }
}

PersonNode* findPerson(PersonNode* head, const char* phone) {
    PersonNode* curr = head;
    while (curr != NULL) {
        if (strcmp(curr->data.phone, phone) == 0)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

int addPerson(PersonNode** head, const char* name, const char* phone, const char* filename) {
    if (findPerson(*head, phone) != NULL) {
        printf("手机号 %s 已存在，添加失败。\n", phone);
        return 0;
    }
    PersonNode* node = (PersonNode*)malloc(sizeof(PersonNode));
    strcpy(node->data.name, name);
    strcpy(node->data.phone, phone);
    node->next = *head;
    *head = node;
    savePersons(*head, filename);
    printf("添加成功：%s %s\n", name, phone);
    return 1;
}

int deletePerson(PersonNode** head, const char* phone, const char* filename) {
    if (*head == NULL) {
        printf("链表为空。\n");
        return 0;
    }
    PersonNode *prev = NULL, *curr = *head;
    while (curr != NULL && strcmp(curr->data.phone, phone) != 0) {
        prev = curr;
        curr = curr->next;
    }
    if (curr == NULL) {
        printf("未找到手机号 %s 的人员。\n", phone);
        return 0;
    }
    if (prev == NULL) *head = curr->next;
    else prev->next = curr->next;
    free(curr);
    savePersons(*head, filename);
    printf("删除成功（手机号 %s）\n", phone);
    return 1;
}

int updatePerson(PersonNode** head, const char* oldPhone, const char* newName, const char* newPhone, const char* filename) {
    PersonNode* target = findPerson(*head, oldPhone);
    if (target == NULL) {
        printf("未找到手机号 %s 的人员。\n", oldPhone);
        return 0;
    }
    if (strcmp(oldPhone, newPhone) != 0) {
        PersonNode* exist = findPerson(*head, newPhone);
        if (exist != NULL && exist != target) {
            printf("新手机号 %s 已被其他人员使用，修改失败。\n", newPhone);
            return 0;
        }
    }
    strcpy(target->data.name, newName);
    strcpy(target->data.phone, newPhone);
    savePersons(*head, filename);
    printf("修改成功：%s %s\n", newName, newPhone);
    return 1;
}

void printAllPersons(PersonNode* head) {
    if (head == NULL) {
        printf("（无数据）\n");
        return;
    }
    int i = 1;
    PersonNode* curr = head;
    while (curr != NULL) {
        printf("%d. 姓名: %s, 手机: %s\n", i++, curr->data.name, curr->data.phone);
        curr = curr->next;
    }
}

// ==================== 主程序 ====================
int main() {
    const char* merchantFile = "merchants.txt";
    const char* customerFile = "customers.txt";
    const char* riderFile = "riders.txt";

    // 加载数据
    MerchantNode* merchantHead = loadMerchants(merchantFile);
    PersonNode* customerHead = loadPersons(customerFile);
    PersonNode* riderHead = loadPersons(riderFile);

    int choice, subChoice;
    char shop[50], boss[50], phone[20], name[50];
    char newShop[50], newBoss[50], newPhone[20], newName[50];

    while (1) {
        printf("\n========== 外卖系统信息管理 ==========\n");
        printf("1. 管理商家\n");
        printf("2. 管理顾客\n");
        printf("3. 管理骑手\n");
        printf("4. 查看所有商家\n");
        printf("5. 查看所有顾客\n");
        printf("6. 查看所有骑手\n");
        printf("0. 退出并保存\n");
        printf("请选择: ");
        scanf("%d", &choice);

        if (choice == 0) break;

        // 根据选择进入不同角色管理
        if (choice == 1) {
            // 管理商家
            while (1) {
                printf("\n--- 商家管理 ---\n");
                printf("1. 添加商家\n");
                printf("2. 删除商家（按老板电话）\n");
                printf("3. 修改商家信息\n");
                printf("4. 查看所有商家\n");
                printf("5. 返回主菜单\n");
                printf("请选择: ");
                scanf("%d", &subChoice);
                if (subChoice == 5) break;

                switch (subChoice) {
                    case 1:
                        printf("请输入店名: ");
                        scanf("%49s", shop);
                        printf("请输入老板姓名: ");
                        scanf("%49s", boss);
                        printf("请输入老板电话: ");
                        scanf("%19s", phone);
                        addMerchant(&merchantHead, shop, boss, phone, merchantFile);
                        break;
                    case 2:
                        printf("请输入要删除的老板电话: ");
                        scanf("%19s", phone);
                        deleteMerchant(&merchantHead, phone, merchantFile);
                        break;
                    case 3:
                        printf("请输入要修改的商家的当前老板电话: ");
                        scanf("%19s", phone);
                        printf("请输入新店名: ");
                        scanf("%49s", newShop);
                        printf("请输入新老板姓名: ");
                        scanf("%49s", newBoss);
                        printf("请输入新老板电话（若不变请重复输入原号）: ");
                        scanf("%19s", newPhone);
                        updateMerchant(&merchantHead, phone, newShop, newBoss, newPhone, merchantFile);
                        break;
                    case 4:
                        printAllMerchants(merchantHead);
                        break;
                    default:
                        printf("无效选择。\n");
                }
            }
        } else if (choice == 2) {
            // 管理顾客
            while (1) {
                printf("\n--- 顾客管理 ---\n");
                printf("1. 添加顾客\n");
                printf("2. 删除顾客（按手机号）\n");
                printf("3. 修改顾客信息\n");
                printf("4. 查看所有顾客\n");
                printf("5. 返回主菜单\n");
                printf("请选择: ");
                scanf("%d", &subChoice);
                if (subChoice == 5) break;

                switch (subChoice) {
                    case 1:
                        printf("请输入姓名: ");
                        scanf("%49s", name);
                        printf("请输入手机号: ");
                        scanf("%19s", phone);
                        addPerson(&customerHead, name, phone, customerFile);
                        break;
                    case 2:
                        printf("请输入要删除的手机号: ");
                        scanf("%19s", phone);
                        deletePerson(&customerHead, phone, customerFile);
                        break;
                    case 3:
                        printf("请输入要修改的顾客的当前手机号: ");
                        scanf("%19s", phone);
                        printf("请输入新姓名: ");
                        scanf("%49s", newName);
                        printf("请输入新手机号（若不变请重复输入原号）: ");
                        scanf("%19s", newPhone);
                        updatePerson(&customerHead, phone, newName, newPhone, customerFile);
                        break;
                    case 4:
                        printAllPersons(customerHead);
                        break;
                    default:
                        printf("无效选择。\n");
                }
            }
        } else if (choice == 3) {
            // 管理骑手（与顾客完全相同的操作，只是文件不同）
            while (1) {
                printf("\n--- 骑手管理 ---\n");
                printf("1. 添加骑手\n");
                printf("2. 删除骑手（按手机号）\n");
                printf("3. 修改骑手信息\n");
                printf("4. 查看所有骑手\n");
                printf("5. 返回主菜单\n");
                printf("请选择: ");
                scanf("%d", &subChoice);
                if (subChoice == 5) break;

                switch (subChoice) {
                    case 1:
                        printf("请输入姓名: ");
                        scanf("%49s", name);
                        printf("请输入手机号: ");
                        scanf("%19s", phone);
                        addPerson(&riderHead, name, phone, riderFile);
                        break;
                    case 2:
                        printf("请输入要删除的手机号: ");
                        scanf("%19s", phone);
                        deletePerson(&riderHead, phone, riderFile);
                        break;
                    case 3:
                        printf("请输入要修改的骑手的当前手机号: ");
                        scanf("%19s", phone);
                        printf("请输入新姓名: ");
                        scanf("%49s", newName);
                        printf("请输入新手机号（若不变请重复输入原号）: ");
                        scanf("%19s", newPhone);
                        updatePerson(&riderHead, phone, newName, newPhone, riderFile);
                        break;
                    case 4:
                        printAllPersons(riderHead);
                        break;
                    default:
                        printf("无效选择。\n");
                }
            }
        } else if (choice == 4) {
            printf("\n=== 所有商家 ===\n");
            printAllMerchants(merchantHead);
        } else if (choice == 5) {
            printf("\n=== 所有顾客 ===\n");
            printAllPersons(customerHead);
        } else if (choice == 6) {
            printf("\n=== 所有骑手 ===\n");
            printAllPersons(riderHead);
        } else {
            printf("无效选择，请重试。\n");
        }
    }

    // 退出前保存（已在每次修改后即时保存，此处做双重保险）
    saveMerchants(merchantHead, merchantFile);
    savePersons(customerHead, customerFile);
    savePersons(riderHead, riderFile);

    // 释放内存
    freeMerchants(merchantHead);
    freePersons(customerHead);
    freePersons(riderHead);

    printf("\n数据已保存，程序退出。\n");
    return 0;
}
