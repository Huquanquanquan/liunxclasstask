#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATABASE_FILE "database.txt"

// 登陆检测 
void login(User user) {
    FILE *fp = fopen(DATABASE_FILE, "r");
    if (fp == NULL) {
        printf("Error opening database file!\n");
        return;
    }

    char line[1024];
    int user_found = 0;
    while (fgets(line, 1024, fp)) {
        char *token = strtok(line, ",");
        int user_id = atoi(token);
        if (user_id == user.id) {
            token = strtok(NULL, ",");
            char stored_password[20];
            strncpy(stored_password, token, sizeof(stored_password));
            if (strcmp(stored_password, user.password) == 0) {
                fclose(fp);
                printf("Login successful!\n");
                return;
            } else {
                fclose(fp);
                printf("Login failed!\n");
                return;
            }
        }
    }
    fclose(fp);
    printf("User not found!\n");
    return;
}

// 新增用户 
void add_user(User user) {
    FILE *fp = fopen(DATABASE_FILE, "a+");
    if (fp == NULL) {
        printf("Error opening database file!\n");
        return;
    }

    // Check if user with same id already exists
    rewind(fp); // Move file pointer to beginning of file
    char line[1024];
    int user_exists = 0;
    while (fgets(line, 1024, fp)) {
        char *token = strtok(line, ",");
        int user_id = atoi(token);
        if (user_id == user.id) {
            printf("System: User with id %d already exists!\n", user.id);
            user_exists = 1;
            break;
        }
    }
    if (!user_exists) {
        // Add new user
        fseek(fp, 0, SEEK_END); // Move file pointer to end of file
        fprintf(fp, "%d,%s,%s,%s,%s\n", user.id, user.password, user.nickname, user.online_status, user.ip_address);
    }
    fclose(fp);
}

