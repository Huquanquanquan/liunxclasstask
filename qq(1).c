#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_BUF_SIZE 1024
#define DATABASE_FILE "database.txt"

int selfid;
int pid;
// 好友数据结构
typedef struct {
    char *name;
    gboolean online;
    int id; // 添加一个id字段
} Friend;

// 创建一个好友列表
Friend friends[10] = {
    {"xiaolv", FALSE, 100},
    {"zhangsan", FALSE, 101},
    {"lisi", FALSE, 102},
    {"wangwu", FALSE, 103},
    {"zhaoliu", FALSE, 104},
    {"cat", FALSE, 105},
    {"dog", FALSE, 106},
    {"tiger", FALSE, 107},
    {"monkey", TRUE, 108},
};

struct message {
	int selfid;	//发送者id 
	int rcvid;	//接收者id 
	char buf[MAX_BUF_SIZE];
	bool msg;	//ture表示传消息，false表示验证登录 
};

struct udp {
	int sockfd;
	struct sockaddr_in *addr;
};

struct thread_data {
    struct message *msg;
    struct udp *udp_info;
};

struct udp *globalUdp = NULL;

void update_friend_status(const gchar *friend_name, gboolean status);

GtkWidget *text_view; // 用于显示聊天记录的 TextView
int *current_friend; // 当前聊天的好友id

// 回调函数，当发送按钮被点击时调用
//void send_message(GtkWidget *widget, gpointer entry) {
//    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
//    
//    // 如果输入框为空，则不发送消息
//    if (g_strcmp0(text, "") == 0)
//        return;
//
//    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
//    GtkTextIter end_iter;
//
//    // 获取当前 TextView 的缓冲区
//    gtk_text_buffer_get_end_iter(buffer, &end_iter);
//
//    // 创建一个右对齐的标签
//    GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, "right_justify", "justification", GTK_JUSTIFY_RIGHT, NULL);
//
//    // 将消息追加到缓冲区末尾并应用右对齐标签
//    gtk_text_buffer_insert_with_tags(buffer, &end_iter, text, -1, tag, NULL);
//    gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);
//
//    // 清空输入框
//    gtk_entry_set_text(GTK_ENTRY(entry), "");
//}

void *send_message_thread(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    struct message *msg = data->msg;
    struct udp *udp_info = data->udp_info;

    sendto(udp_info->sockfd, msg, sizeof(struct message), 0, 
           (struct sockaddr *)udp_info->addr, sizeof(struct sockaddr_in));

    free(msg);  // 发送完毕后释放消息内存
    free(data);  // 释放传递到线程的数据结构内存
    return NULL;
}

void send_message(GtkWidget *widget, gpointer entry) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (g_strcmp0(text, "") == 0)
        return;

    struct message *msg = malloc(sizeof(struct message));
    if (msg == NULL) return;  // 内存分配失败处理
    msg->selfid = selfid;  // 设置消息的发送者ID
    msg->rcvid = &current_friend;  // 设置消息的接收者ID
    strcpy(msg->buf, text);
    msg->msg = true;  // 假设这是一个标志位，表示这是一个消息

    // 创建一个结构体来传递消息和UDP信息
    struct thread_data *data = malloc(sizeof(struct thread_data));

    if (data == NULL) {
        free(msg);  // 确保在分配失败时释放已分配的消息内存
        return;
    }

    data->msg = msg;
    data->udp_info = globalUdp;  // 传递globalUdp结构体

    pthread_t thread;
    pthread_create(&thread, NULL, send_message_thread, data);
    pthread_detach(thread);  // 不需要等待这个线程

    gtk_entry_set_text(GTK_ENTRY(entry), "");  // 清空输入框
}



// 回调函数，用于接收消息并显示在文本框中
void receive_message(const gchar *message) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end_iter;

    // 获取当前 TextView 的缓冲区
    gtk_text_buffer_get_end_iter(buffer, &end_iter);

    // 创建一个左对齐的标签
    GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, "left_justify", "justification", GTK_JUSTIFY_LEFT, NULL);

    // 将消息追加到缓冲区末尾并应用左对齐标签
    gtk_text_buffer_insert_with_tags(buffer, &end_iter, message, -1, tag, NULL);
    gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);
}

