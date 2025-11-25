

#include "Framework.h"
#include "ParseCLArguments.h"
#include <time.h>


//Player movement speed multiplier
#define PLAYER_MOVEMENT_SPEED_FACTOR 0.3f

//Enemy movement speed multiplier
#define ENEMY_MOVEMENT_SPEED_FACTOR 0.05f

//Bullet movement speed multiplier
#define BULLET_MOVEMENT_SPEED_FACTOR 0.2f

//Lesser this number is, more foliage is spawned
#define FOLIAGE_DENSITY_DENOMINATOR 6000

//Enemies dont spawn in this threshold around the player, 
//it is NOT radius, it is box`s halfsize
#define ENEMY_SPAWN_THRESHOLD 100

#define USE_DEBUG_PNG 0

#if  USE_DEBUG_PNG
	#define ENEMY_PNG "data/enemy_debug.png" 
#else
	#define ENEMY_PNG "data/enemy.png" 
#endif


//----------GLOBALS----------
// gameplay globals, can be set from command line
int windowWidth, windowHeight;
int mapWidth, mapHeight;
int numEnemies, numAmmo;

// other gameplay vars
unsigned int DeltaTime, TimeSinceLastTick;
int numTilesX, numTilesY, numTotalTiles, MapTileSize;
int worldOffsetX, worldOffsetY, worldDeltaX, worldDeltaY;
int mouseCursorX, mouseCursorY, mouseCursorCentreX, mouseCursorCentreY;
int playerX, playerY, playerCentreX, playerCentreY;
int playerRadius, playerDiameter, enemyRadius, enemyDiameter, bulletRadius, mouseCursorRadius;
int mapLeftBorder, mapRightBorder, mapTopBorder, mapBottomBorder;
int oldestBulletId;
int spawnThresholdUpBorder, spawnThresholdBottomBorder, spawnThresholdLeftBorder, spawnThresholdRightBorder;
int foliageCount, foliageRadius, foliageDiameter;

// key states
bool bLeftKeyDown;
bool bRightKeyDown;
bool bUpKeyDown;
bool bDownKeyDown;

bool bShowCursor;
bool bRestart;

struct Enemy
{
	float posX, posY;
	float normDeltaX, normDeltaY;
	bool bIsAlive;
};

struct Bullet
{
	float posX, posY;
	float normDeltaX, normDeltaY;
	int id;
	bool bIsAlive;
};

struct Foliage
{
	int posX, posY;
	int foliageId;
};

//Movable actors
Enemy*	enemies;
Bullet* bullets;
Foliage* foliage;

//Sprites
Sprite* playerSprite;
Sprite* enemySprite;
Sprite* mouseCursorSprite;
Sprite* bulletSprite;

Sprite* grass_tile_01;
Sprite* grass_tile_02;
Sprite* map_border_lr;
Sprite* map_border_tb;
Sprite* map_corner;
Sprite* plant_01;
Sprite* plant_02;
Sprite* plant_03;

int* mapTileId;

//Functions
int GetRandomInRange(const int& left, const int& mid);
void ComputeMaxEnemyCountToSpawn();
bool DoEnemiesCollide(const int& index, const float& potentialX, const float& potentialY);
void SpawnGivenEnemyAtValidLocation(const int& index);
void ComputeEnemiesNormDeltas(const int& index);
void UpdateEnemiesWorldPositions();
void UpdateEnemiesNormDeltas();
void AdvanceEnemies();
void ResolveEnemyToBulletCollisions();
void ResolveEnemyToPlayerCollisions();
void SpawnBullet();
void UpdateBulletsWorldPositions();
void AdvanceBullets();
void UpdateWorldOffset();
void InitFoliagePositions();
void UpdateFoliageWorldPositions();
void DrawMap();

/* Test Framework realization */
class MyFramework : public Framework {

public:

	virtual void PreInit(int& width, int& height, bool& fullscreen)
	{
		width = windowWidth;
		height = windowHeight;
		fullscreen = false;
	}