// 注销用户
void delete_user(User user) {
    FILE *fp = fopen(DATABASE_FILE, "r");
    if (fp == NULL) {
        printf("Error opening database file!\n");
        return;
    }

    FILE *temp_fp = fopen("temp.txt", "w");
    if (temp_fp == NULL) {
        printf("Error creating temporary file!\n");
        fclose(fp);
        return;
    }

    char line[1024];
    int user_id = 1;
    int num_users = 0;
    while (fgets(line, 1024, fp)) {
        char *token = strtok(line, ",");
        int old_id = atoi(token);
        if (old_id != user.id) {
            fprintf(temp_fp, "%d,", user_id++);
            token += strlen(token) + 1; // skip the ID
            fprintf(temp_fp, "%s", token);
            num_users++;
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove(DATABASE_FILE);
    rename("temp.txt", DATABASE_FILE);
}

// 查询用户
User *get_user(int id) {
    FILE *fp = fopen(DATABASE_FILE, "r");
    if (fp == NULL) {
        printf("Error opening database file!\n");
        return NULL;
    }

    char line[1024];
    User *user = NULL;
    while (fgets(line, 1024, fp)) {
        int user_id, num_matched;
        char password[20], nickname[20], online_status[10], ip_address[20];
        num_matched = sscanf(line, "%d,%19[^,],%19[^,],%9[^,],%19[^\n]\n",
                              &user_id, password, nickname, online_status, ip_address);
        if (num_matched == 5 && user_id == id) {
            user = (User *)malloc(sizeof(User));
            user->id = user_id;
            strcpy(user->password, password);
            strcpy(user->nickname, nickname);
            strcpy(user->online_status, online_status);
            strcpy(user->ip_address, ip_address);

            fclose(fp);
            return user;
        }
    }

    fclose(fp);
    return NULL;
}

// 更新信息 , 这里不更新密码 , 单独做密码的修改
void update_user(User *user) {
    FILE *fp = fopen(DATABASE_FILE, "r");
    if (fp == NULL) {
        printf("Error opening database file!\n");
        return;
    }

    FILE *temp_fp = fopen("temp.txt", "w");
    if (temp_fp == NULL) {
        printf("Error creating temporary file!\n");
        fclose(fp);
        return;
    }

    char line[1024];
    int user_id;
    char password[20], nickname[20], online_status[10], ip_address[20];
    while (fgets(line, 1024, fp)) {
        sscanf(line, "%d,%19[^,],%19[^,],%9[^,],%19[^\n]\n",
                &user_id, password, nickname, online_status, ip_address);
        if (user_id == user->id) {
            fprintf(temp_fp, "%d,%s,%s,%s,%s\n", user->id, password, user->nickname, user->online_status, user->ip_address);
        } else {
            fprintf(temp_fp, "%s", line);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove(DATABASE_FILE);
    rename("temp.txt", DATABASE_FILE);
}

// change password
void change_password(User *user, char *new_password) {
    FILE *fp = fopen(DATABASE_FILE, "r");
    if (fp == NULL) {
        printf("Error opening database file!\n");
        return;
    }

    FILE *temp_fp = fopen("temp.txt", "w");
    if (temp_fp == NULL) {
        printf("Error creating temporary file!\n");
        fclose(fp);
        return;
    }

    char line[1024];
    int user_id, num_matched;
    char password[20], nickname[20], online_status[10], ip_address[20];
    while (fgets(line, 1024, fp)) {
        num_matched = sscanf(line, "%d,%19[^,],%19[^,],%9[^,],%19[^\n]\n",
                              &user_id, password, nickname, online_status, ip_address);
        if (num_matched == 5 && user_id == user->id) {
            fprintf(temp_fp, "%d,%s,%s,%s,%s\n", user_id, new_password, nickname, online_status, ip_address);
        } else {
            fprintf(temp_fp, "%s", line);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove(DATABASE_FILE);
    rename("temp.txt", DATABASE_FILE);
}

//// 添加好友
//void add_friend(User *user, Friend friend) {
//    if (user->num_friends < 10) {
//        user->friends[user->num_friends] = friend;
//        user->num_friends++;
//    } else {
//        printf("Maximum number of friends reached!\n");
//    }
//}
//
//// 删除好友
//void remove_friend(User *user, int friend_id) {
//    int i, j;
//    for (i = 0; i < user->num_friends; i++) {
//        if (user->friends[i].id == friend_id) {
//            for (j = i; j < user->num_friends - 1; j++) {
//                user->friends[j] = user->friends[j + 1];
//            }
//            user->num_friends--;
//            return;
//        }
//    }
//    printf("Friend not found!\n");
//}
//
//// 查找好友信息
//Friend *get_friend(User *user, int friend_id) {
//    for (int i = 0; i < user->num_friends; i++) {
//        if (user->friends[i].id == friend_id) {
//            return &user->friends[i];
//        }
//    }
//    return NULL;
//}
//
//// 更新好友信息
//void update_friend(User *user, Friend *friend) {
//    for (int i = 0; i < user->num_friends; i++) {
//        if (user->friends[i].id == friend->id) {
//            strcpy(user->friends[i].nickname, friend->nickname);
//            strcpy(user->friends[i].online_status, friend->online_status);
//            return;
//        }
//    }
//    printf("Friend not found!\n");
//}


int main() {

    // Create A user
    User user;
    user.id = 1;
    strcpy(user.password, "password");
    strcpy(user.nickname, "nickname");
    strcpy(user.online_status, "online");
    strcpy(user.ip_address, "192.168.1.1");


    // Add a user
    add_user(user);


	// Updata User information
//	User update_user_info;
//	update_user_info.id = 1;

//	strcpy(update_user_info.online_status, "online");

//	strcpy(update_user_info.nickname, "Nssssew Nickname");
//	strcpy(update_user_info.ip_address, "192.168.1.1");
//
//	update_user(&update_user_info);
	
	
//    // Change password
//    User change_password_info;
//    change_password_info.id = 1;
//    strcpy(change_password_info.password, "mypassword");
//
//    char new_password[] = "newpassword";
//    change_password(&change_password_info, new_password);
	
	
	
	
	
	
//    // 添加用户
//    add_user(user);
//
//    // 添加好友
//    Friend friend1 = {2, "Friend1", "online"};
//    add_friend(&user, friend1);
//
//    // 查找好友
//    Friend *found_friend = get_friend(&user, 2);
//    if (found_friend) {
//        printf("Friend found: %d, %s, %s\n", found_friend->id, found_friend->nickname, found_friend->online_status);
//    } else {
//        printf("Friend not found!\n");
//    }
//
//    // 更新好友信息
//    strcpy(found_friend->nickname, "Updated Friend1");
//    update_friend(&user, found_friend);
//
//    // 删除好友
//    remove_friend(&user, 2);
	
    return 0;
}