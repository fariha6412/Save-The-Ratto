#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <sstream>
#include <bits/stdc++.h>
using namespace std;

//Screen dimension constants
const int SCREEN_WIDTH = 700;
const int SCREEN_HEIGHT = 480;
int positionx, positiony;
//Button constants
const int BUTTON_WIDTH = 150;
const int BUTTON_HEIGHT = 100;
const int TOTAL_BUTTONS = 4;

Uint64 startTime = 0;

//In memory text stream
std::stringstream timeText;

SDL_Rect wall[10];
SDL_Rect love[1];
SDL_Rect food[1];
const int parts = 1;
SDL_Rect gSpriteClips[parts];

const int foody = 1;
SDL_Rect gfoodyClips[foody];

bool quit = false;
long long int score;
int life, checklife, noncollied;
//Texture wrapper class
class LTexture
{
public:
	//Initializes variables
	LTexture();

	//Deallocates memory
	~LTexture();

#if defined(_SDL_TTF_H) || defined(SDL_TTF_H)
	//Creates image from font string
	bool loadFromRenderedText(std::string textureText, SDL_Color textColor);
#endif
	//Loads image at specified path
	bool loadFromFile(std::string path);

	void free();

	//Set color modulation
	void setColor(Uint8 red, Uint8 green, Uint8 blue);

	//Set blending
	void setBlendMode(SDL_BlendMode blending);

	//Set alpha modulation
	void setAlpha(Uint8 alpha);

	//Renders texture at given point
	void render(int x, int y, SDL_Rect *clip = NULL, double angle = 0.0, SDL_Point *center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

	//Gets image dimensions
	int getWidth();
	int getHeight();

private:
	//The actual hardware texture
	SDL_Texture *mTexture;

	//Image dimensions
	int mWidth;
	int mHeight;
};

//The dot that will move around on the screen
class Dot
{
public:
	//The dimensions of the dot
	static const int DOT_WIDTH = 115;
	static const int DOT_HEIGHT = 75;

	//Maximum axis velocity of the dot
	static const int DOT_VEL = 7;

	//Initializes the variables
	Dot();

	//Takes key presses and adjusts the dot's velocity
	void handleEvent(SDL_Event &e);

	//Moves the dot
	void move(SDL_Rect &wall);

	//Shows the dot on the screen
	void render();

	//The X and Y offsets of the dot
	int mPosX, mPosY;

	//The velocity of the dot
	int mVelX, mVelY;
	SDL_Rect mCollider;
};

enum LButtonSprite
{
	BUTTON_SPRITE_MOUSE_OUT = 0,
	BUTTON_SPRITE_MOUSE_OVER_MOTION = 1,
	BUTTON_SPRITE_MOUSE_DOWN = 2,
	BUTTON_SPRITE_MOUSE_UP = 3,
	BUTTON_SPRITE_TOTAL = 4
};

//The mouse button
class LButton
{
public:
	//Initializes internal variables
	LButton();

	//Sets top left position
	void setPosition(int x, int y);

	//Handles mouse event
	void handleEvent(SDL_Event *e);

	//Shows button sprite
	void render();

private:
	//Top left position
	SDL_Point mPosition;
	//Currently used global sprite
	LButtonSprite mCurrentSprite;
};

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

int loadmenu();
void loadscoreboard();
void loadhighscore();
void loadinstructions();
//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window *gWindow = NULL;
//Globally used font
TTF_Font *gFont = NULL;

//Scene textures
LTexture gTimeTextTexture;
LTexture gPromptTextTexture;


//The window renderer
SDL_Renderer *gRenderer = NULL;
bool checkCollision(SDL_Rect a, SDL_Rect b);
//Scene textures
LTexture gDotTexture;
LTexture gBGTexture;
LTexture gSpriteSheetTexture;
LTexture gfoodySheetTexture;

//The music that will be played
Mix_Music *gMusic = NULL;

Mix_Chunk *gScratch = NULL;
LTexture::LTexture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}

