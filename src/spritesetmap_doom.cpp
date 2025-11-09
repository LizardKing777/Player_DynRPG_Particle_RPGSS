#include <spritesetmap_doom.h>
#include <output.h>
#include "bitmap.h"
#include <filefinder.h>
#include <player.h>
#include <cmath>
#include <main_data.h>
#include <game_variables.h>
#include <map>
#include <iostream>
#include <input.h>
#include "game_map.h"
#include "game_player.h"
#include <cache.h>
#include "game_event.h"
#include "scene_map.h"

int Spriteset_MapDoom::mapWidth() {
	return mapW;
}
int Spriteset_MapDoom::mapHeight() {
	return mapH;
}

float Spriteset_MapDoom::castRay(float rayAngle, int& ray, std::vector<DrawingDoom>& d, int x, int &mx, int &my) {

	float rayPosX = player.x;
	float rayPosY = player.y;

	// Direction du rayon
	float rayDirX = cos(rayAngle);
	float rayDirY = sin(rayAngle);

	int startX = (int)(rayPosX / TILE_SIZE);
	int startY = (int)(rayPosY / TILE_SIZE);
	int mapX = startX;
	int mapY = startY;

	float rayDistance = 0.0;
	bool hit = false;

	while (!hit) {
		// Vérification si le rayon sort des limites de la carte
		if (mapX < 0 || mapX >= mapWidth() || mapY < 0 || mapY >= mapHeight()) {
			//rayDistance = -1;
			break;  // Sortir de la boucle si on dépasse la carte
		}

		if ((int)rayPosX % TILE_SIZE == 8 && (int)rayPosY % TILE_SIZE == 8)
		{
			int t_id = tilemapUp->GetTileDoom(mapX, mapY, 1);
			if (t_id > 146) {
				DrawingDoom de = { 6, x, rayDistance + 8.0f, (int)rayPosX % TILE_SIZE, t_id, {mapX, mapY} };

				auto b = std::find_if(d.begin(),
					d.end(),
					[&d1 = de]
					(const DrawingDoom& d2) -> bool { return d2.type == 6 && (d1.position.x == d2.position.x && d1.position.y == d2.position.y); });


				if (b == d.end()) {
					d.push_back(de);
				}

			}

		}

		if ((int)rayPosX % TILE_SIZE == 0 || (int)rayPosY % TILE_SIZE == 0
			|| (int)rayPosX % TILE_SIZE == 15 || (int)rayPosY % TILE_SIZE == 15)
			for (auto e : events_wall) {
				if (e.x == mapX && e.y == mapY) {

					float distance = rayDistance;
				/*	if (Main_Data::game_player->doomMoveType == 0)
						distance *= cos(rayAngle - player.angle);*/
					int rayD = (int)rayPosX % TILE_SIZE;
					int tx = e.tileSprite;

					if (rayD == 0 || rayD == TILE_SIZE - 1)
						rayD = (int)rayPosY % TILE_SIZE;

					DrawingDoom de;

					int type = 3;
					if (e.type == 2) {
						type = 4;
						// if (!((int)rayPosX % TILE_SIZE == 8 || (int)rayPosY % TILE_SIZE == 8)) {
						if (!(((int)rayPosX % TILE_SIZE >= 6 && (int)rayPosX % TILE_SIZE <= 10) || (int)rayPosY % TILE_SIZE >= 6 && (int)rayPosY % TILE_SIZE <= 10)) {
							continue;
						}
						de = { type, x, distance - 0.1f, rayD, e.id, {tx,0} };

						auto b = std::find_if(d.begin(),
							d.end(),
							[&d1 = de]
							(const DrawingDoom& d2) -> bool { return d2.type == 4 && d1.evID == d2.evID; });


						if (b != d.end()) {
							continue;
						}
						distance -= 1.0f;
					} else if(e.type == 3) {
						type = 5;
					}

					de = { type, x, distance - 0.1f, rayD, e.id, {tx,0} };

					d.push_back(de);

					if (e.type == 0 || e.type == 3) {
						hit = true;
						/*if (map[mapY][mapX] != 1)
							rayDistance = -1;*/
					}
				}
			}

		// Vérifier si le rayon touche un mur
		if ((map[mapY][mapX] == 1 || map[mapY][mapX] == 2) && (mapX != startX || mapY != startY)) {
			hit = true;
		}
		else if (hit == false) {
			// Incrémenter la position du rayon
			rayPosX += rayDirX;
			rayPosY += rayDirY;

			mapX = (int)(rayPosX / TILE_SIZE);
			mapY = (int)(rayPosY / TILE_SIZE);

			// Calcul de la distance actuelle du rayon
			rayDistance += sqrt(rayDirX * rayDirX + rayDirY * rayDirY);
		}
	}

	mx = mapX;
	my = mapY;

	ray = (int) rayPosX % TILE_SIZE;
	if (ray == 0 || ray == TILE_SIZE - 1)
		ray = (int)rayPosY % TILE_SIZE;

	float distance = rayDistance;
	return distance;  // Retourner la distance si un mur est touché
}

float Spriteset_MapDoom::renderTexturedFloor(float x1, float wallHeight, float rayAngle) {
	//int start = Player::screen_height / 2 + wallHeight + 1;
	int start = Player::screen_height / 2 + 1;

	// Calcul des directions cosinus et sinus en fonction de l'angle du rayon
	float directionCos = std::cos(rayAngle);
	float directionSin = std::sin(rayAngle);
	float playerAngle = player.angle;

	int lastMapX = -1;
	int lastMapY = -1;

	int y = 0;
	float distance = 0;
	float tilex, tiley;
	int mapX;
	int mapY;
	BitmapRef texture;

	const int inf = std::numeric_limits<int>::max();

	//for (int y = start; y < Player::screen_height; y++) {
	for (y = Player::screen_height; y >= start ; y--) {
		// Créer et calculer la distance
		distance = (Player::screen_height) / (2.0f * y - Player::screen_height);
		if (distance < 0) {
			distance = abs(distance);
			//Output::Debug("NEG");
		}
		if (distance >= inf) {
			//Output::Debug("inf - {}", distance);
			distance = 120;
			break;
		}
		//if (Main_Data::game_player->doomMoveType == 0)
		//	distance = distance / cos(rayAngle - player.angle);

		// Calcul de la position en tiles
		tilex = distance * directionCos;
		tiley = distance * directionSin;

		// Ajouter la position du joueur pour obtenir la position dans le monde
		tilex += player.x / TILE_SIZE;
		tiley += player.y / TILE_SIZE;

		// Obtenir la position du tile correspondant dans la carte
		mapX = static_cast<int>(std::floor(tilex));
		mapY = static_cast<int>(std::floor(tiley));

		// Vérifier si la position du tile est valide dans la carte
		if (mapX < 0 || mapY < 0 || mapX >= mapWidth() || mapY >= mapHeight()) {
			break;
		}

		if (map[mapY][mapX] != 1) {

			if (lastMapX != mapX || lastMapY != mapY) {
				texture = mapTexture(mapX, mapY);
				lastTile = texture;
				lastMapX = mapX;
				lastMapY = mapY;
			}
			else {
				texture = lastTile;
			}

			int texture_x = static_cast<int>(std::floor(tilex * TILE_SIZE)) % TILE_SIZE;
			int texture_y = static_cast<int>(std::floor(tiley * TILE_SIZE)) % TILE_SIZE;

			// Définir les coordonnées de la texture

			if (texture) {
				Rect srcRect = { texture_x, texture_y, 1, 1 };

				sprite->BlitFast(x1, y, *texture, srcRect, 255);

			}

		}
		else {
			break;
		}

	}

	return distance;
}