	virtual bool Init()
	{
		//Set all entites radiuses and diameters
		playerRadius = 32;
		playerDiameter = playerRadius *2;
		enemyRadius = 17;
		enemyDiameter = enemyRadius * 2;
		bulletRadius = 6;
		mouseCursorRadius = 16;
		foliageRadius = 16;
		foliageDiameter = foliageRadius * 2;

		//Various other vars
		MapTileSize = 64;
		oldestBulletId = 0;
		worldOffsetX = worldOffsetY = worldDeltaX = worldDeltaY = 0;

		//Init player properties
		playerX = windowWidth / 2 - playerRadius;
		playerY = windowHeight / 2 - playerRadius;
		playerCentreX = windowWidth / 2;
		playerCentreY = windowHeight / 2;

		//Set all keys to be up by default
		bLeftKeyDown = false;
		bRightKeyDown = false;
		bUpKeyDown = false;
		bDownKeyDown = false;

		bShowCursor = true; 
		bRestart = false;		

		//Init map borders
		mapLeftBorder	= (windowWidth - mapWidth) / 2;
		mapRightBorder	= windowWidth - (windowWidth - mapWidth) / 2;
		mapTopBorder	= (windowHeight - mapHeight) / 2;
		mapBottomBorder = windowHeight - (windowHeight - mapHeight) / 2;

		//Init enemy spawn borders, 
		//coordinates of sides of a cube inside which enemies newer spawn
		spawnThresholdLeftBorder	= playerCentreX - ENEMY_SPAWN_THRESHOLD - enemyDiameter;
		spawnThresholdRightBorder	= playerCentreX + ENEMY_SPAWN_THRESHOLD;
		spawnThresholdUpBorder		= playerCentreY - ENEMY_SPAWN_THRESHOLD - enemyDiameter;
		spawnThresholdBottomBorder	= playerCentreY + ENEMY_SPAWN_THRESHOLD;

		//Compute how many "whole" tiles fit into maps width and height
		//it`s assumed that all tiles are square and same in size
		numTilesX = (mapWidth / MapTileSize) + 1;
		numTilesY = (mapHeight / MapTileSize) + 1;

		//Init map tile Id`s
		numTotalTiles = numTilesX * numTilesY;
		mapTileId = new int[numTotalTiles];

		for (int i = 0; i < (numTotalTiles - 1); i++) mapTileId[i] = rand() % 50;

		InitFoliagePositions();		

		//Initializing player sprite
		playerSprite = createSprite("data/avatar.jpg");

		//Initializing mouse cursor 
		mouseCursorSprite = createSprite("data/circle.tga");
		mouseCursorX = 0;
		mouseCursorY = 0;
		
		//Initializing bullets
		bullets = new Bullet[numAmmo];
		bulletSprite = createSprite("data/bullet2.png");
		for (int i = 0; i < numAmmo; i++) bullets[i].bIsAlive = false;
		
		//Compute max enemies to spawn
		ComputeMaxEnemyCountToSpawn();

		//Initializing all enemies positions
		enemies = new Enemy[numEnemies];
		enemySprite = createSprite(ENEMY_PNG);
		for (int i = 0; i < numEnemies; i++)
		{
			enemies[i].bIsAlive = true;
			enemies[i].posX = playerCentreX;
			enemies[i].posY = playerCentreY;
		}

		//Give all enemies valid spawn locations 
		//and give them vectors pointed to player
		for (int i = 0; i < numEnemies; i++)
		{
			SpawnGivenEnemyAtValidLocation(i);
			ComputeEnemiesNormDeltas(i);
		}
		
		//Load map tiles
		grass_tile_01	= createSprite("data/grass_tile_01.png");
		grass_tile_02	= createSprite("data/grass_tile_02.png");
		map_border_lr	= createSprite("data/map_border_lr.jpg");
		map_border_tb	= createSprite("data/map_border_tb.jpg");
		map_corner		= createSprite("data/map_corner.jpg");

		plant_01 = createSprite("data/plant_01.png");
		plant_02 = createSprite("data/plant_02.png");
		plant_03 = createSprite("data/plant_03.png");

		showCursor(bShowCursor);
		return true;
	}

