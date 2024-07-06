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
            fprintf(temp_fp, "%d,%s,%s,%s,%s\n", user->id, user->password, user->nickname, user->online_status, user->ip_address);
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
void change_password(User user, char *new_password) {
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
        if (num_matched == 5 && user_id == user.id) {
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

int main() {



    // 创建一个用户
    User user;
    user.id = 1;
    strcpy(user.password, "password");
    strcpy(user.nickname, "nickname");
    strcpy(user.online_status, "online");
    strcpy(user.ip_address, "192.168.1.1");



    // 添加用户
    add_user(user);

	/*
	// 登陆检测
    User login_user;
    login_user.id = 1;
    strcpy(login_user.password, "password");
    login(login_user);

    // 获取用户
    User *get_user_ptr = get_user(1);
    if (get_user_ptr != NULL) {
        printf("User found: id=%d, password=%s, nickname=%s, online_status=%s, ip_address=%s\n",
               get_user_ptr->id, get_user_ptr->password, get_user_ptr->nickname, get_user_ptr->online_status, get_user_ptr->ip_address);
    } else {
        printf("User not found\n");
    }

	*/


	// 更新用户
	User update_user;
	update_user.id = 1;  
	strcpy(update_user.nickname, "new_nickname");
	strcpy(update_user.online_status, "new_online_status");
	strcpy(update_user.ip_address, "new_ip_address");
	
	// Initialize the password field
	strcpy(update_user.password, "new_password"); // or copy from database
	
	update_user(&update_user);  // Pass the address of update_user
	

	/*
    // 修改密码
    User change_password_user;
    change_password_user.id = 1;
    change_password(change_password_user, "new_password");

    // 删除用户
    User delete_user;
    delete_user.id = 1;
    delete_user(delete_user);
	
	*/

    //add_user(1, "mypassword", "Alice", "offline", "127.0.0.1");
    //add_user(2, "mypassword", "DDD", "offline", "127.0.0.1");
    //add_user(3, "mypassword", "EEE", "offline", "127.0.0.1");
    
    // Try to login with the correct password
    //login(1, "mypassword");

    // Try to login with an incorrect password
    //login(1, "wrongpassword");

	//delete_user(2);
	
	/*
	User *user = get_user(1);
	if (user != NULL) {
		printf("Nickname: %s, Password: %s, Status: %s, IP: %s\n", user->nickname, user->password, user->online_status, user->ip_address);
		free(user);
	} else {
    	printf("User not found!\n");
	}
	*/
	
	//update_user(1, "New Nickname", "online", "192.168.1.1");
	
	//change_password(1, "mypassword", "newpassword");
	
    return 0;
}