bool LTexture::loadFromFile(std::string path)
{
	//Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture *newTexture = NULL;

	//Load image at specified path
	SDL_Surface *loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Color key image
		SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 180, 225));

		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}
#if defined(_SDL_TTF_H) || defined(SDL_TTF_H)
bool LTexture::loadFromRenderedText(std::string textureText, SDL_Color textColor)
{
	//Get rid of preexisting texture
	free();

	//Render text surface
	SDL_Surface *textSurface = TTF_RenderText_Solid(gFont, textureText.c_str(), textColor);
	if (textSurface != NULL)
	{
		//Create texture from surface pixels
		mTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
		if (mTexture == NULL)
		{
			printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = textSurface->w;
			mHeight = textSurface->h;
		}

		//Get rid of old surface
		SDL_FreeSurface(textSurface);
	}
	else
	{
		printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
	}

	//Return success
	return mTexture != NULL;
}
#endif

bool checkCollision(SDL_Rect a, SDL_Rect b)
{
	//The sides of the rectangles
	int leftA, leftB;
	int rightA, rightB;
	int topA, topB;
	int bottomA, bottomB;

	//Calculate the sides of rect A
	leftA = a.x;
	rightA = a.x + a.w;
	topA = a.y;
	bottomA = a.y + a.h;

	//Calculate the sides of rect B
	leftB = b.x;
	rightB = b.x + b.w;
	topB = b.y;
	bottomB = b.y + b.h;
	//If any of the sides from A are outside of B
	if (bottomA <= topB)
	{
		return false;
	}

	if (topA + 20 >= bottomB)
	{
		return false;
	}

	if (rightA <= leftB + 7)
	{
		return false;
	}

	if ((leftA + 40) >= rightB)
	{
		return false;
	}

	return true;
}

void LTexture::free()
{
	//Free texture if it exists
	if (mTexture != NULL)
	{
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	//Modulate texture rgb
	SDL_SetTextureColorMod(mTexture, red, green, blue);
}

void LTexture::setBlendMode(SDL_BlendMode blending)
{
	//Set blending function
	SDL_SetTextureBlendMode(mTexture, blending);
}

void LTexture::setAlpha(Uint8 alpha)
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod(mTexture, alpha);
}

void LTexture::render(int x, int y, SDL_Rect *clip, double angle, SDL_Point *center, SDL_RendererFlip flip)
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = {x, y, mWidth, mHeight};

	//Set clip rendering dimensions
	if (clip != NULL)
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(gRenderer, mTexture, clip, &renderQuad, angle, center, flip);
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}

Dot::Dot()
{
	//Initialize the offsets
	mPosX = 0;
	mPosY = 0;

	//Set collision box dimension
	mCollider.w = DOT_WIDTH;
	mCollider.h = DOT_HEIGHT;

	//Initialize the velocity
	mVelX = 0;
	mVelY = 0;
}