	virtual void Close()
	{
		delete enemies;
		enemies = nullptr;	

		delete bullets;
		bullets = nullptr;

		delete foliage;
		foliage = nullptr;

		destroySprite(playerSprite);
		destroySprite(enemySprite);
		destroySprite(mouseCursorSprite);
		destroySprite(bulletSprite);
		
		destroySprite(grass_tile_01);
		destroySprite(grass_tile_02);
		destroySprite(map_border_lr);
		destroySprite(map_border_tb);		
		destroySprite(map_corner);
		destroySprite(plant_01);
		destroySprite(plant_02);
		destroySprite(plant_03);
	}

	virtual bool Tick()
	{
		//Compute time passed since last tick
		DeltaTime = getTickCount() - TimeSinceLastTick;
		TimeSinceLastTick = getTickCount();

		if (bRestart) Init();

		drawTestBackground();		

		UpdateWorldOffset();

		//Update map borders
		mapLeftBorder	-= worldDeltaX;
		mapRightBorder	-= worldDeltaX;
		mapTopBorder	-= worldDeltaY;
		mapBottomBorder -= worldDeltaY;		

		//Draw map
		UpdateFoliageWorldPositions();
		DrawMap();

		//Draw player, we are always drawing 
		//player in centre of the map
		drawSprite(playerSprite, playerX, playerY);
		
		//Update all enemies positions/advance enemies/resolve 
		//enemy to bullet and enemy to player collisions
		UpdateEnemiesWorldPositions();
		UpdateEnemiesNormDeltas();
		AdvanceEnemies();
		ResolveEnemyToBulletCollisions();
		ResolveEnemyToPlayerCollisions();
		//Draw all enemies
		for (int i = 0; i < numEnemies; i++)
		{
			if (enemies[i].bIsAlive == true)
			drawSprite(enemySprite, enemies[i].posX, enemies[i].posY);
		}

		//Draw cursor
		drawSprite(mouseCursorSprite, mouseCursorX, mouseCursorY);

		//Move all bullets
		UpdateBulletsWorldPositions();
		AdvanceBullets();
		//Draw all bullets
		for (int i = 0; i < numAmmo; i++)
		{
			if (bullets[i].bIsAlive == true)
			drawSprite(bulletSprite, bullets[i].posX, bullets[i].posY);
		}

		return false;
	}

	virtual void onMouseMove(int x, int y, int xrelative, int yrelative)
	{
		mouseCursorCentreX = x;
		mouseCursorCentreY = y;
		mouseCursorX = x - mouseCursorRadius;
		mouseCursorY = y - mouseCursorRadius;
	}

	virtual void onMouseButtonClick(FRMouseButton button, bool isReleased)
	{
		if ((button == FRMouseButton::LEFT) && !isReleased)
		{
			SpawnBullet();
		}

		if ((button == FRMouseButton::MIDDLE) && !isReleased)
		{
			bRestart = true;
		}
	}

	virtual void onKeyPressed(FRKey k)
	{
		if (k == FRKey::LEFT)	bLeftKeyDown = true;
		if (k == FRKey::RIGHT)	bRightKeyDown = true;
		if (k == FRKey::UP)		bUpKeyDown = true;
		if (k == FRKey::DOWN)	bDownKeyDown = true;
	}

	virtual void onKeyReleased(FRKey k)
	{

		if (k == FRKey::LEFT)	bLeftKeyDown = false;
		if (k == FRKey::RIGHT)	bRightKeyDown = false;
		if (k == FRKey::UP)		bUpKeyDown = false;
		if (k == FRKey::DOWN)	bDownKeyDown = false;
	}
};

int main(int argc, char* argv[])
{
	//Setting default values, will be overridden 
	//by values from command line, if any available
	windowWidth = 1024;
	windowHeight = 768;
	mapWidth = 700;
	mapHeight = 700;
	numEnemies = 10;
	numAmmo = 3;

	//Parse command line arguments and write found values to global variables
	TestGame::ParseCLArguments(argc, argv);
	
	srand(time(NULL));	

	return run(new MyFramework);
} // main()

