/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 *
 * Based on DynRPG Particle Effects by Kazesui. (MIT license)
 */

// Headers
#include <array>
#include <cmath>
#include <map>

#include "async_handler.h"
#include "drawable.h"
#include "drawable_mgr.h"
#include "dynrpg_particle.h"
#include "baseui.h"
#include "bitmap.h"
#include "cache.h"
#include "game_screen.h"
#include "game_pictures.h"
#include "game_map.h"
#include "game_switches.h"
#include "main_data.h"
#include "graphics.h"

//	static void load() = 0;

// Lowest Z-order is drawn above. wtf
// Follows the logic of RPGSS to prevent confusion
constexpr Drawable::Z_t layer_mask = (5 << 16);
constexpr Drawable::Z_t default_priority = Priority_Timer + layer_mask;

class ParticleEffect;

namespace {
	typedef std::map<std::string, ParticleEffect*> ptag_t;

	ptag_t pfx_list;
}

void linear_fade(ParticleEffect* effect, uint32_t color0, uint32_t color1, int fade, int delay);
void linear_fade_texture(uint32_t color0, uint32_t color1, int fade, int delay, uint8_t* dst_r, uint8_t* dst_g, uint8_t* dst_b);



class ParticleEffect : public Drawable {
public:
	ParticleEffect();
	void Draw(Bitmap& dst) override {};
	virtual void clear() {};
	virtual void setSimul(int newSimul) {};
	virtual void setAmount(int newAmount);
	void setAngle(float v1, float v2);
	void setSecondaryAngle(float v);
	virtual void setTimeout(int fade, int delay);
	void setRad(int new_rad);
	void setSpd(float new_spd);
	void setGrowth(float ini_size, float end_size);
	void setRandRad(int new_rnd_rad);
	void setRandSpd(float new_rnd_spd);
	void setRandPos(int new_rnd_x, int new_rnd_y);
	void setInterval(uint32_t new_interval);
	virtual void setTexture(std::string filename);
	virtual void unloadTexture();
	void useScreenRelative(bool enabled);
	virtual void setGeneratingFunction(std::string type) {}
	void setGravityDirection(float angle, float factor);
	void setAccelerationPoint(float x, float y, float factor);
	void setColor0(uint8_t r, uint8_t g, uint8_t b);
	void setColor1(uint8_t r, uint8_t g, uint8_t b);

	static void create_trig_lut();

	std::array<Color, 256> palette;

protected:

	enum ColoringMode {
		LINEAR,
		LINEAR_TEXTURE
	};
	ColoringMode col_mode;
	bool isScreenRelative;

	int r0;
	int rand_r;
	int rand_x;
	int rand_y;

	float spd;
	float rand_spd;

	float s0;
	float s1;
	float ds;
	float da;

	float gx;
	float gy;

	float ax0;
	float ay0;
	float afc;

	uint8_t* r;
	uint8_t* g;
	uint8_t* b;
	BitmapRef image;
	BitmapRef tone_image;

	float beta;
	float alpha;
	float theta;
	uint8_t fade;
	uint8_t delay;
	uint16_t amount;
	uint32_t color0;
	uint32_t color1;
	uint32_t interval = 1;
	uint32_t cur_interval = 1;

	void free_rgb();
	void alloc_rgb();
	void update_color();
	static float sin_lut[32];
};

void linear_fade(ParticleEffect* effect, uint32_t color0, uint32_t color1, int fade, int delay) {
	std::array<Color, 256> palette;
	float r = (color0 >> 16) & 0xff;
	float g = (color0 >> 8) & 0xff;
	float b = (color0 & 0xff);
	if (delay >= fade) delay = fade - 1;

	float dr, dg, db;
	{
		float end_r = (color1 >> 16) & 0xff;
		float end_g = (color1 >> 8) & 0xff;
		float end_b = (color1 & 0xff);

		dr = (end_r - r) / (fade - delay);
		dg = (end_g - g) / (fade - delay);
		db = (end_b - b) / (fade - delay);
	}

	int i = 0;
	for (; i < delay; ++i) {
		palette[i] = Color(r, g, b, 255);
	}
	for (; i < fade; ++i) {
		palette[i] = Color(r, g, b, 255);
		r += dr;
		g += dg;
		b += db;
	}

	effect->palette = palette;
}

void linear_fade_texture(uint32_t color0, uint32_t color1, int fade, int delay, uint8_t* dst_r, uint8_t* dst_g, uint8_t* dst_b) {
	float src_r = (color0 >> 16) & 0xff;
	float src_g = (color0 >> 8) & 0xff;
	float src_b = (color0 & 0xff);
	if (delay >= fade) delay = fade - 1;

	float dr, dg, db;
	{
		float end_r = (color1 >> 16) & 0xff;
		float end_g = (color1 >> 8) & 0xff;
		float end_b = (color1 & 0xff);

		dr = (end_r - src_r) / (fade - delay);
		dg = (end_g - src_g) / (fade - delay);
		db = (end_b - src_b) / (fade - delay);
	}

	int i = 0;
	for (; i < delay; ++i) {
		dst_r[i] = src_r;
		dst_g[i] = src_g;
		dst_b[i] = src_b;
	}
	for (; i < fade; ++i) {
		dst_r[i] = src_r;
		dst_g[i] = src_g;
		dst_b[i] = src_b;
		src_r += dr;
		src_g += dg;
		src_b += db;
	}
}

float ParticleEffect::sin_lut[32];

ParticleEffect::ParticleEffect() : Drawable(default_priority), r0(50), rand_r(0), rand_x(0), rand_y(0), spd(0.5), rand_spd(0.5),
s0(1), s1(1), ds(0), gx(0), gy(0), ax0(0), ay0(0), afc(0), beta(6.2832),
alpha(0), theta(0), fade(30), delay(0), amount(50) {
	r = nullptr;
	g = nullptr;
	b = nullptr;
	da = 255 / fade;
	col_mode = LINEAR;
	color0 = 0x00ffffff;
	color1 = 0x00ffffff;
	isScreenRelative = false;

	image = Bitmap::Create(1, 1, true);
     tone_image = Bitmap::Create(image->GetWidth(), image->GetHeight(), true);


	DrawableMgr::Register(this);
}

void ParticleEffect::setTexture(std::string filename) {
	// When the name ends with .png, remove it
	if (StringView(filename).ends_with(".png")) {
		filename = filename.substr(0, filename.length() - 4);
	}
	FileRequestAsync* req = AsyncHandler::RequestFile("Picture", filename);
	req->Start();
	image = Cache::Picture(filename, true);
	tone_image = Bitmap::Create(image->GetWidth(), image->GetHeight(), true);
	linear_fade_texture(color0, color1, fade, delay, r, g, b);
}

void ParticleEffect::unloadTexture() {
	linear_fade(this, color0, color1, fade, delay);
}

void ParticleEffect::setGravityDirection(float angle, float factor) {
	angle *= 0.0174532925;
	gx = factor * cosf(angle) / 600.0;
	gy = factor * sinf(angle) / 600.0;
}