void *receive_thread_func(void *arg) {
    struct udp *udp_info = (struct udp *)arg;
    struct message rcvmsg;
    int n;

    while (1) {
        memset(rcvmsg.buf, 0, MAX_BUF_SIZE);
        n = recvfrom(udp_info->sockfd, &rcvmsg, sizeof(rcvmsg), 0, NULL, NULL);
        if (n > 0) {
            if (!strcmp(rcvmsg.buf, "online")) {
                strcpy(rcvmsg.buf, "Not online!!!");
            }
            g_idle_add((GSourceFunc)receive_message, g_strdup(rcvmsg.buf));
        }
    }
    return NULL;
}

// 创建并显示聊天窗口
void show_chat_window(const gint *friend_id) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *entry;
    GtkWidget *sendButton;
    GtkWidget *receiverLabel;  // 标签用于存储ID

    // 初始化GTK+
    gtk_init(NULL, NULL);

    // 创建窗口
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Window");  // 窗口标题可以设置为通用的
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    // 创建一个垂直盒子
    grid = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), grid);

    // 创建标签并隐藏（这里可以设置接收者和发送者ID）
    receiverLabel = gtk_label_new(friend_id);
    gtk_widget_set_visible(receiverLabel, FALSE);  // 隐藏标签

    // 将标签添加到窗口中（虽然它们是隐藏的）
    gtk_box_pack_start(GTK_BOX(grid), receiverLabel, FALSE, FALSE, 0);

    // 创建一个 TextView 用于显示聊天记录
    text_view = gtk_text_view_new();
    gtk_box_pack_start(GTK_BOX(grid), text_view, TRUE, TRUE, 0);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE); // 设置为只读
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(text_buffer, "", -1);
    gtk_widget_set_size_request(text_view, 400, 300); // 设置聊天框的大小

    // 创建一个水平布局的盒子
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(grid), hbox, FALSE, FALSE, 0);

    // 创建一个文本输入框
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

    // 创建一个发送按钮
    sendButton = gtk_button_new_with_label("Send");
    g_signal_connect(sendButton, "clicked", G_CALLBACK(send_message), entry);
    gtk_box_pack_start(GTK_BOX(hbox), sendButton, FALSE, FALSE, 0);

    // 显示所有组件
    gtk_widget_show_all(window);

    // 假设有接收到消息，调用接收消息的函数来显示在文本框中
    pthread_t recv_thread;
	pthread_create(&recv_thread, NULL, receive_thread_func, (void *)globalUdp);	

    // 运行主循环
    gtk_main();
}

// 函数声明
void on_row_selected(GtkTreeSelection *selection, gpointer data);
void on_chat_clicked(GtkWidget *button, gpointer data);
void on_window_closed(GtkWidget *window, gpointer data);

// 当好友行被选中时调用
void on_row_selected(GtkTreeSelection *selection, gpointer data) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkWidget *parent_window = GTK_WIDGET(data);
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *name;
        gint id;
        gtk_tree_model_get(model, &iter, 0, &name, 2, &id, -1);
        current_friend = id; // 将当前聊天好友昵称设置为选中的好友昵称
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Friend information", GTK_WINDOW(parent_window), GTK_DIALOG_MODAL, NULL);

        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

        GtkWidget *label = gtk_label_new(g_strdup_printf("ID: %d\nnickname: %s", id, name));
        GtkWidget *button = gtk_button_new_with_label("Start chatting");

        g_signal_connect(button, "clicked", G_CALLBACK(on_chat_clicked), dialog); // 将对话框作为参数传递

        gtk_container_add(GTK_CONTAINER(content_area), label);
        gtk_container_add(GTK_CONTAINER(content_area), button);

        g_signal_connect(dialog, "destroy", G_CALLBACK(gtk_widget_destroy), dialog);

        gtk_widget_show_all(dialog);

        g_free(name);
    }
}