inline int GetRandomInRange(const int& left, const int& mid)
{
	return ((rand() % mid) + left);
} // GetRandomInRange()

void ComputeMaxEnemyCountToSpawn()
{
	int availableMapArea = mapWidth * mapHeight - ((ENEMY_SPAWN_THRESHOLD * 2) * (ENEMY_SPAWN_THRESHOLD * 2));
	
	// Subtract 10 form enemydiameter bicause on enemy
	// image neary 5 pixels from both sides are empty
	int enemyArea = (enemyDiameter - 10) * (enemyDiameter - 10);
	
	// MaxEnemiesToSpawn is 58% of all available enemies
	int MaxEnemiesToSpawn = (availableMapArea / enemyArea) * 0.58;

	if (numEnemies > MaxEnemiesToSpawn) numEnemies = MaxEnemiesToSpawn;
	if (numEnemies < 0) numEnemies = 0;
}

bool DoEnemiesCollide(const int& index, const float& potentialX, const float& potentialY)
{
	// subtract small number from enemyDiameter so that, when ever enemies pack together,
	// they stick a little closer to each other, cause this way it looks better
	int distanceToCollide = enemyDiameter -10;
	float curEnLeftX, curEnRightX, curEnTopY, curEnBotY;

	curEnLeftX = potentialX;
	curEnRightX = curEnLeftX + enemyRadius * 2;
	curEnTopY = potentialY;
	curEnBotY = curEnTopY + enemyRadius * 2;

	for (int j = 0; j < numEnemies; j++)
	{
		if (j == index) continue;

		float otherEnX = enemies[j].posX;
		float otherEnY = enemies[j].posY;

		if (((curEnLeftX >= (otherEnX + enemyDiameter)) || (curEnRightX <= otherEnX)) &&
			((curEnTopY >= (otherEnY + enemyDiameter)) || (curEnBotY <= otherEnY))) continue;

		float dx = curEnLeftX - otherEnX;
		float dy = curEnTopY - otherEnY;

		double distance = 0;
		distance = sqrt(dx * dx + dy * dy);

		if (distance < distanceToCollide) return true;
	}
	return false;
} // DoEnemiesCollide()

void SpawnGivenEnemyAtValidLocation(const int& index)
{
	bool foundXY = false;
	float potentialX = 0;
	float potentialY = 0;

	int difX = mapRightBorder - mapLeftBorder - enemyDiameter + 1;
	int difY = mapBottomBorder - mapTopBorder - enemyDiameter + 1;

	while (!foundXY)
	{
		potentialY = GetRandomInRange(mapTopBorder, difY);

		if ((potentialY <= spawnThresholdUpBorder) || (potentialY >= spawnThresholdBottomBorder))
		{
			potentialX = GetRandomInRange(mapLeftBorder, difX);
		}
		else
		{
			potentialX = GetRandomInRange(mapLeftBorder, difX);

			if ((potentialX <= (playerCentreX - enemyRadius)) && (potentialX > spawnThresholdLeftBorder))
			{
				continue;
			}
			else if ((potentialX > (playerCentreX - enemyRadius)) && (potentialX <= spawnThresholdRightBorder))
			{
				continue;
			}
		}

		foundXY = !(DoEnemiesCollide(index, potentialX, potentialY));
	}

	enemies[index].posX = potentialX;
	enemies[index].posY = potentialY;

} //  InitGivenEnemyLocation()

void ComputeEnemiesNormDeltas(const int& index)
{
	float enemyCentreX = enemies[index].posX + enemyRadius;
	float enemyCentreY = enemies[index].posY + enemyRadius;

	float deltaX = playerCentreX - enemyCentreX;
	float deltaY = playerCentreY - enemyCentreY;

	double length = sqrt(deltaX * deltaX + deltaY * deltaY);

	enemies[index].normDeltaX = deltaX / length;
	enemies[index].normDeltaY = deltaY / length;

} // ComputeEnemiesNormDeltas()