void ParticleEffect::setAccelerationPoint(float x, float y, float factor) {
	afc = factor / 600.0;
	ax0 = x;
	ay0 = y;
}

void ParticleEffect::setGrowth(float ini_size, float end_size) {
	s0 = ini_size;
	s1 = end_size;
	ds = (s1 - s0) / fade;
}

void ParticleEffect::useScreenRelative(bool enabled) {
	isScreenRelative = enabled;
}

void ParticleEffect::setAmount(int newAmount) {
	amount = newAmount;
}

void ParticleEffect::setAngle(float v1, float v2) {
	v1 *= 0.0174532925;
	v2 *= 0.0174532925;
	beta = (v2 < 0) ? -v2 : v2;
	alpha = v1 - v2 / 2;
}

void ParticleEffect::setSecondaryAngle(float v) {
	while (v >  360) v -= 360;
	while (v < -360) v += 360;
	theta = v * 0.0174532925;
}

void ParticleEffect::setSpd(float new_spd) {
	spd = new_spd / 60.0;
}

void ParticleEffect::setRandSpd(float new_rnd_spd) {
	rand_spd = new_rnd_spd / 60.0;
}

void ParticleEffect::setRad(int new_rad) {
	r0 = new_rad;
}

void ParticleEffect::setRandPos(int new_rnd_x, int new_rnd_y) {
	rand_x = (new_rnd_x < 0) ? -new_rnd_x : new_rnd_x;
	rand_y = (new_rnd_y < 0) ? -new_rnd_y : new_rnd_y;
}

void ParticleEffect::setRandRad(int new_rnd_rad) {
	rand_r = (new_rnd_rad < 0) ? -new_rnd_rad : new_rnd_rad;
}

void ParticleEffect::setTimeout(int fade, int delay) {
	if (fade > 255) fade = 255;
	else if (fade < 0) fade = 1;
	if (delay >= fade) delay = fade - 1;
	else if (delay < 0) delay = 0;
	this->fade = fade;
	this->delay = delay;
	//da = 255 / ( fade - delay );
	da = 255 / fade;
	ds = (s1 - s0) / fade;
	if (r) { free_rgb(); alloc_rgb(); }
	update_color();
}

void ParticleEffect::setColor0(uint8_t r, uint8_t g, uint8_t b) {
	color0 = (r << 16) | (g << 8) | b;
	update_color();
}

void ParticleEffect::setColor1(uint8_t r, uint8_t g, uint8_t b) {
	color1 = (r << 16) | (g << 8) | b;
	update_color();
}

void ParticleEffect::setInterval(uint32_t new_interval) {
	if (new_interval < 1) {
		return;
	}
	cur_interval = new_interval;
	interval = new_interval;
}

void ParticleEffect::update_color() {
	switch (col_mode) {
	case LINEAR:
		linear_fade(this, color0, color1, fade, delay);
		break;
	case LINEAR_TEXTURE:
		linear_fade_texture(color0, color1, fade, delay, r, g, b);
		break;
	}
}

void ParticleEffect::free_rgb() {
	if (r) {
		free(r); r = NULL;
		free(g); g = NULL;
		free(b); b = NULL;
	}
}

void ParticleEffect::alloc_rgb() {
	if (!r) {
		r = (uint8_t*)malloc(sizeof(uint8_t) * fade + 1 * fade);
		g = (uint8_t*)malloc(sizeof(uint8_t) * fade + 1 * fade);
		b = (uint8_t*)malloc(sizeof(uint8_t) * fade + 1 * fade);
	}
}

void ParticleEffect::create_trig_lut() {
	double dr = 3.141592653589793 / 16.0;
	for (int i = 0; i < 32; i++)
		sin_lut[i] = sin(dr * i);
}


class Stream : public ParticleEffect {
public:
	Stream();
	~Stream();
	void Draw(Bitmap& dst) override;
	void clear() override;
	void stopAll();
	void stop(std::string tag);
	void start(int x, int y, std::string tag);

	void unloadTexture() override;
	void setSimul(int newSimul) override;
	void setAmount(int newAmount) override;
	void setTimeout(int fade, int delay) override;
	void setTexture(std::string filename) override;
	void setGeneratingFunction(std::string type) override;
	void setPosition(std::string tag, int x, int y);


private:
	// Drawing Counters
	uint8_t simulBeg;
	uint8_t simulRun;
	uint8_t simulCnt;
	uint16_t simulMax;

	// SOA Style
	float* x;
	float* y;
	float* s;
	float* dx;
	float* dy;
	uint8_t* itr;
	int16_t* str_x;
	int16_t* str_y;
	uint8_t* pfx_ref;
	uint8_t* end_cnt;

	// Stream tags ( for different stream of same type )
	std::map<std::string, int> pfx_tag;

	void resize();
	void sort_pfx();
	void free_mem();
	void alloc_mem();
	void stream_to_end(uint8_t idx);
	void start_to_stream(uint8_t idx);

	void (Stream::*init)(int, int, int);
	void (Stream::*draw_block)(Bitmap& dst, int, uint8_t, uint8_t, uint8_t, int16_t, int16_t);

	void init_basic(int a, int b, int idx);
	void init_radial(int a, int b, int idx);
	void draw_block_basic(Bitmap& dst, int ref, uint8_t n, uint8_t z, uint8_t c0, int16_t cam_x, int16_t cam_y);
	void draw_block_texture(Bitmap& dst, int ref, uint8_t n, uint8_t z, uint8_t c0, int16_t cam_x, int16_t cam_y);
};


Stream::Stream() : ParticleEffect(), simulBeg(0), simulRun(0), simulCnt(0), simulMax(1) {
	amount = 10;
	alloc_mem();
	init = &Stream::init_basic;
	draw_block = &Stream::draw_block_basic;
	update_color();
}

Stream::~Stream() {
	free_mem();
	free_rgb();
}

void Stream::start(int x0, int y0, std::string tag) {
	std::map<std::string, int>::iterator pfx_itr = pfx_tag.find(tag);
	if (pfx_itr != pfx_tag.end()) return;
	if (simulCnt >= simulMax) resize();
	uint8_t idx = pfx_ref[simulCnt];
	uint8_t tmp;

	tmp = pfx_ref[simulCnt];
	pfx_ref[simulCnt] = pfx_ref[simulRun];
	pfx_ref[simulRun] = tmp;

	tmp = pfx_ref[simulRun];
	pfx_ref[simulRun] = pfx_ref[simulBeg];
	pfx_ref[simulBeg] = tmp;

	std::pair<std::string, int> newItem(tag, idx);
	pfx_tag.insert(newItem);

	end_cnt[idx] = fade - 1;
	str_x[idx] = x0;
	str_y[idx] = y0;
	itr[idx] = 0;
	simulBeg++;
	simulRun++;
	simulCnt++;
}