void Dot::handleEvent(SDL_Event &e)
{
	if (e.type == SDL_QUIT)
	{
		quit = true;
	}
	//If a key was pressed
	if (e.type == SDL_KEYDOWN && e.key.repeat == 0)
	{
		//Adjust the velocity
		switch (e.key.keysym.sym)
		{
		case SDLK_UP:
			mVelY -= DOT_VEL;
			Mix_PlayChannel(-1, gScratch, 0);
			break;
		case SDLK_DOWN:
			mVelY += DOT_VEL;
			Mix_PlayChannel(-1, gScratch, 0);
			break;
		case SDLK_LEFT:
			mVelX -= DOT_VEL;
			Mix_PlayChannel(-1, gScratch, 0);
			break;
		case SDLK_RIGHT:
			mVelX += DOT_VEL;
			Mix_PlayChannel(-1, gScratch, 0);
			break;
		}
	}
	//If a key was released
	else if (e.type == SDL_KEYUP && e.key.repeat == 0)
	{
		//Adjust the velocity
		switch (e.key.keysym.sym)
		{
		case SDLK_UP:
			mVelY += DOT_VEL;
			Mix_PlayChannel(-1, gScratch, 0);
			break;
		case SDLK_DOWN:
			mVelY -= DOT_VEL;
			Mix_PlayChannel(-1, gScratch, 0);
			break;
		case SDLK_LEFT:
			mVelX += DOT_VEL;
			Mix_PlayChannel(-1, gScratch, 0);
			break;
		case SDLK_RIGHT:
			mVelX -= DOT_VEL;
			Mix_PlayChannel(-1, gScratch, 0);
			break;
		}
	}
}
void Dot::move(SDL_Rect &wall)
{
	static int r = 0;
	checklife = 0;
	if (r == 0)
	{
		//Move the dot left or right
		mPosX += mVelX;
		mCollider.x = mPosX;

		//Move the dot up or down
		mPosY += mVelY;
		mCollider.y = mPosY;

	}
	//If the dot collided or went too far to the left or right
	if (r < 5)
	{
		if ((mPosX < 0) || (mPosX + DOT_WIDTH > SCREEN_WIDTH))
		{
			//Move back
			mPosX -= mVelX;
			mCollider.x = mPosX;
		}

		//If the dot collided or went too far up or down
		if ((mPosY < 0) || (mPosY + DOT_HEIGHT > SCREEN_HEIGHT))
		{
			//Move back
			mPosY -= mVelY;
			mCollider.y = mPosY;
		}
		if (checkCollision(mCollider, wall) && r != noncollied)
		{
			SDL_Delay(1000);
			life--;
			mPosX = 0;
			mPosY = 200;
			wall.h = 0;
			wall.w = 0;
			noncollied = r;
		}
	}

	else if (r == 5)
	{
		if (checkCollision(mCollider, wall))
		{
			gfoodyClips[0].w = 0;
			gfoodyClips[0].h = 0;
			score += 500;
		}
	}
	else if (r == 6)
	{
		if (checkCollision(mCollider, wall))
		{
			gSpriteClips[0].w = 0;
			gSpriteClips[0].h = 0;
			checklife++;
		}
	}
	r++;
	if (r == 7)r = 0;
}

void Dot::render()
{
	//Show the dot
	gDotTexture.render(mPosX, mPosY);
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//Create window
		gWindow = SDL_CreateWindow("Save The Ratto", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create vsynced renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (gRenderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
					success = false;
				}

				if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
				{
					printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
					success = false;
				}
				if (TTF_Init() == -1)
				{
					printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
					success = false;
				}
			}
		}
	}

	return success;
}

//Mouse button sprites
LTexture gButtonSpriteSheetTexture;

//Buttons objects
LButton gButtons[TOTAL_BUTTONS];
LButton::LButton()
{
	mPosition.x = 0;
	mPosition.y = 0;

	mCurrentSprite = BUTTON_SPRITE_MOUSE_OUT;
}

void LButton::setPosition(int x, int y)
{
	mPosition.x = x;
	mPosition.y = y;
}

void LButton::render()
{
	//Show current button sprite
	gButtonSpriteSheetTexture.render(mPosition.x, mPosition.y, &gSpriteClips[mCurrentSprite]);
}

int gamestate;

void LButton::handleEvent(SDL_Event *m)
{

	if (m->type == SDL_MOUSEMOTION || m->type == SDL_MOUSEBUTTONDOWN || m->type == SDL_MOUSEBUTTONUP)
	{
		//Get mouse position
		int x, y;
		SDL_GetMouseState(&x, &y);

		//Check if mouse is in button
		bool inside = true;

		//Mouse is left of the button
		if (x < mPosition.x)
		{
			inside = false;
		}
		//Mouse is right of the button
		else if (x > mPosition.x + BUTTON_WIDTH)
		{
			inside = false;
		}
		//Mouse above the button
		else if (y < mPosition.y)
		{
			inside = false;
		}
		//Mouse below the button
		else if (y > mPosition.y + BUTTON_HEIGHT)
		{
			inside = false;
		}
		//Mouse is outside button
		if (!inside)
		{
			mCurrentSprite = BUTTON_SPRITE_MOUSE_OUT;
		}
		else
		{
			if (x >= 50 && x <= 200)
			{
				gamestate = 1;
			}
		}
	}
}
int kol;

