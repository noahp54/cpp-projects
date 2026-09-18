#include <raylib.h>
#include <string>

Texture2D texture; 
const int UNIT = 40, W = 15, H = 25 ;// grid of board: 15 x 25
int L[] {11, 3, 1, 0, 0, 1, 0, 2, 0, 2, 1}; // 3, 1= bounding box size and color number 
int I[] {11, 4, 2, 0, 1, 1, 1, 2, 1, 3, 1};  
int o[] {11, 2, 3, 0, 0, 0, 1, 1, 0, 1, 1};
int s[] {11, 3, 4, 1, 1, 1, 2, 2, 0, 2, 1}; 
int t[] {11, 3, 5, 1, 0, 1, 1, 1, 2, 2, 1};
int z[]	{11, 3, 6, 1, 0, 1, 1, 2, 1, 2, 2};
int J[] {11, 3, 7, 0, 1, 1, 1, 2, 1, 2, 0};
int l[] {9, 3, 8, 0, 1, 1, 1, 2, 1};
int j[] {9, 2, 9, 0, 1, 1, 0, 1, 1}; 

int *shapes[] {L, I, o, s, t, z, J, l, j}; // can add l and j if you want; arry will decay and keep all different shapes in "database"
int n = sizeof(shapes)/sizeof(int*);
int *next; // store the next shape
int current[11]; // falling shape should be a copy because if it is rotated we dont want the og data to change
int row, col; 
int board[H][W]; 
double start_time;  
int level, score; 
bool over; 

Music music; 
Sound oversound, removesound, landsound; 

void load_next_shape(){
	int length = next[0];
	for (int i = 0; i < length; ++i){
		current [i] = next[i];
	}
	next = shapes[GetRandomValue(0, n-1)]; 
	row = 0;
	col = W/3; 
}

void init(){
	next = shapes[GetRandomValue(0, n-1)]; 
	load_next_shape();
	for (auto &row : board){
		for (auto &color : row){
			color = 0; 
		}
	}
	start_time = GetTime(); 
	level = 1; 
	score = 0; 
	over = false; 
	StopMusicStream(music);
	PlayMusicStream(music); 
}

Color get_color (int color_number){
	switch(color_number){
	case 1: 
		return ORANGE; 
	case 2: 
		return SKYBLUE; 
	case 3: 
		return GREEN; 
	case 4: 
		return BLUE; 
	case 5: 
		return PINK; 
	case 6: 
		return YELLOW; 
	case 7: 
		return RED;
	case 8:
		return PURPLE; 
	case 9: 
		return DARKGREEN;
	default: 
		return GRAY;
		
	} // cant directly save color in array because data types are different 
}

void draw_next_shape(){
	int length = next[0];
	int box_size = next[1];
	int color = next [2];
	for (int i = 3; i < length; i+= 2){
		int r = next[i];
		int c = next[i+1] + W - box_size; 
		DrawTexture(texture, c*UNIT, r*UNIT, Fade(get_color(color), 0.5f));
	}
}

void draw_current_shape(){
	int length = current[0];
	int color = current[2];
	for (int i = 3; i < length; i += 2){
		int r = current[i] + row;
		int c = current[i+1] + col;
		DrawTexture(texture, c*UNIT, r*UNIT, get_color(color));	
	}
}

bool has_collision_at(int row, int col){
	int length = current[0];
	for (int i = 3; i < length; i += 2){
		int r = current[i] + row;
		int c = current[i+1] + col;
		if (c < 0 || c >= W || r >= H || board[r][c] !=0){
			return true; 
		} 
	}
	return false; 
}

void land_at (int row, int col){
	int length = current[0];
	int color = current[2]; 
	for (int i = 3; i < length; i += 2){
		int r = current[i] + row;
		int c = current[i+1] + col;
		board[r][c]= color; 
	}
	PlaySound(landsound); 
}