void UpdateEnemiesWorldPositions()
{
	for (int i = 0; i < numEnemies; i++)
	{
		if (enemies[i].bIsAlive == true)
		{
			enemies[i].posX -= worldDeltaX;
			enemies[i].posY -= worldDeltaY;
		}
		else // if current enemy is dead, resurrect it at new valid location
		{
			enemies[i].bIsAlive = true;
			SpawnGivenEnemyAtValidLocation(i);
			ComputeEnemiesNormDeltas(i);
		}
	}

} // UpdateEnemiesPositions()

void UpdateEnemiesNormDeltas()
{
	if ((worldDeltaX == 0) && (worldDeltaY == 0)) return;

	for (int i = 0; i < numEnemies; i++) ComputeEnemiesNormDeltas(i);
} // UpdateEnemiesNormDeltas()

void AdvanceEnemies()
{
	for (int i = 0; i < numEnemies; i++)
	{

		float potentialX, potentialY;

		potentialX = enemies[i].posX + enemies[i].normDeltaX * ENEMY_MOVEMENT_SPEED_FACTOR * DeltaTime;
		potentialY = enemies[i].posY + enemies[i].normDeltaY * ENEMY_MOVEMENT_SPEED_FACTOR * DeltaTime;

		bool bWillCollide = false;
		bWillCollide = DoEnemiesCollide(i, potentialX, potentialY);

		if (bWillCollide)
		{
			// try move left
			float newPosX, newPosY, dxL, dyL, normDeltaX, normDeltaY, normDeltaX2, normDeltaY2;
			dxL = dyL = 0;
			normDeltaX = enemies[i].normDeltaX;
			normDeltaY = enemies[i].normDeltaY;
			normDeltaX2 = normDeltaX * normDeltaX;
			normDeltaY2 = normDeltaY * normDeltaY;

			dyL = (normDeltaX2 + normDeltaY2) / (1 + normDeltaY2 / normDeltaX2);
			dxL = -(normDeltaY * dyL / normDeltaX);

			newPosX = enemies[i].posX + dxL * ENEMY_MOVEMENT_SPEED_FACTOR;
			newPosY = enemies[i].posY + dyL * ENEMY_MOVEMENT_SPEED_FACTOR;

			potentialX = newPosX + enemies[i].normDeltaX * ENEMY_MOVEMENT_SPEED_FACTOR;
			potentialY = newPosY + enemies[i].normDeltaY * ENEMY_MOVEMENT_SPEED_FACTOR;

			bool bWillCollide = false;
			bWillCollide = DoEnemiesCollide(i, potentialX, potentialY);

			if (!bWillCollide)
			{
				enemies[i].posX = potentialX;
				enemies[i].posY = potentialY;

				continue;
			}

			// try move rigth
			newPosX = enemies[i].posX - dxL * ENEMY_MOVEMENT_SPEED_FACTOR;
			newPosY = enemies[i].posY - dyL * ENEMY_MOVEMENT_SPEED_FACTOR;

			potentialX = newPosX + enemies[i].normDeltaX * ENEMY_MOVEMENT_SPEED_FACTOR;
			potentialY = newPosY + enemies[i].normDeltaY * ENEMY_MOVEMENT_SPEED_FACTOR;

			bWillCollide = false;
			bWillCollide = DoEnemiesCollide(i, potentialX, potentialY);

			if (!bWillCollide)
			{
				enemies[i].posX = potentialX;
				enemies[i].posY = potentialY;
				continue;
			}
		}
		else
		{
			enemies[i].posX = potentialX;
			enemies[i].posY = potentialY;
		}
	}
} // AdvanceEnemies()