void Stream::stop(std::string tag) {
	std::map<std::string, int>::iterator pfx_itr = pfx_tag.find(tag);
	if (pfx_itr == pfx_tag.end()) return;
	uint8_t i, probe = pfx_itr->second;
	for (i = 0; i < simulCnt; i++) if (pfx_ref[i] == probe) break;
	simulRun--;
	uint8_t tmp = pfx_ref[i];
	pfx_ref[i] = pfx_ref[simulRun];
	pfx_ref[simulRun] = tmp;
	pfx_tag.erase(pfx_itr);
}

void Stream::stopAll() {
	simulBeg = 0;
	simulRun = 0;
	pfx_tag.clear();
}

void Stream::clear() {
	simulBeg = 0;
	simulRun = 0;
	simulCnt = 0;
	pfx_tag.clear();
}

void Stream::setGeneratingFunction(std::string type) {
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);
	if (!type.substr(0, 8).compare("standard")) { init = &Stream::init_basic; return; }
	if (!type.substr(0, 6).compare("radial")) { init = &Stream::init_radial; return; }
}

void Stream::init_basic(int a, int b, int idx) {
	float x0 = str_x[idx];
	float y0 = str_y[idx];
	for (int i = a; i < b; i++) {
		x[i] = x0 + 2 * rand_x * (float)rand() / RAND_MAX - rand_x;
		y[i] = y0 + 2 * rand_y * (float)rand() / RAND_MAX - rand_y;
		s[i] = s0;

		float tmp_angle = (float)rand() / RAND_MAX * beta + alpha;
		float tmp_spd = spd + rand_spd * (float)rand() / RAND_MAX;
		int v = tmp_angle / 0.1963495408;
		tmp_angle = (tmp_angle - v * 0.1963495408) / 0.1963495408;
		dx[i] = tmp_spd * (sin_lut[(v + 9) & 31] * tmp_angle + sin_lut[(v + 8) & 31] * (1 - tmp_angle));
		dy[i] = tmp_spd * (sin_lut[(v + 1) & 31] * tmp_angle + sin_lut[(v + 0) & 31] * (1 - tmp_angle));
	}
}

void Stream::init_radial(int a, int b, int idx) {
	float x0 = str_x[idx];
	float y0 = str_y[idx];
	for (int i = a; i < b; i++) {
		float tmp_rnd = rand_r * (float)rand() / RAND_MAX;
		float tmp_angle = (float)rand() / RAND_MAX * beta + alpha;
		float tmp_spd = spd + rand_spd * (float)rand() / RAND_MAX;
		int   v = tmp_angle / 0.1963495408;
		float p = (tmp_angle - v * 0.1963495408) / 0.1963495408;

		x[i] = x0 + (r0 + tmp_rnd) * (sin_lut[(v + 9) & 31] * p + sin_lut[(v + 8) & 31] * (1 - p));
		y[i] = y0 + (r0 + tmp_rnd) * (sin_lut[(v + 1) & 31] * p + sin_lut[(v + 0) & 31] * (1 - p));
		s[i] = s0;

		v = (tmp_angle + theta) / 0.1963495408;
		p = (tmp_angle + theta - v * 0.1963495408) / 0.1963495408;
		dx[i] = -tmp_spd * (sin_lut[(v + 9) & 31] * p + sin_lut[(v + 8) & 31] * (1 - p));
		dy[i] = -tmp_spd * (sin_lut[(v + 1) & 31] * p + sin_lut[(v + 0) & 31] * (1 - p));
	}
}

void Stream::draw_block_basic(Bitmap& dst, int ref, uint8_t n, uint8_t z, uint8_t c0, int16_t cam_x, int16_t cam_y) {
	if (!image.get()) {
		return;
	}

	for (uint8_t i = 0; i < n; i++) {
		image->Fill(palette[i + c0]);
		int idx = ref + z * amount;
		float tx, ty, tsqr;
		for (int j = 0; j < amount; j++) {
			x[idx] += dx[idx];
			y[idx] += dy[idx];
			tx = ax0 - x[idx];
			ty = ay0 - y[idx];
			tsqr = sqrtf(tx*tx + ty*ty + 0.001);
			dx[idx] += gx + afc * tx / tsqr;
			dy[idx] += gy + afc * ty / tsqr;
			s[idx] += ds;
			Rect dst_rect(x[idx] - cam_x - s[idx] / 2, y[idx] - cam_y - s[idx] / 2, s[idx], s[idx]);
			dst.StretchBlit(dst_rect, *image, image->GetRect(), Opacity::Opaque());
			idx++;
		}
		z = (z + 1) % fade;
	}

}

void Stream::draw_block_texture(Bitmap& dst, int ref, uint8_t n, uint8_t z, uint8_t c0, int16_t cam_x, int16_t cam_y) {
	if (!image.get()) {
		return;
	}

	float w = image->width();
	float h = image->height();
	for (uint8_t i = 0; i < n; i++) {
		// FIXME: Order is bgr instead of rgb
		Tone tone(b[i + c0], g[i + c0], r[i + c0], 128);
		int alpha = ( 255 - da * ( i + c0 ) );
		tone_image->ToneBlit(0, 0, *image, image->GetRect(), tone, Opacity::Opaque());

		int idx = ref + z * amount;
		float tx, ty, tsqr;
		for (int j = 0; j < amount; j++) {
			x[idx] += dx[idx];
			y[idx] += dy[idx];
			tx = ax0 - x[idx];
			ty = ay0 - y[idx];
			tsqr = sqrtf(tx*tx + ty*ty + 0.001);
			dx[idx] += gx + afc * tx / tsqr;
			dy[idx] += gy + afc * ty / tsqr;
			s[idx] += ds;
			Rect dst_rect(x[idx] - cam_x - s[idx] / 2, y[idx] - cam_y - s[idx] / 2, w*s[idx], h*s[idx]);
			dst.StretchBlit(dst_rect, *tone_image, tone_image->GetRect(), Opacity::Opaque());
			idx++;
		}
		z = (z + 1) % fade;
	}
}

void Stream::setPosition(std::string tag, int x, int y) {
	std::map<std::string, int>::iterator pfx_itr = pfx_tag.find(tag);
	if (pfx_itr == pfx_tag.end()) return;
	uint8_t i, probe = pfx_itr->second;
	for (i = 0; i < simulCnt; i++) if (pfx_ref[i] == probe) break;
	str_x[i] = x;
	str_y[i] = y;
}

void Stream::setTexture(std::string filename) {
	FileRequestAsync* req = AsyncHandler::RequestFile("Picture", filename);
	req->Start();
	alloc_rgb();
	image = Cache::Picture(filename, true);
	tone_image = Bitmap::Create(image->GetWidth(), image->GetHeight(), true);
	draw_block = &Stream::draw_block_texture;
	col_mode = LINEAR_TEXTURE;
	update_color();
}

void Stream::unloadTexture() {
	free_rgb();
	draw_block = &Stream::draw_block_basic;
	col_mode = LINEAR;
	update_color();
}

