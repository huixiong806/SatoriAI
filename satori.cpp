#include<iostream>
#include<cstdio>
#include<cstring>
#include<algorithm>
#include<fstream>
#include<vector>
#include<set>
using namespace std;
struct Vec2i
{
	int x,y;
	Vec2i(){x=y=0;}
	Vec2i(int xx,int yy):x(xx),y(yy){}
	Vec2i operator+(const Vec2i& rhs)
	{
		return Vec2i(x+rhs.x,y+rhs.y);
	}
	bool operator<(const Vec2i& rhs)const
	{
		return (x==rhs.x)?(y<rhs.y):(x<rhs.x);
	}
	bool operator==(const Vec2i& rhs)const
	{
		return (x==rhs.x)&&(y==rhs.y);
	}
};
enum class Prop
{
	empty,
	candy,//win 
	cake,//+1hp
	up,
	down,
	left,
	right,
};
struct Npc
{
	Vec2i position;
	int direction;
	int hp;
};
struct GameProp
{
	Prop type;
	Vec2i position;
	bool exist;
	GameProp(){}
	GameProp(Prop type_,Vec2i position_)
	{
		type=type_;
		position=position_;
		exist=true;
	}
};
struct Map
{
	Npc koishi;
	Vec2i size;
	int height[16][16];
	//空=-1，地=0，上层=1，更高=2,3。0层~1层的楼梯(从低层到高层)上下左右分别为4,5,6,7。 
	Prop prop[16][16];
	int propid[16][16];
	vector<GameProp> prop_pool;
};
enum class MovementType
{
	move,//移来 
	remove,//移走(存入道具仓库) 
};
struct Movement
{
	int time;
	MovementType type;
	Vec2i position;//生效目标的位置
	int id;//道具id
	Movement(){}
	Movement(MovementType type_,int time_,int id_)
	{
		type=type_;
		time=time_;
		id=id_;
	}
	Movement(MovementType type_,int time_,Vec2i target_position_,int id_)
	{
		type=type_;
		time=time_;
		position=target_position_;
		id=id_;
	}
};
const Vec2i delta[4]={{-1,0},{1,0},{0,-1},{0,1}};
//IDS
ostream& operator <<(ostream& os,const Vec2i& vec2i)
{
	return os<<"("<<vec2i.x<<","<<vec2i.y<<")";
}
bool operator<(const pair<Vec2i,int>& lhs,const pair<Vec2i,int>& rhs)
{
	return (lhs.first==rhs.first)?(lhs.second<rhs.second):(lhs.first<rhs.first);
}
Map start_state;
int distant[256][256];
Vec2i satori_start_position;
bool can_moveto(Map& state,Vec2i from,Vec2i to,int direction)
{
	if(state.height[to.x][to.y]==-1||state.height[from.x][from.y]==-1)
		return false;
	if(state.height[from.x][from.y]<=3)
	{
		if(state.height[to.x][to.y]<=3)
			return state.height[to.x][to.y]<=state.height[from.x][from.y];
		if(state.height[from.x][from.y]>=1)return true;
		return direction==(state.height[to.x][to.y]%4);
	}else return (state.height[to.x][to.y]>3&&direction==state.height[from.x][from.y]%4)||state.height[to.x][to.y]<=1;
} 
void pretreatment(Map state)
{
	memset(distant,0x3f,sizeof(distant));
	for(int i=0;i<16;++i)
	{
		for(int j=0;j<16;++j)
		{
			if(i<15)
			{
				if(can_moveto(state,Vec2i(i,j),Vec2i(i+1,j),1))
					distant[i<<4|j][(i+1<<4)|j]=1;
				if(can_moveto(state,Vec2i(i+1,j),Vec2i(i,j),0))
					distant[(i+1<<4)|j][i<<4|j]=1;
			}
			if(j<15)
			{
				if(can_moveto(state,Vec2i(i,j),Vec2i(i,j+1),3))
					distant[i<<4|j][(i<<4)|j+1]=1;
				if(can_moveto(state,Vec2i(i,j+1),Vec2i(i,j),2))
					distant[(i<<4)|j+1][i<<4|j]=1;
			}
		}
	}
	for(int k=0;k<256;++k)
		for(int i=0;i<256;++i)
			for(int j=0;j<256;++j)
				if(distant[i][j]>distant[i][k]+distant[k][j])
					distant[i][j]=distant[i][k]+distant[k][j];
}
bool strategy_is_feasible(Map state,Vec2i satori_position,vector<Movement>& movement)
{
	double time=0.0;
	for(int i=0;i<movement.size();++i)
	{
		if(movement[i].type==MovementType::remove)return false;
		Vec2i prop_position=state.prop_pool[movement[i].id].position;
		Vec2i target_position=movement[i].position;
		time+=(double)distant[satori_position.x<<4|satori_position.y][prop_position.x<<4|prop_position.y]/5.0;
		time+=(double)distant[prop_position.x<<4|prop_position.y][target_position.x<<4|target_position.y]/5.0;
		if(time>(double)movement[i].time)return false;
		satori_position=target_position;
	}
	return true;
}
bool get_strategy(Map& state,vector<Movement>& movement,int time,int guidance_time,int max_guidance_time)
{
	//cout<<time<<endl;
	if(guidance_time>max_guidance_time)return false;
	set<pair<Vec2i,int>>visit;
	if(!strategy_is_feasible(start_state,satori_start_position,movement))return false;
	while(true)//TODO:考虑原地打转的情况 
	{
		Vec2i& now_position=state.koishi.position;
		Vec2i next_position=now_position+delta[state.koishi.direction];
		if(visit.insert(make_pair(now_position,state.koishi.direction)).second==false)return false;
		//越界(撞墙) 
		//for(int i=0;i<guidance_time;++i)
		//	cout<<"-";
		//cout<<"pos: "<<now_position<<" hp:"<<state.koishi.hp<<" time:"<<time<<" tag:"<<(int)state.prop[next_position.x][next_position.y]<<endl;
		//掉悬崖(游戏结束) 
		if(state.height[next_position.x][next_position.y]==-1)return false;
		else
		//撞墙,出界或者撞楼梯(撞)
		if((next_position.x<0||next_position.x>15||next_position.y<0||next_position.y>15)||
			(state.height[next_position.x][next_position.y]>state.height[now_position.x][now_position.y]&&state.height[next_position.x][next_position.y]<4)||
		  (state.height[next_position.x][next_position.y]>=4&&state.height[now_position.x][now_position.y]==(state.height[next_position.x][next_position.y]/4-1)&&state.koishi.direction!=(state.height[next_position.x][next_position.y])%4)	
		)
		{
			if(state.koishi.hp>1)
			{
				state.koishi.hp--;
				state.koishi.direction^=1;
				time++;	
			}else return false;
		}else 
		//平地或者上下楼梯(正常行走)
		if(state.height[next_position.x][next_position.y]==state.height[now_position.x][now_position.y]||//是平地 或
		   	(
			   state.height[next_position.x][next_position.y]>=4&&//是楼梯 且 
		   		( 
		   			((state.height[now_position.x][now_position.y]==(state.height[next_position.x][next_position.y]/4-1))&&
				   		state.koishi.direction==(state.height[next_position.x][next_position.y])%4
					)||//是可上楼梯 或 
					state.height[now_position.x][now_position.y]==(state.height[next_position.x][next_position.y]/4-2)//是可下楼梯
				) 
			)
		  )
		{
			//遇到礼品盒(指导成功) 
			if(state.prop[next_position.x][next_position.y]==Prop::candy)
			{
				//TODO:验证方案可行性 
				//return strategy_is_feasible(start_state,satori_start_position,movement);
				return true;
			}
			//遇到蛋糕
			if(state.prop[next_position.x][next_position.y]==Prop::cake)
			{
				//拿走蛋糕(指导次数+1，深入搜索)(此处不考虑具体放在哪个格子，可以认为放在了路线无关的地方)
				Map next_state=state;
				next_state.prop[next_position.x][next_position.y]=Prop::empty;
				next_state.propid[next_position.x][next_position.y]=0;
				movement.push_back(Movement(MovementType::remove,time,state.propid[next_position.x][next_position.y]));
				bool result=get_strategy(next_state,movement,time,guidance_time+1,max_guidance_time);
				if(result==true)return true;
				movement.pop_back();
				//吃掉蛋糕并向前走 
				state.koishi.hp=3;
				state.prop_pool[state.propid[next_position.x][next_position.y]].exist=false;
				state.propid[next_position.x][next_position.y]=0;
				state.prop[next_position.x][next_position.y]=Prop::empty;
				now_position=next_position;
				time++;	
			}
			//遇到路牌 
			else if((int)state.prop[next_position.x][next_position.y]>=(int)Prop::up&&(int)state.prop[next_position.x][next_position.y]<=(int)Prop::right)
			{
				//拿走路牌(指导次数+1，深入搜索)
				Map next_state=state;
				next_state.prop[next_position.x][next_position.y]=Prop::empty;
				next_state.propid[next_position.x][next_position.y]=0;
				movement.push_back(Movement(MovementType::remove,time,state.propid[next_position.x][next_position.y]));
				bool result=get_strategy(next_state,movement,time,guidance_time+1,max_guidance_time);
				if(result)return true;
				movement.pop_back();
				//根据路牌指示转向(如果同向就破坏掉) 
				if(state.koishi.direction==(int)state.prop[next_position.x][next_position.y]-(int)Prop::up)
				{
					state.prop_pool[state.propid[next_position.x][next_position.y]].exist=false;
					state.propid[next_position.x][next_position.y]=0;
					state.prop[next_position.x][next_position.y]=Prop::empty;
				}else state.koishi.direction=(int)state.prop[next_position.x][next_position.y]-(int)Prop::up;
				time++;
			}
			//遇到空地
			else if(state.prop[next_position.x][next_position.y]==Prop::empty)
			{
				//拿来一个道具(指导次数+1，深入搜索)
				for(int id=0;id<state.prop_pool.size();++id)
				{
					auto&& item=state.prop_pool[id];
					if(!item.exist)continue;
					//cout<<"*"<<endl;
					Map next_state=state;
					next_state.prop[item.position.x][item.position.y]=Prop::empty;
					next_state.prop[next_position.x][next_position.y]=item.type;
					next_state.propid[item.position.x][item.position.y]=0;
					next_state.propid[next_position.x][next_position.y]=id;
					movement.push_back(Movement(MovementType::move,time,next_position,id));
					bool result=get_strategy(next_state,movement,time,guidance_time+1,max_guidance_time);
					if(result)return true;
					movement.pop_back();
				}
				//正常向前走
				now_position=next_position;
				time++;	
			}
		}
		//跳方块或者跳楼梯(踩坏道具) 
		else if(state.height[next_position.x][next_position.y]<state.height[now_position.x][now_position.y]&&state.height[now_position.x][now_position.y]<4) 
		{
			//如果有道具 
			if(state.prop[next_position.x][next_position.y]!=Prop::empty)
			{
				//如果是礼品盒则指导失败 
				if(state.prop[next_position.x][next_position.y]==Prop::candy)return false;
				//拿走道具(指导次数+1，深入搜索)
				Map next_state=state;
				next_state.prop[next_position.x][next_position.y]=Prop::empty;
				next_state.propid[next_position.x][next_position.y]=0;
				movement.push_back(Movement(MovementType::remove,time,state.propid[next_position.x][next_position.y]));
				bool result=get_strategy(next_state,movement,time,guidance_time+1,max_guidance_time);
				if(result)return true;
				movement.pop_back();
				//踩坏道具
				state.prop_pool[state.propid[next_position.x][next_position.y]].exist=false;
				state.propid[next_position.x][next_position.y]=0;
				state.prop[next_position.x][next_position.y]=Prop::empty;
			}
			now_position=next_position; 
			time++;
		}
	}
	return false;
}
//map格式
/*
第一行 satori的坐标
第二行 koishi的坐标
接下来16*16个整数
表示高度
接下来16*16个整数
表示道具 
*/ 
void printMap(Map& map)
{
	for(int i=0;i<16;++i)
	{
		for(int j=0;j<16;++j)
			printf("%4.0f",(float)map.prop[i][j]);
		cout<<endl;
	}
}
Map readMap()
{
	fstream fs;
	fs.open("map\\map4.txt", ios::in); 
	Map map;
	int satori_dir;
	fs >>satori_start_position.x>>satori_start_position.y>>satori_dir;
	fs >>map.koishi.position.x>>map.koishi.position.y>>map.koishi.direction;
	map.koishi.hp=3;
	for(int i=0;i<16;++i)
		for(int j=0;j<16;++j)
			fs >>map.height[i][j];
	for(int i=0;i<16;++i)
		for(int j=0;j<16;++j)
		{
			int temp;
			fs >>temp;
			map.prop[i][j]=(Prop)temp;
			if(temp>=2)
			{
				map.prop_pool.push_back(GameProp((Prop)temp,Vec2i(i,j)));
				map.propid[i][j]=map.prop_pool.size()-1;
			}else map.propid[i][j]=0;
		}
	return map;
}

int main()
{
	start_state=readMap();
	//printMap(start_state);
	pretreatment(start_state);
	vector<Movement> movement;
	int guidance_time;
	for(guidance_time=0;guidance_time<8;++guidance_time)
	{
		Map state=start_state;
		if(get_strategy(state,movement,1,0,guidance_time))break;
		cout << "已搜索完成" << guidance_time << "次指导，没有发现可行解" << endl;
	}
	cout<<"指导次数:"<<guidance_time<<endl;
	for(int i=0;i<movement.size();++i)
	{
		if((int)movement[i].type==0)
			cout<<"第"<<movement[i].time<<"个时刻，第"<<movement[i].id<<"个道具应当被放在"<<movement[i].position<<endl;
		else cout<<"第"<<movement[i].time<<"个时刻，第"<<movement[i].id<<"个道具应当被移开"<<endl;
	}
	while(getchar()); 
	return 0;
}