void Spriteset_MapDoom::DrawEvents(std::vector<DrawingDoom> &d, float FOV) {
	for (auto& ev : Game_Map::GetEvents()) {
		bool not_event_wall = true;
		if (ev.GetName() == "[WALL]" || ev.GetName() == "[WALL_D]" || ev.GetName() == "[WALL_S]")
			not_event_wall = false;

		if (ev.IsActive() && not_event_wall) {

			float evX = ev.GetSpriteX() / 256.0 + 0.5;
			float evY = ev.GetSpriteY() / 256.0 + 0.5;
			float playerX = player.x / 16;
			float playerY = player.y / 16;

			// Output::Debug("P {} {} {}", playerX, playerY, ev.GetSpriteX() / 16);

			float spriteXRelative = evX - playerX;
			float spriteYRelative = evY - playerY;

			float spriteAngleRadians = std::atan2(spriteYRelative, spriteXRelative);
			float ang = player.angle - FOV / 2;
			int spriteAngle = (spriteAngleRadians - ang) * 180 / M_PI;

			//Output::Debug(" {} {} {} {}", spriteAngle, spriteAngleRadians, ang, player.angle * 180 / M_PI);

			// Sprite angle checking
			if (spriteAngle > 360) spriteAngle -= 360;
			if (spriteAngle < 0) spriteAngle += 360;

			int screen_width = Player::screen_width / scale;

			int spriteX = std::floor(spriteAngle * screen_width / (FOV * 180 / M_PI));

			float distance = std::sqrt(std::pow(playerX - evX, 2) + std::pow(playerY - evY, 2)) * 16;

			//if (spriteX < 0 || screen_width) {
			//	Output::Debug("SKIP");
			//	continue; // Ne dessine pas les sprites hors écran
			//}

			// SpriteX right position fix
			if (spriteX > screen_width) {
				spriteX %= screen_width;
				spriteX -= screen_width;
			}
			DrawingDoom de = { 1, spriteX, distance, 0, ev.GetId() };

			// Output::Debug("B {} {} {} {}", spriteX, spriteAngle, distance, ev.GetId());

			d.push_back(de);


		}
	}
}
// Fonction pour dessiner la scène en utilisant les fonctions de Spriteset_MapDoom
void Spriteset_MapDoom::renderScene() {

	// Output::Debug("Render Scene");

	float FOV = player.fov;
	std::vector<DrawingDoom> drawings = {};

	bool drawFloor = true;

	for (int x = 0; x < Player::screen_width; x++) {
		// Calculer l'angle du rayon
		float rayAngle = player.angle + (x - Player::screen_width / 2) * (FOV / Player::screen_width);

		int ray = 0;

		int mx;
		int my;
		// Cast le rayon et obtenir la distance
		float distance = castRay(rayAngle, ray, drawings, x, mx, my);

		if (distance >= 0) {
			if (drawFloor) {
				float df = renderTexturedFloor(x, distance, rayAngle);
				if (df > 3) {
					//drawFloor = false;
					//x++;
					//renderTexturedFloor(x, distance, rayAngle);

					//d = { 0, x, distance, ray, 0, {mx, my} };
					//drawings.push_back(d);
				}
			}
			else {
				drawFloor = true;
			}

			DrawingDoom d = { 0, x, distance, ray, 0, {mx, my} };
			drawings.push_back(d);

			/*if (x == Player::screen_width / 2)
				Output::Debug("D {}", distance);*/

		}

	}

	DrawEvents(drawings, FOV);

	std::sort(drawings.begin(), drawings.end(), std::greater<DrawingDoom>());

	for (auto d : drawings) {

		if ( d.type == 2) {
			float distance = d.distance;

			//Output::Debug(" {}", distance);

			if (distance > 0) {



			}
		}
		else if (d.type == 0 || d.type == 3 || d.type == 5) {
			float distance = d.distance;

			// Output::Debug(" {}", map[d.position.x][d.position.y]);
			bool b = true;
			if (d.position.y >= 0 && d.position.y < mapHeight() && d.position.x >= 0 && d.position.x < mapWidth())
				if (map[d.position.y][d.position.x] == 2)
					b = false;
			if (distance > 0 && b) {
				int lineHeight = static_cast<int>(TILE_SIZE * Player::screen_height * 1.1 / distance);
				int drawStart = (Player::screen_height - lineHeight) / 2;
				int drawEnd = drawStart + lineHeight;

				int textureX = d.textureX;

				Rect r = Rect(d.x, drawStart, 1, drawEnd - drawStart);
				BitmapRef texture;
				if (d.type == 3 || d.type == 5) {
					texture = ((Scene_Map*)scene_map)->GetEventSprite(d.evID);
				}
				else
					texture = mapTexture(d.position.x, d.position.y);

				if (texture) {
					Rect srcRect;

					if (d.type == 3) {
						if (d.position.x == 1) {
							srcRect = { texture->width() - textureX - 9, 0, 1, TILE_SIZE};
						}
						else {
							srcRect = { texture->width() - textureX - 5, 0, 1, texture->height() };
						}
					}
					else if (d.type == 5) {
						//if (textureX % 2 == 1)
						{
							//Output::Debug(" {}", textureX);
							float w = (float)(texture->width()) / TILE_SIZE;
							int ww = (int)(ceil(w));

							int tx = textureX * 100 / (TILE_SIZE - 1) * (texture->width() - 1) / 100;
							float scaleFactor = static_cast<float>(texture->width()) / 16.0f;
							if (scaleFactor < 0)
								scaleFactor = 1;

							// Calcul précis de texX
							float texX_float = textureX * scaleFactor;
							tx = static_cast<int>(texX_float);


							srcRect = { tx, 0, 1,  texture->height() };
							//r.width = ww;
						}
						/*else
							srcRect = { 0,0,0,0 };*/
					}
					else
						srcRect = { textureX, 0, 1, texture->height() };

					if (srcRect.width != 0 && srcRect.height != 0)
						sprite->StretchBlit(r, *texture, srcRect, 255);
				}

			}
		}
		else if (d.type == 1 || d.type == 4 || d.type == 6) {
			float distance = d.distance;

			//Output::Debug("AA {} {}", d.evID, d.distance);

			if (distance > 0) {

				BitmapRef texture;
				if (d.type == 6)
					texture = mapTexture(d.position.x, d.position.y, 1);
				else
					texture = ((Scene_Map*)scene_map)->GetEventSprite(d.evID);

				if (texture) {

					int lineHeight = static_cast<int>(TILE_SIZE * Player::screen_height / distance);
					int drawStart = (Player::screen_height - lineHeight) / 2;
					int drawEnd = drawStart + lineHeight;
					// int textureX = d.textureX;

					float d5 = (float) lineHeight / Player::screen_height;
					int zx = d5 * 6 * texture->width();
					int zy = d5 * 6 * texture->height();

					int DoomEventWidth = texture->width();
					int DoomEventHeight = texture->height();

					Rect srcRect = { 0, 0, texture->width(), texture->height()};
					Rect srcRect2 = { 0, 0, texture->width(), texture->height() };

					float sprCorrection = 1;
					if (d.type == 6)
						sprCorrection = 2.0f;
					else
						if (Game_Map::GetEvent(d.evID)->HasTileSprite())
							sprCorrection = 2.0f;
                        else
              		if (!Game_Map::GetEvent(d.evID)->HasTileSprite()){
                       if (DoomEventWidth   >   80){
                       if (DoomEventHeight  <   80) {
                        sprCorrection = 2.4f;
                       }
                        else
                        sprCorrection =  2.4f;
                        }
                        else
                        if (DoomEventWidth > 24 && DoomEventWidth < 80);   {
                        sprCorrection =  1.4f;
                        }
                        if (DoomEventWidth <= 24) {
                        sprCorrection = 1.1f;}}

					Rect r = Rect(d.x - zx / 2, drawEnd - zy / sprCorrection, zx, zy);
					if (DoomEventWidth > 64) {
						sprite->StretchBlit(r, *texture, srcRect2, 255);
					}
					else {
						sprite->StretchBlit(r, *texture, srcRect, 255);
					}


					// Output::Debug("E {} {}", r.x, r.y);
				}
			}
		}
	}

}


