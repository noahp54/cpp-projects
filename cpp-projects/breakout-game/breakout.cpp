#include <raylib.h>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace std; 

class Context{ //singelton: only one object 
public:
	static int width, height; 
	Texture2D ball_skin, paddle_skin, brick_skin;
	static Context &getResources(){
		static Context context; 
		return context; 
	}
	Context (const Context&) = delete;
	void playGameOver(){PlaySound(gameover);}
	void playWin(){PlaySound(win);}
	void playHitBrick() {PlaySound(hit_brick);}
	void playHitOthers() {PlaySound(hit_others);}
	void startBackgroundMusic(){PlayMusicStream(background);}
	void updateBackgroundMusic(){UpdateMusicStream(background);}
	void stopBackgroundMusic() {StopMusicStream(background);}
	private: 
	Music background; // long audio
	Sound gameover, win, hit_brick, hit_others; 
	Context (){ // load resources in con; releae resources in destruct- which is called auto
		InitWindow(width, height, "CS 161 Breakout Game"); 
		InitAudioDevice();
		SetTargetFPS(60);
		ball_skin = LoadTexture("image/ball.png");
		paddle_skin = LoadTexture("image/paddle.png");
		brick_skin = LoadTexture("image/brick.png"); // loading images
		background = LoadMusicStream("sound/background.ogg");
		gameover = LoadSound("sound/gameover.wav");
		win = LoadSound("sound/win.wav");
		hit_brick = LoadSound("sound/hitbrick.wav");
		hit_others = LoadSound("sound/hitothers.wav");
	} 
	~ Context (){
		UnloadMusicStream(background);
		UnloadSound(gameover);
		UnloadSound(win);
		UnloadSound(hit_brick);
		UnloadSound(hit_others);
		UnloadTexture(ball_skin);
		UnloadTexture(paddle_skin);
		UnloadTexture(brick_skin);
		CloseAudioDevice();
	}
};
int Context::width {400};
int Context::height {600}; 

template<typename T>
class Collectable {
public:
	static vector<T*> all;
	protected: 
	Collectable (){all.push_back((T*)this);
	}
	Collectable (const Collectable &r){all.push_back((T*)this);
	}
	virtual ~Collectable (){
		auto i = find(all.begin(), all.end(),this);
		if (i != all.end()) all.erase(i);
	}
};
template<typename T> vector <T*> Collectable<T>::all{}; 

class Drawable: public Collectable <Drawable> {
	public: 
	virtual void draw () = 0; 
};

class Movable: public Collectable<Movable>{
	public: 
	int dx, dy; 
	Movable(int dx, int dy): dx{dx}, dy{dy} {}
	void virtual move () = 0; 
};

class Collidable: public Collectable<Collidable> {
	protected: 
	Rectangle intersection; 
	public: 
	virtual void processCollision () = 0; 
	virtual Rectangle getBounds () = 0; 
	bool isCollidingWith (Collidable &c){
		Rectangle self = getBounds(); 
		Rectangle other = c.getBounds(); 
		intersection = (&c == this) ? Rectangle {0,0,0,0}: GetCollisionRec(self, other); 
		return intersection.width > 0 && intersection.height > 0; 
	}
};

class EventListener: public Collectable <EventListener> {
	public: 
	enum class Event {SCORE, GAMEOVER, WIN};
	virtual void processEvent (Event event) = 0; 
};

class GameObject: public Drawable, public Collidable {
protected:
	Texture2D &skin;
	GameObject (int x, int y, int w, int h, Color color, Texture2D &skin)
	:skin{skin}, color{color}, x{x}, y{y}, w{w}, h{h} {}
	GameObject (const GameObject &) = default;
	public: 
	Color color;
	int x, y, w, h;
	void draw () override {
		DrawTexturePro(skin,
			Rectangle {0, 0, (float)skin.width, (float)skin.height}, // source
			Rectangle {(float)x, (float)y, (float)w, (float)h},      // target 
			Vector2 {0, 0}, // offset from the top-left of the target
			0,      // rotation in degrees
			color   // tint
			);
	}
	Rectangle getBounds () override {
		return Rectangle {(float)x, (float)y, (float)w, (float)h};
	}
	void processCollision () {}
};

class MovingGameObject: public GameObject, public Movable {
protected:
	MovingGameObject (int x, int y, int w, int h, int dx, int dy, Color color, Texture2D &skin)
	:GameObject (x, y, w, h, color, skin), Movable (dx, dy) {}
	MovingGameObject (const MovingGameObject &) = default;
public:
	void move () override {
		x = x + dx;
		y = y + dy;
		if(x <= 0) { 
			x = 0; 
			dx = -dx; 
		} 
		if (x + w >= Context::width) { 
			x = Context::width - w; 
			dx = -dx;
		} 
		if(y <= 0) { 
			y = 0; 
			dy = -dy;
		}
	}
};

class Ball: public MovingGameObject {
public:
	Ball (int x, int y, int w, int h, int dx, int dy, Color color)
	:MovingGameObject(x, y, w, h, dx, dy, color, Context::getResources().ball_skin) {}
	void move () override {
		MovingGameObject::move();
		if(y > Context::height)
			for (auto each : EventListener::all) each->processEvent(EventListener::Event::GAMEOVER);
	}
	void processCollision() override { 
		if (intersection.width < w) {// bounce horizontally
			if (x < intersection.x) {
				x -= intersection.width;
				if (dx > 0) dx = -dx;
			} else if (x == intersection.x){
				x += intersection.width;
				if (dx < 0) dx = -dx;
			}
		}    
		if (intersection.height < h) {// bounce vertically
			if (y < intersection.y) {
				y -= intersection.height;
				if (dy > 0) dy = -dy;
			} else if (y == intersection.y){
				y += intersection.height;
				if (dy < 0) dy = -dy;
			}
		}
	}
};