int loadmenu()
{
	bool success = true;
	//Load background texture
	SDL_Surface *image = SDL_LoadBMP("BG.bmp");
	SDL_Texture *texture = SDL_CreateTextureFromSurface(gRenderer, image);

	bool menuRun = true;

	while (menuRun)
	{
		SDL_RenderCopy(gRenderer, texture, NULL, NULL);
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				return 0;
			if (e.type == SDL_MOUSEBUTTONDOWN)
			{
				int x, y;
				SDL_GetMouseState(&x, &y);

				if (x > 25 && x < 160 && y > 370 && y < 415)
				{
				    kol=1;
					return 1;
				}

				else if (x > 200 && x < 325 && y > 370 && y < 415)
				{
					return 2;
				}

				else if (x > 365 && x < 480 && y > 370 && y < 415)
				{
					return 3;
				}

				else if (x > 535 && x < 660 && y > 370 && y < 415)
				{
					return 0;
				}
			}

			if (e.type == SDL_KEYDOWN)
			{
				menuRun = false;
			}
		}
		SDL_RenderPresent(gRenderer);
	}

	return success;
}

bool loadMedia()
{
	//Loading success flag
	bool success = true;

	//Open the font
	gFont = TTF_OpenFont("lazy.ttf", 34);
	if (gFont == NULL)
	{
		printf("Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError());
		success = false;
	}
	else
	{
		//Set text color
		SDL_Color textColor = {176, 48, 96, 255};

		//Load prompt texture
		if (!gPromptTextTexture.loadFromRenderedText(" ", textColor))
		{
			printf("Unable to render prompt texture!\n");
			success = false;
		}
	}

	//Load background texture
	if (!gBGTexture.loadFromFile("background.png"))
	{
		printf("Failed to load background texture!\n");
		success = false;
	}
	//Load dot texture
	if (!gDotTexture.loadFromFile("ourexactplan.png"))
	{
		printf("Failed to load dot texture!\n");
		success = false;
	}

	//Load sprite sheet texture
	if (!gSpriteSheetTexture.loadFromFile("love2.png"))
	{
		printf("Failed to load love texture!\n");
		success = false;
	}
	else
	{
		//Set sprite clips

		gSpriteClips[0].x = 0;
		gSpriteClips[0].y = 0;
		gSpriteClips[0].w = 20;
		gSpriteClips[0].h = 20;
	}

	if (!gfoodySheetTexture.loadFromFile("kkk.bmp"))
	{
		printf("Failed to load food texture!\n");
		success = false;
	}
	else
	{
		//Set sprite clips

		gfoodyClips[0].x = 0;
		gfoodyClips[0].y = 0;
		gfoodyClips[0].w = 35;
		gfoodyClips[0].h = 35;
	}

	gScratch = Mix_LoadWAV("scratch.wav");
	if (gScratch == NULL)
	{
		printf("Failed to load scratch sound effect! SDL_mixer Error: %s\n", Mix_GetError());
		success = false;
	}
	return success;
}

void loadhighscore()
{

	SDL_Surface *image = SDL_LoadBMP("t.bmp");
	SDL_Texture *texture = SDL_CreateTextureFromSurface(gRenderer, image);
	SDL_RenderCopy(gRenderer, texture, NULL, NULL);

	FILE *fPtr;
	long long int a[3];
	char p[10000];
	int i = 0;
	fPtr = fopen("highscore.txt", "r");

	while (fscanf(fPtr, "%lld", &a[i]) == 1)
	{
		i++;
		if (i == 3)
			break;
	}
	fclose(fPtr);

	sprintf(p,"%lld  %lld  %lld",a[0],a[1],a[2]);
	int ll=strlen(p);
	p[ll-1]='\0';

	     SDL_Color textColor = {176, 48, 96, 255};
         timeText.str(" ");

		timeText << "" << p;
		if (!gTimeTextTexture.loadFromRenderedText(timeText.str().c_str(), textColor))
		{
			printf("Unable to render time texture!\n");
		}
		if (!gPromptTextTexture.loadFromRenderedText("Top Three Highscores", textColor))
		{
			printf("Unable to render prompt texture!\n");
		}
		gPromptTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 2, 80);
		gTimeTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 1.8, (SCREEN_HEIGHT - gPromptTextTexture.getHeight()) / 2.9);


	SDL_RenderPresent(gRenderer);

	SDL_Delay(6000);
	return;
}