void draw_board(){
	for(int r = 0; r < H; ++r){
		for (int c = 0; c < W; ++c){
			if (board[r][c] !=0){
				DrawTexture(texture, c*UNIT, r*UNIT, get_color(board[r][c]));	
			}		
		}
	}
}

void turn(){
	int length = current[0];
	int box_size = current[1]; 
	for (int i = 3; i < length; i += 2){
		int r = current[i];
		int c = current[i+1];
		current[i] = c; // new r
		current [i+1] = box_size - r - 1; // new c
	}
}

void remove_full_rows(){
	int r {H-1}, k {H-1};
	while (r >= 0){ 
		if(k != r){
			for (int c = 0; c < W; ++c){
				if (k < 0){
					board[r][c] = 0; 
				} else{
					board[r][c] = board[k][c]; 
				}
			}
		}
		for (int c = 0; c < W; ++c){
			if (board[r][c] == 0){
				--r; 
				break; 
			}		
		}
		--k;
	}
	if (k < -1){ 
		score += (-k-1) * (-k-1) *40;
		PlaySound(removesound); 
	}
}

void draw_text(){
	std::string scoreboard = "score: " + std::to_string(score) + "\n" + "level: " + std::to_string(level); 
	int fontSize = 0.8*UNIT; 
	DrawText(scoreboard.c_str(), 5, 5, fontSize, WHITE); 
	if (over){
		fontSize = W*UNIT/8; 
		DrawText("GAME OVER", W*UNIT/2 - MeasureText("GAME OVER", fontSize)/2, H*UNIT/2 - fontSize/2, fontSize, GRAY); 
		fontSize -= 3; 
		DrawText("GAME OVER", W*UNIT/2 - MeasureText("GAME OVER", fontSize)/2, H*UNIT/2 - fontSize/2, fontSize, RED); 
	}
}

int main(){
	InitWindow(W*UNIT, H*UNIT, "Classic Tetris"); // Width, height, title
	SetTargetFPS(30); // controls default speed (upper bound)
	InitAudioDevice(); 
	texture = LoadTexture("block.png"); 
	music = LoadMusicStream("background.ogg");
	oversound = LoadSound("gameover.wav"); 
	removesound = LoadSound("remove.wav"); 
	landsound = LoadSound("landing.wav"); 
	PlayMusicStream(music);

	init();
	while(!WindowShouldClose()){
		UpdateMusicStream(music); 
		if (!over && (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) && !has_collision_at(row, col-1)){
			--col;
		} else if (!over && (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) && !has_collision_at(row, col+1)){
			++col; 
		} else if (!over && (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))){
			turn(); // 90 degrees clockwise
			if(has_collision_at(row,col)){
				turn(); turn(); turn(); 
			}
		} else if (IsKeyPressed(KEY_ENTER)){
			init();
		}
		
		if (!over && (GetTime()-start_time >= 0.6 - 0.05*level || IsKeyDown(KEY_DOWN))){ // w/ this logic it can have five levels
			if (has_collision_at(row+1, col)){ // can change 0.1 to smaller number like 0.05 to have more levels
			land_at(row, col); 
			remove_full_rows();
			load_next_shape(); 
			if (has_collision_at(row,col)){
				over = true; 
				StopMusicStream(music); 
				PlaySound(oversound); 
			}
		}else {
			++row; 
		}
			start_time = GetTime(); 
		}
		
		if (score > 400*level && level < 10){ // how to level up conditions; every 50 you will level up
			++level; 
		}  
		
		BeginDrawing();
		ClearBackground(BLACK); 
		
		draw_current_shape();
		draw_board();
		draw_next_shape(); 
		draw_text(); 
		
		
		EndDrawing();
	}
	UnloadMusicStream(music);
	UnloadSound(removesound);
	UnloadSound(landsound);
	UnloadSound(oversound);
	CloseAudioDevice(); 
	CloseWindow(); 
	return 0;
}