// 当聊天按钮被点击时调用
void on_chat_clicked(GtkWidget *button, gpointer data) {
    GtkWidget *dialog = GTK_WIDGET(data); // 获取传递的对话框对象
    gtk_widget_destroy(dialog); // 销毁对话框
    show_chat_window(current_friend); // 弹出聊天窗口，传递当前聊天好友昵称
}

// 当窗口关闭时调用
void on_window_closed(GtkWidget *window, gpointer data) {
    // 更新 "me" 的在线状态为 false
    update_friend_status("me", FALSE);
    // 终止GTK主事件循环
    gtk_main_quit();
}

// 获取好友数量
#define NUM_FRIENDS (sizeof(friends) / sizeof(Friend))

// 创建一个好友列表窗口
GtkWidget *create_list_window() {
    // 创建一个新的窗口
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Friends");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 300);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    // 创建一个新的列表
    GtkWidget *list = gtk_tree_view_new();

    // 创建昵称列
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("nickname", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

    // 创建在线状态列
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Presence", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

    // 创建列表存储模型
    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

    // 将好友数据添加到列表模型中
    for (int i = 0; i < NUM_FRIENDS; i++) {
        gtk_list_store_insert_with_values(store, NULL, -1, 0, friends[i].name, 1, friends[i].online ? "online" : "offline", 2, friends[i].id, -1);
    }

    // 将列表模型设置为列表的模型
    gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));

    // 释放列表模型
    g_object_unref(store);

    // 将列表添加到窗口中
    gtk_container_add(GTK_CONTAINER(window), list);

    return window;
}

// 更新好友在线状态的函数
void update_friend_status(const gchar *friend_name, gboolean status) {
    for (int i = 0; i < NUM_FRIENDS; i++) {
        if (g_strcmp0(friends[i].name, friend_name) == 0) {
            friends[i].online = status;
            break;
        }
    }
}


//用户数据更新 
void update_user(int id, char *online_status) {
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
    char password[20], nickname[20], ip_address[20], status[20];
    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%d,%19[^,],%19[^,],%19[^,],%19[^\n]",
               &user_id, password, nickname, status, ip_address);
        if (user_id == id) {
            // 只更新 online_status 字段
            fprintf(temp_fp, "%d,%s,%s,%s,%s\n", user_id, password, nickname, online_status, ip_address);
        } else {
            fprintf(temp_fp, "%s", line);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove(DATABASE_FILE);
    rename("temp.txt", DATABASE_FILE);
}