void Stream::Draw(Bitmap& dst) {
	if (simulCnt <= 0) return;
	int cam_x = (isScreenRelative) ? 0 : Game_Map::GetDisplayX() / 16;
	int cam_y = (isScreenRelative) ? 0 : Game_Map::GetDisplayY() / 16;
	int block_size = amount * fade;
	int i = 0;

	--cur_interval;

	/// Starting
	for (; i < simulBeg; i++) {
		uint8_t idx = pfx_ref[i];
		if (itr[idx] < fade) {
			uint8_t z = fade - itr[idx]++ - 1;
			if (cur_interval == 0) {
				(this->*init)(z * amount + idx * block_size, (z + 1) * amount + idx * block_size, idx);
			}
			(this->*draw_block)(dst, idx * block_size, itr[idx], z, 0, cam_x, cam_y);
		} else start_to_stream(i--);
	}
	/// Streaming
	for (; i < simulRun; i++) {
		uint8_t idx = pfx_ref[i];
		uint8_t z = fade - itr[idx] - 1;
		itr[idx] = (itr[idx] + 1) % fade;
		if (cur_interval == 0) {
			(this->*init)(z * amount + idx * block_size, (z + 1) * amount + idx * block_size, idx);
		}
		(this->*draw_block)(dst, idx * block_size, fade, z, 0, cam_x, cam_y);
	}
	/// Stopping
	for (; i < simulCnt; i++) {
		uint8_t idx = pfx_ref[i];
		uint8_t z = (fade - itr[idx]) % fade;
		(this->*draw_block)(dst, idx * block_size, end_cnt[idx]--, z, fade - end_cnt[idx], cam_x, cam_y);
		if (end_cnt[idx] <= 0)
			stream_to_end(i);
	}

	if (cur_interval == 0) {
		cur_interval = interval;
	}
}

void Stream::resize() {
	float* tmp_x = (float*)malloc(sizeof(float) * amount * fade * 2 * simulMax);
	float* tmp_y = (float*)malloc(sizeof(float) * amount * fade * 2 * simulMax);
	float* tmp_s = (float*)malloc(sizeof(float) * amount * fade * 2 * simulMax);
	float* tmp_dx = (float*)malloc(sizeof(float) * amount * fade * 2 * simulMax);
	float* tmp_dy = (float*)malloc(sizeof(float) * amount * fade * 2 * simulMax);
	uint8_t* tmp_itr = (uint8_t*)malloc(sizeof(uint8_t) * 2 * simulMax);
	int16_t* tmp_srx = (int16_t*)malloc(sizeof(int16_t) * 2 * simulMax);
	int16_t* tmp_sry = (int16_t*)malloc(sizeof(int16_t) * 2 * simulMax);
	uint8_t* tmp_ref = (uint8_t*)malloc(sizeof(uint8_t) * 2 * simulMax);
	uint8_t* tmp_cnt = (uint8_t*)malloc(sizeof(uint8_t) * 2 * simulMax);

	memcpy(tmp_x, x, sizeof(float) * simulMax * amount * fade);
	memcpy(tmp_y, y, sizeof(float) * simulMax * amount * fade);
	memcpy(tmp_s, s, sizeof(float) * simulMax * amount * fade);
	memcpy(tmp_dx, dx, sizeof(float) * simulMax * amount * fade);
	memcpy(tmp_dy, dy, sizeof(float) * simulMax * amount * fade);
	memcpy(tmp_itr, itr, sizeof(uint8_t) * simulMax);
	memcpy(tmp_srx, str_x, sizeof(int16_t) * simulMax);
	memcpy(tmp_sry, str_y, sizeof(int16_t) * simulMax);
	memcpy(tmp_ref, pfx_ref, sizeof(uint8_t) * simulMax);
	memcpy(tmp_cnt, end_cnt, sizeof(uint8_t) * simulMax);
	for (int i = simulMax; i < 2 * simulMax; i++) {
		tmp_itr[i] = 0;
		tmp_srx[i] = -(s1 + 1);
		tmp_sry[i] = -(s1 + 1);
		tmp_cnt[i] = 0;
		tmp_ref[i] = i;
	}
	for (int i = simulMax * amount * fade; i < 2 * simulMax * amount * fade; i++) {
		tmp_x[i] = 0;
		tmp_y[i] = 0;
		tmp_s[i] = 0;
		tmp_dx[i] = 0;
		tmp_dy[i] = 0;
	}
	free_mem();

	x = tmp_x;
	y = tmp_y;
	s = tmp_s;
	dx = tmp_dx;
	dy = tmp_dy;
	itr = tmp_itr;
	str_x = tmp_srx;
	str_y = tmp_sry;
	pfx_ref = tmp_ref;
	end_cnt = tmp_cnt;
	simulMax *= 2;
}

void Stream::setAmount(int newAmount) {
	free_mem();
	amount = newAmount;
	alloc_mem();
}

void Stream::setSimul(int newSimul) {
	free_mem();
	simulMax = newSimul;
	alloc_mem();
	simulBeg = 0;
	simulRun = 0;
	simulCnt = 0;
}

void Stream::setTimeout(int _fade, int _delay) {
	free_mem();
	if (_fade > 255) _fade = 255;
	else if (_fade < 0) _fade = 1;
	if (_delay >= _fade) _delay = _fade - 1;
	else if (_delay < 0) _delay = 0;
	fade = _fade;
	delay = _delay;
	//da = 255 / ( fade - delay );
	da = 255 / _fade;
	ds = (s1 - s0) / _fade;
	alloc_mem();
	if (r) { free_rgb(); alloc_rgb(); }
	update_color();
}

void Stream::free_mem() {
	free(x);
	free(y);
	free(s);
	free(dx);
	free(dy);
	free(itr);
	free(str_x);
	free(str_y);
	free(pfx_ref);
	free(end_cnt);
}

void Stream::alloc_mem() {
	x = (float*)malloc(sizeof(float) * amount * fade * simulMax);
	y = (float*)malloc(sizeof(float) * amount * fade * simulMax);
	s = (float*)malloc(sizeof(float) * amount * fade * simulMax);
	dx = (float*)malloc(sizeof(float) * amount * fade * simulMax);
	dy = (float*)malloc(sizeof(float) * amount * fade * simulMax);
	itr = (uint8_t*)malloc(sizeof(uint8_t) * simulMax);
	str_x = (int16_t*)malloc(sizeof(int16_t) * simulMax);
	str_y = (int16_t*)malloc(sizeof(int16_t) * simulMax);
	pfx_ref = (uint8_t*)malloc(sizeof(uint8_t) * simulMax);
	end_cnt = (uint8_t*)malloc(sizeof(uint8_t) * simulMax);
	for (int i = 0; i < simulMax; i++) {
		itr[i] = 0;
		str_x[i] = -(s1 + 1);
		str_y[i] = -(s1 + 1);
		pfx_ref[i] = i;
		end_cnt[i] = 0;
	}
	for (int i = 0; i < amount * fade * simulMax; i++) {
		x[i] = 0;
		y[i] = 0;
		s[i] = 0;
		dx[i] = 0;
		dy[i] = 0;
	}
}