float Spriteset_MapDoom::castRay7() {

	float distance = 0;

	int mapX = 0;
	int mapY = 0;

	float rayX = 0;
	float rayY = 0;

	float incrementAngle = player.fov / Player::screen_width;

	float rayAngle = player.angle - player.fov / 2;
	for (int rayCount = 0; rayCount < Player::screen_height; rayCount++) {

		// Ray path incrementers

		float raySin = sin(rayAngle);
		float rayCos = cos(rayAngle);

		// Wall finder
		int wall = 0;
		while (wall == 0) {
			rayX += rayCos;
			rayY += raySin;
			mapY = rayX / TILE_SIZE;
			mapY = rayY / TILE_SIZE;
			if (mapX >= 0 && mapX < mapWidth() && mapY >= 0 && mapY < mapHeight()) {
				wall = (map[mapY][mapX] == 1);
			}
			else {
				break;
			}
		}



		// Pythagoras theorem
		distance = sqrt(pow(player.x - rayX, 2) + pow(player.y - rayY, 2));
		//Math.sqrt(Math.pow(data.player.x - ray.x, 2) + Math.pow(data.player.y - ray.y, 2));

		// Fish eye fix
		distance = distance * cos(rayAngle - player.angle);

		// Wall height
		int wallHeight = floor((Player::screen_height / 2) / distance);

		// Get texture
		//let texture = data.textures[wall - 1];

		// Calcule texture position
		//int texturePositionX = (int) floor(TILE_SIZE * (rayX + rayY)) % TILE_SIZE);

		// Draw
		//drawLine(rayCount, 0, rayCount, data.projection.halfHeight - wallHeight, "black");


		//drawTexture(rayCount, wallHeight, texturePositionX, texture);


		//drawLine(rayCount, data.projection.halfHeight + wallHeight, rayCount, data.projection.height, "rgb(95, 87, 79)");

		// Increment
		rayAngle += incrementAngle;
	}

	return distance;
}
void Spriteset_MapDoom::renderMode7() {

	std::vector<DrawingDoom> drawings = {};

	float x2 = 1;
	float y2 = 1;
	float z = 1;

	float moveX = player.x;
	float moveY = player.y;

	float rayAngle = player.angle;

	float sinV = sin(rayAngle);
	float cosV = cos(rayAngle);

	int mapX = 0;
	int mapY = 0;

	int startY = Player::screen_height / 8;

	for (int y = startY; y < Player::screen_height; ++y) {

		mapX = 0;
		for (int x = 0; x < Player::screen_width; ++x) {

			//float rayAngle = player.angle + (x - Player::screen_width / 2) * (player.fov / Player::screen_width);
			//sinV = sin(rayAngle);
			//cosV = cos(rayAngle);

			y2 = ((Player::screen_width - x) * cosV - x * sinV) / z;
			x2 = ((Player::screen_width - x) * sinV + x * cosV) / z;

			if (z < 0) {
				x2 -= moveX / scaleX;
				y2 -= moveY / scaleY;
			}
			else {
				x2 += moveX / scaleX;
				y2 += moveY / scaleY;
			}

			y2 *= scaleY;
			x2 *= scaleX;

			int mx = (int) (x2) % TILE_SIZE;
			int my = (int) (y2) % TILE_SIZE;

			mapX = mapWidth() - x2 / TILE_SIZE;
			mapY = y2 / TILE_SIZE;

			//Output::Debug("{} {}", mapX, mapY);
			if (Game_Map::LoopHorizontal()) {
				if (mapX<0)
					mapX = mapWidth() * TILE_SIZE - abs(mapX) % mapWidth();
			}
			if (Game_Map::LoopVertical()) {
				if (mapY < 0)
					mapY = mapHeight() * TILE_SIZE - abs(mapY) % mapHeight();
			}

			if (mapX >= 0 && mapX < mapWidth() && mapY >= 0 && mapY < mapHeight()) {

				if (mx == 8 && my == 8) {
					auto event = Game_Map::GetEventAt(mapX, mapY, true);
					if (event) {

						if (event->GetName() != "[WALL]" && event->GetName() != "[WALL_D]") {
							float distance = 0.01f;

							DrawingDoom de = { 1, x, distance, 0, event->GetId(), {mapX, mapY} };

							auto b = std::find_if(drawings.begin(),
								drawings.end(),
								[&d1 = de]
								(const DrawingDoom& d2) -> bool { return d2.type == 1 && d1.evID == d2.evID; });


							if (b == drawings.end()) {
								drawings.push_back(de);
							}
						}

					}
				}

				BitmapRef texture = mapTexture(mapX, mapY);

				if (texture) {
					Rect srcRect = {TILE_SIZE - 1 - mx, my, 1, 1 };
					sprite->BlitFast(x, y, *texture, srcRect, 200);
				}


				/*//// Paramètres pour la position du personnage (en coordonnées de tuile)
				//float characterTileX = Main_Data::game_player->GetX();  // Position X du personnage en tiles
				//float characterTileY = Main_Data::game_player->GetY();   // Position Y du personnage en tiles

				//// Zoom maximal et minimal (ajustez selon vos besoins)
				//float maxZoom = 2.0f;
				//float minZoom = 0.5f;

				//// Distance entre le joueur et le personnage en coordonnées de tuiles
				//float distanceX = (characterTileX * TILE_SIZE) - player.x;
				//float distanceY = (characterTileY * TILE_SIZE) - player.y;
				//float distanceToCharacter = sqrt(distanceX * distanceX + distanceY * distanceY);

				//// Calcul du zoom basé sur la distance (inversement proportionnel)
				//float zoomFactor = 1.0f / (distanceToCharacter / 100.0f);  // Ajustez le diviseur pour régler le facteur de zoom

				//// Limiter le zoom pour éviter des tailles de sprite trop grandes ou trop petites
				//if (zoomFactor > maxZoom) zoomFactor = maxZoom;
				//if (zoomFactor < minZoom) zoomFactor = minZoom;

				//// Rendu du personnage avec un zoom proportionnel à la distance
				//int characterScreenX = Player::screen_width / 2;  // Ajustez la position sur l'écran
				//int characterScreenY = Player::screen_height / 2;  // Ajustez la position sur l'écran

				//// Rendu du sprite avec zoom
				//if (mapX == characterTileX && mapY == characterTileY) {
				//	BitmapRef characterTexture = scene->GetEventSprite(0);

				//	if (characterTexture) {
				//		Rect srcRect = { 0, 0, TILE_SIZE, TILE_SIZE };  // Taille originale du sprite
				//		Rect dstRect = {
				//			characterScreenX - (int) (TILE_SIZE * zoomFactor) / 2,  // Position X
				//			characterScreenY - (int) (TILE_SIZE * zoomFactor) / 2,  // Position Y
				//			(int) (TILE_SIZE * zoomFactor),  // Largeur avec zoom
				//			(int) (TILE_SIZE * zoomFactor)   // Hauteur avec zoom
				//		};

				//		sprite->StretchBlit(dstRect, *characterTexture, srcRect, 255);
				//	}
				//}*/
			}
		}

		z++;
	}

	for (auto d : drawings) {

		if (d.type == 2) {
			float distance = d.distance;

			//Output::Debug(" {}", distance);

			if (distance > 0) {



			}
		}
		else if (d.type == 0 || d.type == 3) {
			float distance = d.distance;

			//Output::Debug(" {}", distance);

			if (distance > 0) {
				int lineHeight = static_cast<int>(TILE_SIZE * Player::screen_height * 1.1 / distance);
				int drawStart = (Player::screen_height - lineHeight) / 2;
				int drawEnd = drawStart + lineHeight;

				int textureX = d.textureX;

				Rect r = Rect(d.x, drawStart, 1, drawEnd - drawStart);
				BitmapRef texture;
				if (d.type == 3)
					texture = ((Scene_Map*)scene_map)->GetEventSprite(d.evID);
				else
					texture = mapTexture(d.position.x, d.position.y);

				if (texture) {
					Rect srcRect;

					if (d.type == 3) {
						srcRect = { texture->width() - textureX - 5, 0, 1, texture->height() };
					}
					else
						srcRect = { textureX, 0, 1, texture->height() };

					sprite->StretchBlit(r, *texture, srcRect, 255);
				}

			}
		}
		else if (d.type == 1) {
			float distance = d.distance;

			// Output::Debug(" {} {}", d.evID, d.distance);

			if (distance > 0) {

				BitmapRef texture;
				texture = ((Scene_Map*)scene_map)->GetEventSprite(d.evID);

				if (texture) {

					int lineHeight = static_cast<int>(TILE_SIZE * Player::screen_height / distance);
					int drawStart = (Player::screen_height - lineHeight) / 2;
					int textureX = d.textureX;

					float d5 = (float)lineHeight / Player::screen_height;
					int zx = d5 * 6 * texture->width();
					int zy = d5 * 6 * texture->height();

					Rect srcRect = { 0, 0, texture->width(), texture->height() };
					float sprCorrection = 4;
					if (Game_Map::GetEvent(d.evID)->HasTileSprite())
						sprCorrection = 1.2f;
					Rect r = Rect(d.x - zx / 2, drawStart + zy / sprCorrection, zx, zy);
					sprite->StretchBlit(r, *texture, srcRect, 255);
				}
			}
		}
	}
	drawings.clear();

	/*//// Ajout du dessin du personnage avec zoom proportionnel
	////// Récupérer la position du personnage en coordonnées de tile
	////float playerTileX = Main_Data::game_player->GetX();
	////float playerTileY = Main_Data::game_player->GetY();

	////// Calculer les coordonnées de dessin du personnage sur l'écran
	////float playerScreenX = (playerTileX - moveX) / TILE_SIZE;
	////float playerScreenY = (playerTileY - moveY / TILE_SIZE);

	////// Ajuster la taille du sprite du personnage en fonction de sa profondeur (zoom proportionnel à sa distance)
	////float playerDepth = z; // La profondeur actuelle (z) détermine le zoom
	////float zoomFactor = 1.0f / (playerDepth * 0.05f); // Ajuster ce facteur selon les besoins

	////// Définir les dimensions du sprite du personnage
	////int playerSpriteWidth = (int)(24 * zoomFactor);
	////int playerSpriteHeight = (int)(32 * zoomFactor);

	////// Centrer le sprite sur sa position à l'écran
	////int playerDrawX = (int)(Player::screen_width / 2 + playerScreenX - playerSpriteWidth / 2);
	////int playerDrawY = (int)(Player::screen_height / 2 + playerScreenY - playerSpriteHeight / 2);

	////Output::Debug(" {} {} {} {}", playerDrawX, playerDrawY, playerSpriteWidth, playerSpriteHeight);

	////// Dessiner le personnage à l'écran avec un zoom proportionnel
	////BitmapRef playerTexture = scene->GetEventSprite(0);
	////if (playerTexture) {
	////	Rect playerSrcRect = { 0, 0, 24, 32 };
	////	Rect playerDestRect = { playerDrawX, playerDrawY, playerSpriteWidth, playerSpriteHeight };

	////	// Dessiner le sprite du personnage avec zoom
	////	sprite->StretchBlit(playerDestRect, *playerTexture, playerSrcRect, 255);
	////}*/


	BitmapRef texture;
	texture = ((Scene_Map*) scene_map)->GetEventSprite(0);

	if (texture) {
		Rect srcRect = { 0, 0, texture->width(), texture->height() };
		//Rect r = Rect(Game_Map::GetEvent(d.evID)->GetX() * TILE_SIZE - zx / 2, drawStart + zy / 4, zx, zy);
		int sprCorrection = 4;
		Rect r = Rect(205 - 4, 144 + 20, 24,32);
		sprite->StretchBlit(r, *texture, srcRect, 255);
	}


	castRay7();
}