void ResolveEnemyToBulletCollisions()
{
	int distanceToCollide = enemyRadius + bulletRadius - 5;

	for (int i = 0; i < numAmmo; i++)
	{
		if (bullets[i].bIsAlive == true)
		{
			float bulletLeftX, bulletRightX, bulletTopY, bulletBotY;

			bulletLeftX = bullets[i].posX;
			bulletRightX = bulletLeftX + bulletRadius * 2;
			bulletTopY = bullets[i].posY;
			bulletBotY = bulletTopY + bulletRadius * 2;

			for (int j = 0; j < numEnemies; j++)
			{
				if (enemies[j].bIsAlive == false) continue;

				float enemyjX = enemies[j].posX;
				float enemyjY = enemies[j].posY;

				if ((bulletLeftX >= (enemyjX + enemyDiameter)) || (bulletRightX <= enemyjX) &&
					(bulletTopY >= (enemyjY + enemyDiameter)) || (bulletBotY <= enemyjY)) continue;

				float bulletCentreX, bulletCentreY, enemyCentreX, enemyCentreY;

				bulletCentreX = bulletLeftX + bulletRadius;
				bulletCentreY = bulletTopY + bulletRadius;

				enemyCentreX = enemyjX + enemyRadius;
				enemyCentreY = enemyjY + enemyRadius;

				float dx = bulletCentreX - enemyCentreX;
				float dy = bulletCentreY - enemyCentreY;

				double distance = 0;
				distance = sqrt(dx * dx + dy * dy);

				if (distance < distanceToCollide)
				{
					bullets[i].bIsAlive = false;
					enemies[j].bIsAlive = false;
					//DrawExplosionAtLocation();
				}
			}
		}
	}
} // ResolveEnemyToBulletCollisions()

void ResolveEnemyToPlayerCollisions()
{
	int distanceToCollide = enemyRadius + playerRadius - 5;
	float enemyLeftX, enemyRightX, enemyTopY, enemyBotY;

	for (int i = 0; i < numEnemies; i++)
	{
		enemyLeftX = enemies[i].posX;
		enemyRightX = enemyLeftX + enemyRadius * 2;
		enemyTopY = enemies[i].posY;
		enemyBotY = enemyTopY + enemyRadius * 2;

		if (enemies[i].bIsAlive == false) continue;

		if ((enemyLeftX >= (playerCentreX + playerRadius)) || (enemyRightX <= (playerCentreX - playerRadius)) &&
			(enemyTopY >= (playerCentreY + playerRadius)) || (enemyBotY <= (playerCentreY - playerRadius))) continue;

		float enemyCentreX, enemyCentreY;

		enemyCentreX = enemyLeftX + enemyRadius;
		enemyCentreY = enemyTopY + enemyRadius;

		float dx = playerCentreX - enemyCentreX;
		float dy = playerCentreY - enemyCentreY;

		double distance = 0;
		distance = sqrt(dx * dx + dy * dy);

		if (distance < distanceToCollide)
		{
			bRestart = true;
		}
	}
} // ResolveEnemyToPlayerCollisions()

void SpawnBullet()
{
	if (numAmmo == 0) return;
	for (int i = 0; i < numAmmo; i++)
	{
		if (bullets[i].bIsAlive == false)
		{
			bullets[i].bIsAlive = true;
			bullets[i].posX = playerX + playerRadius - bulletRadius;
			bullets[i].posY = playerY + playerRadius - bulletRadius;
			bullets[i].id = oldestBulletId++;

			int deltaX = playerCentreX - mouseCursorCentreX;
			int deltaY = playerCentreY - mouseCursorCentreY;

			double length = sqrt(deltaX * deltaX + deltaY * deltaY);

			bullets[i].normDeltaX = deltaX / length;
			bullets[i].normDeltaY = deltaY / length;
			return;
		}
	}

	int lowestId = oldestBulletId;
	int lowestIdIndex = 0;
	for (int i = 0; i < numAmmo; i++)
	{
		if (bullets[i].bIsAlive == true)
		{
			if (bullets[i].id < lowestId)
			{
				lowestId = bullets[i].id;
				lowestIdIndex = i;
			}
		}
	}

	bullets[lowestIdIndex].bIsAlive = true;
	bullets[lowestIdIndex].posX = playerX + playerRadius - bulletRadius;
	bullets[lowestIdIndex].posY = playerY + playerRadius - bulletRadius;
	bullets[lowestIdIndex].id = oldestBulletId++;

	int deltaX = playerCentreX - mouseCursorCentreX;
	int deltaY = playerCentreY - mouseCursorCentreY;

	double length = sqrt(deltaX * deltaX + deltaY * deltaY);

	bullets[lowestIdIndex].normDeltaX = deltaX / length;
	bullets[lowestIdIndex].normDeltaY = deltaY / length;

	//bShowCursor = !bShowCursor;
	//showCursor(bShowCursor);

} // SawnBullet()