void loadscoreboard()
{

	SDL_Delay(1000);
	SDL_Surface *image = SDL_LoadBMP("now.bmp");
	SDL_Texture *texture = SDL_CreateTextureFromSurface(gRenderer, image);
	SDL_RenderCopy(gRenderer, texture, NULL, NULL);
	/* File pointer to hold reference to our file */
	FILE *fPtr;
	long long int a[3], temp;
	char p[10000];
	int i = 0;
	fPtr = fopen("highscore.txt", "r");

	while (fscanf(fPtr, "%lld", &a[i]) == 1)
	{
		i++;
		if (i == 3)
			break;
	}
	if (score >= a[0])
	{
		a[1] = a[0];
		a[2] = a[1];
		a[0] = score;
	}
	else if (score >= a[1])
	{
		a[2] = a[1];
		a[1] = score;
	}
	else if (score >= a[2])
	{
		a[2] = score;
	}

	FILE *fPtw = fopen("highscore.txt", "w");
	fprintf(fPtw, "%lld\n%lld\n%lld\n", a[0], a[1], a[2]);


	fclose(fPtw);
	fclose(fPtr);


	SDL_Color textColor = {199, 66, 219, 255};

	if (score >= a[0])
	{
		timeText.str(" ");

		timeText << "Your Position:" << 1;
		if (!gTimeTextTexture.loadFromRenderedText(timeText.str().c_str(), textColor))
		{
			printf("Unable to render time texture!\n");
		}
		if (!gPromptTextTexture.loadFromRenderedText("Congratulations!!!", textColor))
		{
			printf("Unable to render prompt texture!\n");
		}
		gPromptTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 6, 280);
		gTimeTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 6, (SCREEN_HEIGHT - gPromptTextTexture.getHeight()) / 2.3);
	}
	else if (score >= a[1])
	{
		timeText.str(" ");

		timeText << "Your Position:" << 2;
		if (!gTimeTextTexture.loadFromRenderedText(timeText.str().c_str(), textColor))
		{
			printf("Unable to render time texture!\n");
		}
		if (!gPromptTextTexture.loadFromRenderedText("Wow!!!", textColor))
		{
			printf("Unable to render prompt texture!\n");
		}
		gPromptTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 6, 280);
		gTimeTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 6, (SCREEN_HEIGHT - gPromptTextTexture.getHeight()) / 2.3);
	}
	else if (score >= a[2])
	{
		timeText.str(" ");

		timeText << "Your Position:" << 3;
		if (!gTimeTextTexture.loadFromRenderedText(timeText.str().c_str(), textColor))
		{
			printf("Unable to render time texture!\n");
		}
		if (!gPromptTextTexture.loadFromRenderedText("Nice!!!", textColor))
		{
			printf("Unable to render prompt texture!\n");
		}
		gPromptTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 6, 280);
		gTimeTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 6, (SCREEN_HEIGHT - gPromptTextTexture.getHeight()) / 2.3);
	}
	else
	{
		timeText.str(" ");
		timeText << "Your Position:" << 0;

		if (!gTimeTextTexture.loadFromRenderedText(timeText.str().c_str(), textColor))
		{
			printf("Unable to render time texture!\n");
		}
		if (!gPromptTextTexture.loadFromRenderedText("Better luck next time!", textColor))
		{
			printf("Unable to render prompt texture!\n");
		}
		gTimeTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 6, (SCREEN_HEIGHT - gPromptTextTexture.getHeight()) / 2.3);
		gPromptTextTexture.render((SCREEN_WIDTH - gPromptTextTexture.getWidth()) / 6, 250);
	}


	SDL_RenderPresent(gRenderer);
	SDL_Delay(3000);
	return;
}