BitmapRef Spriteset_MapDoom::mapTexture(int x, int y, int layer) {
	if (x >= 0 && y >= 0 && x < mapWidth() && y < mapHeight()) {
		//int id = ((Scene_Map*)scene_map)->GetTileID(x, y, 0);
		int id = 0;
		if (layer == 0)
			id = tilemapDown->GetTileDoom(x, y, 0);
		else
			id = tilemapUp->GetTileDoom(x, y, 1);

		auto mt = mapTexturesID;
		if (layer != 0) {
			mt = mapTexturesUpperID;
		}

		if (!mt[id]) {
			mt[id] = ((Scene_Map*)scene_map)->GetTile(x, y, layer);
			return mt[id];
		}
		return mt[id];
	}

	BitmapRef b = Bitmap::Create(TILE_SIZE, TILE_SIZE, Color(0, 0, 0, 0));
	return b;
}

void Spriteset_MapDoom::renderTexturedFloor() {

	float x2;
	float y2;
	float z = 1;

	float moveX = player.x;
	float moveY = player.y;


	for (int y = Player::screen_height / 2; y < Player::screen_height; y++) {
		float correctX = 0;
		for (int x = 0; x < Player::screen_width; x++) {

			float rayAngle = player.angle + (x - Player::screen_width / 2) * (player.fov / Player::screen_width);

			float sinV = sin(rayAngle);
			float cosV = cos(rayAngle);

			y2 = ((Player::screen_width - x) * cosV - (x * sinV)) / z;
			x2 = ((Player::screen_width - x) * sinV - (x * cosV)) / z;

			if (z < 0) {
				x2 -= moveX / scaleX;
				y2 -= moveY / scaleY;
			}
			else {
				x2 += moveX / scaleX;
				y2 += moveY / scaleY;
			}
			if (y2 < 0)
				y2 *= -1;
			if (x2 < 0) {
				x2 *= -1;
			}
			y2 *= scaleY;
			x2 *= scaleX;

			x2 = fmod(x2,TILE_SIZE);
			y2 = fmod(y2, TILE_SIZE);


			Rect r = Rect(x, y, 1, 1);
			BitmapRef texture = mapTexture(x2, y2);
			int mx = floor(x2);
			int my = floor(y2);

			if (texture) {
				Rect srcRect = { mx, my, 1, 1 };
				//sprite->StretchBlit(r, *texture, srcRect, 255);
				sprite->BlitFast(x, y, *texture, srcRect, 255);
			}

		}
		z++;
	}

	return;
}


