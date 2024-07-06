#ifndef FRIENDS_H
#define FRIENDS_H

// 好友结构体
typedef struct {
    int id;
    char nickname[20];
    char online_status[10];
} Friend;

// 用户结构体(包含好友列表)
typedef struct {
    int id;
    char password[20];
    char nickname[20];
    char online_status[10];
    char ip_address[20];
    Friend friends[10]; // 最多10个好友
    int num_friends; // 当前好友数量
} User;

// 添加好友
void add_friend(User *user, Friend friend);

// 删除好友
void remove_friend(User *user, int friend_id);

// 查找好友信息
Friend *get_friend(User *user, int friend_id);

// 更新好友信息
void update_friend(User *user, Friend *friend);

#endif // FRIENDS_H