// 当登录按钮被点击时的回调函数
void on_login_button_clicked(GtkWidget *button, gpointer data) {
    // 获取用户名输入框指针
    GtkEntry *username_entry = GTK_ENTRY(data);
    // 获取用户名字符串
    const gchar *username = gtk_entry_get_text(username_entry);

    // 获取密码输入框指针
    GtkEntry *password_entry = GTK_ENTRY(g_object_get_data(G_OBJECT(button), "password_entry"));
    // 获取密码字符串
    const gchar *password = gtk_entry_get_text(password_entry);
    
    
    struct udp *loadudp = g_object_get_data(G_OBJECT(button), "loadudp"); 
    globalUdp = loadudp;
    
    int n;
	struct message sendmsg;
	struct message rcvmsg;
	int sockfd = loadudp->sockfd;
	struct sockaddr_in *addr = loadudp->addr;
	sendmsg.selfid = atoi(username);
	selfid = atoi(username);
	strcpy(sendmsg.buf,password);
	sendmsg.msg = false;
	n = sendto(sockfd, (struct message *)&sendmsg, sizeof(struct message), 0, (struct sockadrr *)addr, sizeof(struct sockaddr));
	n = recvfrom(sockfd, (struct message *)&rcvmsg, sizeof(struct message), 0, NULL, NULL);
	if(!strcmp(rcvmsg.buf,"Login OK!")){	//登录成功 
		if((pid = fork()) < 0) {
		perror("create process error!\n");
		exit(0);
		}
    	sendmsg.msg = true;
		// 关闭窗口
        gtk_widget_destroy(GTK_WIDGET(gtk_widget_get_toplevel(GTK_WIDGET(button))));
        //更新自己的在线状态 
		update_user(atoi(username),"line");
    	friends[atoi(username)%10].online = TRUE;
    	// 创建并显示好友列表窗口
	    GtkWidget *window = create_list_window();
	    gtk_widget_show_all(window);
	
	    // 添加点击事件处理器
	    GtkTreeView *list = GTK_TREE_VIEW(gtk_bin_get_child(GTK_BIN(window)));
	    GtkTreeSelection *selection = gtk_tree_view_get_selection(list);
	    g_signal_connect(selection, "changed", G_CALLBACK(on_row_selected), window);
	
	    // 添加窗口关闭事件处理器
	    g_signal_connect(window, "destroy", G_CALLBACK(on_window_closed), NULL);
	    
	    // 运行主GTK事件循环
    	gtk_main();
		//进入聊天界面 
//		if((pid = fork()) < 0) {
//		perror("create process error!\n");
//		exit(0);
//		}
		for(;;){
			//子进程发送消息 
		   if(pid == 0){
			memset(sendmsg.buf, 0, MAX_BUF_SIZE);
			if(fgets(sendmsg.buf, MAX_BUF_SIZE, stdin) == NULL) break;
			//确定发送方的id 
			if(sendmsg.selfid==100){
				sendmsg.rcvid = 101;
			}else{
				sendmsg.rcvid = 100;
			}
			n = sendto(sockfd, (struct message *)&sendmsg, sizeof(struct\
			 message), 0, (struct sockadrr *)addr, sizeof(struct sockaddr));
		   }
		   else{	//父进程监听消息 
			memset(rcvmsg.buf, 0, MAX_BUF_SIZE);
			n = recvfrom(sockfd, (struct message *)&rcvmsg, sizeof(struct\
			 message), 0, NULL, NULL);
			 //如果对方不在线回复Not online 
			 if(!strcmp(rcvmsg.buf,"online")){
			 	strcpy(rcvmsg.buf,"Not online!!!");
			 }
			printf("%d# user say:",rcvmsg.selfid);
			fputs(rcvmsg.buf, stdout);
		   }
		}
	}else{
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "The account or password is incorrect!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
	}
}

int main(int argc ,char *argv[]){
	//创建套接字 
	int sockfd;
	struct sockaddr_in clientaddr;
	struct udp* loadudp = malloc(sizeof(struct udp));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(sockfd < 0) {
		perror("create socket error!\n");
		exit(0);
	}

	bzero(&clientaddr,sizeof(clientaddr));

	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(8003);
	clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	loadudp->sockfd = sockfd;
	loadudp->addr = &clientaddr;
	
	// 初始化GTK
    gtk_init(&argc, &argv);
    // 创建顶级窗口
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Login");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 300, 150);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    // 创建垂直布局盒子
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);
    // 创建用户名输入框
    GtkWidget *username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "QQ account");
//    gtk_entry_set_filter_func(GTK_ENTRY(username_entry), entry_input_filter, NULL, NULL); // 设置输入过滤函数
    gtk_box_pack_start(GTK_BOX(box), username_entry, TRUE, TRUE, 0);
    // 创建密码输入框
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "QQ password");
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(box), password_entry, TRUE, TRUE, 0);
    // 创建登录按钮
    GtkWidget *login_button = gtk_button_new_with_label("login");
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_button_clicked), username_entry);
    g_object_set_data(G_OBJECT(login_button), "password_entry", password_entry);
//    g_object_set_data(G_OBJECT(login_button), "loadudp", loadudp);
    g_object_set_data(G_OBJECT(login_button), "password_entry", password_entry);
    gtk_box_pack_start(GTK_BOX(box), login_button, TRUE, TRUE, 0);
    // 创建注册按钮
//    GtkWidget *register_button = gtk_button_new_with_label("sign");
//    g_signal_connect(register_button, "clicked", G_CALLBACK(on_register_button_clicked), NULL);
//    gtk_box_pack_start(GTK_BOX(box), register_button, TRUE, TRUE, 0);
    // 显示所有窗口部件
    gtk_widget_show_all(window);
    // 运行GTK主循环
    gtk_main();

	close(sockfd);
}