Spriteset_MapDoom::Spriteset_MapDoom() {

	scene_map = Scene::Find(Scene::Map).get();
	chipset = ((Scene_Map*)scene_map)->GetChipset();

	tilemapDown = ((Scene_Map*)scene_map)->GetTilemap(0);
	tilemapUp = ((Scene_Map*)scene_map)->GetTilemap(1);

	doomMap = true;

	mapW = Game_Map::GetTilesX();
	mapH = Game_Map::GetTilesY();

	Output::Debug("Map size {} {}", mapWidth(), mapHeight());

	auto m = Game_Map::GetPassagesDown();

	for (int x = 0;x < mapWidth(); x++) {
		for (int y = 0; y < mapHeight(); y++) {
			map[y][x] = Game_Map::GetTerrainTag(x, y) - 1;
		}
	}

	player.x = Main_Data::game_player->GetX() * TILE_SIZE + TILE_SIZE / 2;
	player.y = Main_Data::game_player->GetY() * TILE_SIZE + TILE_SIZE / 2;
	player.angle = (Main_Data::game_player->GetDirection() * 90 - 90) * M_PI / 180;
	if (Main_Data::game_player->doomMoveType == 2) {
		player.angle = (135) * M_PI / 180;
	}

	sprite = Bitmap::Create(Player::screen_width, Player::screen_height);
	spriteUpper = Bitmap::Create(Player::screen_width, Player::screen_height);

	for (int i = 0;i <= Game_Map::GetHighestEventId();i++) {
		auto e = Game_Map::GetEvent(i);
		if (e) {
			if (e->GetName() == "[WALL]" || e->GetName() == "[WALL_D]" || e->GetName() == "[WALL_S]") {
				int t = 0;
				if (e->GetName() == "[WALL_D]")
					t = 1;
				if (e->GetName() == "[WALL_S]")
					t = 3;

				int tileSprite = 0;
				if (e->HasTileSprite())
					tileSprite = 1;

				EventWall p = { e->GetX(), e->GetY(), e->GetId(), t, tileSprite };
				events_wall.push_back(p);
			}
		}
	}
}

Spriteset_MapDoom::Spriteset_MapDoom(std::string n, int zoom, int dx, int dy, int rx, int ry, int rz) {

	Load_OBJ(n);

	displayX = dx;
	displayY = dy;
	rotationX = rx;
	rotationY = ry;
	rotationZ = rz;

	angleX = 0;
	angleY = 0;
	angleZ = 0;

	for (auto& p : points3D) {
		p.x *= zoom;
		p.y *= zoom;
		p.z *= zoom;
	}

	centeroid = { 0,0,0 };
	for (auto& p : points3D) {
		centeroid.x += p.x;
		centeroid.y += p.y;
		centeroid.z += p.z;
	}
	centeroid.x /= points3D.size();
	centeroid.y /= points3D.size();
	centeroid.z /= points3D.size();

	sprite = Bitmap::Create(Player::screen_width, Player::screen_height);
	spriteUpper = Bitmap::Create(Player::screen_width, Player::screen_height);

	Update(true);

	rotationX = 0;
	rotationY = 0;
	rotationZ = 0;

}

void Spriteset_MapDoom::setRotation(int rx, int ry, int rz) {
	rotationX = rx;
	rotationY = ry;
	rotationZ = rz;
}

void Spriteset_MapDoom::getRotation(int varX, int varY, int varZ) {

	//int x = (int)(angleX * 180 / M_PI / 1000) % 360;
	//if (x < 0)
	//	x = 360 + x;

	//int y = (int)(angleY * 180 / M_PI / 1000) % 360;
	//if (y < 0)
	//	y = 360 + y;

	//int z = (int)(angleZ * 180 / M_PI / 1000) % 360;
	//if (z < 0)
	//	z = 360 + z;

	int x = (int)(angleX * 180 / M_PI) % 360;
	int y = (int)(angleY * 180 / M_PI) % 360;
	int z = (int)(angleZ * 180 / M_PI) % 360;

	// S'assurer que les angles sont positifs
	if (x < 0) x = 360 + x;
	if (y < 0) y = 360 + y;
	if (z < 0) z = 360 + z;


	auto p = points3D[points3D.size() - 1];
	x = p.x;
	y = p.y;
	z = p.z;

	Main_Data::game_variables->Set(varX, x);
	Main_Data::game_variables->Set(varY, y);
	Main_Data::game_variables->Set(varZ, z);

	//Output::Debug("A {} {} {}", x, y, z);
}