void UpdateBulletsWorldPositions()
{
	for (int i = 0; i < numAmmo; i++)
	{
		if (bullets[i].bIsAlive == true)
		{
			bullets[i].posX -= worldDeltaX;
			bullets[i].posY -= worldDeltaY;
		}
	}
} // UpdateBulletsPositions()

void AdvanceBullets()
{
	for (int i = 0; i < numAmmo; i++)
	{
		if (bullets[i].bIsAlive == true)
		{
			float bulletNewX, bulletNewY;
			bulletNewX = bullets[i].posX - bullets[i].normDeltaX * BULLET_MOVEMENT_SPEED_FACTOR * DeltaTime;
			bulletNewY = bullets[i].posY - bullets[i].normDeltaY * BULLET_MOVEMENT_SPEED_FACTOR * DeltaTime;

			if (bulletNewX < mapLeftBorder - bulletRadius)
			{
				bullets[i].bIsAlive = false;
				continue;
			}
			else if (bulletNewX > (mapRightBorder - bulletRadius))
			{
				bullets[i].bIsAlive = false;
				continue;
			}
			else bullets[i].posX = bulletNewX;

			if (bulletNewY > (mapBottomBorder - bulletRadius))
			{
				bullets[i].bIsAlive = false;
				continue;
			}
			else if (bulletNewY < mapTopBorder - bulletRadius)
			{
				bullets[i].bIsAlive = false;
				continue;
			}
			else bullets[i].posY = bulletNewY;
		}
	}


} // AdvanceBullets()

void UpdateWorldOffset()
{
	int worldOrigX = worldOffsetX;
	int worldOrigY = worldOffsetY;
	if (bLeftKeyDown && !bRightKeyDown)
	{
		float worldNewX = worldOffsetX + PLAYER_MOVEMENT_SPEED_FACTOR * DeltaTime;
		if (worldNewX >= (mapWidth / 2 - playerRadius)) worldOffsetX = mapWidth / 2 - playerRadius;
		else worldOffsetX = worldNewX;
	}
	else if (bRightKeyDown && !bLeftKeyDown)
	{
		float worldNewX = worldOffsetX - PLAYER_MOVEMENT_SPEED_FACTOR * DeltaTime;
		if (worldNewX <= (-mapWidth / 2 + playerRadius)) worldOffsetX = -mapWidth / 2 + playerRadius;
		else worldOffsetX = worldNewX;
	}

	if (bUpKeyDown && !bDownKeyDown)
	{
		float worldNewY = worldOffsetY + PLAYER_MOVEMENT_SPEED_FACTOR * DeltaTime;
		if (worldNewY >= (mapHeight / 2 - playerRadius)) worldOffsetY = mapHeight / 2 - playerRadius;
		else worldOffsetY = worldNewY;
	}
	else if (bDownKeyDown && !bUpKeyDown)
	{
		float worldNewY = worldOffsetY - PLAYER_MOVEMENT_SPEED_FACTOR * DeltaTime;
		if (worldNewY <= (-mapHeight / 2 + playerRadius)) worldOffsetY = -mapHeight / 2 + playerRadius;
		else worldOffsetY = worldNewY;
	}

	worldDeltaX = worldOrigX - worldOffsetX;
	worldDeltaY = worldOrigY - worldOffsetY;
} // UpdateWorldOffset()

