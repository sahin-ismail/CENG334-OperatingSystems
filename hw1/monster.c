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

int distance(int x1, int y1, int x2, int y2) {
	int res = 0;
	res = abs(x1 - x2) + abs(y1 - y2);
	return res;
}

int main(int argc, char const *argv[]){
	int health = atoi( argv[1] );
	int damage = atoi( argv[2] );
	int defence = atoi( argv[3] );
	int range = atoi( argv[4] );
	coordinate position;
	coordinate player;
	int dis,dis1,tmp;
	coordinate up,upper_right,right,bottom_right,down,bottom_left,left,upper_left;


	monster_response   response;
	response.mr_type =mr_ready;
	write(0, &response, sizeof(monster_response));

	while(1){
		monster_message m;
		read(0, &m, sizeof(monster_message));
		
		if(m.game_over==true){
			response.mr_type = mr_dead;
			write(0, &response, sizeof(monster_response));
			break;
		}
		position.x = m.new_position.x;
		position.y = m.new_position.y;

		if((m.damage - defence) > 0){
			health = health - (m.damage - defence);
		}
		if(health<=0){
			response.mr_type = mr_dead;
			write(0, &response, sizeof(monster_response));
			break;
		}

		player.x = m.player_coordinate.x;
		player.y = m.player_coordinate.y;

		dis = distance(position.x,position.y,player.x,player.y);
		if(dis <= range){
			response.mr_type = mr_attack;
			response.mr_content.attack = damage;
		}else{
			response.mr_type = mr_move;
			up.x = position.x;
			up.y = position.y - 1;
			upper_right.x = position.x + 1;
			upper_right.y = position.y - 1;
			right.x = position.x + 1;
			right.y = position.y;
			bottom_right.x = position.x + 1;
			bottom_right.y = position.y + 1;
			down.x = position.x;
			down.y = position.y + 1;
			bottom_left.x = position.x - 1;
			bottom_left.y = position.y + 1;
			left.x = position.x - 1;
			left.y = position.y;
			upper_left.x = position.x - 1;
			upper_left.y = position.y - 1;
			dis1 = 99999;
			tmp = 0;

			if(distance(up.x,up.y,player.x,player.y)<dis1){
				tmp = 1;
				dis1 = distance(up.x,up.y,player.x,player.y);
			}
			if(distance(upper_right.x,upper_right.y,player.x,player.y)<dis1){
				tmp = 2;
				dis1 = distance(upper_right.x,upper_right.y,player.x,player.y);
			}
			if(distance(right.x,right.y,player.x,player.y)<dis1){
				tmp = 3;
				dis1 = distance(right.x,right.y,player.x,player.y);
			}
			if(distance(bottom_right.x,bottom_right.y,player.x,player.y)<dis1){
				tmp = 4;
				dis1 = distance(bottom_right.x,bottom_right.y,player.x,player.y);
			}
			if(distance(down.x,down.y,player.x,player.y)<dis1){
				tmp = 5;
				dis1 = distance(down.x,down.y,player.x,player.y);
			}
			if(distance(bottom_left.x,bottom_left.y,player.x,player.y)<dis1){
				tmp = 6;
				dis1 = distance(bottom_left.x,bottom_left.y,player.x,player.y);
			}
			if(distance(left.x,left.y,player.x,player.y)<dis1){
				tmp = 7;
				dis1 = distance(left.x,left.y,player.x,player.y);
			}
			if(distance(upper_left.x,upper_left.y,player.x,player.y)<dis1){
				tmp = 8;
				dis1 = distance(upper_left.x,upper_left.y,player.x,player.y);
			}

			if(tmp==1){
				response.mr_content.move_to.x = up.x;
				response.mr_content.move_to.y = up.y;
			}else if(tmp==2){
				response.mr_content.move_to.x = upper_right.x;
				response.mr_content.move_to.y = upper_right.y;
			}else if(tmp==3){
				response.mr_content.move_to.x = right.x;
				response.mr_content.move_to.y = right.y;
			}else if(tmp==4){
				response.mr_content.move_to.x = bottom_right.x;
				response.mr_content.move_to.y = bottom_right.y;
			}else if(tmp==5){
				response.mr_content.move_to.x = down.x;
				response.mr_content.move_to.y = down.y;
			}else if(tmp==6){
				response.mr_content.move_to.x = bottom_left.x;
				response.mr_content.move_to.y = bottom_left.y;
			}else if(tmp==7){
				response.mr_content.move_to.x = left.x;
				response.mr_content.move_to.y = left.y;
			}else if(tmp==8){
				response.mr_content.move_to.x = upper_left.x;
				response.mr_content.move_to.y = upper_left.y;
			}
		}
		write(0, &response, sizeof(monster_response));
	}

	close(0);
	close(1);

	return 0;
}