void loadinstructions(){
    SDL_Surface *image = SDL_LoadBMP("er.bmp");
	SDL_Texture *texture = SDL_CreateTextureFromSurface(gRenderer, image);
	SDL_RenderCopy(gRenderer, texture, NULL, NULL);
	SDL_RenderPresent(gRenderer);
	SDL_Delay(10000);
	return;
}

void close()
{
	//Free loaded images
	gDotTexture.free();
	gBGTexture.free();
	gSpriteSheetTexture.free();
	gfoodySheetTexture.free();

	Mix_FreeChunk(gScratch);
	gScratch = NULL;
	Mix_FreeMusic(gMusic);
	gMusic = NULL;

	//Free global font
	TTF_CloseFont(gFont);
	gFont = NULL;

	//Destroy window
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	Mix_Quit();
	IMG_Quit();
	SDL_Quit();
	TTF_Quit();
}

int main(int argc, char *args[])
{

	if (!init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//Load media
		if (!loadMedia())
		{
			printf("Failed to load media!\n");
		}


        else{
			int x = 5;
			while (x)
			{
			    SDL_Event m;
			    kol=0;
				x = loadmenu();

				if (x == 1 && kol==1)
				{

                            SDL_Event e;
                            //Set text color as black
                            SDL_Color textColor = {176, 48, 96, 255};

                            //The dot that will be moving around on the screen
                            Dot dot;

                            double scrollingOffset = 0;
                            int scroll = 0, changecolori[6], changecolorj[6], changecolork[6];
                            double loveWidth = 900, loveHeight = 350;
                            double foodWidth = SCREEN_WIDTH + 350, foodHeight = 250;

                            memset(changecolori, 0, sizeof(changecolori));
                            memset(changecolorj, 0, sizeof(changecolorj));
                            memset(changecolork, 0, sizeof(changecolork));
                            double scrollspeed = 2;
                            wall[0].x = SCREEN_WIDTH + 20;
                            wall[0].y = 0;
                            wall[0].w = 40;
                            wall[0].h = 170;

                            wall[1].x = SCREEN_WIDTH + 220;
                            wall[1].w = 35;
                            wall[1].h = 130;
                            wall[1].y = SCREEN_HEIGHT - wall[1].h;

                            wall[2].x = SCREEN_WIDTH + 430;
                            wall[2].w = 50;
                            wall[2].h = 200;
                            wall[2].y = SCREEN_HEIGHT - wall[2].h;

                            wall[3].x = SCREEN_WIDTH + 590;
                            wall[3].y = 60;
                            wall[3].w = 27;
                            wall[3].h = 60;

                            wall[4].x = SCREEN_WIDTH + 300;
                            wall[4].y = 0;
                            wall[4].w = 50;
                            wall[4].h = 140;


                            int liferecheck = 0;
                            int frame = 0;
                            dot.mPosX = dot.mPosY = dot.mVelX = dot.mVelY = 0;
                            score = 0;
                            life = 0, checklife = 0, noncollied = -1;
                            quit=false;
                            while (!quit)
                            {

                                while (SDL_PollEvent(&e))
                                {
                                    if (e.type == SDL_QUIT)
                                    {
                                        quit = true;
                                    }
                                    dot.handleEvent(e);
                                }

                                dot.move(wall[0]);
                                dot.move(wall[1]);
                                dot.move(wall[2]);
                                dot.move(wall[3]);
                                dot.move(wall[4]);
                                dot.move(food[0]);
                                dot.move(love[0]);
                                if (checklife > 0 && liferecheck == 0)
                                {
                                    life++;
                                    liferecheck = 1;
                                }

                                if (life < 0)
                                {
                                    loadscoreboard();
                                    quit = true;
                                    x=5;
                                    break;
                                }


                                //Scroll background
                                scrollingOffset -= scrollspeed;
                                wall[0].x -= scrollspeed;
                                wall[1].x -= scrollspeed;
                                wall[2].x -= scrollspeed;
                                wall[3].x -= scrollspeed;
                                wall[4].x -= scrollspeed;

                                foodWidth -= scrollspeed;
                                loveWidth -= scrollspeed;
                                if (loveWidth <= -20)
                                {
                                    loveWidth = 1800;
                                    loveHeight += 100;
                                    gSpriteClips[0].w = 20;
                                    gSpriteClips[0].h = 20;
                                    if (loveHeight >= 390)
                                        loveHeight = 80;
                                }

                                if (foodWidth <= -30)
                                {
                                    foodWidth = 1200;
                                    if (foodHeight >= 80)
                                        foodHeight -= 30;
                                    else
                                        foodHeight = 350;
                                    gfoodyClips[0].w = 35;
                                    gfoodyClips[0].h = 35;
                                }
                                if (scrollingOffset < -gBGTexture.getWidth())
                                {

                                    scrollingOffset = 0;
                                    scroll++;
                                    liferecheck = 0;
                                }

                                for (int i = 0; i <= 4; i++)
                                {

                                    if (wall[i].x <= -70)
                                    {
                                        if (i == noncollied)
                                            noncollied = -1;
                                        if (i != 5)
                                            wall[i].x = SCREEN_WIDTH;
                                        else
                                        {
                                            wall[i].x = SCREEN_WIDTH + 1400;
                                            wall[i].h = 14;
                                            wall[i].w = 14;
                                        }
                                        if (i == 3)
                                        {
                                            wall[i].y -= 60;
                                            if (wall[i].w < 70)
                                                wall[i].w += 3;
                                            else
                                                wall[i].w = 40;
                                            if (wall[i].h > 60)
                                                wall[i].h -= 40;
                                            else
                                                wall[i].h = 150;
                                            if (wall[3].y <= 0)
                                                wall[3].y = 250;
                                        }

                                        if (i % 2 == 0)
                                        {
                                            if (wall[i].w < 70)
                                                wall[i].w += 3;
                                            else
                                                wall[i].w = 30;
                                            if (wall[i].h > 60)
                                                wall[i].h -= 40;
                                            else
                                                wall[i].h = 200;
                                            if (i == 2)
                                                wall[i].y = SCREEN_HEIGHT - wall[i].h;
                                        }
                                        else if (i == 1)
                                        {
                                            if (wall[i].w < 60)
                                                wall[i].w += 3;
                                            else
                                                wall[i].w = 40;
                                            if (wall[i].h > 60)
                                                wall[i].h -= 40;
                                            else
                                                wall[i].h = 200;
                                            wall[i].y = SCREEN_HEIGHT - wall[i].h;
                                        }
                                        else if (i == 5)
                                        {
                                            if (wall[i].y >= 60)
                                                wall[i].y -= 50;
                                            else
                                                wall[i].y = 350;
                                        }
                                        int j = wall[i].h - wall[i + 1].h;
                                        if (j < 0)
                                            j = -j;
                                        if (j <= 70)
                                        {
                                            if (i == 2)
                                            {
                                                wall[i].h = 130;
                                                wall[i].y = SCREEN_HEIGHT - wall[i].h;
                                            }
                                            else if (i != 5)
                                            {
                                                wall[i].h = 130;
                                            }
                                        }
                                        else if (j > 100)
                                        {
                                            if (i == 2 || i == 1)
                                            {
                                                wall[i].h = 130;
                                                wall[i].y = SCREEN_HEIGHT - wall[i].h;
                                            }
                                            else if (i != 3 && i != 5)
                                            {
                                                wall[i].h = 130;
                                            }
                                        }
                                        changecolori[i] += 60;
                                        changecolorj[i] += 80;
                                        changecolork[i] += 50;
                                        if (changecolori[i] >= 400)
                                        {
                                            changecolori[i] = 0;
                                            changecolorj[i] = 0;
                                            changecolork[i] = 0;
                                        }
                                    }

                                }
                                SDL_SetRenderDrawColor(gRenderer, 190 + changecolori[0], 107 + changecolorj[0], 191 + changecolork[0], 255);

                                gBGTexture.render(scrollingOffset, 0);
                                gBGTexture.render(scrollingOffset + gBGTexture.getWidth(), 0);
                                dot.render();

                                SDL_RenderDrawRect(gRenderer, &wall[0]);
                                SDL_RenderFillRect(gRenderer, &wall[0]);

                                SDL_SetRenderDrawColor(gRenderer, 90 + changecolori[1], 17 + changecolorj[1], 91 + changecolork[1], 255);
                                SDL_RenderDrawRect(gRenderer, &wall[1]);
                                SDL_RenderFillRect(gRenderer, &wall[1]);

                                SDL_SetRenderDrawColor(gRenderer, 10 + changecolori[2], 10 + changecolorj[2], 19 + changecolork[2], 255);
                                SDL_RenderDrawRect(gRenderer, &wall[2]);
                                SDL_RenderFillRect(gRenderer, &wall[2]);

                                SDL_SetRenderDrawColor(gRenderer, 40 + changecolori[3], 57 + changecolorj[3], 40 + changecolork[2], 255);
                                SDL_RenderDrawRect(gRenderer, &wall[3]);
                                SDL_RenderFillRect(gRenderer, &wall[3]);

                                SDL_SetRenderDrawColor(gRenderer, 90 + changecolori[4], 10 + changecolorj[4], 19 + changecolork[4], 255);
                                SDL_RenderDrawRect(gRenderer, &wall[4]);
                                SDL_RenderFillRect(gRenderer, &wall[4]);

                                SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255);
                                SDL_RenderDrawRect(gRenderer, &wall[5]);
                                SDL_RenderFillRect(gRenderer, &wall[5]);

                                //Render textures
                                gTimeTextTexture.render(200, (SCREEN_HEIGHT - gPromptTextTexture.getHeight()) / 20);

                                SDL_Rect *currentClip = &gSpriteClips[0];
                                gSpriteSheetTexture.render(loveWidth, 60 + loveHeight, currentClip);

                                SDL_Rect *idontknowClip = &gfoodyClips[0];
                                gfoodySheetTexture.render(foodWidth, foodHeight, idontknowClip);

                                SDL_RenderPresent(gRenderer);

                                for (int k = 1; k < scrollspeed; k++)
                                {
                                    score++;
                                    timeText.str("");
                                    timeText << "Score:" << score;
                                    timeText << " Life:" << life;
                                }
                                if (!gTimeTextTexture.loadFromRenderedText(timeText.str().c_str(), textColor))
                                {
                                    printf("Unable to render time texture!\n");
                                }

                                love[0].x = loveWidth;
                                love[0].y = loveHeight + 60;
                                love[0].w = gSpriteClips[0].w;
                                love[0].h = gSpriteClips[0].h;

                                food[0].x = foodWidth;
                                food[0].y = foodHeight;
                                food[0].w = gfoodyClips[0].w;
                                food[0].h = gfoodyClips[0].h;

                                scrollspeed += 0.005;
                            }

                }
				if (x == 2)
				{
					loadinstructions();
				}

				if (x == 3)
				{
					loadhighscore();
				}

		}
		}

		//Free resources and close SDL
		close();

		return 0;
	}
}