void Stream::start_to_stream(uint8_t idx) {
	itr[pfx_ref[idx]] = 0;
	int tmp = pfx_ref[--simulBeg];
	pfx_ref[simulBeg] = pfx_ref[idx];
	pfx_ref[idx] = tmp;
	sort_pfx();
}

void Stream::stream_to_end(uint8_t idx) {
	--simulCnt;
	uint8_t temp = pfx_ref[simulCnt];
	pfx_ref[simulCnt] = pfx_ref[idx];
	pfx_ref[idx] = temp;
}

void Stream::sort_pfx() {
	/// General Idea: improve cache efficiency
	/// Not implemented yet
}


class Burst : public ParticleEffect {
public:
	Burst();
	~Burst();
	void Draw(Bitmap& dst) override;
	void clear() override;
	void newBurst(int x, int y);

	void unloadTexture() override;
	void setSimul(int newSimul) override;
	void setAmount(int newAmount) override;
	void setTexture(std::string filename) override;
	void setGeneratingFunction(std::string type) override;

private:

	uint8_t simulCnt;
	uint16_t simulMax;

	// SOA Style
	float* x;
	float* y;
	float* s;
	float* dx;
	float* dy;
	uint8_t* itr;

	void resize();
	void free_mem();
	void alloc_mem();

	void (Burst::*init)(int, int, int, int);
	void (Burst::*draw_function)(Bitmap& dst, int, int);

	void init_basic(int x0, int y0, int a, int b);
	void init_radial(int x0, int y0, int a, int b);
	void draw_texture(Bitmap& dst, int cam_x, int cam_y);
	void draw_standard(Bitmap& dst, int cam_x, int cam_y);
};


Burst::Burst() : ParticleEffect(), simulCnt(0), simulMax(1) {
	alloc_mem();
	init = &Burst::init_basic;
	draw_function = &Burst::draw_standard;
	update_color();
}

Burst::~Burst() {
	free_mem();
	free_rgb();
	//RPG::Image::destroy(image);
}

void Burst::setGeneratingFunction(std::string type) {
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);
	if (!type.substr(0, 8).compare("standard")) { init = &Burst::init_basic; return; }
	if (!type.substr(0, 6).compare("radial")) { init = &Burst::init_radial; return; }
}

void Burst::clear() {
	simulCnt = 0;
}

void Burst::newBurst(int x0, int y0) {
	if (simulCnt >= simulMax) resize();
	itr[simulCnt] = 0;
	(this->*init)(x0, y0, simulCnt * amount, (simulCnt + 1) * amount);
	simulCnt++;
}

void Burst::init_basic(int x0, int y0, int a, int b) {
	for (int i = a; i < b; i++) {
		x[i] = x0 + 2 * rand_x * (float)rand() / RAND_MAX - rand_x;
		y[i] = y0 + 2 * rand_y * (float)rand() / RAND_MAX - rand_y;
		s[i] = s0;

		float tmp_angle = (float)rand() / RAND_MAX * beta + alpha;
		float tmp_spd = spd + rand_spd * (float)rand() / RAND_MAX;
		int v = tmp_angle / 0.1963495408;
		tmp_angle = (tmp_angle - v * 0.1963495408) / 0.1963495408;
		dx[i] = tmp_spd * (sin_lut[(v + 9) & 31] * tmp_angle + sin_lut[(v + 8) & 31] * (1 - tmp_angle));
		dy[i] = tmp_spd * (sin_lut[(v + 1) & 31] * tmp_angle + sin_lut[(v + 0) & 31] * (1 - tmp_angle));
	}
}

void Burst::init_radial(int x0, int y0, int a, int b) {
	for (int i = a; i < b; i++) {
		float tmp_rnd = rand_r * (float)rand() / RAND_MAX;
		float tmp_angle = (float)rand() / RAND_MAX * beta + alpha;
		float tmp_spd = spd + rand_spd * (float)rand() / RAND_MAX;
		int   v = tmp_angle / 0.1963495408;
		float p = (tmp_angle - v * 0.1963495408) / 0.1963495408;

		x[i] = x0 + (r0 + tmp_rnd) * (sin_lut[(v + 9) & 31] * p + sin_lut[(v + 8) & 31] * (1 - p));
		y[i] = y0 + (r0 + tmp_rnd) * (sin_lut[(v + 1) & 31] * p + sin_lut[(v + 0) & 31] * (1 - p));
		s[i] = s0;

		v = (tmp_angle + theta) / 0.1963495408;
		p = (tmp_angle + theta - v * 0.1963495408) / 0.1963495408;
		dx[i] = -tmp_spd * (sin_lut[(v + 9) & 31] * p + sin_lut[(v + 8) & 31] * (1 - p));
		dy[i] = -tmp_spd * (sin_lut[(v + 1) & 31] * p + sin_lut[(v + 0) & 31] * (1 - p));
	}
}

void Burst::Draw(Bitmap& dst) {
	if (simulCnt <= 0) return;
	if (simulCnt > 1) {
		for (int i = 0; i < simulCnt - 1; i++) {
			if (itr[i] >= fade) {
				simulCnt--;
				for (int j = 0; j < amount; j++) {
					x[j + i*amount] = x[j + simulCnt*amount];
					y[j + i*amount] = y[j + simulCnt*amount];
					s[j + i*amount] = s[j + simulCnt*amount];
					dx[j + i*amount] = dx[j + simulCnt*amount];
					dy[j + i*amount] = dy[j + simulCnt*amount];
				}
				itr[i] = itr[simulCnt];
			}
		}
		if (itr[simulCnt - 1] >= fade) simulCnt--;
	} else if (itr[0] >= fade) --simulCnt;

	int cam_x = (isScreenRelative) ? 0 : Game_Map::GetDisplayX() / 16;
	int cam_y = (isScreenRelative) ? 0 : Game_Map::GetDisplayY() / 16;
	(this->*draw_function)(dst, cam_x, cam_y);
}

void Burst::draw_standard(Bitmap& dst, int cam_x, int cam_y) {
	if (!image.get()) {
		return;
	}

	for (int i = 0; i < simulCnt; i++) {
		image->Fill(palette[itr[i]]);
		itr[i]++;
		float tx, ty, tsqr;
		for (int j = i * amount; j < (i + 1) * amount; j++) {
			x[j] += dx[j];
			y[j] += dy[j];
			tx = ax0 - x[j];
			ty = ay0 - y[j];
			tsqr = sqrtf(tx*tx + ty*ty + 0.001);
			dx[j] += gx + afc * tx / tsqr;
			dy[j] += gy + afc * ty / tsqr;
			s[j] += ds;
			Rect dst_rect(x[j] - cam_x - s[j] / 2, y[j] - cam_y - s[j] / 2, s[j], s[j]);
			dst.StretchBlit(dst_rect, *image, image->GetRect(), Opacity::Opaque());
		}
	}
}