std::vector<std::string> Spriteset_MapDoom::split(const std::string& s, const std::string& delimiter) {
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	std::string token;
	std::vector<std::string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

void Spriteset_MapDoom::Load_OBJ(std::string name) {

	Output::Debug("Load_OBJ");

	points3D = {};
	connections3D = {};

	surfaces = {};

	Output::Debug(".mtl");

	std::string s = "";
	std::string ini_file = FileFinder::Game().FindFile("Models/" + name + ".mtl");
	auto ini_stream = FileFinder::Game().OpenInputStream(ini_file, std::ios_base::in);

	std::map<std::string, Color> colors;
	std::string material_name = "";

	for (std::string line; getline(ini_stream, line); )
	{
		// Output::Debug("{}", line);
		if (line._Starts_with("newmtl ")) {
			material_name = line.substr(7,line.length() - 7);
		} else if (line._Starts_with("Kd ")) {
			std::vector<std::string> v = split(line, " ");
			int r = std::stof(v[1]) * 255;
			int g = std::stof(v[2]) * 255;
			int b = std::stof(v[3]) * 255;
			colors[material_name] = Color(r, g, b, 255);

			Output::Debug(" {} {} {} {}", material_name, r, g, b);
		}
	}

	Output::Debug(".obj");

	std::vector <Point> pointsNoMaterial;
	s = "";
	ini_file = FileFinder::Game().FindFile("Models/" + name + ".obj");
	ini_stream = FileFinder::Game().OpenInputStream(ini_file, std::ios_base::in);
	Color color;

	for (std::string line; getline(ini_stream, line); )
	{
		// Output::Debug("{}", line);
		if (line._Starts_with("v ")) {
			std::vector<std::string> v = split(line, " ");
			float x = std::stof(v[1]);
			float y = std::stof(v[2]);
			float z = std::stof(v[3]);

			Point v3 = { x,y,z, false };
			points3D.push_back(v3);
			/*pointsNoMaterial.push_back(v3);*/
		}
		// Material
		else if (line._Starts_with("usemtl ")) {
			material_name = line.substr(7, line.length() - 7);
			Output::Debug("{}", material_name);
			color = colors[material_name];

			Output::Debug(" {} {} {}", color.red, color.green, color.blue);

			//for (auto p : pointsNoMaterial) {
			//	p.color = color;
			//	points3D.push_back(p);
			//}
			//pointsNoMaterial.clear();

		}
		// Surface
		else if (line._Starts_with("f ")) {
			std::vector<std::string> v = split(line, " ");
			std::vector<Point> s;
			Point p;
			p.color = color;
			for (int i = 1; i < v.size(); i++) {
				auto vc = v[i];
				std::vector<std::string> f = split(vc, "/");

				if (i == 1) {
					p.y = std::stof(f[0]) - 1;
				}
				else if (i == v.size() - 1) {
					p.x = p.y;
					p.y = std::stof(f[0]) - 1;
					s.push_back(p);
					p.x = p.y;
					p.y = s[0].x;
					s.push_back(p);
				}
				else {
					p.x = p.y;
					p.y = std::stof(f[0]) - 1;
					s.push_back(p);
				}

			}

			connection c;
			int cc;
			for (int i = 1; i < v.size(); i++) {
				auto vc = v[i];
				std::vector<std::string> f = split(vc, "/");
				if (i == 1) {
					c.a = std::stoi(f[0]) - 1;
					cc = c.a;
				}
				else {
					c.b = std::stoi(f[0]) - 1;
					connections3D.push_back(c);
					c.a = c.b;
					if (i == v.size() - 1) {
						c.b = cc;
						connections3D.push_back(c);
					}
				}
			}

			surfaces.push_back(s);

		}
		 // => Wireframe
		/*else if (line._Starts_with("f ")) {
			std::vector<std::string> v = split(line, " ");
			connection c;
			int cc;
			for (int i = 1; i < v.size(); i++) {
				auto vc = v[i];
				std::vector<std::string> f = split(vc, "/");
				if (i == 1) {
					c.a = std::stoi(f[0]) - 1;
					cc = c.a;
				}
				else {
					c.b = std::stoi(f[0]) - 1;
					connections3D.push_back(c);
					c.a = c.b;
					if (i == v.size() - 1) {
						c.b = cc;
						connections3D.push_back(c);
					}
				}
			}
		}*/
	}

	points3D.push_back({ 0,0,0 });

	//for (auto p : pointsNoMaterial) {
	//	p.color = color;
	//	points3D.push_back(p);
	//}
	//pointsNoMaterial.clear();
}

void Spriteset_MapDoom::Update(bool first) {
	// Output::Debug("Update");
	if (Input::IsRawKeyTriggered(Input::Keys::F3)) {
		refresh_index = (refresh_index + 1) % 6;
		Output::Info("Refresh rate : {} fps", 60 / refresh[refresh_index]);
	}
	int refresh_rate = refresh[refresh_index];

	if (doomMap) {

		if (Main_Data::game_player->doomMoveType == 2) {

			float angle = 0.02;

			float x = Main_Data::game_player->GetX() * TILE_SIZE + TILE_SIZE / 2;
			float y = Main_Data::game_player->GetY() * TILE_SIZE + TILE_SIZE / 2;

			if (Main_Data::game_player->doomWait != 0) {
				if (x != player7.x || y != player7.y) {

					float dx = (x - player7.x) / Main_Data::game_player->doomWait;
					float dy = (y - player7.y) / Main_Data::game_player->doomWait;

					if (dx != 0)
						player7.x += dx;
					if (dy != 0)
						player7.y += dy;

					player.x = (mapWidth()) * TILE_SIZE - player7.x;
					player.y = player7.y + Player::screen_height * 3 / 4;
				}
			}
			else {
				float x = Main_Data::game_player->GetX() * TILE_SIZE + TILE_SIZE / 2;
				float y = Main_Data::game_player->GetY() * TILE_SIZE + TILE_SIZE / 2;
				//float y = Main_Data::game_player->GetY() * TILE_SIZE + TILE_SIZE / 2 + scaleX * 2;

				if (x != player.x || y != player.y) {

					player.x = mapWidth() * TILE_SIZE - x;
					player.y = y + Player::screen_height * 3 / 4;

					player7.x = x;
					player7.y = y;

				}
			}


			//float x = Main_Data::game_player->GetX() * TILE_SIZE + TILE_SIZE / 2;
			//float y = Main_Data::game_player->GetY() * TILE_SIZE + TILE_SIZE / 2;
			////float y = Main_Data::game_player->GetY() * TILE_SIZE + TILE_SIZE / 2 + scaleX * 2;

			//if (x != player.x || y != player.y) {

			//		player.x = mapWidth() * TILE_SIZE - x;
			//		player.y = y + Player::screen_height * 3 / 4;

			//}

			if (Input::IsPressed(Input::DIVIDE)) {  // Tourner à gauche
				player.angle -= angle;
				if (player.angle < 0)
					player.angle += 2 * M_PI;
				if (player.angle > 2 * M_PI)
					player.angle -= 2 * M_PI;
			}
			if (Input::IsPressed(Input::MULTIPLY)) {  // Tourner à droite
				player.angle += angle;
				if (player.angle < 0)
					player.angle += 2 * M_PI;
				if (player.angle > 2 * M_PI)
					player.angle -= 2 * M_PI;

				int a = player.angle * 180 / M_PI;

				Output::Debug(" {} {}", player.angle, a);
			}

			if (timer % refresh_rate == 1 || refresh_rate == 1 || first) {
				sprite->Clear();
				renderMode7();
			}
			points.clear();

			timer++;

			return;
		}

		int s = 1;
		float angle = 0.02;

		/*if (Input::IsPressed(Input::DIVIDE)) {
			player.FOVangle -= 1;
			player.fov = player.FOVangle * (M_PI / 180.0f);
		}
		if (Input::IsPressed(Input::MULTIPLY)) {
			player.FOVangle += 1;
			player.fov = player.FOVangle * (M_PI / 180.0f);
		}*/


		if (Main_Data::game_player->doomMoveType == 0) {// Tile Movement
			if (Input::IsPressed(Input::MINUS)) {
				scale -= 0.1f;
			}
			if (Input::IsPressed(Input::MULTIPLY)) {
				scale += 0.1f;
			}

			if (Main_Data::game_player->doomWait != 0) {

				static const int move_speed[] = { 64, 32, 24, 16, 12, 8 };

				float x = Main_Data::game_player->GetX() * TILE_SIZE + TILE_SIZE / 2;
				float y = Main_Data::game_player->GetY() * TILE_SIZE + TILE_SIZE / 2;

				if (x != player.x || y != player.y) {

					float dx = (x - player.x) / Main_Data::game_player->doomWait;
					float dy = (y - player.y) / Main_Data::game_player->doomWait;

					if (dx != 0)
						player.x += dx;
					if (dy != 0)
						player.y += dy;

				}

				float a = (Main_Data::game_player->GetDirection() * 90 - 90) * M_PI / 180;
				if (player.angle < 0 && a > 0)
					player.angle += 2 * M_PI;

				if (player.angle > 0 && a < 0) {
					player.angle -= 2 * M_PI;
				}

				angle = abs(a - player.angle) / Main_Data::game_player->doomWait;


				if (a != player.angle) {
					if (a < player.angle)
						player.angle -= angle;
					if (a > player.angle)
						player.angle += angle;
					if (Main_Data::game_player->doomWait == 0)
						player.angle = a;
				}
				// Output::Debug("l {} {} {}", player.angle, a, angle);
			}

		} else { // Pixel Movement
			if (Main_Data::game_player->canMove) {
				if (Input::IsPressed(Input::UP)) {  // Avancer
					float px = player.x + cos(player.angle) * s * TILE_SIZE;
					float py = player.y + sin(player.angle) * s * TILE_SIZE;

					int ppx = px / TILE_SIZE;
					int ppy = py / TILE_SIZE;

					//Output::Debug(" {} {} {}", ppx, ppy, map[ppx][ppy]);

					if (ppx < mapWidth() && ppy < mapHeight() && ppx >= 0 && ppy >= 0) {
						int x = player.x / TILE_SIZE;
						int y = player.y / TILE_SIZE;

						int dx = ppx;
						int dy = ppy;

						//Output::Debug(" {} {} {} {} {} {}", x, y, ppx, ppy, Main_Data::game_player->MakeWay(x, y, dx, y) , Main_Data::game_player->MakeWay(x, y, x, dy));

						if (Main_Data::game_player->MakeWay(x, y, dx, dy) || (x == dx && y == dy)) {
							player.x = player.x + cos(player.angle) * s;
							player.y = player.y + sin(player.angle) * s;

							Main_Data::game_player->SetX(player.x / TILE_SIZE);
							Main_Data::game_player->SetY(player.y / TILE_SIZE);
						}
						else {
							if (Main_Data::game_player->MakeWay(x, y, dx, y)) {
								player.x = player.x + cos(player.angle) * s;

								Main_Data::game_player->SetX(player.x / TILE_SIZE);
								Main_Data::game_player->SetY(player.y / TILE_SIZE);
							}
							else if (Main_Data::game_player->MakeWay(x, y, x, dy)) {
								player.y = player.y + sin(player.angle) * s;

								Main_Data::game_player->SetX(player.x / TILE_SIZE);
								Main_Data::game_player->SetY(player.y / TILE_SIZE);
							}
						}


						int front_x = Game_Map::XwithDirection(Main_Data::game_player->GetX(), Main_Data::game_player->GetDirection());
						int front_y = Game_Map::YwithDirection(Main_Data::game_player->GetY(), Main_Data::game_player->GetDirection());

						Output::Debug("XY {} {}", front_x, front_y);

						bool action = Main_Data::game_player->CheckEventTriggerThere({ lcf::rpg::EventPage::Trigger_touched, lcf::rpg::EventPage::Trigger_collision }, front_x, front_y, false);
                        bool action2 = Main_Data::game_player->CheckEventTriggerHere({ lcf::rpg::EventPage::Trigger_touched, lcf::rpg::EventPage::Trigger_collision }, false);

						Output::Debug("Action {}", action);

					}
				}
				if (Input::IsPressed(Input::DOWN)) {  // Reculer
					float px = player.x - cos(player.angle) * s;
					float py = player.y - sin(player.angle) * s;

					int ppx = px / TILE_SIZE;
					int ppy = py / TILE_SIZE;

					//Output::Debug(" {} {} {}", ppx, ppy, map[ppx][ppy]);

					if (ppx < mapWidth() && ppy < mapHeight() && ppx >= 0 && ppy >= 0)
						if (map[ppy][ppx] == 0) {
							player.x = px;
							player.y = py;
						}
				}
				if (Input::IsPressed(Input::LEFT)) {  // Tourner à gauche
					player.angle -= angle;
					if (player.angle < 0)
						player.angle += 2 * M_PI;
					if (player.angle > 2 * M_PI)
						player.angle -= 2 * M_PI;

					float a = (player.angle) * 180.0f / M_PI + 45;
					if (a < 0)
						a += 360;
					if (a > 360)
						a -= 360;
					int dir = ((int)(a / 90) + 1) % 4;

					Main_Data::game_player->SetFacing(dir);
					Main_Data::game_player->SetDirection(dir);
				}
				if (Input::IsPressed(Input::RIGHT)) {  // Tourner à droite
					player.angle += angle;
					if (player.angle < 0)
						player.angle += 2 * M_PI;
					if (player.angle > 2 * M_PI)
						player.angle -= 2 * M_PI;

					float a = (player.angle) * 180.0f / M_PI + 45;
					if (a < 0)
						a += 360;
					if (a > 360)
						a -= 360;
					int dir = ((int)(a / 90) + 1) % 4;

					Main_Data::game_player->SetFacing(dir);
					Main_Data::game_player->SetDirection(dir);
				}
			}
		}

		if (timer % refresh_rate == 1 || refresh_rate == 1 || first) {
			sprite->Clear();
			renderScene();
		}
		points.clear();


		timer++;

		return;
	}

	if (rotationX != 0 || rotationY != 0 || rotationZ != 0 || first) {


		angleX += rotationX / 1000.0;
		angleY += rotationY / 1000.0;
		angleZ += rotationZ / 1000.0;

		normalizeAngle(angleX);
		normalizeAngle(angleY);
		normalizeAngle(angleZ);

		for (auto& p : points3D) {

			p.x -= centeroid.x;
			p.y -= centeroid.y;
			p.z -= centeroid.z;
			// rotate(p, rotationX / 1000.0, rotationY / 1000.0, rotationZ / 1000.0);
			rotate(p, rotationX / 1000.0, 0, 0);
			rotate(p, 0, rotationY / 1000.0, 0);
			rotate(p, 0, 0, rotationZ / 1000.0);
			p.x += centeroid.x;
			p.y += centeroid.y;
			p.z += centeroid.z;

			//if (timer % refresh_rate == 1 || refresh_rate == 1)
				//pixel(p.x, p.y, p.z, p.color);
		}

		if (timer % refresh_rate == 1 || refresh_rate == 1 || first) {

			for (auto& c : connections3D) {
				// line(points3D[c.a], points3D[c.b]);
			}
			for (auto s : surfaces) {

				std::vector<Point> s2;
				for (auto p : s) {
					Point p2 = { points3D[p.x].x, points3D[p.x].y, points3D[p.x].z, points3D[p.x].upper, p.color };
					s2.push_back(p2);

					p2 = { points3D[p.y].x, points3D[p.y].y, points3D[p.y].z, points3D[p.y].upper, p.color };
					s2.push_back(p2);

					// Output::Debug(" {} {} {}", p2.color.red, p2.color.green , p2.color.blue);
				}
				sortPoints(s2);
				drawPolygon(s2);

			}

			Show();

		}
		timer++;
		points.clear();
	}
}

// Normalisation des angles pour rester entre 0 et 360
void Spriteset_MapDoom::normalizeAngle(double& angle) {
	angle = fmod(angle, 2 * M_PI);  // Modulo 2π pour rester dans l'intervalle [0, 2π]
	if (angle < 0) {
		angle += 2 * M_PI;  // Si négatif, on ajoute 2π pour ramener l'angle dans l'intervalle [0, 2π]
	}
}


void Spriteset_MapDoom::pixel(float x, float y, float z, Color c) {

	Point p = { x, y, z, false, c, true};
	if (z < 0) {
		p.upper = true;
	}

	points.emplace_back(p);
}

void Spriteset_MapDoom::line(Point p1, Point p2) {

	float x1, y1, x2, y2, z1, z2;
	x1 = p1.x;
	y1 = p1.y;
	x2 = p2.x;
	y2 = p2.y;
	z1 = p1.z;
	z2 = p2.z;

	float dx = x2 - x1;
	float dy = y2 - y1;
	float length = std::sqrt(dx * dx + dy * dy);
	float angle = std::atan2(dy, dx);

	//Output::Debug(". {}", length);

	for (float i = 0; i < length; i++) {
		float zz = z1 + std::tanf(angle) * i;

		float f = i / length;
		zz = lerp(z1, z2, f);

		pixel(x1 + std::cosf(angle) * i, y1 + std::sin(angle) * i, zz, p1.color);
		// Output::Debug("..");
	}
}

// Fonction pour calculer le centroïde du polygone
Spriteset_MapDoom::Point Spriteset_MapDoom::computeCentroid(const std::vector<Spriteset_MapDoom::Point>& points) {
	int x_sum = 0, y_sum = 0;
	for (const auto& p : points) {
		x_sum += p.x;
		y_sum += p.y;
	}
	Point p;
	p.x = x_sum / static_cast<int>(points.size());
	p.y = y_sum / static_cast<int>(points.size());
	return p;
}

// Fonction pour trier les points en fonction de leur angle par rapport au centroïde
void Spriteset_MapDoom::sortPoints(std::vector<Spriteset_MapDoom::Point>& points) {
	Point centroid = computeCentroid(points);
	std::sort(points.begin(), points.end(), [centroid](const Point& a, const Point& b) {
		return std::atan2(a.y - centroid.y, a.x - centroid.x) < std::atan2(b.y - centroid.y, b.x - centroid.x);
		});
}

void Spriteset_MapDoom::drawPolygon(std::vector<Spriteset_MapDoom::Point> vertices) {

	if (vertices.size() < 3) return;  // Pas un polygone valide

	Color color = Color(255, 255, 0, 255);

	color = vertices[0].color;

	int z = 0;
	float z_sum = 0.0;

	// Additionner toutes les valeurs de Z
	for (const auto& point : vertices) {
		z_sum += point.z;
	}

	// Calculer la moyenne
	z = z_sum / vertices.size();

	// Trouver les bornes verticales (y_min et y_max)
	int y_min = vertices[0].y;
	int y_max = vertices[0].y;
	for (const auto& p : vertices) {
		y_min = std::min(y_min, (int) p.y);
		y_max = std::max(y_max, (int) p.y);
	}

	// Pour chaque ligne horizontale (scanline), déterminer les vertices d'intersection
	for (int y = y_min; y <= y_max; ++y) {
		std::vector<int> intersections;

		// Trouver les vertices d'intersection entre la ligne de balayage et les arêtes du polygone
		for (size_t i = 0; i < vertices.size(); ++i) {
			Point p1 = vertices[i];
			Point p2 = vertices[(i + 1) % vertices.size()];  // Cycle pour connecter le dernier point au premier

			// Vérifier si l'arête coupe la ligne de balayage
			if ((p1.y <= y && p2.y > y) || (p2.y <= y && p1.y > y)) {
				// Calculer le point d'intersection en x par interpolation linéaire
				int x = p1.x + (y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
				intersections.push_back(x);
			}
		}

		// Trier les intersections pour dessiner les segments horizontaux
		std::sort(intersections.begin(), intersections.end());

		// Remplir les pixels entre chaque paire de vertices d'intersection
		for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
			int x_start = intersections[i];
			int x_end = intersections[i + 1];
			// TODO Remplacer pr la fonction line ?
			for (int x = x_start; x <= x_end; ++x) {
				pixel(x, y, z, color);  // Dessiner les pixels dans l'intervalle
			}
		}
	}
}


float Spriteset_MapDoom::lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

//void Spriteset_MapDoom::rotate(Point& point, float x, float y, float z) {
//
//	float rad = 0;
//
//	rad = x;
//	point.y = std::cos(rad) * point.y - std::sin(rad) * point.z;
//	point.z = std::sin(rad) * point.y + std::cos(rad) * point.z;
//
//	rad = y;
//	point.x = std::cos(rad) * point.x + std::sin(rad) * point.z;
//	point.z = -std::sin(rad) * point.x + std::cos(rad) * point.z;
//
//	rad = z;
//	point.x = std::cos(rad) * point.x - std::sin(rad) * point.y;
//	point.y = std::sin(rad) * point.x + std::cos(rad) * point.y;
//
//}

void Spriteset_MapDoom::rotate(Point& point, float x, float y, float z) {
	float rad;

	// Variables temporaires pour stocker les nouvelles valeurs
	float new_x, new_y, new_z;

	// Rotation autour de l'axe X
	rad = x;
	new_y = std::cos(rad) * point.y - std::sin(rad) * point.z;
	new_z = std::sin(rad) * point.y + std::cos(rad) * point.z;

	point.y = new_y;
	point.z = new_z;

	// Rotation autour de l'axe Y
	rad = y;
	new_x = std::cos(rad) * point.x + std::sin(rad) * point.z;
	new_z = -std::sin(rad) * point.x + std::cos(rad) * point.z;

	point.x = new_x;
	point.z = new_z;

	// Rotation autour de l'axe Z
	rad = z;
	new_x = std::cos(rad) * point.x - std::sin(rad) * point.y;
	new_y = std::sin(rad) * point.x + std::cos(rad) * point.y;

	point.x = new_x;
	point.y = new_y;
}


bool Spriteset_MapDoom::comparePoints(const Point& p1, const Point& p2) {
	return p1.z < p2.z;
}

void Spriteset_MapDoom::Show() {

	//Output::Debug("Start render");

	sprite->Clear();
	spriteUpper->Clear();

	float zmin = 0;
	float zmax = 0;
	bool f = true;
	for (const auto& p : points) {
		//Output::Debug(" {}", p.z);
		if (f) {
			zmax = p.z;
			f = false;
		}
		else {
			zmin = std::max(zmin, p.z);
			zmax = std::min(zmax, p.z);
		}
	}

	std::sort(points.begin(), points.end(), std::greater<Point>());

	bool pointsZB[999][999] = { false };

	Rect r;
	Color c = Color(255,0,0,255);
	for (auto p : points) {

		//Output::Debug("AZE");

		int i = p.x + 999 / 2;
		int j = p.y + 999 / 2;

		if (p.exist && !pointsZB[i][j])
		{

			pointsZB[i][j] = true;

			r = Rect(p.x + displayX + Player::screen_width / 2, Player::screen_height - (p.y + displayY + Player::screen_height / 2), 1, 1);

			int mult = ((p.z - zmin) / (zmax - zmin)) * 100;

			int red, green, blue;
			red = p.color.red;
			green = p.color.green;
			blue = p.color.blue;

			// Output::Debug(" {} {} {}", red, green, blue);

			c = Color(red * mult / 100, green * mult / 100, blue * mult / 100, 255);

			sprite->FillRect(r, c);

			if (p.upper) {
				c = Color(255, 0, 0, 255);
				spriteUpper->FillRect(r, c);
			}
		}
	}


	//Output::Debug("Rendered");
}