class Paddle: public MovingGameObject {
public:
	Paddle (int w, int h, int dx, Color color)
	:MovingGameObject(Context::width/2-w/2, Context::height-h, 
		w, h, dx, 0, color, Context::getResources().paddle_skin) {}
	void move () override {
		if(IsKeyDown(KEY_LEFT)) x -= dx; 
		if(IsKeyDown(KEY_RIGHT)) x += dx; 
		if(x <= 0) x = 0; 
		if(x + w >= Context::width) x = Context::width - w;
	}
	void processCollision() override {
		Context::getResources().playHitOthers();
	}
};

class Brick: public GameObject {
public:
	static int population;
	Brick (int x, int y, int w, int h, Color color)
	:GameObject(x, y, w, h, color, Context::getResources().brick_skin) {++population;}
	void processCollision () override {
		w = 0;
		--population;
		for (auto each : EventListener::all) {
			each->processEvent(EventListener::Event::SCORE);
			if (population == 0) each->processEvent(EventListener::Event::WIN);
		} 
		Context::getResources().playHitBrick();
	}
};
int Brick::population {0};

class ScoreBoard: public Drawable {
private:
	string scoreString;
	Color color;
	int score {0};
public:
	string centralString;
	ScoreBoard (Color color): color{color} {reset();}
	void setScore (int score) { 
		this->score = score; 
		scoreString = "score: " + to_string(score); 
	}
	int getScore () {return score;}
	void reset () { 
		setScore(0); 
		centralString = "";    
	}
	void draw () override {
		DrawText(scoreString.c_str(), 5, 5, 25, color);
		int font_size = 50;
		int x = Context::width/2 - MeasureText(centralString.c_str(), font_size)/2;
		int y = Context::height/2 - font_size/2;
		DrawText(centralString.c_str(), x, y, font_size, color);
	}
};

class Game: public EventListener {
private:
	ScoreBoard score_board;
	int ROW_BRICKS {5};//number of rows of the bricks 
	int COL_BRICKS {6};//number of columns of the bricks 
	int W_BRICK {60};//width of the brick 
	int H_BRICK {30};//height of the brick 
	int GAP_BRICK {(Context::width - COL_BRICKS*W_BRICK)/(COL_BRICKS+1)};
	int W_PADDLE {100};//width of the paddle 
	int H_PADDLE {20};//height of the paddle 
	int DX_PADDLE {5};//dx (speed along x, increasement to the right) of the paddle 
	int W_BALL {20};//width of the ball 
	int H_BALL {20};//height of the ball 
	int DX_BALL {5};//dx (speed along x, increasement to the right) of the ball 
	int DY_BALL {5};//dy (speed along y, increasement to the bottom) of the ball
	Paddle paddle;
	Ball ball;
	vector<Brick> bricks;
	bool isRunning {false};
	
	void init () {
		isRunning = false;
		paddle.x = Context::width/2 - W_PADDLE/2;
		paddle.y = Context::height - H_PADDLE;
		ball.x = paddle.x + (W_PADDLE/2-W_BALL/2);
		ball.y = paddle.y - H_BALL;
		score_board.reset();
		Brick::population = 0;
		bricks.clear();
		for(int r {0}; r < ROW_BRICKS; ++r) {
			for(int c {0}; c < COL_BRICKS; ++c) { 
				Brick b {GAP_BRICK+c*(W_BRICK+GAP_BRICK), //x 
					30+GAP_BRICK+r*(H_BRICK+GAP_BRICK), //y 
					W_BRICK, //w 
					H_BRICK, //h 
					GREEN};
				bricks.push_back(b); /* call obj's copy constructor */
			} 
		}
		Context::getResources().stopBackgroundMusic();
		Context::getResources().startBackgroundMusic();
	}
	
	public:
	Game (): score_board{MAROON}, paddle{W_PADDLE, H_PADDLE, DX_PADDLE, YELLOW}, 
	ball{paddle.x + (W_PADDLE/2-W_BALL/2), paddle.y - H_BALL, W_BALL, H_BALL, DX_BALL, DY_BALL, RED} {
		init();
	}
	
	void processEvent(Event event) override { 
		switch(event){ 
			case Event::SCORE: 
			score_board.setScore(score_board.getScore()+1); 
			break; 
			case Event::GAMEOVER: 
			isRunning = false;
			Context::getResources().stopBackgroundMusic();
			Context::getResources().playGameOver();
			score_board.centralString = "GAME OVER!";
			break; 
		case Event::WIN:
			isRunning = false;
			Context::getResources().stopBackgroundMusic();
			Context::getResources().playWin();
			score_board.centralString = "YOU WIN!";
			break; 
		} 
	}
	
	void runGameLoop () {
		while (!WindowShouldClose()) {
			Context::getResources().updateBackgroundMusic();
			if (IsKeyPressed(KEY_ENTER)) {
				init();
				isRunning = true;
			}
			if(isRunning){
				for(auto m: Movable::all) m->move();
				for(auto c: Collidable::all){
					if(ball.isCollidingWith(*c)) {
						ball.processCollision();
						c->processCollision();
					}
				}
			}
			BeginDrawing();
			ClearBackground(BLACK);
			for (auto &d : Drawable::all) d->draw();
			EndDrawing();
		}
	};
};

int main (){
	Game breakout;
	breakout.runGameLoop(); 
	return 0; 
}