void Burst::draw_texture(Bitmap& dst, int cam_x, int cam_y) {
	if (!image.get()) {
		return;
	}

	float w = image->width();
	float h = image->height();
	for (int i = 0; i < simulCnt; i++) {
		uint8_t idx = itr[i];

		// FIXME: Order is bgr instead of rgb
		Tone tone(b[idx], g[idx], r[idx], 128);
		int alpha = ( 255 - da * idx );
		tone_image->Clear();
		tone_image->ToneBlit(0, 0, *image, image->GetRect(), tone, Opacity::Opaque());

		itr[i]++;
		float tx, ty, tsqr;
		for (int j = i * amount; j < (i + 1) * amount; j++) {
			x[j] += dx[j];
			y[j] += dy[j];
			tx = ax0 - x[j];
			ty = ay0 - y[j];
			tsqr = sqrtf(tx*tx + ty*ty + 0.001);
			dx[j] += gx + afc * tx / tsqr;
			dy[j] += gy + afc * ty / tsqr;
			s[j] += ds;
			Rect dst_rect(x[j] - cam_x - s[j] / 2, y[j] - cam_y - s[j] / 2, w*s[j], h*s[j]);
			dst.StretchBlit(dst_rect, *tone_image, tone_image->GetRect(), alpha);
		}
	}
}

void Burst::setTexture(std::string filename) {
	// When the name ends with .png, remove it
	if (StringView(filename).ends_with(".png")) {
		filename = filename.substr(0, filename.length() - 4);
	}
	FileRequestAsync* req = AsyncHandler::RequestFile("Picture", filename);
	req->Start();

	alloc_rgb();
	image = Cache::Picture(filename, true);
	tone_image = Bitmap::Create(image->GetWidth(), image->GetHeight(), true);
	draw_function = &Burst::draw_texture;
	col_mode = LINEAR_TEXTURE;
	update_color();
}

void Burst::unloadTexture() {
	free_rgb();
	draw_function = &Burst::draw_standard;
	col_mode = LINEAR;
	update_color();
}

void Burst::resize() {
	float* tmp_x = (float*)malloc(sizeof(float) * amount * 2 * simulMax);
	float* tmp_y = (float*)malloc(sizeof(float) * amount * 2 * simulMax);
	float* tmp_s = (float*)malloc(sizeof(float) * amount * 2 * simulMax);
	float* tmp_dx = (float*)malloc(sizeof(float) * amount * 2 * simulMax);
	float* tmp_dy = (float*)malloc(sizeof(float) * amount * 2 * simulMax);
	uint8_t* tmp_itr = (uint8_t*)malloc(sizeof(uint8_t) * 2 * simulMax);

	memcpy(tmp_x, x, sizeof(float) * simulMax * amount);
	memcpy(tmp_y, y, sizeof(float) * simulMax * amount);
	memcpy(tmp_s, s, sizeof(float) * simulMax * amount);
	memcpy(tmp_dx, dx, sizeof(float) * simulMax * amount);
	memcpy(tmp_dy, dy, sizeof(float) * simulMax * amount);
	memcpy(tmp_itr, itr, sizeof(uint8_t) * simulMax);
	free_mem();

	x = tmp_x;
	y = tmp_y;
	s = tmp_s;
	dx = tmp_dx;
	dy = tmp_dy;
	itr = tmp_itr;
	simulMax *= 2;
}

void Burst::setAmount(int newAmount) {
	free_mem();
	amount = newAmount;
	alloc_mem();
}

void Burst::setSimul(int newSimul) {
	free_mem();
	simulMax = newSimul;
	alloc_mem();
	simulCnt = 0;
}

void Burst::free_mem() {
	free(x);
	free(y);
	free(s);
	free(dx);
	free(dy);
	free(itr);
}

void Burst::alloc_mem() {
	x = (float*)malloc(sizeof(float) * amount * simulMax);
	y = (float*)malloc(sizeof(float) * amount * simulMax);
	dx = (float*)malloc(sizeof(float) * amount * simulMax);
	dy = (float*)malloc(sizeof(float) * amount * simulMax);
	s = (float*)malloc(sizeof(float) * amount * simulMax);
	itr = (uint8_t*)malloc(sizeof(uint8_t) * simulMax);
}

static bool create_effect(dyn_arg_list args) {
	auto func = "pfx_create_effect";
	bool okay;
	std::string tag, type;
	std::tie(tag, type) = DynRpg::ParseArgs<std::string, std::string>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr == pfx_list.end()) {
		std::transform(type.begin(), type.end(), type.begin(), ::tolower);
		if (!type.substr(0, 5).compare("burst")) pfx_list[tag] = new Burst();
		else if (!type.substr(0, 6).compare("stream")) pfx_list[tag] = new Stream();
	}

	return true;
}


static bool destroy_effect(dyn_arg_list args) {
	auto func = "pfx_destroy_effect";
	bool okay;
	auto [tag] = DynRpg::ParseArgs<std::string>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		delete itr->second;
		pfx_list.erase(itr);
	}
	return true;
}

static bool destroy_all(dyn_arg_list args) {
    	auto func = "pfx_destroy_all";
	ptag_t::iterator itr = pfx_list.begin();
	while (itr != pfx_list.end()) {
		delete itr->second;
		itr++;
	}
	pfx_list.clear();
	return true;
}

static bool does_effect_exist(dyn_arg_list args) {
	auto func = "pfx_does_effect_exist";
	bool okay;
	std::string tag;
	int idx;
	std::tie(tag, idx) = DynRpg::ParseArgs<std::string, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		Main_Data::game_switches->Set(idx, true);
	} else {
		Main_Data::game_switches->Set(idx, false);
	}

	Game_Map::SetNeedRefresh(true);
	return true;
}

static bool burst(dyn_arg_list args) {
	auto func = "pfx_burst";
	bool okay;
	std::string tag;
	int idx, idx2;
	std::tie(tag, idx, idx2) = DynRpg::ParseArgs<std::string, int, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		((Burst*)itr->second)->newBurst(idx, idx2);
	}
	return true;
}

static bool start(dyn_arg_list args) {
	auto func = "pfx_start";
	bool okay;
	std::string tag1, tag2;
	int idx, idx2;
	std::tie(tag1, tag2, idx, idx2) = DynRpg::ParseArgs<std::string, std::string, int, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag1);
	if (itr != pfx_list.end()) {
		((Stream*)itr->second)->start(idx, idx2, tag2);
	}
	return true;
}

static bool stop(dyn_arg_list args) {
	auto func = "pfx_stop";
	bool okay;
	std::string tag1, tag2;
	std::tie(tag1, tag2) = DynRpg::ParseArgs<std::string, std::string>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag1);
	if (itr != pfx_list.end()) {
		((Stream*)itr->second)->stop(tag2);
	}
	return true;
}

static bool stopall(dyn_arg_list args) {
	auto func = "pfx_stopall";
	bool okay;
	auto [tag] = DynRpg::ParseArgs<std::string>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		((Stream*)itr->second)->stopAll();
	}
	return true;
}