void DrawMap()
{
	int tilePosX, tilePosY;
	for (int row = 0; row < numTilesY; row++)
	{
		for (int column = 0; column < numTilesX; column++)
		{
			tilePosX = -(numTilesX * MapTileSize) / 2 + column * MapTileSize + windowWidth / 2;
			tilePosX += worldOffsetX;
			tilePosY = -(numTilesY * MapTileSize) / 2 + row * MapTileSize + windowHeight / 2;
			tilePosY += worldOffsetY;

			//Draw grass tiles
			if(mapTileId[row * (numTilesY - 1) + column] > 2) drawSprite(grass_tile_01, tilePosX, tilePosY);
			else drawSprite(grass_tile_02, tilePosX, tilePosY);	

			//Draw map borders
			if (row == 0) drawSprite(map_border_tb, tilePosX, mapTopBorder - 22);
			if (row == (numTilesY -1)) drawSprite(map_border_tb, tilePosX, mapBottomBorder);
			if (column == 0) drawSprite(map_border_lr, mapLeftBorder -22, tilePosY);
			if (column == (numTilesX - 1)) drawSprite(map_border_lr, mapRightBorder, tilePosY);
		}
	}

	//Draw map corners
	drawSprite(map_corner, mapLeftBorder - 22, mapTopBorder - 22);
	drawSprite(map_corner, mapLeftBorder - 22, mapBottomBorder);
	drawSprite(map_corner, mapRightBorder, mapTopBorder - 22);
	drawSprite(map_corner, mapRightBorder, mapBottomBorder);

	//Draw bushes/rocks/grass
	for (int i = 0; i < (foliageCount - 1); i++)
	{
		if(foliage[i].foliageId == 0)		drawSprite(plant_01, foliage[i].posX, foliage[i].posY);
		else if (foliage[i].foliageId == 1) drawSprite(plant_02, foliage[i].posX, foliage[i].posY);
		else								drawSprite(plant_03, foliage[i].posX, foliage[i].posY);
	}

} // DrawMap()

void UpdateFoliageWorldPositions()
{
	for (int i = 0; i < (foliageCount - 1); i++)
	{
		foliage[i].posX -= worldDeltaX;
		foliage[i].posY -= worldDeltaY;	
		
	}

}

void InitFoliagePositions()
{
	//Init foliage positions
	foliageCount = mapWidth * mapHeight / FOLIAGE_DENSITY_DENOMINATOR;
	foliage = new Foliage[foliageCount];

	int mapDifX, mapDifY, distanceToCollide;
	mapDifX = mapRightBorder - mapLeftBorder - foliageDiameter;
	mapDifY = mapBottomBorder - mapTopBorder - foliageDiameter;

	//distanceToCollide = foliageRadius * 2;

	for (int i = 0; i < (foliageCount - 1); i++)
	{
		int potentialX, potentialY;
		foliage[i].foliageId = rand() % 3;

		potentialX = GetRandomInRange(mapLeftBorder, mapDifX);
		potentialY = GetRandomInRange(mapTopBorder, mapDifY);

		/*
		bool bIsColliding	= false;
		bool bFoundXY		= false;
	
		while (!bFoundXY)
		{		
			potentialX = GetRandomInRange(mapLeftBorder, mapDifX);
			potentialY = GetRandomInRange(mapTopBorder, mapDifY);

			int potCentreX, potCentreY;
			potCentreX = potentialX + foliageRadius;
			potCentreY = potentialY + foliageRadius;			
			
			if (i == 0)
			{
				bFoundXY = true;
				continue;
			}
				
			for (int j = 0; j < i; j++)
			{
				int otherCentreX, otherCentreY;
				otherCentreX = foliage[j].posX + foliageRadius;
				otherCentreY = foliage[j].posY + foliageRadius;
				
				if ((potCentreX >= (otherCentreX + distanceToCollide)) || (potCentreX <= (otherCentreX - distanceToCollide)))
				{
					if ((potCentreY >= (otherCentreY + distanceToCollide)) || (potCentreY <= (otherCentreY - distanceToCollide)))
					{
						continue;
					}
					else
					{
						bIsColliding = true;
						break;
					}
				}
				else
				{
					bIsColliding = true;
					break;
				}
			}
			
			if (bIsColliding == false) bFoundXY = true;
		}
		*/		

		foliage[i].posX = potentialX;
		foliage[i].posY = potentialY;
	}
}

