#ifndef DATABASE_H
#define DATABASE_H

#include "friends.h"

typedef struct {
    int id;
    char password[20];
    char nickname[20];
    char online_status[10];
    char ip_address[20];
    Friend friends[10];
    int num_friends;
} User;

void login(User user);
void add_user(User user);
void delete_user(User user);
User *get_user(int id);
void update_user(User *user);
void change_password(User *user, char *new_password);

// 好友相关操作
void add_friend(User *user, Friend friend);
void remove_friend(User *user, int friend_id);
Friend *get_friend(User *user, int friend_id);
void update_friend(User *user, Friend *friend);

#endif  // DATABASE_H