static bool set_simul(dyn_arg_list args) {
	auto func = "pfx_set_simul";
	bool okay;
	std::string tag;
	int idx;
	std::tie(tag, idx) = DynRpg::ParseArgs<std::string, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setSimul(idx);
	}
	return true;
}

static bool set_amount(dyn_arg_list args) {
	auto func = "pfx_set_amount";
	bool okay;
	std::string tag;
	int idx;
	std::tie(tag, idx) = DynRpg::ParseArgs<std::string, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setAmount(idx);
	}
	return true;
}

static bool set_timeout(dyn_arg_list args) {
	auto func = "pfx_set_timeout";
	bool okay;
	std::string tag;
	int val, val2;
	std::tie(tag, val, val2) = DynRpg::ParseArgs<std::string, int, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setTimeout(val, val2);
	}
	return true;
}

static bool set_initial_color(dyn_arg_list args) {
	auto func = "pfx_set_initial_color";
	bool okay;
	std::string tag;
	int val, val2, val3;
	std::tie(tag, val, val2, val3) = DynRpg::ParseArgs<std::string, int, int, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setColor0(val, val2, val3);
	}
	return true;
}

static bool set_final_color(dyn_arg_list args) {
	auto func = "pfx_set_final_color";
	bool okay;
	std::string tag;
	int val, val2, val3;
	std::tie(tag, val, val2, val3) = DynRpg::ParseArgs<std::string, int, int, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setColor1(val, val2, val3);
	}
	return true;
}

static bool set_growth(dyn_arg_list args) {
	auto func = "pfx_set_growth";
	bool okay;
	std::string tag;
	float val, val2;
	std::tie(tag, val, val2) = DynRpg::ParseArgs<std::string, float, float>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setGrowth(val, val2);
	}
	return true;
}

static bool set_position(dyn_arg_list args) {
	auto func = "pfx_set_position";
	bool okay;
	std::string tag1, tag2;
	int val, val2;
	std::tie(tag1, tag2, val, val2) = DynRpg::ParseArgs<std::string, std::string, int, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag1);
	if (itr != pfx_list.end()) {
		((Stream*)itr->second)->setPosition(tag2, val, val2);
	}
	return true;
}

static bool set_random_position(dyn_arg_list args) {
	auto func = "pfx_set_random_position";
	bool okay;
	std::string tag;
	int val, val2;
	std::tie(tag, val, val2) = DynRpg::ParseArgs<std::string, int, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setRandPos(val, val2);
	}
	return true;
}

static bool set_random_radius(dyn_arg_list args) {
	auto func = "pfx_set_random_radius";
	bool okay;
	std::string tag;
	int radius;
	std::tie(tag, radius) = DynRpg::ParseArgs<std::string, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setRandRad(radius);
	}
	return true;
}

static bool set_radius(dyn_arg_list args) {
	auto func = "pfx_set_radius";
	bool okay;
	std::string tag;
	int radius;
	std::tie(tag, radius) = DynRpg::ParseArgs<std::string, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setRad(radius);
	}
	return true;
}

static bool set_texture(dyn_arg_list args) {
	auto func = "pfx_set_texture";
	bool okay;
	std::string tag, txt;
	std::tie(tag, txt) = DynRpg::ParseArgs<std::string, std::string>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setTexture(txt);
	}
	return true;
}

static bool set_acceleration_point(dyn_arg_list args) {
	auto func = "pfx_set_acceleration_point";
	bool okay;
	std::string tag;
	float val, val2, val3;
	std::tie(tag, val, val2, val3) = DynRpg::ParseArgs<std::string, float, float, float>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setAccelerationPoint(val, val2, val3);
	}
	return true;
}

static bool set_gravity_direction(dyn_arg_list args) {
	auto func = "pfx_set_gravity_direction";
	bool okay;
	std::string tag;
	float val, val2;
	std::tie(tag, val, val2) = DynRpg::ParseArgs<std::string, float, float>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setGravityDirection(val, val2);
	}
	return true;
}

static bool set_velocity(dyn_arg_list args) {
	auto func = "pfx_set_velocity";
	bool okay;
	std::string tag;
	float val, val2;
	std::tie(tag, val, val2) = DynRpg::ParseArgs<std::string, float, float>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setSpd(val);
		itr->second->setRandSpd(val2);
	}
	return true;
}

static bool set_angle(dyn_arg_list args) {
	auto func = "pfx_set_angle";
	bool okay;
	std::string tag;
	float val, val2;
	std::tie(tag, val, val2) = DynRpg::ParseArgs<std::string, float, float>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setAngle(val, val2);
	}
	return true;
}

static bool set_interval(dyn_arg_list args) {
	auto func = "pfx_set_interval";
	bool okay;
	std::string tag;
	int interval;
	std::tie(tag, interval) = DynRpg::ParseArgs<std::string, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setInterval(interval);
	}
	return true;
}

static bool set_secondary_angle(dyn_arg_list args) {
	auto func = "pfx_set_secondary_angle";
	bool okay;
	std::string tag;
	float val;
	std::tie(tag, val) = DynRpg::ParseArgs<std::string, float>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setSecondaryAngle(val);
	}
	return true;
}

static bool set_generating_function(dyn_arg_list args) {
	auto func = "pfx_set_generating_function";
	bool okay;
	std::string tag, genfn;
	std::tie(tag, genfn) = DynRpg::ParseArgs<std::string, std::string>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->setGeneratingFunction(genfn);
	}
	return true;
}

static bool use_screen_relative(dyn_arg_list args) {
	auto func = "pfx_use_screen_relative";
	bool okay;
	std::string tag, bol;
	std::tie(tag, bol) = DynRpg::ParseArgs<std::string, std::string>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		bool value = (bol[0] == 'T' || bol[0] == 't');
		itr->second->useScreenRelative(value);
	}
	return true;
}

static bool unload_texture(dyn_arg_list args) {
	auto func = "pfx_unload_texture";
	bool okay;

	auto [tag] = DynRpg::ParseArgs<std::string>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr != pfx_list.end()) {
		itr->second->unloadTexture();
	}
	return true;
}

static bool load_effect(dyn_arg_list args) {
	//FILE* fp;
	//char attribute[80];
	//fp = fopen(arg[0].text, "r");

	return true;
}

static bool SetZ(dyn_arg_list args) {
	auto func = "pfx_set_z";
	bool okay;
	std::string tag;
	int z;
	std::tie(tag, z) = DynRpg::ParseArgs<std::string, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr == pfx_list.end()) {
		Output::Debug("DynParticle: Particle not found %s", tag.c_str());
		return true;
	}

	int layer_z = itr->second->GetZ() & 0xFFFF0000;

	itr->second->SetZ(layer_z - z);

	return true;
}

