#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include<sys/select.h>
#include <string.h>
#include<sys/socket.h>
#include <limits.h>

#include "logging.c"

#define PIPE(a) socketpair(AF_UNIX, SOCK_STREAM,PF_UNIX,(a))

typedef struct monster_info {
	int alive_monster_c;
	coordinate monster_coordinates[MONSTER_LIMIT];
	int monster_indexes[MONSTER_LIMIT];
} monster_info;

int player_pipefd[2];
int number_of_monsters;
fd_set readset;

void bubbleSort(struct monster_info *data)
{
	int	n = MONSTER_LIMIT;
	for (int i = 0; i < MONSTER_LIMIT; i++){
		if( data->monster_indexes[i] ==-1 ){
			data->monster_coordinates[i].x =INT_MAX;
			data->monster_coordinates[i].y =INT_MAX;
		}}

		for (int i = 0; i < n-1; i++){
			for (int j = 0; j < n-i-1; j++){
				if (data->monster_coordinates[j].y > data->monster_coordinates[j+1].y){
					int tmp = data->monster_coordinates[j].y;
					data->monster_coordinates[j].y  = data->monster_coordinates[j+1].y;
					data->monster_coordinates[j+1].y = tmp;
					tmp = data->monster_coordinates[j].x;
					data->monster_coordinates[j].x  = data->monster_coordinates[j+1].x;
					data->monster_coordinates[j+1].x = tmp;
					tmp = data->monster_indexes[j];
					data->monster_indexes[j]= data->monster_indexes[j+1];
					data->monster_indexes[j+1] = tmp;
				}
			}
		}

		for (int i = 0; i < n-1; i++){
			for (int j = 0; j < n-i-1; j++){
				if (data->monster_coordinates[j].x > data->monster_coordinates[j+1].x){
					int tmp = data->monster_coordinates[j].y;
					data->monster_coordinates[j].y  = data->monster_coordinates[j+1].y;
					data->monster_coordinates[j+1].y = tmp;
					tmp = data->monster_coordinates[j].x;
					data->monster_coordinates[j].x  = data->monster_coordinates[j+1].x;
					data->monster_coordinates[j+1].x = tmp;
					tmp = data->monster_indexes[j];
					data->monster_indexes[j]= data->monster_indexes[j+1];
					data->monster_indexes[j+1] = tmp;
				}
			}
		}

	}

	int get_alive_count(int alive_monster[]){
		int alive =0;
		for(int i=0;i<number_of_monsters;i++){
			if(alive_monster[i]==1){
				alive++;
			}
		}
		return alive;
	}

	int isIn(int x,int y, coordinate monster_coordinates[], int count){

		for (int i = 0; i < count; ++i)
		{
			if(monster_coordinates[i].x==x && monster_coordinates[i].y==y){
				return 1;
			}
		}
		return 0;
	}
	int isInMonster(int x,int y, coordinate monster_coordinates[], int count, int indexes[]){
		int count1 = 0;

		for (int i = 0; i < count; ++i)
		{
			if(indexes[i]!=-1){
				if(monster_coordinates[i].x==x && monster_coordinates[i].y==y){
					count1++;
				}
			}
		}
		if(count1>0){
			return 1;
		}
		return 0;
	}

	int isValidMove(int x,int y,int w,int h, coordinate monster_coordinates[], int count){
		if((x>0)&&(x<(w-1))&&(y>0)&&(y<(h-1))&&(isIn(x,y,monster_coordinates,count)==0)){
			return 1;
		}
		return 0;
	}

	int isValidMoveMonster(int x,int y,int w,int h, coordinate monster_coordinates[], int indexes[], int count){
		if((x>0)&&(x<(w-1))&&(y>0)&&(y<(h-1))&&(isInMonster(x,y,monster_coordinates,count,indexes)==0)){
			return 1;
		}
		return 0;
	}


	int main(int argc, char *argv[]){

		monster_info m_info;
		int game_world_pid = getpid();
		player_response player_in_msg;

		int game_pid = getpid();

		char player_path[200];
		int width_of_the_room;
		int height_of_the_room;
		coordinate position_of_the_player;
		coordinate position_of_the_door;
		int p_argument_1,p_argument_2,p_argument_3, p_argument_4, p_argument_5;
		int player_pid;
		int pid;
		int player_finished = 0;


		scanf("%d", &width_of_the_room);
		scanf("%d", &height_of_the_room);
		scanf("%d", &position_of_the_door.x);
		scanf("%d", &position_of_the_door.y);
		scanf("%d", &position_of_the_player.x);
		scanf("%d", &position_of_the_player.y);


		scanf("%s",player_path);
		scanf("%d",&p_argument_3);
		scanf("%d",&p_argument_4);
		scanf("%d",&p_argument_5);
		scanf("%d",&number_of_monsters);

		p_argument_1 = position_of_the_door.x;
		p_argument_2 = position_of_the_door.y;

		char player_argv1[10];
		char player_argv2[10];
		char player_argv3[10];
		char player_argv4[10];
		char player_argv5[10];

		sprintf(player_argv1, "%d", p_argument_1);
		sprintf(player_argv2, "%d", p_argument_2);
		sprintf(player_argv3, "%d", p_argument_3);
		sprintf(player_argv4, "%d", p_argument_4);
		sprintf(player_argv5, "%d", p_argument_5);

		char* const player_arguments[] = {player_path,player_argv1, player_argv2,player_argv3,player_argv4,player_argv5, NULL};
		game_world_pid = getpid();
		PIPE(player_pipefd);

		int alive_player =1;
		int alive_monster[number_of_monsters];
		for(int i = 0; i < number_of_monsters; i++)
			alive_monster[i] =1;



		if (player_pid = fork() == 0 ) {

			dup2(player_pipefd[0],0);
			dup2(player_pipefd[0],1);

			execv (player_path, (char **)player_arguments);
			perror("error");

		}
		else{
		////////////
		}

		int maximum_fd=0;
		if(player_pipefd[1]>maximum_fd && alive_player == 1)
			maximum_fd = player_pipefd[1]+1;

		player_response player_response_msg ;
		int res =0;

		while(1){
			size_t size = sizeof(player_response);
			res = read(player_pipefd[1], &player_response_msg, sizeof(player_response));
			
			if(res>0 && player_response_msg.pr_type == pr_ready)
				break;
		}

		char monsters_paths[number_of_monsters][200];
		char monster_symbol[number_of_monsters];

		coordinate monster_coordinate[number_of_monsters];

		int m_argument_1[number_of_monsters];
		int m_argument_2[number_of_monsters];
		int m_argument_3[number_of_monsters];
		int m_argument_4[number_of_monsters];
		int monsters_pid[number_of_monsters];

		char monster_argv1[number_of_monsters][10];
		char monster_argv2[number_of_monsters][10];
		char monster_argv3[number_of_monsters][10];
		char monster_argv4[number_of_monsters][10];
		int monster_pipefd[number_of_monsters][2];

		for(int i = 0; i < number_of_monsters; i++){
			scanf("%s",monsters_paths[i]);
			scanf("%s",&monster_symbol[i]);
			scanf ("%d",&monster_coordinate[i].x);
			scanf ("%d",&monster_coordinate[i].y);
			scanf ("%d",&m_argument_1[i]);
			scanf ("%d",&m_argument_2[i]);
			scanf ("%d",&m_argument_3[i]);
			scanf ("%d",&m_argument_4[i]);
			PIPE(monster_pipefd[i]);
		}


		for(int i = 0; i < number_of_monsters; i++){
			char* const monster_arguments[] = {monsters_paths[i],monster_argv1[i], monster_argv2[i],monster_argv3[i],monster_argv4[i],NULL};
			sprintf(monster_argv1[i], "%d", m_argument_1[i]);
			sprintf(monster_argv2[i], "%d", m_argument_2[i]);
			sprintf(monster_argv3[i], "%d", m_argument_3[i]);
			sprintf(monster_argv4[i], "%d", m_argument_4[i]);

			if (monsters_pid[i] = fork() == 0 ) {

				close(player_pipefd[0]);
				close(player_pipefd[1]);

				for(int k=0;k<number_of_monsters;k++)
				{
					if(i==k )
					{
						dup2(monster_pipefd[i][0],0);
						dup2(monster_pipefd[i][0],1);
						close(monster_pipefd[i][1]);
					}
					else
					{
						close(monster_pipefd[i][1]);
					}
				}

				execv (monsters_paths[i], (char **)monster_arguments);
				perror("error");
			}
		}

		int monster_read_count= get_alive_count(alive_monster);
		monster_response monster_response_msg_arr[number_of_monsters];


		for(int i=0;i<number_of_monsters;i++)
		{
			monster_response monster_response_msg;

			if(alive_monster[i]==1 && monster_read_count > 0 ){

          //read_monster(&readset,monster_pipefd, alive_monster,number_of_monsters);
				res = read(monster_pipefd[i][1], &monster_response_msg, sizeof(monster_response));
				monster_read_count--;
			}

			if(res>0){
				monster_response_msg_arr[i] = monster_response_msg;
			}
			if(monster_read_count==0)
				break;
		}

		map_info m;
		m.map_width = width_of_the_room;
		m.map_height = height_of_the_room;
		m.door.x = position_of_the_door.x;
		m.door.y = position_of_the_door.y;
		m.player.x = position_of_the_player.x;
		m.player.y = position_of_the_player.y;
		m.alive_monster_count = number_of_monsters;
		for(int i = 0; i < number_of_monsters; i++){
			m.monster_types[i] = monster_symbol[i];
			m.monster_coordinates[i].x = monster_coordinate[i].x;
			m.monster_coordinates[i].y = monster_coordinate[i].y;
			m_info.monster_coordinates[i].x = monster_coordinate[i].x;
			m_info.monster_coordinates[i].y = monster_coordinate[i].y;

		}

		print_map(&m);

		for (int i = 0; i < MONSTER_LIMIT; ++i)
		{
			if(i<number_of_monsters){
				m_info.monster_indexes[i] = i;
			}else{
				m_info.monster_indexes[i] = -1;
			}
		}

		player_message p_message;
		coordinate p_cordinate;
		p_cordinate.x = position_of_the_player.x;
		p_cordinate.y = position_of_the_player.y;
		int p_total_damage = 0;
		bool game_over = false;
		int monster_limit = number_of_monsters;
		int attacked_monster[MONSTER_LIMIT];
		int leave_count = p_argument_5;
		for (int i = 0; i < MONSTER_LIMIT; ++i)
		{
			attacked_monster[i] = 0;
		}

		m_info.alive_monster_c = get_alive_count(alive_monster);


		while(alive_player == 1 && m_info.alive_monster_c > 0){



			monster_message m_message;

		//if monster 0
			if(m_info.alive_monster_c<1){
				game_over = true;
				p_message.game_over = game_over;
				write(player_pipefd[1], &p_message, sizeof(player_message));
				m.player.x = p_cordinate.x;
				m.player.y = p_cordinate.y;
				m.alive_monster_count = m_info.alive_monster_c;
				for(int i = 0; i < m_info.alive_monster_c; i++){
					int j = m_info.monster_indexes[i];
					m.monster_types[i] = monster_symbol[j];
					m.monster_coordinates[i].x = m_info.monster_coordinates[i].x;
					m.monster_coordinates[i].y = m_info.monster_coordinates[i].y;
				}

				print_map(&m);
				print_game_over(go_survived);
				break;
			}

		//send message to player

			p_message.new_position.x = p_cordinate.x;
			p_message.new_position.y = p_cordinate.y;
			p_message.total_damage = p_total_damage;
			p_total_damage = 0;
			p_message.alive_monster_count = m_info.alive_monster_c;
			for (int i = 0; i < m_info.alive_monster_c; ++i)
			{
				p_message.monster_coordinates[i].x = m_info.monster_coordinates[i].x;
				p_message.monster_coordinates[i].y = m_info.monster_coordinates[i].y;
			}
			p_message.game_over = game_over;

			write(player_pipefd[1], &p_message, sizeof(player_message));

			if(leave_count==1){
			//when player leaves
				game_over = true;
				for(int i=0; i<m_info.alive_monster_c; i++)
				{
					m_message.new_position.x=m_info.monster_coordinates[i].x;
					m_message.new_position.y=m_info.monster_coordinates[i].y;
					m_message.damage=attacked_monster[i];
					attacked_monster[i] = 0;
					m_message.player_coordinate.x=p_cordinate.x;
					m_message.player_coordinate.y=p_cordinate.y;
					m_message.game_over = game_over;
					int j = m_info.monster_indexes[i];
					write(monster_pipefd[j][1], &m_message, sizeof(monster_message));
				}
				m.player.x = p_cordinate.x;
				m.player.y = p_cordinate.y;
				m.alive_monster_count = m_info.alive_monster_c;
				for(int i = 0; i < m_info.alive_monster_c; i++){
					int j = m_info.monster_indexes[i];
					m.monster_types[i] = monster_symbol[j];
					m.monster_coordinates[i].x = m_info.monster_coordinates[i].x;
					m.monster_coordinates[i].y = m_info.monster_coordinates[i].y;
				}

				print_map(&m);
				print_game_over(go_left);
				break;

			}
			leave_count--;


		// get response and process player

			while(1){

				size_t size = sizeof(player_response);
			//read_player(&readset, player_pipefd,alive_player);
				res = read(player_pipefd[1], &player_response_msg, sizeof(player_response));

				if(res>0 && player_response_msg.pr_type != pr_ready)
					break;
			}


			if(player_response_msg.pr_type == pr_move){
				coordinate temp;
				temp.x  = player_response_msg.pr_content.move_to.x;
				temp.y  = player_response_msg.pr_content.move_to.y;
				if(temp.x == position_of_the_door.x && temp.y == position_of_the_door.y){
					game_over = true;

					for(int i=0; i<m_info.alive_monster_c; i++)
					{
						m_message.new_position.x=m_info.monster_coordinates[i].x;
						m_message.new_position.y=m_info.monster_coordinates[i].y;
						m_message.damage=attacked_monster[i];
						attacked_monster[i] = 0;
						m_message.player_coordinate.x=p_cordinate.x;
						m_message.player_coordinate.y=p_cordinate.y;
						m_message.game_over = game_over;

						int j = m_info.monster_indexes[i];
						write(monster_pipefd[j][1], &m_message, sizeof(monster_message));
					}
					
					p_message.game_over = game_over;
					write(player_pipefd[1], &p_message, sizeof(player_message));
					m.player.x = temp.x;
					m.player.y = temp.y;
					m.alive_monster_count = m_info.alive_monster_c;

					for(int i = 0; i < m_info.alive_monster_c; i++){
						int j = m_info.monster_indexes[i];
						m.monster_types[i] = monster_symbol[j];
						m.monster_coordinates[i].x = m_info.monster_coordinates[i].x;
						m.monster_coordinates[i].y = m_info.monster_coordinates[i].y;
					}

					print_map(&m);
					print_game_over(go_reached);
					break;
				}else{

					if(isValidMove(temp.x,temp.y,width_of_the_room,height_of_the_room,m_info.monster_coordinates, m_info.alive_monster_c) == 1){
						p_cordinate.x = temp.x;
						p_cordinate.y = temp.y;
					}
				}
			}else if(player_response_msg.pr_type == pr_attack){

				for (int i = 0; i < MONSTER_LIMIT; ++i)
				{
					attacked_monster[i] = player_response_msg.pr_content.attacked[i];
				}

			}else if(player_response_msg.pr_type == pr_dead){
				game_over = true;

				for(int i=0; i<m_info.alive_monster_c; i++)
				{
					m_message.new_position.x=m_info.monster_coordinates[i].x;
					m_message.new_position.y=m_info.monster_coordinates[i].y;
					m_message.damage=attacked_monster[i];
					attacked_monster[i] = 0;
					m_message.player_coordinate.x=p_cordinate.x;
					m_message.player_coordinate.y=p_cordinate.y;
					m_message.game_over = game_over;

					int j = m_info.monster_indexes[i];
					write(monster_pipefd[j][1], &m_message, sizeof(monster_message));
				}
				m.player.x = p_cordinate.x;
				m.player.y = p_cordinate.y;
				m.alive_monster_count = m_info.alive_monster_c;
				for(int i = 0; i < m_info.alive_monster_c; i++){
					int j = m_info.monster_indexes[i];
					m.monster_types[i] = monster_symbol[j];
					m.monster_coordinates[i].x = m_info.monster_coordinates[i].x;
					m.monster_coordinates[i].y = m_info.monster_coordinates[i].y;
				}

				print_map(&m);
				print_game_over(go_died);
				break;
			}

			if(m_info.alive_monster_c<1){
				game_over = true;
				p_message.game_over = game_over;
				write(player_pipefd[1], &p_message, sizeof(player_message));
				m.player.x = p_cordinate.x;
				m.player.y = p_cordinate.y;
				m.alive_monster_count = m_info.alive_monster_c;
				for(int i = 0; i < m_info.alive_monster_c; i++){
					int j = m_info.monster_indexes[i];
					m.monster_types[i] = monster_symbol[j];
					m.monster_coordinates[i].x = m_info.monster_coordinates[i].x;
					m.monster_coordinates[i].y = m_info.monster_coordinates[i].y;
				}

				print_map(&m);
				print_game_over(go_survived);
				break;
			}



		//send message to monster


			for(int i=0; i<m_info.alive_monster_c; i++)
			{
				m_message.new_position.x=m_info.monster_coordinates[i].x;
				m_message.new_position.y=m_info.monster_coordinates[i].y;
				m_message.damage=attacked_monster[i];
				attacked_monster[i] = 0;
				m_message.player_coordinate.x=p_cordinate.x;
				m_message.player_coordinate.y=p_cordinate.y;
				m_message.game_over = game_over;


				int j = m_info.monster_indexes[i];
				write(monster_pipefd[j][1], &m_message, sizeof(monster_message));
			}

		//get responser and process monster

			bubbleSort(&m_info);
			monster_read_count=m_info.alive_monster_c;
			int k = 0;

			for(int i=0;i<m_info.alive_monster_c;i++)
			{

				monster_response monster_response_msg;


				int j = m_info.monster_indexes[i];
				res = read(monster_pipefd[j][1], &monster_response_msg, sizeof(monster_response));
				monster_read_count--;


				if(res>0){

					monster_response_msg_arr[k] = monster_response_msg;
					k++;
				}
				if(monster_read_count==0)
					break;
			}

			int count_arr = m_info.alive_monster_c;

			for (int i = 0; i < count_arr; ++i)
			{
				if(monster_response_msg_arr[i].mr_type == mr_move){
					coordinate temp;
					temp.x  = monster_response_msg_arr[i].mr_content.move_to.x;
					temp.y  = monster_response_msg_arr[i].mr_content.move_to.y;
					if(isValidMoveMonster(temp.x,temp.y,width_of_the_room,height_of_the_room,m_info.monster_coordinates,m_info.monster_indexes, MONSTER_LIMIT) == 1){
						m_info.monster_coordinates[i].x = temp.x;
						m_info.monster_coordinates[i].y = temp.y;
					}			
				}else if(monster_response_msg_arr[i].mr_type == mr_attack){
					p_total_damage = p_total_damage + monster_response_msg_arr[i].mr_content.attack;

				}else if(monster_response_msg_arr[i].mr_type == mr_dead){
					m_info.alive_monster_c = m_info.alive_monster_c - 1;
					int j = m_info.monster_indexes[i];
					alive_monster[j] = 0;
					close(monster_pipefd[j][0]);
					close(monster_pipefd[j][1]);
					m_info.monster_indexes[i] = -1;
				}
			}
			bubbleSort(&m_info);
			m.player.x = p_cordinate.x;
			m.player.y = p_cordinate.y;
			m.alive_monster_count = m_info.alive_monster_c;
			for(int i = 0; i < m_info.alive_monster_c; i++){
				int j = m_info.monster_indexes[i];
				m.monster_types[i] = monster_symbol[j];
				m.monster_coordinates[i].x = m_info.monster_coordinates[i].x;
				m.monster_coordinates[i].y = m_info.monster_coordinates[i].y;
			}

			print_map(&m);
			if(m_info.alive_monster_c<1){
				game_over = true;
				p_message.game_over = game_over;
				write(player_pipefd[1], &p_message, sizeof(player_message));
				print_game_over(go_survived);
				break;
			}

		}


		int player_status;
		pid_t wpid_player = waitpid(player_pid, &player_status, 0);

		int monster_status;
		for (int i = 0; i < number_of_monsters; i++){
			pid_t wpid_monster = waitpid(monsters_pid[i], &monster_status, 0);
		}

		close(player_pipefd[0]);
		close(player_pipefd[1]);
		for(int k=0;k<number_of_monsters;k++)
		{
			close(monster_pipefd[k][0]);
			close(monster_pipefd[k][1]);
		}



		return 0;
	}