static bool SetLayer(dyn_arg_list args) {
	auto func = "pfx_set_layer";
	bool okay;
	std::string tag;
	int layer;
	std::tie(tag, layer) = DynRpg::ParseArgs<std::string, int>(func, args, &okay);
	if (!okay) {
		return true;
	}

	ptag_t::iterator itr = pfx_list.find(tag);
	if (itr == pfx_list.end()) {
		Output::Debug("DynParticle: Particle not found %s", tag.c_str());
		return true;
	}

	int z = 0;

	switch (layer) {
		case 1:
			z = Priority_Background;
			break;
		case 2:
			z = Priority_TilesetBelow;
			break;
		case 3:
			z = Priority_EventsBelow;
			break;
		case 4:
			z = Priority_Player;
			break;
		case 5:
			z = Priority_TilesetAbove;
			break;
		case 6:
			z = Priority_EventsAbove;
			break;
		case 7:
			z = Priority_PictureNew;
			break;
		case 8:
			z = Priority_BattleAnimation;
			break;
		case 9:
			z = Priority_Window;
			break;
		case 10:
			z = Priority_Timer;
			break;
		default:
			z = 0;
	}

	int old_z = itr->second->GetZ() & 0x00FFFFFF;

	itr->second->SetZ(z + old_z);

	return true;
}

// old code - LK

// void DynRpg::Particle::RegisterFunctions() {
//	ParticleEffect::create_trig_lut();
//	DynRpg::RegisterFunction("pfx_destroy_all", destroy_all);
//	DynRpg::RegisterFunction("pfx_create_effect", create_effect);
//	DynRpg::RegisterFunction("pfx_destroy_effect", destroy_effect);
//	DynRpg::RegisterFunction("pfx_does_effect_exist", does_effect_exist);
//	DynRpg::RegisterFunction("pfx_burst", burst);
//	DynRpg::RegisterFunction("pfx_start", start);
//	DynRpg::RegisterFunction("pfx_stop", stop);
//	DynRpg::RegisterFunction("pfx_stopall", stopall);
//	DynRpg::RegisterFunction("pfx_set_simul_effects", set_simul);
//	DynRpg::RegisterFunction("pfx_set_amount", set_amount);
//	DynRpg::RegisterFunction("pfx_set_timeout", set_timeout);
//	DynRpg::RegisterFunction("pfx_set_initial_color", set_initial_color);
//	DynRpg::RegisterFunction("pfx_set_final_color", set_final_color);
//	DynRpg::RegisterFunction("pfx_set_growth", set_growth);
//	DynRpg::RegisterFunction("pfx_set_position", set_position);
//	DynRpg::RegisterFunction("pfx_set_random_position", set_random_position);
//	DynRpg::RegisterFunction("pfx_set_random_radius", set_random_radius);
//	DynRpg::RegisterFunction("pfx_set_radius", set_radius);
//	DynRpg::RegisterFunction("pfx_set_texture", set_texture);
//	DynRpg::RegisterFunction("pfx_set_acceleration_point", set_acceleration_point);
//	DynRpg::RegisterFunction("pfx_set_gravity_direction", set_gravity_direction);
//	DynRpg::RegisterFunction("pfx_set_velocity", set_velocity);
//	DynRpg::RegisterFunction("pfx_set_angle", set_angle);
//	DynRpg::RegisterFunction("pfx_set_interval", set_interval);
//	DynRpg::RegisterFunction("pfx_set_secondary_angle", set_secondary_angle);
//	DynRpg::RegisterFunction("pfx_set_generating_function", set_generating_function);
//	DynRpg::RegisterFunction("pfx_use_screen_relative", use_screen_relative);
//	DynRpg::RegisterFunction("pfx_unload_texture", unload_texture);
//	DynRpg::RegisterFunction("pfx_load_effect", load_effect);
//	DynRpg::RegisterFunction("pfx_set_z", SetZ);
//	DynRpg::RegisterFunction("pfx_set_layer", SetLayer);
// }


bool DynRpg::Particle::Invoke(StringView func, dyn_arg_list args, bool&, Game_Interpreter*)
{
    if (func == "pfx_destroy_all") {
                return destroy_all(args);
                }
    else
    if (func == "pfx_create_effect") {
                return create_effect(args);
                }
    else
      if (func == "pfx_destroy_effect") {
                return destroy_effect(args);
                }
    else
    if (func == "pfx_does_effect_exist") {
                return does_effect_exist(args);
                }
    else
    if (func == "pfx_burst") {
                return burst(args);
                }
    else
    if (func == "pfx_start") {
                return start(args);
                }
    else
      if (func == "pfx_stop") {
                return stop(args);
                }
    else
    if (func == "pfx_stopall") {
                return stopall(args);
                }
    else
    if (func == "pfx_set_simul_effects") {
                return set_simul(args);
                }
    else
    if (func == "pfx_set_amount") {
                return set_amount(args);
                }
    else
      if (func == "pfx_set_timeout") {
                return set_timeout(args);
                }
    else
    if (func == "pfx_set_initial_color") {
                return set_initial_color(args);
                }
    else
    if (func == "pfx_set_final_color") {
                return set_final_color(args);
                }
    else
    if (func == "pfx_set_growth") {
                return set_growth(args);
                }
    else
      if (func == "pfx_set_position") {
                return set_position(args);
                }
    else
    if (func == "pfx_set_random_position") {
                return set_random_position(args);
                }
    else
        if (func == "pfx_set_random_radius") {
                return set_random_radius(args);
                }
    else
    if (func == "pfx_set_radius") {
                return set_radius(args);
                }
    else
    if (func == "pfx_set_texture") {
                return set_texture(args);
                }
    else
      if (func == "pfx_set_acceleration_point") {
                return set_acceleration_point(args);
                }
    else
    if (func == "pfx_set_gravity_direction") {
                return set_gravity_direction(args);
                }
    else
    if (func == "pfx_set_velocity") {
                return set_velocity(args);
                }
    else
    if (func == "pfx_set_angle") {
                return set_angle(args);
                }
    else
      if (func == "pfx_set_interval") {
                return set_interval(args);
                }
    else
    if (func == "pfx_set_secondary_angle") {
                return set_secondary_angle(args);
                }
    else
    if (func == "pfx_set_generating_function") {
                return set_generating_function(args);
                }
    else
    if (func == "pfx_use_screen_relative") {
                return use_screen_relative(args);
                }
    else
      if (func == "pfx_unload_texture") {
                return unload_texture(args);
                }
    else
    if (func == "pfx_load_effect") {
                return load_effect(args);
                }
    else
    if (func == "pfx_set_z") {
                return SetZ(args);
                }
    else
    if (func == "pfx_set_layer") {
                return SetLayer(args);
                }

}

void DynRpg::Particle::Update() {
	ptag_t::iterator itr = pfx_list.begin();
	while (itr != pfx_list.end()) {
		//itr->second->draw();
		itr++;
	}
}

DynRpg::Particle::~Particle() {
	OnMapChange();
}

void DynRpg::Particle::OnMapChange() {
	ptag_t::iterator itr = pfx_list.begin();
	while (itr != pfx_list.end()) {
		itr->second->clear();
		itr++;
	}
}



